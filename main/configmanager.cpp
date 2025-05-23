#include "includes.h"

static const char * tag = "CFG";

ConfigManager ConfigManager::s_instance;

ConfigManager::ConfigManager(){
    
    //Load application configuration with default values
    settings.push_back(Config_Params_s((void *)&configs.ssid,32,STRING_TYPE,CFG_TAG_WIFI_SSID,"0"));
    settings.push_back(Config_Params_s((void *)&configs.psk,64,STRING_TYPE,CFG_TAG_WIFI_PSWD,"0"));
    settings.push_back(Config_Params_s((void *)&configs.isstatic,sizeof(int),INT_TYPE,CFG_TAG_WIFI_IP_TYPE,"0"));//0:DHCP 1:Static  
    settings.push_back(Config_Params_s((void *)&configs.staticip,MAX_IP_SIZE,STRING_TYPE,CFG_TAG_WIFI_STATIC_IP,"192.168.9.3"));
	settings.push_back(Config_Params_s((void *)&configs.staticnetmask,MAX_IP_SIZE,STRING_TYPE,CFG_TAG_WIFI_STATIC_NM,"255.255.255.0"));
	settings.push_back(Config_Params_s((void *)&configs.staticgtw,MAX_IP_SIZE,STRING_TYPE,CFG_TAG_WIFI_STATIC_GW,"192.168.9.1"));
    settings.push_back(Config_Params_s((void *)&configs.mqip,MAX_URL_SIZE,STRING_TYPE,CFG_TAG_MQTT_IP,"192.168.9.10"));
	settings.push_back(Config_Params_s((void *)&configs.mqport,sizeof(int),INT_TYPE,CFG_TAG_MQTT_PORT,"1883"));
    settings.push_back(Config_Params_s(nullptr, 0, 0, "", ""));
    esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		// NVS partition was truncated and needs to be erased
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	err = nvs_open("app_configs", NVS_READWRITE, &m_handle);
	if (err != ESP_OK) {
		ESP_LOGE(tag,"Error (%s) opening NVS handle!", esp_err_to_name(err));
	}
    app_desc=esp_app_get_description();
	m_mutex = xSemaphoreCreateMutex();
	xSemaphoreGive(m_mutex);
}

ConfigManager::~ConfigManager(){
    nvs_close(m_handle);
}

ConfigManager *ConfigManager::getInstance(){
    return &s_instance;
}

esp_err_t ConfigManager::write_config_int(const char* key, int value){
	esp_err_t ret = ESP_OK;
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    if((ret = nvs_set_i32(m_handle, key, value))!=ESP_OK){
        ESP_LOGE(tag,"Error (%s) writing int to NVS!", esp_err_to_name(ret));
    }else if((ret = nvs_commit(m_handle))!=ESP_OK){
        ESP_LOGE(tag,"Error (%s) committing int to NVS!", esp_err_to_name(ret));
    }
    xSemaphoreGive(m_mutex);
    return ret;
}

esp_err_t ConfigManager::write_config_string(const char* key, const char* value){
	esp_err_t ret = ESP_OK;
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    if((ret = nvs_set_str(m_handle, key, value))!=ESP_OK){
        ESP_LOGE(tag,"Error (%s) writing string to NVS!", esp_err_to_name(ret));
    }else if((ret = nvs_commit(m_handle))!=ESP_OK){
        ESP_LOGE(tag,"Error (%s) committing string to NVS!", esp_err_to_name(ret));
    }
    xSemaphoreGive(m_mutex);
    return ret;
}

esp_err_t ConfigManager::write_config_blob(const char* key, const void* value, size_t length){
    esp_err_t ret = ESP_OK;
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    if((ret = nvs_set_blob(m_handle, key, value,length))!=ESP_OK){
        ESP_LOGE(tag,"Error (%s) writing blob to NVS!", esp_err_to_name(ret));
    }else if((ret = nvs_commit(m_handle))!=ESP_OK){
        ESP_LOGE(tag,"Error (%s) committing blob to NVS!", esp_err_to_name(ret));
    }
    xSemaphoreGive(m_mutex);
    return ret;
}

esp_err_t ConfigManager::read_config_int(const char* key, int& value){
    esp_err_t ret = ESP_OK;
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    if((ret = nvs_get_i32(m_handle, key,(int32_t *)&value))!=ESP_OK){
        ESP_LOGE(tag,"Error (%s) reading int from NVS!", esp_err_to_name(ret));
    }
    xSemaphoreGive(m_mutex);
    return ret;
}

