#include "includes.h"

static const char * tag = "Task";

TaskManager TaskManager::s_instance;

StaticTask_t TaskManager::task_buff;
StackType_t  TaskManager::task_stack[TaskManager::STACK_SIZE];

TaskManager::TaskManager() : t_handle(nullptr),m_wifi_state(ConnState::Disconnected){
    wifimngr = WiFiManager::getInstance();
    clk = ClockManager::getInstance();
    
    t_handle = xTaskCreateStaticPinnedToCore(
        &TaskManager::processManagerTask,TASK_NAME,STACK_SIZE,this,PRIORITY,task_stack,&task_buff,1);
    if(!t_handle){
        ESP_LOGE(tag,"Failed to create pinned task!");
    }
}

TaskManager::~TaskManager(){
    
}

TaskManager* TaskManager::getInstance(){
    return &s_instance;
}

void TaskManager::initialize(){
    /*Register for WiFi event callbacks*/
    wifimngr->register_event_callback([this](esp_event_base_t event_base,int32_t event_id,void* event_data){
        this->wifi_event_handler(event_base,event_id,event_data);
    });
    /*Register one shot timer for NTP Timer failed callback*/
    m_ntp_update_timer = std::make_unique<TimerEvent>();
    m_ntp_update_timer->registerOneShotTimeoutTask([this](){
        this->timeout_event_handle_ntp_update();
    });
}

void TaskManager::add_task(TaskID id){
    {
        uint64_t cur_time = clk->get_current_time_in_ms();
        std::lock_guard<std::mutex> lock(m_mutex);
        p_tasks.push_back({id,cur_time});
        ESP_LOGI(tag,"Adding new Task:%s pending %u",get_task_name(id),(unsigned)p_tasks.size());
    }
    xTaskNotifyGive(t_handle);
}

void TaskManager::remove_task(TaskID id){
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = std::remove_if(p_tasks.begin(),p_tasks.end(),[id](auto &e){return e.id==id;});
        p_tasks.erase(it,p_tasks.end());
        ESP_LOGW(tag,"Task:%s Removed pending:%u",get_task_name(id),(unsigned)p_tasks.size());
    }
    xTaskNotifyGive(t_handle);
}


const char* TaskManager::get_task_name(TaskID t){
    switch(t){
        case TaskID::DEV_REBOOT  : return "Reboot"; 
        case TaskID::NTP_UPDATE  : return "NTP Update"; 
    }
    return "Unknown";
}

void TaskManager::processManagerTask(void *args){
    auto *self = static_cast<TaskManager*>(args);
    std::vector<TaskEntry> work;
    while(1){
        {
            std::lock_guard<std::mutex> lock(self->m_mutex);
            work = self->p_tasks;
        }
        if(work.empty()){
            ESP_LOGW(tag,"No More tasks");
            ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
            continue;
        }
        for(auto &e : work){
            switch(e.id){
                case TaskID::DEV_REBOOT:
                    ESP_LOGI(tag, "Running Task: Reboot");
                    self->execute_task_device_reboot(e.time);
                    break;
                case TaskID::NTP_UPDATE: 
                    if(self->m_wifi_state == ConnState::Connected){
                        ESP_LOGI(tag, "Running Task: NTP Update");
                        self->execute_task_timesync_from_NTPServer();
                    }
                    break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void TaskManager::wifi_event_handler(esp_event_base_t event_base,int32_t event_id,void* event_data){
    if((event_base == WIFI_EVENT)&&(event_id == WIFI_EVENT_STA_CONNECTED)){
        ESP_LOGI(tag,"WiFi Connected");
        m_wifi_state = ConnState::Connected;
        wifi_ev_time = clk->get_current_time_in_ms();
    }else if((event_base == WIFI_EVENT)&&(event_id == WIFI_EVENT_STA_DISCONNECTED)){
        ESP_LOGI(tag,"WiFi DIsconnected");
        m_wifi_state = ConnState::Disconnected;
        wifi_ev_time = clk->get_current_time_in_ms();

    }
}

void TaskManager::timeout_event_handle_ntp_update(){
    add_task(TaskID::NTP_UPDATE);
}

void TaskManager::execute_task_timesync_from_NTPServer(){
    remove_task(TaskID::NTP_UPDATE);
    if(clk->sync_time_from_ntp_server((char *)"GMT-4")!=0){
        //NTP Update failed :- retry after 10 seconds
        m_ntp_update_timer->startOneShotTimeoutTask(10000);
    }else{
        //when device mode = on cloud or on premise & NTP Update is success :- re-update after 12 hours
        m_ntp_update_timer->startOneShotTimeoutTask(43200000);
    }
}

void TaskManager::execute_task_device_reboot(uint64_t rx_time){
    uint64_t time_now = clk->get_current_time_in_ms();
    if((time_now-rx_time)>=2000){
        remove_task(TaskID::DEV_REBOOT);
        ESP_LOGW(tag,"Rebooting Device Now..");
        esp_restart();
    }
}

