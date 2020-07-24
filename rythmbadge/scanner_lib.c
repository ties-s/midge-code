#include "scanner_lib.h"
#include "ble_lib.h"
#include "advertiser_lib.h"
#include "string.h"
#include "nrf_log.h"
#include "storage.h"
#include "systick_lib.h"
#include "app_scheduler.h"

scanner_scan_report_t scanner_scan_buffer[2][SCANNER_BUFFER_LENGTH];

ret_code_t scanner_start_scanning(uint16_t scan_interval_ms, uint16_t scan_window_ms)
{
	return ble_start_scanning(scan_interval_ms, scan_window_ms);
}

void scanner_stop_scanning(void)
{
	ble_stop_scanning();
}

/**@brief Function that is called when a scan report is available.
 * 
 * @param[in]	scan_report		The scan report of a BLE device.
 */
void internal_on_scan_report_callback(const ble_gap_evt_adv_report_t* scan_report)
{
	static uint8_t active_buffer = 0;
	static uint8_t counter = 0;
	data_source_info_t data_source_info;
	data_source_info.data_source = SCANNER;

	if (scan_report->rssi < SCANNER_MINIMUM_RSSI)  {
		return;  // ignore signals that are too weak.
	}

	scanner_scan_report_t scanner_scan_report;
	scanner_scan_report.rssi = scan_report->rssi;

	BadgeAssignment badge_assignement;

	advertiser_get_badge_assignement_from_advdata(&badge_assignement, &scan_report->data.p_data[11]); // I know the exact location since it is fixed
	scanner_scan_report.badge_assignment = badge_assignement;

//		NRF_LOG_INFO("Scan result. ID: %d, group: %d, rssi: %d", badge_assignement.ID, badge_assignement.group, scanner_scan_report.rssi);
	scanner_scan_report.timestamp = systick_get_millis();

	memcpy(&scanner_scan_buffer[active_buffer][counter++], &scanner_scan_report, sizeof(scanner_scan_report));
	if (counter<SCANNER_BUFFER_LENGTH)
		return;
	data_source_info.scanner_buffer_num = active_buffer;
	if (app_sched_queue_space_get() > 10)
	{
		uint32_t err_code = app_sched_event_put(&data_source_info, sizeof(data_source_info), sd_write);
		APP_ERROR_CHECK(err_code);
		active_buffer = !active_buffer;
		counter = 0;
	}
	else
	{
		NRF_LOG_ERROR("dropped scanner sample");
	}
}

void scanner_init(void)
{
	ble_set_on_scan_report_callback(internal_on_scan_report_callback);
}
