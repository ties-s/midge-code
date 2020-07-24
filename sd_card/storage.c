#include "storage.h"

#include "drv_audio_pdm.h"
#include "boards.h"
#include "ff.h"
#include "diskio_blkdev.h"
#include "nrf_block_dev_sdc.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_gpio.h"
#include "ICM20948_driver_interface.h"
#include "systick_lib.h"
#include "scanner_lib.h"
#include "audio_switch.h"
#include "advertiser_lib.h"
#include "app_timer.h"
#include "sampling_lib.h"

#define F_WRITE_TIMEOUT_MS (75) // 8kB takes max 20ms, stereo at 20kHz at HIGH is 102.4ms
#define F_SYNC_PERIOD_MS (1000*60*5) // 5 min periodic flushing to the sd_card

APP_TIMER_DEF(f_write_timeout_timer);
APP_TIMER_DEF(f_sync_timer);

/**
 * @brief  SDC block device definition
 * */
NRF_BLOCK_DEV_SDC_DEFINE(
        m_block_dev_sdc,
        NRF_BLOCK_DEV_SDC_CONFIG(
                SDC_SECTOR_SIZE,
                APP_SDCARD_CONFIG(SDC_MOSI_PIN, SDC_MISO_PIN, SDC_SCK_PIN, SDC_CS_PIN)
         ),
         NFR_BLOCK_DEV_INFO_CONFIG("Nordic", "SDC", "1.00")
);

FATFS fs;
DIR dir;
FILINFO fno;
FIL audio_file_handle;
FIL imu_file_handle[MAX_IMU_SOURCES];
FIL scanner_file_handle;


void sd_write(void * p_event_data, uint16_t event_size)
{
	static data_source_info_t data_source_info;
	data_source_info = *(data_source_info_t *)p_event_data;

	if (data_source_info.data_source == AUDIO && !audio_file_handle.err)
	{
		app_timer_start(f_write_timeout_timer, APP_TIMER_TICKS(F_WRITE_TIMEOUT_MS), &data_source_info.data_source);
		// size is times two, since this function receives number of bytes, not size of pointer
		FRESULT ff_result = f_write(&audio_file_handle, data_source_info.audio_source_info.audio_buffer, data_source_info.audio_source_info.audio_buffer_length, NULL);
		if (ff_result != FR_OK)
			NRF_LOG_ERROR("Audio write to sd failed: %d", ff_result);
		app_timer_stop(f_write_timeout_timer);
	}
	else if (data_source_info.data_source == IMU && !imu_file_handle[data_source_info.imu_source_info.imu_source].err)
	{
		app_timer_start(f_write_timeout_timer, APP_TIMER_TICKS(F_WRITE_TIMEOUT_MS), &data_source_info.data_source);
		FRESULT ff_result = f_write(&imu_file_handle[data_source_info.imu_source_info.imu_source], data_source_info.imu_source_info.imu_buffer, sizeof(imu_sample_t)*IMU_BUFFER_SIZE, NULL);
		if (ff_result != FR_OK)
			NRF_LOG_ERROR("IMU data write to sd failed: %d", ff_result);
		app_timer_stop(f_write_timeout_timer);
	}
	else if (data_source_info.data_source == SCANNER && !scanner_file_handle.err)
	{
		app_timer_start(f_write_timeout_timer, APP_TIMER_TICKS(F_WRITE_TIMEOUT_MS), &data_source_info.data_source);
		FRESULT ff_result = f_write(&scanner_file_handle, scanner_scan_buffer[data_source_info.scanner_buffer_num], sizeof(scanner_scan_report_t)*SCANNER_BUFFER_LENGTH, NULL);
		if (ff_result != FR_OK)
			NRF_LOG_ERROR("Scanner data write to sd failed: %d", ff_result);
		app_timer_stop(f_write_timeout_timer);
	}
}

