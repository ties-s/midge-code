#include "ble_lib.h"
#include "advertiser_lib.h"		// For the advertising configuration	

#include "system_event_lib.h"

#include "ble_advertising.h"
#include "ble_gap.h"
#include "ble_nus.h"
//#include "softdevice_handler.h"	// Needed for softdevice_ble_evt_handler_set()

#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf_log.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"

#include "ble_hci.h"			// Needed for BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION in disconnect()

#define APP_BLE_OBSERVER_PRIO           3                                           /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG            1                                           /**< A tag identifying the SoftDevice BLE configuration. */


#define SEC_PARAM_BOND                  1                                           /**< Perform bonding. */
#define SEC_PARAM_MITM                  0                                           /**< Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                        /**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                           /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                           /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                          /**< Maximum encryption key size. */


BLE_NUS_DEF(m_nus, NRF_SDH_BLE_TOTAL_LINK_COUNT);                                   /**< BLE NUS service instance. */
NRF_BLE_GATT_DEF(m_gatt);                                                           /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                                             /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);                                                 /**< Advertising module instance. */


static ble_uuid_t adv_uuids[] = {{BLE_UUID_NUS_SERVICE,     BLE_UUID_TYPE_BLE}};  	/**< Universally unique service identifiers of the NUS service for advertising. */
static ble_gap_sec_params_t		ble_sec_params;                               		/**< Security requirements for this application. */
static uint16_t                 ble_conn_handle = BLE_CONN_HANDLE_INVALID;    		/**< Handle of the current connection. */

static uint8_t	ble_state;														/**< The current BLE-state. */ 



static ble_on_receive_callback_t		external_ble_on_receive_callback = NULL;		/**< The external on receive callback function */
static ble_on_transmit_callback_t		external_ble_on_transmit_callback = NULL;		/**< The external on transmit callback function */
static ble_on_connect_callback_t		external_ble_on_connect_callback = NULL;		/**< The external on connect callback function */
static ble_on_disconnect_callback_t		external_ble_on_disconnect_callback = NULL;		/**< The external on disconnect callback function */
static ble_on_scan_timeout_callback_t	external_ble_on_scan_timeout_callback = NULL;	/**< The external on scan timeout callback function */
static ble_on_scan_report_callback_t	external_ble_on_scan_report_callback = NULL;	/**< The external on scan report callback function */


static ret_code_t ble_init_services(void);
static ret_code_t ble_init_advertising(void);


static void ble_nus_on_receive_callback(ble_nus_evt_t * p_evt);
static void ble_nus_on_transmit_complete_callback(void);
static void ble_on_connect_callback(void);
static void ble_on_disconnect_callback(void);
static void ble_on_scan_report_callback(const ble_gap_evt_adv_report_t* scan_report);
static void ble_on_scan_timeout_callback(void);


static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);



