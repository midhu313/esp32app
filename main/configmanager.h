#ifndef CONFIGMANAGER_H_
#define CONFIGMANAGER_H_

/*Variable Data Types*/
#define STRING_TYPE 1
#define INT_TYPE    2
#define BLOB_TYPE   3

// Configuration tag constants
#define CFG_TAG_WIFI_SSID           "ssid"
#define CFG_TAG_WIFI_PSWD           "psk"
#define CFG_TAG_WIFI_IP_TYPE        "iptype"
#define CFG_TAG_WIFI_STATIC_IP      "sip"
#define CFG_TAG_WIFI_STATIC_NM      "snetmask"
#define CFG_TAG_WIFI_STATIC_GW      "sgateway"
#define CFG_TAG_MQTT_IP             "mqttip"
#define CFG_TAG_MQTT_PORT           "mqttport"

typedef struct{
    char ssid[32];
    char psk[64];
    int  isstatic;//0:DHCP 1:Static  
    char staticip[MAX_IP_SIZE];
	char staticnetmask[MAX_IP_SIZE];
	char staticgtw[MAX_IP_SIZE];
    char mqip[MAX_URL_SIZE];
	int  mqport;
}App_Configurations_s;

struct Config_Params_s {
    void* var_addr;
    int var_len;
    int var_type;
    const char* param;
    const char* def_value;

    // Constructor
    Config_Params_s(void* addr, int len, int type, const char* param, const char* def_value)
        : var_addr(addr), var_len(len), var_type(type), param(param), def_value(def_value) {}
};

class ConfigManager{
    private:
        static ConfigManager s_instance;

        ConfigManager();
        ~ConfigManager();
    
        ConfigManager(const ConfigManager&) = delete;
        ConfigManager& operator = (const ConfigManager&) = delete;
       
        std::vector<Config_Params_s> settings;
        nvs_handle_t m_handle;
        SemaphoreHandle_t m_mutex;

        esp_err_t write_config_int(const char* key, int value);
        esp_err_t write_config_string(const char* key, const char* value);
        esp_err_t write_config_blob(const char* key, const void* value, size_t length);
        esp_err_t read_config_int(const char* key, int& value);
        esp_err_t read_config_string(const char* key, char* value, size_t max_length);
        esp_err_t read_config_blob(const char* key, void* value, size_t max_length);
        esp_err_t erase_config_key(const char* key);
        

    public:
        App_Configurations_s configs;
        static ConfigManager* getInstance();

        const esp_app_desc_t* app_desc;

        esp_err_t erase_all_configs();
        void read_application_configurations();
        /**
         * @brief Update Configuration for the Wi-Fi STA mode
         * 
         * @param ssid SSID
         * @param psk  Password
         * @param net_type 0:DHCP 1:Static
         * @param s_ip static ip
         * @param s_nm netmask
         * @param s_gw gateway
         * @return esp_err_t ESP_OK on success
         */
        esp_err_t update_configuration_wifi(char *ssid,char *psk,int net_type,char *s_ip,char *s_nm,char *s_gw);
        /**
         * @brief Update Configuration for the MQTT Client
         * 
         * @param ip IP/URL of the MQTT broker
         * @param port Port of the MQTT broker
         * @return esp_err_t ESP_OK on success
         */
        esp_err_t update_configuration_mqtt(char *ip,int port);

};

#endif /* !CONFIGMANAGER_H_ */

