#include "includes.h"

static const char * tag = "MQTT";

static char broker_url[100];

MQTTClient MQTTClient::s_instance;

MQTTClient::MQTTClient():m_client(nullptr),m_wifi_state(ConnState::Unknown){

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
    if(status != ESP_OK)
        ESP_LOGE(tag,"Failed to stop MQTT Client");
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

void MQTTClient::set_message_handler(MessageHandler handler){
    msg_handler = handler;
}

void MQTTClient::mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data){
    MQTTClient *self = static_cast<MQTTClient *>(handler_args);
    esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);
    self->handle_event(event);
    
}

void handle_event(esp_mqtt_event_handle_t event){
    
    switch(event->event_id){
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(tag, "MQTT_EVENT_CONNECTED");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(tag, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(tag, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(tag, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(tag, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(tag, "MQTT_EVENT_DATA topic=%.*s len=%d", event->topic_len, event->topic, event->data_len);
        
            
            // if(msg_handler) {
                // msg_handler(event->topic,reinterpret_cast<const uint8_t*>(event->data),event->data_len);
            // }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(tag, "MQTT_EVENT_ERROR");
            if (event->error_handle) {
                if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                    ESP_LOGE(tag, "transport errno %d: %s",
                             event->error_handle->esp_transport_sock_errno,
                             strerror(event->error_handle->esp_transport_sock_errno));
                }
            }
            break;
        default:
            ESP_LOGI(tag, "Other MQTT Event, id=%d", event->event_id);
            break;
    }
}