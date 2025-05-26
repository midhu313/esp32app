#include "includes.h"

static const char * tag = "cmd";

CmdManager CmdManager::s_instance;

StaticTask_t CmdManager::task_buff;
StackType_t  CmdManager::task_stack[CmdManager::STACK_SIZE];

static struct DeviceCommand dev_rsp;

CmdManager::CmdManager(){
    cfg = ConfigManager::getInstance();
    tmngr = TaskManager::getInstance();
    queue_handle = xQueueCreate(5,sizeof(DeviceCommand));
    if(!queue_handle){
        ESP_LOGE(tag,"Failed to create Queue Handle!");
        return;
    }
    TaskHandle_t tsk_handle = xTaskCreateStaticPinnedToCore(
        &CmdManager::taskProcessNewCommand,TASK_NAME,STACK_SIZE,this,PRIORITY,task_stack,&task_buff,1);
    if(!tsk_handle){
        ESP_LOGE(tag,"Failed to create pinned task!");
    }
}

CmdManager::~CmdManager(){
    if(queue_handle)
        vQueueDelete(queue_handle);
}

CmdManager* CmdManager::getInstance(){
    return &s_instance;
}

void CmdManager::registerCmdResponse(std::function<void(const DeviceCommand&)> cb){
    std::lock_guard<std::mutex> lock(m_cb_mutex);
    callback = std::move(cb);
}

void CmdManager::sendCommandResponse(const DeviceCommand &rsp){
    std::function<void(const DeviceCommand&)> cb_copy;
    {
        std::lock_guard<std::mutex> lock(this->m_cb_mutex);
        cb_copy = this->callback;
    }
    if(cb_copy){
        cb_copy(rsp);
    }
}

void CmdManager::taskProcessNewCommand(void *args){
    auto *self = static_cast<CmdManager*>(args);
    struct DeviceCommand cmd;
    while(1){
        /* Modify this section as required*/
        if(xQueueReceive(self->queue_handle,&cmd,portMAX_DELAY) == pdTRUE){
            ESP_LOGI(tag,"Got Packet:- msg:%s",cmd.value);
            memset(&dev_rsp,0,sizeof(DeviceCommand));
            dev_rsp.source = cmd.source;
            if(cmd.source == CmdSource::BT){
                dev_rsp.char_type = cmd.char_type;
                if(cmd.char_type == BTCharType::CHAR_1){
                    self->manage_incoming_char1_commands(cmd);
                    //Form JSON Acknowledgement when command is SET
                    if(dev_rsp.type == CmdType::SET){
                        cJSON *resp_obj = cJSON_CreateObject();
                        char *ack_string=(char*)"";
                        cJSON_AddNumberToObject(resp_obj,"cmd",dev_rsp.command);
                        cJSON_AddNumberToObject(resp_obj,"status",(dev_rsp.status==ESP_OK)?1:0);
                        ack_string = cJSON_PrintUnformatted(resp_obj);
                        strcpy(dev_rsp.value,ack_string);
                        delete(resp_obj);
                        cJSON_free(ack_string);
                    }
                }else{
                    self->manage_incoming_char2_commands(cmd);
                    /*Handle as required*/
                }
            }else if(cmd.source == CmdSource::MQTT){
                if(!strcmp((const char *)cmd.mqtt_topic,(const char *)"esp/test/topic1")){
                    ESP_LOGI(tag,"Char-1 Command-1");
                    self->manage_incoming_char1_commands(cmd);
                    //Form JSON Acknowledgement when command is SET
                    if(dev_rsp.type == CmdType::SET){
                        cJSON *resp_obj = cJSON_CreateObject();
                        char *ack_string=(char*)"";
                        cJSON_AddNumberToObject(resp_obj,"cmd",dev_rsp.command);
                        cJSON_AddNumberToObject(resp_obj,"status",(dev_rsp.status==ESP_OK)?1:0);
                        ack_string = cJSON_PrintUnformatted(resp_obj);
                        strcpy(dev_rsp.value,ack_string);
                        delete(resp_obj);
                        cJSON_free(ack_string);
                    }
                }else if(!strcmp((const char *)cmd.mqtt_topic,(const char *)"esp/test/topic2")){
                    ESP_LOGI(tag,"Char-2 Command");
                    self->manage_incoming_char2_commands(cmd);
                    /*Handle as required*/
                }
            }
            self->sendCommandResponse(dev_rsp);    
        }
    }
}