ret_code_t ble_init(void) {
	
	
	ble_state = BLE_STATE_INACTIVE;
	
	
	ret_code_t ret;    
  
    ret = nrf_sdh_enable_request();
    if(ret != NRF_SUCCESS) return ret;

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    ret = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    if(ret != NRF_SUCCESS) return ret;

    // Enable BLE stack.
    ret = nrf_sdh_ble_enable(&ram_start);
    if(ret != NRF_SUCCESS) return ret;

    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
    

	// TODO: call in main.c
	system_event_init();


//    peer_manager_init();
//    gap_params_init();
//    gatt_init();

    ret = nrf_ble_gatt_init(&m_gatt, NULL);
    if(ret != NRF_SUCCESS) return ret;

	
	// Initializing the security params
	ble_sec_params.bond         = SEC_PARAM_BOND;
    ble_sec_params.mitm         = SEC_PARAM_MITM;
    ble_sec_params.io_caps      = SEC_PARAM_IO_CAPABILITIES;
    ble_sec_params.oob          = SEC_PARAM_OOB;
    ble_sec_params.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    ble_sec_params.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
	
	

	ret = ble_init_services();
	//NRF_LOG_INFO("BLE: Ret init services %u\n", ret);
	if(ret != NRF_SUCCESS) return ret;
	
	ret = ble_init_advertising();
	//NRF_LOG_INFO("BLE: Ret init advertising %u\n", ret);
    if(ret != NRF_SUCCESS) return ret;
	
	uint8_t mac[6];
	ble_get_MAC_address(mac);
	NRF_LOG_INFO("MAIN: MAC address: %.2X:%.2X:%.2X:%.2X:%.2X:%.2X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	
	return NRF_SUCCESS;	
}


/**@brief	Function to initialize the Nordic Uart Service.

 * @retval	NRF_SUCCESS On success, else an error code indicating reason for failure.
 */
static ret_code_t ble_init_services(void)
{
	ret_code_t ret;
    ble_nus_init_t nus_init;           //Nordic UART Service - for emulating a UART over BLE
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = NULL;

    ret = nrf_ble_qwr_init(&m_qwr, &qwr_init);
	if(ret != NRF_SUCCESS) return ret;

    memset(&nus_init,0,sizeof(nus_init));

    nus_init.data_handler = ble_nus_on_receive_callback;


    ret = ble_nus_init(&m_nus, &nus_init);
	if(ret != NRF_SUCCESS) return ret;
	
	return NRF_SUCCESS;
}

/**@brief	Function to initialize the BLE advertising-process.

 * @retval	NRF_SUCCESS On success, else an error code indicating reason for failure.
 */
static ret_code_t ble_init_advertising(void) {
	ret_code_t ret;

	// The sys-event handler is needed to synchronize flash and advertising processes. TODO: do we need this? no flash operation is taking place
	ret = system_event_register_handler(ble_advertising_on_sys_evt);
	if(ret != NRF_SUCCESS) return ret;
	

    ble_gap_conn_sec_mode_t sec_mode;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);  //no security needed
	
    //set BLE name
    ret = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *) ADVERTISING_DEVICE_NAME, strlen(ADVERTISING_DEVICE_NAME));
    if(ret != NRF_SUCCESS) return ret;

    //set BLE appearance
    ret = sd_ble_gap_appearance_set(BLE_APPEARANCE_GENERIC_TAG);
    if(ret != NRF_SUCCESS) return ret;

    ble_gap_conn_params_t   gap_conn_params;
    memset(&gap_conn_params, 0, sizeof(gap_conn_params));


#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)            /**< Minimum acceptable connection interval (0.1 seconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(200, UNIT_1_25_MS)            /**< Maximum acceptable connection interval (0.2 second). */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds). */

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    ret =  sd_ble_gap_ppcp_set(&gap_conn_params);
    if(ret != NRF_SUCCESS) return ret;


    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance		 = 0;
    init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;


	ble_advdata_manuf_data_t custom_manuf_data;

	uint8_t man_data[11] = {};
	custom_manuf_data.company_identifier = 0xFF00;
    custom_manuf_data.data.p_data = man_data;
    custom_manuf_data.data.size = 11;

    init.advdata.p_manuf_specific_data = &custom_manuf_data;

    init.evt_handler = NULL;

    init.advdata.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
    init.advdata.uuids_complete.p_uuids  = adv_uuids;

    init.config.ble_adv_fast_enabled	= 1;
    init.config.ble_adv_fast_interval	= (uint32_t)(ADVERTISING_INTERVAL_MS/0.625f); // In units of 0.625ms
    init.config.ble_adv_fast_timeout	= ADVERTISING_TIMEOUT_SECONDS;

    ret = ble_advertising_init(&m_advertising, &init);
    if(ret != NRF_SUCCESS) return ret;

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);

	return NRF_SUCCESS;
}


ret_code_t ble_set_advertising_custom_advdata(uint16_t company_identifier, uint8_t* custom_advdata, uint16_t custom_advdata_size)
{
	ble_gap_adv_data_t gap_adv_data  = m_advertising.adv_data;

	// found pos 11 by dumping adv_data after init
	memcpy(&gap_adv_data.adv_data.p_data[11], custom_advdata, custom_advdata_size);
	return ble_advertising_advdata_update(&m_advertising, &gap_adv_data, true);
	
}


