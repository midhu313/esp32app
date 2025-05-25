#ifndef CMDMANAGER_H_
#define CMDMANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

class CmdManager{
    private:
        static CmdManager s_instance;

        CmdManager();
        ~CmdManager();

        CmdManager(const CmdManager&) = delete;
        CmdManager& operator=(const CmdManager&) = delete;    

        static void taskProcessNewCommand(void *args);
        
        static constexpr const char * TASK_NAME = "CmdProcess";
        static constexpr uint32_t STACK_SIZE = 4098;
        static constexpr UBaseType_t PRIORITY = 5;
        
        QueueHandle_t queue_handle;
        
        static StaticTask_t task_buff;
        static StackType_t  task_stack[STACK_SIZE];

        std::mutex m_cb_mutex;
        std::function<void(const DeviceCommand&)> callback;

        ConfigManager *cfg;
        TaskManager *tmngr;

        void sendCommandResponse(const DeviceCommand &rsp);
        
        /**
         * @brief Update configuration for wifi. 
         * 
         *     Command Format: 
         *          - when DHCP:`{"cmd":1,"ssid":"<ssid>","psk":"psk","static":0}`
         *          - when Static: `{"cmd":1,"ssid":"<ssid>","psk":"psk","static":1,"ip":"<ip>","nm":"<nm>","gw":"<gw>"}`
         * 
         * @param object JSON Object
         * @return esp_err_t ESP_OK on success
         */
        esp_err_t configure_wifi_params(cJSON *object);
        /**
         * @brief Update configuration for MQTT Params.
         *        
         *      Command Format: `{"cmd":3,"ip":"<ip>","port":<port>}`         
         * @param object JSON Object 
         * @return esp_err_t ESP_OK on success
         */
        esp_err_t configure_mqtt_params(cJSON *object);

        /**
         * @brief Read application configured and returns in expected JSON format
         * 
         * @param cmd_id Command ID
         * @param value Return String in the JSON Format `{"cmd":cmd_id,"param_name1":"param1_value"...}`
         * @return esp_err_t 
         */
        esp_err_t read_configuration(int cmd_id,char * value);

    public:
        
        static CmdManager *getInstance();
        
        QueueHandle_t getQueHandle() const {return queue_handle;}

        void registerCmdResponse(std::function<void(const DeviceCommand&)> cb);

        void manage_incoming_char1_commands(const struct DeviceCommand &cmd_info);
        void manage_incoming_char2_commands(const struct DeviceCommand &cmd_info);

};

#ifdef __cplusplus
}
#endif

#endif /* !CMDMANAGER_H_ */
