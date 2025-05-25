#include "includes.h"

static const char * tag  = "Wi-Fi";

WiFiManager WiFiManager::s_instance;

WiFiManager::WiFiManager():ip_mode(WiFiIPMode::DHCP),sta_net_if(nullptr),is_reconn_required(true){

}

WiFiManager::~WiFiManager(){
    esp_wifi_stop();
    esp_wifi_deinit();
    if(sta_net_if){
        esp_netif_destroy(sta_net_if);
    }
    esp_event_loop_delete_default();
}

WiFiManager* WiFiManager::getInstance(){
    return &s_instance;
}

esp_err_t WiFiManager::init(WiFiIPMode mode,const StaticIpConfig *cfg){
    ip_mode = mode;
    if(mode == WiFiIPMode::STATIC){
        static_ip_cfg = *cfg;
    }
 
    // Initialize TCP/IP stack and default event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // Create default STA netif
    sta_net_if = esp_netif_create_default_wifi_sta();
    if(!sta_net_if){
        ESP_LOGE(tag, "Failed to create default STA netif");
        return ESP_FAIL;
    }
    // Initialize WiFi driver
    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));
    // Register internal event handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID,&WiFiManager::event_handler,this,nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,IP_EVENT_STA_GOT_IP,&WiFiManager::event_handler,this,nullptr));
    // Configure IP mode
    if(ip_mode == WiFiIPMode::STATIC){
        // stop DHCP Client
        ESP_ERROR_CHECK(esp_netif_dhcpc_stop(sta_net_if));
        esp_netif_ip_info_t info;
        std::memset(&info,0,sizeof(info));
        info.ip = static_ip_cfg.ip;
        info.gw = static_ip_cfg.gateway;
        info.netmask = static_ip_cfg.netmask;
        ESP_ERROR_CHECK(esp_netif_set_ip_info(sta_net_if,&info));
    }   
    // Configure WiFi as station
    wifi_config_t wifiCfg{};
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiCfg));
    //Register Timer Event callback
    m_timerevent = std::make_unique<TimerEvent>();
    m_timerevent->registerOneShotTimeoutTask([this](){
		this->timeout_event_from_disconnection();
	});
    // Start WiFi driver
    return esp_wifi_start();
}

void WiFiManager::set_credentials(const char* ssid,const char *pswd){
    std::memset(&wifi_sta_cfg,0,sizeof(wifi_sta_cfg));
    std::strncpy((char *)wifi_sta_cfg.sta.ssid,ssid,sizeof(wifi_sta_cfg.sta.ssid));
    std::strncpy((char *)wifi_sta_cfg.sta.password,pswd,sizeof(wifi_sta_cfg.sta.password));
    wifi_sta_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_sta_cfg.sta.pmf_cfg.capable = true;
    wifi_sta_cfg.sta.pmf_cfg.required = false;
}

esp_err_t WiFiManager::connect(){
    is_reconn_required = true;
    ESP_LOGI(tag, "Connecting to SSID \"%s\"", wifi_sta_cfg.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_cfg));
    return esp_wifi_connect();
}

esp_err_t WiFiManager::disconnect(bool reconn_status){
    ESP_LOGI(tag, "%s Disconnect command",(reconn_status)?"Normal":"Forced");
    is_reconn_required = reconn_status;
    return esp_wifi_disconnect();
}

void WiFiManager::timeout_event_from_disconnection(){
    ESP_LOGI(tag, "Timeout Event");
    connect();
}

void WiFiManager::register_event_callback(std::function<void(esp_event_base_t,int32_t,void*)> cb){
    std::lock_guard<std::mutex> lock(m_mutex);
    user_callback = std::move(cb);
}

void WiFiManager::event_handler(void *arg,esp_event_base_t event_base,int32_t event_id,void* event_data){
    auto *self = static_cast<WiFiManager*>(arg);
    if(event_base == WIFI_EVENT){
        switch(event_id){
            case WIFI_EVENT_STA_START:
                ESP_LOGI(tag, "WiFi STA Started!");
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(tag, "WiFi STA Connected!");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGW(tag, "WiFi STA Disconnected!");
                if(self->is_reconn_required)
                    self->m_timerevent->startOneShotTimeoutTask(10000);
                break;
            default:
                break;    
        }
    }else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        auto* evt = static_cast<ip_event_got_ip_t*>(event_data);
        ESP_LOGI(tag, "Got IP: " IPSTR, IP2STR(&evt->ip_info.ip));
    }
    // Forward to user callback if present
    std::function<void(esp_event_base_t,int32_t,void*)> cb_copy;
    {
        std::lock_guard<std::mutex> lock(self->m_mutex);
        cb_copy = self->user_callback;
    }
    if (cb_copy) {
        cb_copy(event_base, event_id, event_data);
    }
}