ret_code_t ble_start_advertising(void)
{
//	ble_gap_adv_data_t gap_adv_data  = m_advertising.adv_data;
//
//	NRF_LOG_RAW_HEXDUMP_INFO(gap_adv_data.adv_data.p_data, gap_adv_data.adv_data.len)

	ret_code_t ret = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
	if(ret != NRF_SUCCESS) return ret;
	
	
	ble_state |= BLE_STATE_ADVERTISING;
	
	return ret;
}

void ble_stop_advertising(void) {
	// Advertising can't be stopped directly, but if an advertising timeout occurs, the advertising shouldn't be started again.
	ble_state &= ~BLE_STATE_ADVERTISING;
}



ret_code_t ble_start_scanning(uint16_t scan_interval_ms, uint16_t scan_window_ms, uint16_t scan_duration_seconds) {
	ble_gap_scan_params_t scan_params;
	
	scan_params.active = 0;  			// passive scanning, only looking for advertising packets
//    scan_params.selective = 0;  		// non-selective, don't use whitelist
//    scan_params.p_whitelist = NULL;  	// no whitelist
    scan_params.interval 	= (uint16_t)((((uint32_t)(scan_interval_ms)) * 1000) / 625);	// scan_params uses interval in units of 0.625ms
    scan_params.window 		= (uint16_t)((((uint32_t)(scan_window_ms)) * 1000) / 625);  	// window also in units of 0.625ms
    scan_params.timeout 	= scan_duration_seconds;                			// timeout is in s
	
	sd_ble_gap_scan_stop();  // stop any in-progress scans

	ble_data_t scan_data;
	ret_code_t ret = sd_ble_gap_scan_start(&scan_params, &scan_data); // start a new scan

	if(ret != NRF_SUCCESS) return ret;
	
	ble_state |= BLE_STATE_SCANNING;
	
	return NRF_SUCCESS;
}

void ble_stop_scanning(void) {
	sd_ble_gap_scan_stop();  // stop any in-progress scans

	ble_state &= ~BLE_STATE_SCANNING;
}



