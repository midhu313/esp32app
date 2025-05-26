#include "includes.h"

static const char* tag = "BLE";

static const ble_uuid128_t app_service_uuid = BLE_UUID128_INIT(0x64,0x36,0xe0,0x7e,0x56,0xde,0xd8,0x9e,0x72,0x73,0x7c,0x7c,0x7a,0xf7,0x96,0x01);
static const ble_uuid16_t ble_char1_uuid = BLE_UUID16_INIT(BLE_SV_CHAR1);
static const ble_uuid16_t ble_char2_uuid = BLE_UUID16_INIT(BLE_SV_CHAR2);

static uint16_t gatt_svr_chr_val_handle;
static uint8_t own_addr_type;

static struct ble_gatt_svc_def gatt_service[1];
static struct ble_gatt_chr_def *gatt_chrs = NULL;

static struct DeviceCommand dev_rsp;

BLEManager BLEManager::s_instance;

BLEManager::BLEManager(){
	cmd = CmdManager::getInstance();
	clk = ClockManager::getInstance();
}

BLEManager::~BLEManager() {

}

BLEManager* BLEManager::getInstance(){
	return &s_instance;
}


void BLEManager::processedCommandResponseCallback(const DeviceCommand& rsp){
	dev_rsp = rsp;
	if(dev_rsp.source == CmdSource::BT){
		ESP_LOGI(tag,"Response:%s",dev_rsp.value); 
	}
}

/**
 * Access callback whenever a characteristic/descriptor is read or written to.
 * Here reads and writes need to be handled.
 * ctxt->op tells weather the operation is read or write and
 * weather it is on a characteristic or descriptor,
 * ctxt->dsc->uuid tells which characteristic/descriptor is accessed.
 * attr_handle give the value handle of the attribute being accessed.
 * Accordingly do:
 *     Append the value to ctxt->om if the operation is READ
 *     Write ctxt->om to the value if the operation is WRITE
 **/
int BLEManager::gatt_svc_access(uint16_t conn_handle, uint16_t attr_handle,struct ble_gatt_access_ctxt *ctxt,void *arg){
	int rc;
    uint16_t uuid = ble_uuid_u16(ctxt->chr->uuid);
    struct DeviceCommand rx_cmd;
    BTCharType type = (uuid == BLE_SV_CHAR1)?BTCharType::CHAR_1:(uuid == BLE_SV_CHAR2)?BTCharType::CHAR_2:BTCharType::CHAR_UNKNWN;
	if(type == BTCharType::CHAR_UNKNWN){//It won't happen
		ESP_LOGE(tag,"Unknown Characteristics uuid:%u",uuid);
		return -1;
	}
    switch (ctxt->op) {
		case BLE_GATT_ACCESS_OP_READ_CHR:
			if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
				ESP_LOGI(tag,"Characteristic-%d read :- conn_handle=%d attr_handle=%d",(type == BTCharType::CHAR_1)?1:2,conn_handle, attr_handle);
            	if(type==dev_rsp.char_type){
					rc = os_mbuf_append(ctxt->om,dev_rsp.value,strlen(dev_rsp.value));
				}else{
					rc = os_mbuf_append(ctxt->om,nullptr,0);
				}
				memset(&dev_rsp,0,sizeof(DeviceCommand));
			return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
			}
    	break;
		case BLE_GATT_ACCESS_OP_WRITE_CHR:
			if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
				struct os_mbuf *om = ctxt->om;
				int len = OS_MBUF_PKTLEN(om);
                ESP_LOGI(tag,"Characteristic-%d write :- conn_handle=%d attr_handle=%d",(type == BTCharType::CHAR_1)?1:2,conn_handle, attr_handle);
				memset(&rx_cmd,0,sizeof(DeviceCommand));
                if((rc = ble_hs_mbuf_to_flat(om,rx_cmd.value,len,NULL)) != 0){
					ESP_LOGE(tag,"ble_hs_mbuf_to_flat:failed :%d",rc);
					return BLE_ATT_ERR_UNLIKELY;
				}
                rx_cmd.char_type = type;
                rx_cmd.time = s_instance.clk->get_current_time_in_ms();
                ESP_LOGI(tag, "BLE Command: \"%s\"",rx_cmd.value);
				auto q = s_instance.cmd->getQueHandle();
				if(xQueueSend(q,&rx_cmd,pdMS_TO_TICKS(100))!= pdTRUE){
					ESP_LOGE(tag, "Command Enqueue Full!!");
				}
			}
    	break;
	}
	return 0;
}

void BLEManager::gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg){
	char buf[BLE_UUID_STR_LEN];
	switch (ctxt->op) {
		case BLE_GATT_REGISTER_OP_SVC:
			ESP_LOGI(tag, "registered service %s with handle=%d",ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),ctxt->svc.handle);
			break;
		case BLE_GATT_REGISTER_OP_CHR:
			ESP_LOGI(tag, "registering characteristic %s with def_handle=%d val_handle=%d",ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
						ctxt->chr.def_handle,ctxt->chr.val_handle);
			break;
		default:
			assert(0);
			break;
	}
}