esp_err_t ConfigManager::read_config_string(const char* key, char* value, size_t max_length){
    esp_err_t ret = ESP_OK;
    size_t req_size;
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    if((ret = nvs_get_str(m_handle, key,NULL,&req_size))!=ESP_OK){
        ESP_LOGE(tag,"Error (%s) reading string size from NVS!", esp_err_to_name(ret));
    }else if(req_size>max_length){
        ESP_LOGE(tag,"Error: string size is too big!");
        ret = ESP_FAIL;
    }else if((ret = nvs_get_str(m_handle, key, value, &req_size))!=ESP_OK){
        ESP_LOGE(tag,"Error (%s) reading string from NVS!", esp_err_to_name(ret));
    }
    xSemaphoreGive(m_mutex);
    return ret;
}
esp_err_t ConfigManager::read_config_blob(const char* key, void* value, size_t max_length){
    esp_err_t ret = ESP_OK;
    size_t req_size;
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    if((ret = nvs_get_blob(m_handle, key,NULL,&req_size))!=ESP_OK){
        ESP_LOGE(tag,"Error (%s) reading blob size from NVS!", esp_err_to_name(ret));
    }else if(req_size>max_length){
        ESP_LOGE(tag,"Error: string size is too big!");
        ret = ESP_FAIL;
    }else if((ret = nvs_get_blob(m_handle, key, value, &req_size))!=ESP_OK){
        ESP_LOGE(tag,"Error (%s) reading blob from NVS!", esp_err_to_name(ret));
    }
    xSemaphoreGive(m_mutex);
    return ret;
}

esp_err_t ConfigManager::erase_config_key(const char* key){
    esp_err_t ret = ESP_OK;
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    if((ret = nvs_erase_key(m_handle, key))!=ESP_OK){
        ESP_LOGE(tag,"Error (%s) erasing key from NVS!", esp_err_to_name(ret));
    }else if((ret = nvs_commit(m_handle))!=ESP_OK){
        ESP_LOGE(tag,"Error (%s) committing key to NVS!", esp_err_to_name(ret));
    }
    xSemaphoreGive(m_mutex);
    return ret;   
}

esp_err_t ConfigManager::erase_all_configs(){
    esp_err_t ret = ESP_OK;
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    if((ret = nvs_erase_all(m_handle))!=ESP_OK){
        ESP_LOGE(tag,"Error (%s) erasing all from NVS!", esp_err_to_name(ret));
    }else if((ret = nvs_commit(m_handle))!=ESP_OK){
        ESP_LOGE(tag,"Error (%s) committing clear config to NVS!", esp_err_to_name(ret));
    }
    xSemaphoreGive(m_mutex);
    return ret;   
}

void ConfigManager::read_application_configurations(){
    ESP_LOGW(tag,"Application Version:%s",app_desc->version);
    for (auto& setting : settings) {
        switch(setting.var_type){
            case STRING_TYPE:
                {
                    char temp[setting.var_len];
                    if(read_config_string(setting.param,temp,sizeof(temp))==ESP_OK){
                        memcpy(setting.var_addr,temp,setting.var_len);
                        ESP_LOGI(tag,"%s = %s",setting.param,temp);
                    }else{
                        ESP_LOGW(tag,"couldn't find %s! adding %s = %s",setting.param,setting.param,setting.def_value);
                        write_config_string(setting.param,setting.def_value);
                        memcpy(setting.var_addr,setting.def_value,setting.var_len);
                    } 
                }
                break;
            case INT_TYPE:
                {
                    int temp = 0;
                    if(read_config_int(setting.param,temp)==ESP_OK){
                        memcpy(setting.var_addr,&temp,setting.var_len);
                        ESP_LOGI(tag,"%s = %d",setting.param,temp);
                    }else{
                        temp = atoi(setting.def_value);
                        ESP_LOGW(tag,"couldn't find %s! adding %s = %d",setting.param,setting.param,temp);
                        write_config_int(setting.param,temp);
                        memcpy(setting.var_addr,&temp,setting.var_len);
                    }
                }
                break;
            case BLOB_TYPE:
                ESP_LOGW(tag,"Not Implemented!!!"); 
            break;
        }
    }
}

esp_err_t ConfigManager::update_configuration_wifi(char *ssid,char *psk,int net_type,char *s_ip,char *s_nm,char *s_gw){
    uint8_t status = 0;
    if(write_config_string(CFG_TAG_WIFI_SSID,(const char *)ssid)==ESP_OK) status = 0x01;
    if(write_config_string(CFG_TAG_WIFI_PSWD,(const char *)psk)==ESP_OK) status |= 0x02;
    if(write_config_int(CFG_TAG_WIFI_IP_TYPE,net_type)==ESP_OK) status |= 0x04;
    if(net_type == 0){//DHCP
        return (status==0x07)?ESP_OK:ESP_FAIL;
    }else{
        if(write_config_string(CFG_TAG_WIFI_STATIC_IP,(const char *)s_ip)==ESP_OK) status |= 0x08;
        if(write_config_string(CFG_TAG_WIFI_STATIC_NM,(const char *)s_nm)==ESP_OK) status |= 0x10;
        if(write_config_string(CFG_TAG_WIFI_STATIC_GW,(const char *)s_gw)==ESP_OK) status |= 0x20;
        return (status==0x3F)?ESP_OK:ESP_FAIL;
    }
}

esp_err_t ConfigManager::update_configuration_mqtt(char *ip,int port){
    uint8_t status = 0;
    if(write_config_string(CFG_TAG_MQTT_IP,(const char *)ip)==ESP_OK) status |= 0x01;
    if(write_config_int(CFG_TAG_MQTT_PORT,port)==ESP_OK) status |= 0x02;
    return (status==0x03)?ESP_OK:ESP_FAIL;
}