// In case the low-quality sdcards block for writting, restart the audio file in order to time-stamp it again.
static void f_write_timeout_handler(void* p_context)
{
	data_source_t data_source = *(data_source_t *)p_context;
	NRF_LOG_ERROR("source: %d caused a timeout at: %lu", data_source, systick_get_millis()/1000);
	if (data_source == AUDIO)
	{
		sampling_stop_microphone();
		nrf_delay_ms(200); // delay to fill the buffer and close the files before opening the new ones
		sampling_start_microphone(-1); //mode = -1 to restart with last requested mode
	}

}

static void f_sync_timeout_handler(void* p_context)
{
//	NRF_LOG_INFO("f_sync timer");
	FRESULT ff_result = FR_OK;

	for (uint8_t sensor=0; sensor<MAX_IMU_SOURCES; sensor++)
	{
		if (!imu_file_handle[sensor].err)
			ff_result |= f_sync(&imu_file_handle[sensor]);
	}
	if (!audio_file_handle.err)
		ff_result |= f_sync(&audio_file_handle);
	if (!scanner_file_handle.err)
		ff_result |= f_sync(&scanner_file_handle);

	if (ff_result != FR_OK)
		NRF_LOG_ERROR("f_sync error: %d", ff_result);

}

uint32_t storage_close_file(data_source_t source)
{
	FRESULT ff_result;

	if (source == AUDIO)
	{
		audio_file_handle.err = 1;
		ff_result = f_close(&audio_file_handle);
		if (ff_result)
		{
			NRF_LOG_INFO("close audio file error: %d", ff_result);
			return -1;
		}
	}
	if (source == IMU)
	{
		for (uint8_t sensor=0; sensor<MAX_IMU_SOURCES; sensor++)
		{
			imu_file_handle[sensor].err = 1;
			ff_result = f_close(&imu_file_handle[sensor]);
			if (ff_result)
			{
				NRF_LOG_INFO("close IMU file error: %d", ff_result);
				return -1;
			}
		}
	}
	if (source == SCANNER)
	{
		scanner_file_handle.err = 1;
		ff_result = f_close(&scanner_file_handle);
		if (ff_result)
		{
			NRF_LOG_INFO("close proximity file error: %d", ff_result);
			return -1;
		}
	}
	return 0;
}

void list_directory(void)
{
	FRESULT ff_result;

	NRF_LOG_INFO("\r\n Listing directory: /");
	ff_result = f_opendir(&dir, "/");
	if (ff_result)
	{
		NRF_LOG_INFO("Directory listing failed!");
	}

	do
	{
		ff_result = f_readdir(&dir, &fno);
		if (ff_result != FR_OK)
		{
			NRF_LOG_INFO("Directory read failed: %d", ff_result);
			break;
		}

		if (fno.fname[0])
		{
			if (fno.fattrib & AM_DIR)
			{
				NRF_LOG_RAW_INFO("   <DIR>   %s\n",(uint32_t)fno.fname);
			}
			else
			{
				NRF_LOG_RAW_INFO("%9lu  %s\n", fno.fsize, (uint32_t)fno.fname);
			}
		}
	}
	while (fno.fname[0]);
}

uint32_t storage_get_free_space(uint32_t *total_MB, uint32_t *free_MB)
{
	FRESULT ff_result;
    DWORD fre_clust, fre_sect, tot_sect;
    FATFS * p_fs = &fs;

    ff_result = f_getfree("", &fre_clust, &p_fs);
    if (ff_result)
    {
    	NRF_LOG_INFO("get free failed: %d", ff_result);
    	return -1;
    }
    /* Get total sectors and free sectors */
    tot_sect = (p_fs->n_fatent - 2) * p_fs->csize;
    fre_sect = fre_clust * p_fs->csize;

    *total_MB = tot_sect / 2048;
    *free_MB = fre_sect / 2048;

    /* Print the free space (assuming 512 bytes/sector) */
    NRF_LOG_INFO("\n%10lu MiB total drive space.\n%10lu MiB available.\n", tot_sect / 2/1024, fre_sect / 2/1024);

    return NRF_SUCCESS;
}

