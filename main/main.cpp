#include "includes.h"

static const char* tag = "main";

extern "C"{
	void app_main();
}

void app_main(void){
    esp_err_t ret;
    /* Initialize NVS. */
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK( ret );
    ESP_LOGW(tag, "Free heap size: %lu bytes", esp_get_free_heap_size());
	TaskManager *tm;
	tm = TaskManager::getInstance();
	tm->initialize();

	/*Read Application Configurations*/
	ConfigManager *cfg;
	cfg = ConfigManager::getInstance();
	cfg->read_application_configurations();
	/*Start BLE */
	BLEManager *ble;
	ble = BLEManager::getInstance();
	ble->initialize();
	/*Start Wi-Fi Manager*/
	if((strlen(cfg->configs.ssid)>2)&&(strlen(cfg->configs.psk)>5)){
		WiFiManager *wifi;
		wifi = WiFiManager::getInstance();
		wifi->set_credentials(cfg->configs.ssid,cfg->configs.psk);
		if(cfg->configs.isstatic){
			// StaticIpConfig sip_cfg = {
			// 	.gateway = 
			// };
		}else{
			wifi->init();
		}
		wifi->connect();
		tm->add_task(TaskID::NTP_UPDATE);
		MQTTClient *mq_client;
		mq_client = MQTTClient::getInstance();
		mq_client->initialize(cfg->configs.mqip,cfg->configs.mqport);
		// tm->update_mqtt_req(true);
	}
}