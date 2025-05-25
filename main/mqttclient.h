#ifndef MQTTCLIENT_H_
#define MQTTCLIENT_H_

class MQTTClient{
    public:
        // Topic callback signature: topic, data pointer, data length
        using MessageHandler = std::function<void(const char * topic, const uint8_t* data, size_t len)>;

        static MQTTClient* getInstance();

        void initialize(char *ip,int port);
        esp_err_t start();
        esp_err_t stop();
        esp_err_t destroy();

        int publish(const char * topic, const char * payload, int qos = 0, bool retain = false);
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

        /*
        * @brief Event handler registered to receive MQTT events
        *
        *  This function is called by the MQTT client event loop.
        *
        * @param handler_args user data registered to the event.
        * @param base Event base for the handler(always MQTT Base in this example).
        * @param event_id The id for the received event.
        * @param event_data The data for the event, esp_mqtt_event_handle_t.
        */
        static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

        void handle_event(esp_mqtt_event_handle_t event);

};

#endif /* !MQTTCLIENT_H_ */