/**@brief The BLE-event handler that is called when there is a BLE-event generated by the BLE-Stack/Softdevice.
 *
 * @param[in]	p_ble_evt	Pointer to the BLE-event generated by the Softdevice.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context) {
	
//	NRF_LOG_INFO("ble_evt: %d", p_ble_evt->header.evt_id);
	
	uint32_t ret;
	
	// security-related
    static ble_gap_evt_auth_status_t m_auth_status;
    bool                             master_id_matches;
    ble_gap_sec_kdist_t *            p_distributed_keys;
    ble_gap_enc_info_t *             p_enc_info;
    ble_gap_irk_t *                  p_id_info;
    ble_gap_sign_info_t *            p_sign_info;
    static ble_gap_enc_key_t         m_enc_key;           /**< Encryption Key (Encryption Info and Master ID). */
    static ble_gap_id_key_t          m_id_key;            /**< Identity Key (IRK and address). */
    static ble_gap_sign_info_t       m_sign_key;          /**< Signing Key (Connection Signature Resolving Key). */
    static ble_gap_sec_keyset_t      m_keys = {.keys_own = {&m_enc_key, &m_id_key, &m_sign_key}};


    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_HVN_TX_COMPLETE:
            ble_nus_on_transmit_complete_callback();
            break;
        case BLE_GAP_EVT_CONNECTED:  //on BLE connect event
            NRF_LOG_INFO("BLE: connection intervals: %u, %u, %u, %u\n", p_ble_evt->evt.gap_evt.params.connected.conn_params.min_conn_interval, p_ble_evt->evt.gap_evt.params.connected.conn_params.max_conn_interval, p_ble_evt->evt.gap_evt.params.connected.conn_params.slave_latency, p_ble_evt->evt.gap_evt.params.connected.conn_params.conn_sup_timeout);
            ble_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            nrf_ble_qwr_conn_handle_assign(&m_qwr, ble_conn_handle);
            ble_on_connect_callback();
            break;
        case BLE_GAP_EVT_DISCONNECTED:  //on BLE disconnect event
            ble_conn_handle = BLE_CONN_HANDLE_INVALID;
            ble_on_disconnect_callback();
            break;
        case BLE_GAP_EVT_ADV_REPORT:  //On receipt of a response to an advertising request (during a scan)
            ble_on_scan_report_callback(&(p_ble_evt->evt.gap_evt.params.adv_report));
            break;        
        case BLE_GAP_EVT_TIMEOUT:
            if(p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_SCAN)  {
                ble_on_scan_timeout_callback();
            }            
            break;
            
        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            ret = sd_ble_gap_sec_params_reply(ble_conn_handle,
                                                   BLE_GAP_SEC_STATUS_SUCCESS,
                                                   &ble_sec_params,
                                                   &m_keys);
            (void) ret;
            break;
        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            ret = sd_ble_gatts_sys_attr_set(ble_conn_handle,
                                                 NULL,
                                                 0,
                                                 BLE_GATTS_SYS_ATTR_FLAG_SYS_SRVCS | BLE_GATTS_SYS_ATTR_FLAG_USR_SRVCS);
            (void) ret;
            break;
        case BLE_GAP_EVT_AUTH_STATUS:
            m_auth_status = p_ble_evt->evt.gap_evt.params.auth_status;
            break;
        case BLE_GAP_EVT_SEC_INFO_REQUEST:
            master_id_matches  = memcmp(&p_ble_evt->evt.gap_evt.params.sec_info_request.master_id,
                                        &m_enc_key.master_id,
                                        sizeof(ble_gap_master_id_t)) == 0;
            p_distributed_keys = &m_auth_status.kdist_own;

            p_enc_info  = (p_distributed_keys->enc  && master_id_matches) ? &m_enc_key.enc_info : NULL;
            p_id_info   = (p_distributed_keys->id   && master_id_matches) ? &m_id_key.id_info   : NULL;
            p_sign_info = (p_distributed_keys->sign && master_id_matches) ? &m_sign_key         : NULL;

            ret = sd_ble_gap_sec_info_reply(ble_conn_handle, p_enc_info, p_id_info, p_sign_info);
            (void) ret;
            break;
        default:
            break;
    }
	
	
	
	// Intercept advertising timeout events to implement infinite advertising, and disconnect events to let radio-conflicting 
    //   operations to execute after a connection closes
    if( p_ble_evt->header.evt_id == BLE_GAP_EVT_ADV_SET_TERMINATED    // if timeout event from advertising
        || p_ble_evt->header.evt_id == BLE_GAP_EVT_DISCONNECTED) {                              // OR if disconnect event
		
		
		// Restart the advertiting
		if(ble_state & BLE_STATE_ADVERTISING) {
			ble_start_advertising();
		}		
    } else {
        ble_advertising_on_ble_evt(p_ble_evt,&m_advertising);
    }
	
	
	
    
}




/**@brief Handler function that is called when some data were received via the Nordic Uart Service.
 *
 * @param[in]	p_nus	Pointer to the nus-identifier.
 * @param[in]	p_data	Pointer to the received data.
 * @param[in]	length	Length of the received data.
 */
static void ble_nus_on_receive_callback(ble_nus_evt_t * p_evt)
{
	// ffuuuu 12.2 does it differently, not everything is pushed to the nus handler so here I need to check that it is only data, not notifications
	if( (external_ble_on_receive_callback != NULL) && (p_evt->type == BLE_NUS_EVT_RX_DATA))
	{
		external_ble_on_receive_callback(p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);
	}
}

/**@brief Handler function that is called when data were transmitted via the Nordic Uart Service.
 */
static void ble_nus_on_transmit_complete_callback(void) {
	if(external_ble_on_transmit_callback != NULL) 
		external_ble_on_transmit_callback();
}