void CmdManager::manage_incoming_char1_commands(const struct DeviceCommand &cmd_info){
    cJSON *json_cmd = cJSON_Parse(cmd_info.value);
    int command = 0;
    if(cJSON_GetObjectItem(json_cmd,"cmd")){
        command = cJSON_GetObjectItem(json_cmd,"cmd")->valueint;
        dev_rsp.command = command;
        switch(command){
            case CMD_ID_SET_WIFI_PARAMS:
                dev_rsp.type = CmdType::SET;
                dev_rsp.status = configure_wifi_params(json_cmd);
                break;
            case CMD_ID_GET_WIFI_PARAMS:
                dev_rsp.type = CmdType::GET;
                dev_rsp.status = read_configuration(command,dev_rsp.value);
                break;
            case CMD_ID_SET_MQTT_PARAMS:
                dev_rsp.type = CmdType::SET;
                dev_rsp.status = configure_mqtt_params(json_cmd);
                break;
            case CMD_ID_GET_MQTT_PARAMS:
                dev_rsp.type = CmdType::GET;
                dev_rsp.status = read_configuration(command,dev_rsp.value);
                break;
            case CMD_ID_REBOOT_DEVICE  :
                dev_rsp.type = CmdType::SET;
                tmngr->add_task(TaskID::DEV_REBOOT);
                break;
            case CMD_ID_RESTORE_CONFIGS:
                dev_rsp.type = CmdType::SET;
                dev_rsp.status = cfg->erase_all_configs();
                break;
        }
    }else{
        dev_rsp.status = ESP_FAIL;
    }
}

void CmdManager::manage_incoming_char2_commands(const struct DeviceCommand &cmd_info){
    cJSON *json_cmd = cJSON_Parse(cmd_info.value);
    int command = 0;
    if(cJSON_GetObjectItem(json_cmd,"cmd")){
        command = cJSON_GetObjectItem(json_cmd,"cmd")->valueint;
        switch(command){
            
        }
    }else{
        //Invalid JSON Command
    }
}
//MPJHS B1sm1ll@h
esp_err_t CmdManager::configure_wifi_params(cJSON *object){
    char ssid[32]="", psk[64]="", ip[MAX_IP_SIZE]="", mask[MAX_IP_SIZE]="",gtw[MAX_IP_SIZE]="";
    int isstatic = 0;
    if(cJSON_GetObjectItem(object,"ssid"))
        strcpy(ssid,cJSON_GetObjectItem(object,"ssid")->valuestring);
    else 
        return ESP_FAIL;
    if(cJSON_GetObjectItem(object,"psk"))
        strcpy(psk,cJSON_GetObjectItem(object,"psk")->valuestring);
    else 
        return ESP_FAIL;
    if(cJSON_GetObjectItem(object,"static"))
        isstatic = cJSON_GetObjectItem(object,"static")->valueint;
    else 
        return ESP_FAIL;
    if(isstatic){
        if(cJSON_GetObjectItem(object,"ip"))
            strcpy(ip,cJSON_GetObjectItem(object,"ip")->valuestring);
        else 
            return ESP_FAIL;
        if(cJSON_GetObjectItem(object,"nm"))
            strcpy(mask,cJSON_GetObjectItem(object,"nm")->valuestring);
        else 
            return ESP_FAIL;
        if(cJSON_GetObjectItem(object,"gw"))
            strcpy(gtw,cJSON_GetObjectItem(object,"gw")->valuestring);
        else 
            return ESP_FAIL;
    }
    return cfg->update_configuration_wifi(ssid,psk,isstatic,ip,mask,gtw);    
}

esp_err_t CmdManager::configure_mqtt_params(cJSON *object){
    char mqip[MAX_URL_SIZE];
	int  mqport;
    if(cJSON_GetObjectItem(object,"ip"))
        strcpy(mqip,cJSON_GetObjectItem(object,"ip")->valuestring);
    else 
        return ESP_FAIL;
    if(cJSON_GetObjectItem(object,"port"))
        mqport = cJSON_GetObjectItem(object,"port")->valueint;
    else 
        return ESP_FAIL;
    return cfg->update_configuration_mqtt(mqip,mqport);    
}

esp_err_t CmdManager::read_configuration(int cmd_id,char * value){
    cJSON *resp_obj = cJSON_CreateObject();
    char *ack_string=(char*)"";
    cJSON_AddNumberToObject(resp_obj,"cmd",cmd_id);
    switch(cmd_id){
        case CMD_ID_GET_WIFI_PARAMS:
            cJSON_AddStringToObject(resp_obj,"ssid",cfg->configs.ssid);
            cJSON_AddStringToObject(resp_obj,"psk",cfg->configs.psk);
            cJSON_AddNumberToObject(resp_obj,"static",cfg->configs.isstatic);
            cJSON_AddStringToObject(resp_obj,"ip",cfg->configs.staticip);
            cJSON_AddStringToObject(resp_obj,"nm",cfg->configs.staticnetmask);
            cJSON_AddStringToObject(resp_obj,"gw",cfg->configs.staticgtw);
        break;
        case CMD_ID_GET_MQTT_PARAMS:
            cJSON_AddStringToObject(resp_obj,"ip",cfg->configs.mqip);
            cJSON_AddNumberToObject(resp_obj,"port",cfg->configs.mqport);
        break;
    }
    ack_string = cJSON_PrintUnformatted(resp_obj);
    strcpy(value,ack_string);
    delete(resp_obj);
    cJSON_free(ack_string);
    return ESP_OK;
}