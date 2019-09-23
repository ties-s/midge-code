#include <drv_audio_pdm.h>
#include "fat_test.h"
#include "boards.h"
#include "ff.h"
#include "diskio_blkdev.h"
#include "nrf_block_dev_sdc.h"
#include "nrf_delay.h"
#include "nrf_log.h"

#define FILE_NAME   "clip"

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

void sd_write(void * p_event_data, uint16_t event_size)
{
	if (audio_file_handle.err)
		return;

	uint32_t bytes_written = 0;
	uint8_t buffer_num = *(uint8_t*)p_event_data;
	// size is times two, since this function receives number of bytes, not size of pointer
	FRESULT ff_result = f_write(&audio_file_handle, pdm_buf[buffer_num].mic_buf, PDM_BUF_SIZE*2, (UINT *) &bytes_written);
	if (ff_result != FR_OK)
	{
		NRF_LOG_INFO("Write failed\r\n.");
	}
	else
	{
//		NRF_LOG_INFO("%d bytes written.", bytes_written);
	}
	f_sync(&audio_file_handle);
}

void sd_close(void * p_event_data, uint16_t event_size)
{
	if (f_sync(&audio_file_handle))
		NRF_LOG_INFO("sync file error during closing");
	if (f_close(&audio_file_handle))
		NRF_LOG_INFO("close file error");

	// flag to stop from writing after closing - could look if it is open but would take more time?
	audio_file_handle.err = 1;

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


uint32_t sd_init()
{
	//TODO: add checks for CD and SW pins? - return error if sd card not present?

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
        return -1;
    }

    uint32_t blocks_per_mb = (1024uL * 1024uL) / m_block_dev_sdc.block_dev.p_ops->geometry(&m_block_dev_sdc.block_dev)->blk_size;
    uint32_t capacity = m_block_dev_sdc.block_dev.p_ops->geometry(&m_block_dev_sdc.block_dev)->blk_count / blocks_per_mb;
    NRF_LOG_INFO("Capacity: %d MB", capacity);

    NRF_LOG_INFO("Mounting volume...");
    ff_result = f_mount(&fs, "", 1);
    if (ff_result)
    {
        NRF_LOG_INFO("Mount failed.");
        return -2;
    }

    NRF_LOG_INFO("\r\n Listing directory: /");
    ff_result = f_opendir(&dir, "/");
    if (ff_result)
    {
        NRF_LOG_INFO("Directory listing failed!");
        return -3;
    }

    do
    {
        ff_result = f_readdir(&dir, &fno);
        if (ff_result != FR_OK)
        {
            NRF_LOG_INFO("Directory read failed.");
            return -4;
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
    NRF_LOG_RAW_INFO("");

    ff_result = f_open(&audio_file_handle, FILE_NAME, FA_READ | FA_WRITE | FA_OPEN_ALWAYS);
    if (ff_result != FR_OK)
    {
        NRF_LOG_INFO("Unable to open or create file: " FILE_NAME ".");
        return -5;
    }
    else
    {
    	NRF_LOG_INFO("opened audio file successfully");
    }

    return 0;
}
