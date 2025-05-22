#ifndef CONFIGMANAGER_H_
#define CONFIGMANAGER_H_

/*Variable Data Types*/
#define STRING_TYPE 1
#define INT_TYPE    2
#define LONG_TYPE   3
#define FLOAT_TYPE  4

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

    public:
        App_Configurations_s configs;
        static ConfigManager* getInstance();

};

#endif /* !CONFIGMANAGER_H_ */

