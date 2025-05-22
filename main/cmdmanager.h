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

        void sendCommandResponse(const DeviceCommand &rsp);
        
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
