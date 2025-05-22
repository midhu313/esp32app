#ifndef CLOCKMANAGER_H_
#define CLOCKMANAGER_H_

class ClockManager{
    public:
        static ClockManager* getInstance();

        uint64_t get_current_time_in_ms();
        void set_current_time(uint64_t timestamp,char* timezone);
        uint8_t sync_time_from_ntp_server(char * timezone);

    private:
        static ClockManager s_instance;
            
        ClockManager();
        ~ClockManager();
        ClockManager(const ClockManager&) = delete;
        ClockManager& operator = (const ClockManager&) = delete;
};

#endif /* !CLOCKMANAGER_H_ */

