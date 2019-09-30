#include "storage.h"

#include <drv_audio_pdm.h>
#include "boards.h"
#include "ff.h"
#include "diskio_blkdev.h"
#include "nrf_block_dev_sdc.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_gpio.h"
#include "ICM20948_driver_interface.h"
#include "systick_lib.h"

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
FIL accel_file_handle;

void sd_write(void * p_event_data, uint16_t event_size)
{
	data_source_info_t data_source_info = *(data_source_info_t *)p_event_data;
	uint32_t bytes_written = 0;

	if (data_source_info.data_source == AUDIO && !audio_file_handle.err)
	{
		// size is times two, since this function receives number of bytes, not size of pointer
		FRESULT ff_result = f_write(&audio_file_handle, pdm_buf[data_source_info.audio_buffer_num].mic_buf, PDM_BUF_SIZE*2, (UINT *) &bytes_written);
		if (ff_result != FR_OK)
		{
			NRF_LOG_INFO("Audio write to sd failed.");
		}
		f_sync(&audio_file_handle);
	}
	else if (data_source_info.data_source == IMU && !accel_file_handle.err)
	{
		FRESULT ff_result = f_printf(&accel_file_handle, "%ld %f %f %f", accel_sample.timestamp, accel_sample.accel[0], accel_sample.accel[1], accel_sample.accel[2]);
		if (ff_result != FR_OK)
		{
			NRF_LOG_INFO("Accel data write to sd failed.");
		}
		f_sync(&accel_file_handle);
	}
}

uint32_t storage_close_file(data_source_t source)
{
	FIL file;
	FRESULT ff_result;

	if (source == AUDIO)
	{
		file = audio_file_handle;
		audio_file_handle.err = 1;
	}
	else if (source == IMU)
	{
		file = accel_file_handle;
		accel_file_handle.err = 1;
	}

	ff_result = f_sync(&file);
	if (ff_result)
	{
		NRF_LOG_INFO("sync file error during closing");
		return -1;
	}
	ff_result = f_close(&file);
	if (ff_result)
	{
		NRF_LOG_INFO("close file error");
		return -1;
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


uint32_t storage_init(void)
{
	//TODO: add checks for CD and SW pins? - return error if sd card not present?
	// don't need to since if there is no sd card, init will fail under here.

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

    uint32_t blocks_per_mb = (1024uL * 1024uL) / m_block_dev_sdc.block_dev.p_ops->geometry(&m_block_dev_sdc.block_dev)->blk_size;
    uint32_t capacity = m_block_dev_sdc.block_dev.p_ops->geometry(&m_block_dev_sdc.block_dev)->blk_count / blocks_per_mb;
    NRF_LOG_INFO("Capacity: %d MB", capacity);

    NRF_LOG_INFO("Mounting volume...");
    ff_result = f_mount(&fs, "", 1);
    if (ff_result)
    {
        NRF_LOG_INFO("Mount failed.");
        return 2;
    }

    list_directory();
    return 0;
}

uint32_t storage_init_folder(uint32_t sync_time_seconds)
{
	NRF_LOG_INFO("open folder");
	FRESULT ff_result;

	TCHAR folder[10] = {};
	sprintf(folder, "/%ld", sync_time_seconds);
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

	uint32_t seconds;
	uint16_t milliseconds;
	systick_get_timestamp(&seconds, &milliseconds);
	TCHAR filename[20] = {};

	if (source == AUDIO)
	{
		sprintf(filename, "%ld_%d_audio", seconds, milliseconds);
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
		sprintf(filename, "%ld_%d_accel", seconds, milliseconds);
		ff_result = f_open(&accel_file_handle, filename, FA_WRITE | FA_CREATE_ALWAYS);
		if (ff_result != FR_OK)
		{
			NRF_LOG_INFO("Unable to open or create file: %s", filename);
			return -1;
		}
		accel_file_handle.err = 0;
	}

    return 0;
}