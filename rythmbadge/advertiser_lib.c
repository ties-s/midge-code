#include "advertiser_lib.h"
#include "ble_lib.h"
#include "string.h" // For memset
#include "nrf_log.h"

#define CUSTOM_COMPANY_IDENTIFIER	0xFF00
#define CUSTOM_ADVDATA_LEN			11

/**< Structure for organizing custom badge advertising data */
typedef struct
{
    uint8_t battery;
    uint8_t status_flags;
    uint16_t ID;
    uint8_t group;
    uint8_t MAC[6];
} custom_advdata_t;


static custom_advdata_t custom_advdata;	/**< The custom advertising data structure where the current configuration is stored. */


void advertiser_init(void)
{
	memset(&custom_advdata, 0, sizeof(custom_advdata));
	// We need to swap the MAC address to be compatible with old code..
	uint8_t MAC[6];
	ble_get_MAC_address(MAC);
	for(uint8_t i = 0; i < 6; i++) {
		custom_advdata.MAC[i] = MAC[5-i];
	}
	
	custom_advdata.group = ADVERTISING_RESET_GROUP;
	custom_advdata.ID = ADVERTISING_RESET_ID;
	
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, CUSTOM_ADVDATA_LEN);
}

ret_code_t advertiser_start_advertising(void) {
	return ble_start_advertising();
}

void advertiser_set_battery_percentage(uint8_t battery_percentage)
{
	custom_advdata.battery = battery_percentage;
	
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, CUSTOM_ADVDATA_LEN);
}




ret_code_t advertiser_set_badge_assignement(BadgeAssignment badge_assignement)
{
	custom_advdata.ID = badge_assignement.ID;
	custom_advdata.group = badge_assignement.group;	
	
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, CUSTOM_ADVDATA_LEN);
	
	return NRF_SUCCESS;
}

void advertiser_set_status_flag_is_clock_synced(uint8_t is_clock_synced) {
	if(is_clock_synced)
		custom_advdata.status_flags |= (1 << 0);
	else
		custom_advdata.status_flags &= ~(1 << 0);
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, CUSTOM_ADVDATA_LEN);
}


void advertiser_set_status_flag_microphone_enabled(uint8_t microphone_enabled) {
	if(microphone_enabled)
		custom_advdata.status_flags |= (1 << 1);
	else
		custom_advdata.status_flags &= ~(1 << 1);
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, CUSTOM_ADVDATA_LEN);
}


void advertiser_set_status_flag_scan_enabled(uint8_t scan_enabled) {
	if(scan_enabled)
		custom_advdata.status_flags |= (1 << 2);
	else
		custom_advdata.status_flags &= ~(1 << 2);
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, CUSTOM_ADVDATA_LEN);
}


void advertiser_set_status_flag_imu_enabled(uint8_t imu_enabled) {
	if(imu_enabled)
		custom_advdata.status_flags |= (1 << 3);
	else
		custom_advdata.status_flags &= ~(1 << 3);
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, CUSTOM_ADVDATA_LEN);
}



void advertiser_get_badge_assignement(BadgeAssignment* badge_assignement) {
	badge_assignement->ID = custom_advdata.ID;
	badge_assignement->group = custom_advdata.group;
}

void advertiser_get_badge_assignement_from_advdata(BadgeAssignment* badge_assignement, void* custom_advdata) {
	custom_advdata_t tmp;
	memset(&tmp, 0, sizeof(tmp));
	memcpy(&tmp, custom_advdata, CUSTOM_ADVDATA_LEN);
	
	badge_assignement->ID = tmp.ID;
	badge_assignement->group = tmp.group;
}

uint8_t advertiser_get_manuf_data_len(void) {
	return (uint8_t) (CUSTOM_ADVDATA_LEN + 2);	// + 2 for the CUSTOM_COMPANY_IDENTIFIER
}


uint8_t advertiser_get_status_flag_is_clock_synced(void) {
	return (custom_advdata.status_flags & (1 << 0)) ? 1 : 0;
}


uint8_t advertiser_get_status_flag_microphone_enabled(void) {
	return (custom_advdata.status_flags & (1 << 1)) ? 1 : 0;
}


uint8_t advertiser_get_status_flag_scan_enabled(void) {
	return (custom_advdata.status_flags & (1 << 2)) ? 1 : 0;
}


uint8_t advertiser_get_status_flag_imu_enabled(void) {
	return (custom_advdata.status_flags & (1 << 3)) ? 1 : 0;
}
