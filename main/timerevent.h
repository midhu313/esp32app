#ifndef TIMEREVENT_H_
#define TIMEREVENT_H_

class TimerEvent{
    public:
        using Callback = std::function<void()>;
        TimerEvent();
        ~TimerEvent();

        int8_t registerOneShotTimeoutTask(Callback cb);
        int8_t startOneShotTimeoutTask(uint64_t timeout_ms);
        int8_t stopOneShotTimeoutTask();

        int8_t registerPeriodicTimeoutTask(Callback cb);
        int8_t startPeriodicTimeoutTask(uint64_t timeout_ms);
        int8_t stopPeriodicTimeoutTask();

    private:
        static void oneShotTimerCallback(void *arg);
        static void periodicTimerCallback(void *arg);


        esp_timer_handle_t oneshot_timhandle;
        uint64_t oneshot_timout_in_ms;
        Callback oneshot_callback;
        
        esp_timer_handle_t periodic_timhandle;
        uint64_t periodic_timout_in_ms;
        Callback periodic_callback;
        
        std::mutex p_mutex;
        
};

#endif /* !TIMEREVENT_H_ */