/**@brief Handler function that is called when a BLE-connection was established.
 */
static void ble_on_connect_callback(void) {
	ble_state |= BLE_STATE_CONNECTED;
	if(external_ble_on_connect_callback != NULL)
		external_ble_on_connect_callback();	
}

/**@brief Handler function that is called when disconnecting from an exisiting BLE-connection.
 */
static void ble_on_disconnect_callback(void) {
	ble_state &= ~BLE_STATE_CONNECTED;
	if(external_ble_on_disconnect_callback != NULL)
		external_ble_on_disconnect_callback();
}

/**@brief Handler function that is called when there is an advertising report event during scanning.
 *
 * @param[in]	scan_report		Pointer to the advertsising report event.
 */
static void ble_on_scan_report_callback(const ble_gap_evt_adv_report_t* scan_report) {
	if(external_ble_on_scan_report_callback != NULL)
		external_ble_on_scan_report_callback(scan_report);

	//NRF_LOG_INFO("BLE: BLE on scan report callback. RSSI: %d\n", scan_report->rssi);
}

/**@brief Handler function that is called when the scan process timed-out.
 */
static void ble_on_scan_timeout_callback(void) {
	ble_state &= ~BLE_STATE_SCANNING;
	if(external_ble_on_scan_timeout_callback != NULL)
		external_ble_on_scan_timeout_callback();
	
	//NRF_LOG_INFO("BLE: BLE on scan timeout callback\n");
}



// BLE_GAP_ADDR_LEN = 6 from ble_gap.h
void ble_get_MAC_address(uint8_t* MAC_address) {
	ble_gap_addr_t MAC;
    sd_ble_gap_addr_get(&MAC);
   
	for(uint8_t i = 0; i < BLE_GAP_ADDR_LEN; i++) {
		MAC_address[i] = MAC.addr[BLE_GAP_ADDR_LEN - 1 - i];
	}
}



ble_state_t ble_get_state(void) {
	if(ble_state & BLE_STATE_CONNECTED) {	// BLE_STATE_CONNECTED has higher priority than BLE_STATE_SCANNING and BLE_STATE_ADVERTISING!
		return BLE_STATE_CONNECTED;
	} else if (ble_state & BLE_STATE_SCANNING) {	// BLE_STATE_SCANNING has higher priority than BLE_STATE_ADVERTISING!
		return BLE_STATE_SCANNING;
	} else if(ble_state & BLE_STATE_ADVERTISING) {
		return BLE_STATE_ADVERTISING;
	} 
	return BLE_STATE_INACTIVE;
}



ret_code_t ble_transmit(uint8_t* data, uint16_t len) {

	ret_code_t ret = ble_nus_data_send(&m_nus, data, &len, ble_conn_handle);
	NRF_LOG_INFO("sent: %d bytes, ret: %x", len, ret);

	return ret;
}


void ble_disconnect(void) {
	sd_ble_gap_disconnect(ble_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
}



void ble_set_on_receive_callback(ble_on_receive_callback_t ble_on_receive_callback) {
	external_ble_on_receive_callback = ble_on_receive_callback;
}

void ble_set_on_transmit_callback(ble_on_transmit_callback_t ble_on_transmit_callback) {
	external_ble_on_transmit_callback = ble_on_transmit_callback;
}

void ble_set_on_connect_callback(ble_on_connect_callback_t ble_on_connect_callback) {
	external_ble_on_connect_callback = ble_on_connect_callback;
}

void ble_set_on_disconnect_callback(ble_on_disconnect_callback_t ble_on_disconnect_callback) {
	external_ble_on_disconnect_callback = ble_on_disconnect_callback;
}

void ble_set_on_scan_timeout_callback(ble_on_scan_timeout_callback_t ble_on_scan_timeout_callback) {
	external_ble_on_scan_timeout_callback = ble_on_scan_timeout_callback;
}

void ble_set_on_scan_report_callback(ble_on_scan_report_callback_t ble_on_scan_report_callback) {
	external_ble_on_scan_report_callback = ble_on_scan_report_callback;
}

