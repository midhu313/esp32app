#include "includes.h"

static const char * tag = "MQTT";

static char broker_url[100];

MQTTClient MQTTClient::s_instance;

MQTTClient::MQTTClient():m_client(nullptr){

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
}
