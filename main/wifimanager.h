#ifndef WIFIMANAGER_H_
#define WIFIMANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

enum class WiFiIPMode : uint8_t {DHCP,STATIC};

struct StaticIpConfig{
    esp_ip4_addr_t ip;
    esp_ip4_addr_t gateway;
    esp_ip4_addr_t netmask;
};

class WiFiManager{
    private:
        WiFiManager();
        ~WiFiManager();
        
        WiFiManager(const WiFiManager&) = delete;
        WiFiManager& operator = (const WiFiManager&) = delete;

        static WiFiManager s_instance;
        std::unique_ptr<TimerEvent> m_timerevent;

        WiFiIPMode ip_mode;
        struct StaticIpConfig static_ip_cfg;
        wifi_config_t wifi_sta_cfg;
        std::function<void(esp_event_base_t,int32_t,void*)> user_callback;
        std::mutex m_mutex;
        esp_netif_t* sta_net_if;
        bool is_reconn_required;

        void timeout_event_from_disconnection();
        
        static void event_handler(void *arg,esp_event_base_t event_base,int32_t event_id,void* event_data);
        
    public:
        static WiFiManager* getInstance();   
        /**
         * @brief Initialize WiFi in STA mode, configure IP mode in DHCP/Static mode
         * 
         * @param mode DHCP/Static
         * @param cfg Required if mode = static
         * @return esp_err_t ESP_OK on succes
         */
        esp_err_t init(WiFiIPMode mode = WiFiIPMode::DHCP,const StaticIpConfig *cfg = nullptr);

        void set_credentials(const char* ssid,const char *pswd);

        esp_err_t connect();

        esp_err_t disconnect(bool reconn_status);

        void register_event_callback(std::function<void(esp_event_base_t,int32_t,void*)> cb);

};


#ifdef __cplusplus
}
#endif

#endif /* !WIFIMANAGER_H_ */
