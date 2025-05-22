#include "includes.h"

static const char * tag = "cmd";

CmdManager CmdManager::s_instance;

StaticTask_t CmdManager::task_buff;
StackType_t  CmdManager::task_stack[CmdManager::STACK_SIZE];

static struct DeviceCommand dev_rsp;

CmdManager::CmdManager(){
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
        if(xQueueReceive(self->queue_handle,&cmd,portMAX_DELAY) == pdTRUE){
            ESP_LOGI(tag,"Got Packet:- msg:%s",cmd.value);
            memset(&dev_rsp,0,sizeof(DeviceCommand));
            
            dev_rsp.char_type = cmd.char_type;
           
            if(cmd.char_type == BTCharType::CHAR_1)
                self->manage_incoming_char1_commands(cmd);
            else
                self->manage_incoming_char2_commands(cmd);

            self->sendCommandResponse(dev_rsp);    
        }
    }
}

void CmdManager::manage_incoming_char1_commands(const struct DeviceCommand &cmd_info){
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