uint32_t storage_init(void)
{
    FRESULT ff_result;
    DSTATUS disk_state = STA_NOINIT;

    // Initialize FATFS disk I/O interface by providing the block device.
    static diskio_blkdev_t drives[] =
    {
            DISKIO_BLOCKDEV_CONFIG(NRF_BLOCKDEV_BASE_ADDR(m_block_dev_sdc, block_dev), NULL)
    };

    diskio_blockdev_register(drives, ARRAY_SIZE(drives));

    NRF_LOG_INFO("Initializing disk 0 (SDC)...");
    for (uint32_t retries = 3; retries && disk_state; --retries)
    {
        disk_state = disk_initialize(0);
    }
    if (disk_state)
    {
        NRF_LOG_INFO("Disk initialization failed.");
        return 1;
    }

    NRF_LOG_INFO("Mounting volume...");
    ff_result = f_mount(&fs, "", 1);
    if (ff_result)
    {
        NRF_LOG_INFO("Mount failed.");
        return 2;
    }

	// Create the f_write supervisor
	uint32_t ret = app_timer_create(&f_write_timeout_timer, APP_TIMER_MODE_SINGLE_SHOT, f_write_timeout_handler);
	if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;

	// init the error flag so no syncing will occur
	for (uint8_t sensor=0; sensor<MAX_IMU_SOURCES; sensor++)
		imu_file_handle[sensor].err = 1;
	audio_file_handle.err = 1;
	scanner_file_handle.err = 1;
	// Create the f_sync repeated timer
	ret = app_timer_create(&f_sync_timer, APP_TIMER_MODE_REPEATED, f_sync_timeout_handler);
	if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
	app_timer_start(f_sync_timer, APP_TIMER_TICKS(F_SYNC_PERIOD_MS), NULL);

    return 0;
}


uint32_t storage_init_folder(uint32_t sync_time_seconds)
{
	NRF_LOG_INFO("open folder");
	FRESULT ff_result;
	BadgeAssignment badge_assignment;
	advertiser_get_badge_assignement(&badge_assignment);
	TCHAR folder[30] = {};

	sprintf(folder, "/%d_%ld", badge_assignment.ID, sync_time_seconds);
	ff_result = f_mkdir(folder);
	if (ff_result)
	{
		NRF_LOG_INFO("Making of new directory failed!");
		return -1;
	}

	ff_result = f_chdir(folder);
	if (ff_result)
	{
		NRF_LOG_INFO("Changing into new directory failed!");
		return -1;
	}

	return 0;
}

uint32_t storage_open_file(data_source_t source)
{
	FRESULT ff_result;

	uint64_t millis = systick_get_millis();
	uint32_t seconds = millis/1000;
	TCHAR filename[50] = {};

	if (source == AUDIO)
	{
		sprintf(filename, "%ld%ld_audio_%d", seconds, (uint32_t)millis%1000, audio_switch_get_position()); //printing 64bit requires full version of newlib, that doesn't fit the RAM now
	    ff_result = f_open(&audio_file_handle, filename, FA_WRITE | FA_CREATE_ALWAYS);
	    if (ff_result != FR_OK)
	    {
	        NRF_LOG_INFO("Unable to open or create file: %s", filename);
	        return -1;
	    }
		audio_file_handle.err = 0;
	}
	if (source == IMU)
	{
		for (uint8_t sensor=0; sensor<MAX_IMU_SOURCES; sensor++)
		{
			sprintf(filename, "%ld_%s", seconds, imu_sensor_name[sensor]);
			ff_result = f_open(&imu_file_handle[sensor], filename, FA_WRITE | FA_CREATE_ALWAYS);
			if (ff_result != FR_OK)
			{
				NRF_LOG_INFO("Unable to open or create file: %s", filename);
				return -1;
			}
			imu_file_handle[sensor].err = 0;
		}
	}
	if (source == SCANNER)
	{
		sprintf(filename, "%ld_proximity", seconds);
	    ff_result = f_open(&scanner_file_handle, filename, FA_WRITE | FA_CREATE_ALWAYS);
	    if (ff_result != FR_OK)
	    {
	        NRF_LOG_INFO("Unable to open or create file: %s", filename);
	        return -1;
	    }
	    scanner_file_handle.err = 0;
	}


    return 0;
}