int BLEManager::gatt_svr_init(void){
	int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();
    // ble_svc_ans_init();
	
	/* GATT Service Creation */
	memset(&gatt_service[0],0,sizeof(ble_gatt_svc_def));
	gatt_chrs = (ble_gatt_chr_def *)malloc(sizeof(*gatt_chrs) * 3);
	if (!gatt_chrs) {
        ESP_LOGE(tag,"Couldn't allocate memory for characteristics");
        return -1;
    }
	memset(gatt_chrs, 0, sizeof(*gatt_chrs) * 3);
	/* Characteristics :- 1 */
	gatt_chrs[0].uuid = &ble_char1_uuid.u; 
	gatt_chrs[0].access_cb = &BLEManager::gatt_svc_access;
	gatt_chrs[0].flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE;
	gatt_chrs[0].val_handle = &gatt_svr_chr_val_handle;
	/* Characteristics :- 2 */
	gatt_chrs[1].uuid = &ble_char2_uuid.u; 
	gatt_chrs[1].access_cb = &BLEManager::gatt_svc_access;
	gatt_chrs[1].flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE;
	gatt_chrs[1].val_handle = &gatt_svr_chr_val_handle;
	/* Service Description*/
	gatt_service[0].type = BLE_GATT_SVC_TYPE_PRIMARY;
	gatt_service[0].uuid = &app_service_uuid.u;
	gatt_service[0].characteristics = gatt_chrs;
	
    rc = ble_gatts_count_cfg(gatt_service);
    if (rc != 0) {
        return rc;
    }
	rc = ble_gatts_add_svcs(gatt_service);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

void BLEManager::bleprph_advertise(void){
	struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    const char *name;
    int rc;
	/**
     *  Set the advertisement data included in our advertisements:
     *     o Flags (indicates advertisement type and other general info).
     *     o Advertising tx power.
     *     o Device name.
     *     o 16-bit service UUIDs (alert notifications).
     */
	memset(&fields, 0, sizeof fields);
	/* Advertise two flags:
     *     o Discoverability in forthcoming advertisement (general)
     *     o BLE-only (BR/EDR unsupported).
     */
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
	/* Indicate that the TX power level field should be included; have the
     * stack fill this value automatically.  This is done by assigning the
     * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
     */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
	name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;
	fields.uuids128 = &app_service_uuid;
	fields.num_uuids128 = 1;
	fields.uuids128_is_complete = 1;
	
	if((rc = ble_gap_adv_set_fields(&fields))!=0){
		ESP_LOGE(tag,"error setting advertisement data; rc=%d", rc);
		return;
	}
	/* Begin advertising. */
    memset(&adv_params, 0, sizeof adv_params);
	adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
	if((rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER,&adv_params, &BLEManager::bleprph_gap_event, NULL))!=0){
		ESP_LOGE(tag,"error enabling advertisement data; rc=%d", rc);
		return;
	}
}


