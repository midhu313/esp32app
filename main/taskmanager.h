#ifndef TASKMANAGER_H_
#define TASKMANAGER_H_


enum class TaskID : int {
    DEV_REBOOT,//Execute Device Reboot Command
    NTP_UPDATE,//Execute NTP Update command
};

struct TaskEntry{
    TaskID id;
    uint64_t time;
};

class TaskManager{
    public:
        static TaskManager* getInstance();
        
        void initialize();
        uint8_t add_task(TaskID id);
        void remove_task(TaskID id);
    
    private:
        static TaskManager s_instance;
        
        TaskManager();
        ~TaskManager();
        TaskManager(const TaskManager&) = delete;
        TaskManager& operator = (const TaskManager&) = delete;
        
        WiFiManager *wifimngr;
        ClockManager *clk;

        std::unique_ptr<TimerEvent> m_ntp_update_timer;
      
        static constexpr const char * TASK_NAME = "ManagerTask";
        static constexpr uint32_t STACK_SIZE = 4098;
        static constexpr UBaseType_t PRIORITY = 4;
        static StaticTask_t task_buff;
        static StackType_t  task_stack[STACK_SIZE];
        
        TaskHandle_t t_handle;
        std::mutex m_mutex;
        
        std::vector<TaskEntry> p_tasks;

        ConnState m_wifi_state;
    
        const char* get_task_name(TaskID t);
        static void processManagerTask(void *args);
        void execute_task_timesync_from_NTPServer();
        void execute_task_device_reboot(uint64_t rx_time);

        void wifi_event_handler(esp_event_base_t event_base,int32_t event_id,void* event_data);

        void timeout_event_handle_ntp_update();
};



#endif /* !TASKMANAGER_H_ */

