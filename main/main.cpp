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

	/*Start BLE */
	BLEManager *ble;
	ble = BLEManager::getInstance();
	ble->initialize();

}