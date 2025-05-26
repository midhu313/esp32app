#include "includes.h"

static const char * tag = "MQTT";

static const esp_mqtt_topic_t subscribe_topic_list[]={
    {.filter = "esp/test/topic1",.qos = 0},
    {.filter = "esp/test/topic2",.qos = 0},
};

static char broker_url[100];

MQTTClient MQTTClient::s_instance;

MQTTClient::MQTTClient():m_client(nullptr),m_wifi_state(ConnState::Unknown),m_mqtt_state(ConnState::Unknown){
    wifimngr = WiFiManager::getInstance();
    clk = ClockManager::getInstance();
    cmd = CmdManager::getInstance();
}

MQTTClient::~MQTTClient(){
    stop();
    destroy();
}

MQTTClient* MQTTClient::getInstance(){
    return &s_instance;
}

void MQTTClient::initialize(char *ip,int port){
    memset(broker_url,0,100);
    sprintf((char *)broker_url,"mqtt://%s:%d",ip,port);
    esp_mqtt_client_config_t mq_config = {};
    mq_config.broker.address.uri = broker_url;
    mq_config.credentials.client_id = NULL;
    m_client = esp_mqtt_client_init(&mq_config);
    if(!m_client){
        ESP_LOGE(tag,"Failed to initialize MQTT Client");
        return;
    }
    esp_mqtt_client_register_event(m_client,(esp_mqtt_event_id_t)ESP_EVENT_ANY_ID,mqtt_event_handler,this);
    /*Register for WiFi event callbacks*/
    wifimngr->register_event_callback([this](esp_event_base_t event_base,int32_t event_id,void* event_data){
        this->wifi_event_handler(event_base,event_id,event_data);
    });
    /*Register one shot timer for MQTT connection*/
    m_reconnect_timer = std::make_unique<TimerEvent>();
    m_reconnect_timer->registerOneShotTimeoutTask([this](){
        this->timeout_event_handle_mqtt_connect();
    });
    /*Register Command response callback*/
    cmd->registerCmdResponse([this](const DeviceCommand& rsp){
		this->processedCommandResponseCallback(rsp);
	});
}

esp_err_t MQTTClient::start(){
    esp_err_t status = ESP_FAIL;
    if(m_client)
        status = esp_mqtt_client_start(m_client);
    if(status != ESP_OK)
        ESP_LOGE(tag,"Failed to start MQTT Client");
    return status;
}

esp_err_t MQTTClient::stop(){
    esp_err_t status = ESP_FAIL;
    if(m_client)
        status = esp_mqtt_client_stop(m_client);
    if(status != ESP_OK){
        ESP_LOGE(tag,"Failed to stop MQTT Client");
        m_mqtt_state = ConnState::Disconnected;
        std::function<void(esp_mqtt_event_id_t)> cb_copy;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            cb_copy = user_event_callback;
        }
        if(cb_copy){
            cb_copy(MQTT_EVENT_DISCONNECTED);
        }
    }
        
    return status;
}

esp_err_t MQTTClient::destroy(){
    esp_err_t status = ESP_FAIL;
    if(m_client){
        status = esp_mqtt_client_destroy(m_client);
        m_client = nullptr;
    }
    if(status != ESP_OK)
        ESP_LOGE(tag,"Failed to destroy MQTT Client");
    return status;
}

int MQTTClient::publish(const char * topic, const char * payload, int qos, bool retain){
    if(!m_client){
        ESP_LOGE(tag,"Publish called before client initialize!");
        return ESP_FAIL;
    }
    return esp_mqtt_client_publish(m_client,topic,payload,strlen(payload),qos,retain);
}

void MQTTClient::subscribe_to_topics(){
    int status = esp_mqtt_client_subscribe_multiple(m_client,subscribe_topic_list,2);
    if(status<0)
        ESP_LOGE(tag,"Subscribing topics : %s",(status==-1)?"Failed":" Full outbox");
    else
        ESP_LOGI(tag,"Successfully subscribed to topics");
}

void MQTTClient::mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data){
    MQTTClient *self = static_cast<MQTTClient *>(handler_args);
    esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);
    switch(event->event_id){
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(tag, "Event:- Connected");
            self->subscribe_to_topics();
            self->m_mqtt_state = ConnState::Connected;            
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(tag, "Event:- Disconnected");
            self->m_mqtt_state = ConnState::Disconnected;            
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(tag, "Event:- Subscribed to msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGW(tag, "Event:- Unsubscribed msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(tag, "Event:- Published msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(tag, "Event:- Data Received topic=%.*s len=%d payload=%.*s", event->topic_len, event->topic, event->data_len,event->data_len,event->data);
            {
                DeviceCommand dev_cmd;
                dev_cmd.source = CmdSource::MQTT;
                dev_cmd.time = self->clk->get_current_time_in_ms();
                size_t copy_len = std::min<size_t>(event->topic_len, MQTT_TOPIC_MAX_LENGTH - 1);
                std::memcpy(dev_cmd.mqtt_topic, event->topic, copy_len);
                dev_cmd.mqtt_topic[copy_len] = '\0';
                copy_len = std::min<size_t>(event->data_len, MQTT_PAYLOAD_MAX_LENGTH - 1);
                std::memcpy(dev_cmd.value, event->data, copy_len);
                dev_cmd.value[copy_len] = '\0';
                auto q = self->cmd->getQueHandle();
				if(xQueueSend(q,&dev_cmd,pdMS_TO_TICKS(100))!= pdTRUE){
					ESP_LOGE(tag, "Command Enqueue Full!!");
				}
            }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(tag, "Event:- Error");
            if (event->error_handle) {
                if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                    ESP_LOGE(tag, "transport errno %d: %s",
                             event->error_handle->esp_transport_sock_errno,
                             strerror(event->error_handle->esp_transport_sock_errno));
                }
            }
            break;
        default:
            ESP_LOGI(tag, "Unhandled MQTT Event, id=%d", event->event_id);
            break;
    }

    std::function<void(esp_mqtt_event_id_t)> cb_copy;
    {
        std::lock_guard<std::mutex> lock(self->m_mutex);
        cb_copy = self->user_event_callback;
    }
    if(cb_copy){
        cb_copy(event->event_id);
    }
}

void MQTTClient::register_event_callback(std::function<void(esp_mqtt_event_id_t)> cb){
    std::lock_guard<std::mutex> lock(m_mutex);
    user_event_callback = std::move(cb);
}

void MQTTClient::wifi_event_handler(esp_event_base_t event_base,int32_t event_id,void* event_data){
    if((event_base == WIFI_EVENT)&&(event_id == WIFI_EVENT_STA_DISCONNECTED)){
        ESP_LOGI(tag,"WiFi Disconnected");
        m_wifi_state = ConnState::Disconnected;
        if(m_mqtt_state == ConnState::Connected){
            stop();
        }
    }else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ESP_LOGI(tag,"WiFi Connected & Got IP Address");
        m_wifi_state = ConnState::Connected;
        m_reconnect_timer->startOneShotTimeoutTask(10000);
    }
}

void MQTTClient::timeout_event_handle_mqtt_connect(){
    start();
}

void MQTTClient::processedCommandResponseCallback(const DeviceCommand& rsp){
    if(rsp.source == CmdSource::MQTT){
        ESP_LOGI(tag,"Response: %s",rsp.value);
        publish("esp/test/pub1",rsp.value);
    }
}