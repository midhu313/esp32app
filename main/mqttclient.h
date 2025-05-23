#ifndef MQTTCLIENT_H_
#define MQTTCLIENT_H_
//mosquitto_pub -h 45.79.122.60 -t "dgps/command/stod" -m "{\"cmd\":\"get,survey\"}"
class MQTTClient{
    public:
        // Topic callback signature: topic, data pointer, data length
        using MessageHandler = std::function<void(const char * topic, const uint8_t* data, size_t len)>;

        static MQTTClient* getInstance();

        void initialize(char *ip,int port);
        void start();
        void stop();
        void destroy();

        int publish(const char * topic, const char * payload, int qos = -1, bool retain = false);
        void set_message_handler(MessageHandler handler);
    private:
        static MQTTClient s_instance;
        MQTTClient();
        ~MQTTClient();
        MQTTClient(const MQTTClient&) = delete;
        MQTTClient& operator = (const MQTTClient&) = delete;

        WiFiManager *wifimngr;
        ClockManager *clk;

        esp_mqtt_client_handle_t m_client;
        MessageHandler msg_handler;

        std::unique_ptr<TimerEvent> m_reconnect_timer;
        ConnState m_wifi_state;

        static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);
        void handle_event(esp_mqtt_event_handle_t event);

};

#endif /* !MQTTCLIENT_H_ */