int BLEManager::bleprph_gap_event(struct ble_gap_event *event, void *arg){
	int rc;
	struct ble_gap_conn_desc desc;
	switch (event->type) {
		case BLE_GAP_EVENT_CONNECT:
		if(event->connect.status == 0){
			ESP_LOGI(tag, "--CONNECTED--");	
		}else{
			ESP_LOGI(tag, "Failed to Connect");	
		}
		break;
		case BLE_GAP_EVENT_DISCONNECT:
			ESP_LOGE(tag, "disconnect; reason=%d ", event->disconnect.reason);
			s_instance.bleprph_advertise();
		break;
		case BLE_GAP_EVENT_LINK_ESTAB:
			if(event->link_estab.status == 0){
				ESP_LOGI(tag,"connection Established");
				rc = ble_gap_conn_find(event->link_estab.conn_handle, &desc);
				assert(rc == 0);
			}else{
				ESP_LOGW(tag,"connection failed");
				s_instance.bleprph_advertise();
			}
		break;
		case BLE_GAP_EVENT_CONN_UPDATE:
			/* The central has updated the connection parameters. */
			ESP_LOGI(tag, "connection updated; status=%d",event->conn_update.status);
			rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
        	assert(rc == 0);
		break;
		case BLE_GAP_EVENT_ADV_COMPLETE:
			ESP_LOGI(tag, "advertise complete; reason=%d",event->adv_complete.reason);
			// s_instance.bleprph_advertise();
		break;
		case BLE_GAP_EVENT_ENC_CHANGE:
        	/* Encryption has been enabled or disabled for this connection. */
        	ESP_LOGI(tag,  "encryption change event; status=%d ",event->enc_change.status);
		break;
		case BLE_GAP_EVENT_NOTIFY_TX:
        	ESP_LOGI(tag, "notify_tx event; conn_handle=%d attr_handle=%d status=%d is_indication=%d",
                    event->notify_tx.conn_handle,event->notify_tx.attr_handle,event->notify_tx.status,event->notify_tx.indication);
		break;
		case BLE_GAP_EVENT_SUBSCRIBE:
        	ESP_LOGI(tag, "subscribe event; conn_handle=%d attr_handle=%d reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
                    event->subscribe.conn_handle,event->subscribe.attr_handle,event->subscribe.reason,event->subscribe.prev_notify,
                    event->subscribe.cur_notify,event->subscribe.prev_indicate,event->subscribe.cur_indicate);
		break;
		case BLE_GAP_EVENT_MTU:
        	ESP_LOGI(tag, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",event->mtu.conn_handle,event->mtu.channel_id,event->mtu.value);
		break;
		case BLE_GAP_EVENT_REPEAT_PAIRING:
			/* We already have a bond with the peer, but it is attempting to
			* establish a new secure link.  This app sacrifices security for
			* convenience: just throw away the old bond and accept the new link.
			*/

			/* Delete the old bond. */
			rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
			assert(rc == 0);
			ble_store_util_delete_peer(&desc.peer_id_addr);

			/* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
			* continue with the pairing operation.
			*/
			return BLE_GAP_REPEAT_PAIRING_RETRY;
		break;
		case BLE_GAP_EVENT_PASSKEY_ACTION:
			ESP_LOGI(tag, "PASSKEY_ACTION_EVENT started");
		break;
		case BLE_GAP_EVENT_AUTHORIZE:
        	ESP_LOGI(tag, "authorize event: conn_handle=%d attr_handle=%d is_read=%d",
                    event->authorize.conn_handle,
                    event->authorize.attr_handle,
                    event->authorize.is_read);

        	/* The default behaviour for the event is to reject authorize request */
        	event->authorize.out_response = BLE_GAP_AUTHORIZE_REJECT;
		break;
		default:
		ESP_LOGW(tag, "Unhandled gap event:%u",event->type);
		break;
	}
	return 0;
}

void BLEManager::bleprph_on_reset(int reason){
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}


void BLEManager::bleprph_on_sync(void){
	int rc;
	rc = ble_hs_util_ensure_addr(0);
	assert(rc == 0);

	/* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* Printing ADDR */
    uint8_t addr_val[6] = {0};
    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);
	ESP_LOGI(tag, "Device Address: ");
    print_addr(addr_val);
	s_instance.bleprph_advertise();
}

void bleprph_host_task(void *param){
	ESP_LOGI(tag, "BLE Host Task Started");
	/* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();
	nimble_port_freertos_deinit();
}

void BLEManager::initialize(void){
	int rc;
	esp_err_t ret;
	
	if((ret = nimble_port_init()) != ESP_OK){
		ESP_LOGE(tag, "Failed to init nimble %d ", ret);
		return;
	}
	

	ble_hs_cfg.reset_cb = &BLEManager::bleprph_on_reset;
	ble_hs_cfg.sync_cb = &BLEManager::bleprph_on_sync;
	ble_hs_cfg.gatts_register_cb = &BLEManager::gatt_svr_register_cb;
	ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
	/**
	 * 0 : BLE_SM_IO_CAP_DISP_ONLY     :- "DISPLAY ONLY"
	 * 1 : BLE_SM_IO_CAP_DISP_YES_NO   :- "DISPLAY YESNO"
	 * 2 : BLE_SM_IO_CAP_KEYBOARD_ONLY :- "KEYBOARD ONLY"
	 * 3 : BLE_SM_IO_CAP_NO_IO		   :- "Just works"	
	 * 4 : BLE_SM_IO_CAP_KEYBOARD_DISP :- "Both KEYBOARD & DISPLAY"
	 */
	ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO;
	/* Bonding*/
	ble_hs_cfg.sm_bonding = 1;
    // /* Enable the appropriate bit masks to make sure the keys
    //  * that are needed are exchanged
    //  */
    ble_hs_cfg.sm_our_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC;
    ble_hs_cfg.sm_their_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC;
	ble_hs_cfg.sm_mitm = 1;
	ble_hs_cfg.sm_sc = 1;//Use this option to enable/disable Security Manager Secure Connection 4.2 feature.
	/*Use this option to enable resolving peer's address.*/
	 /* Stores the IRK */
	ble_hs_cfg.sm_our_key_dist |= BLE_SM_PAIR_KEY_DIST_ID;
	ble_hs_cfg.sm_their_key_dist |= BLE_SM_PAIR_KEY_DIST_ID;

	rc = gatt_svr_init();
    assert(rc == 0);

    /* Set the default device name. */
    rc = ble_svc_gap_device_name_set("MPJ");
    assert(rc == 0);


	cmd->registerCmdResponse([this](const DeviceCommand& rsp){
		this->processedCommandResponseCallback(rsp);
	});
	

    /* XXX Need to have template for store */
    // ble_store_config_init();
    nimble_port_freertos_init(bleprph_host_task);
}

