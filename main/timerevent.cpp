#include "includes.h"


TimerEvent::TimerEvent():
    oneshot_timhandle(nullptr),oneshot_timout_in_ms(0),oneshot_callback(nullptr),
    periodic_timhandle(nullptr),periodic_timout_in_ms(0),periodic_callback(nullptr)
{
        
}

TimerEvent::~TimerEvent(){
    std::lock_guard<std::mutex> lock(p_mutex);
    if(oneshot_timhandle){
        esp_timer_stop(oneshot_timhandle);
        esp_timer_delete(oneshot_timhandle);
        oneshot_timhandle = nullptr;
    }
    if(periodic_timhandle){
        esp_timer_stop(periodic_timhandle);
        esp_timer_delete(periodic_timhandle);
        periodic_timhandle = nullptr;
    }
}

int8_t TimerEvent::registerOneShotTimeoutTask(Callback cb){
    std::lock_guard<std::mutex> lock(p_mutex);
    oneshot_callback = std::move(cb);
    if(!oneshot_timhandle){
        //One-shot Timer Decalaration
        esp_timer_create_args_t os_timerArgs = { };
        os_timerArgs.callback = &TimerEvent::oneShotTimerCallback;
        os_timerArgs.arg = this;
        os_timerArgs.name = "osEvTimer";
        if(esp_timer_create(&os_timerArgs,&oneshot_timhandle) != ESP_OK){
            oneshot_timhandle = nullptr;
            return -1;
        }
    }
    return 0;
}

int8_t TimerEvent::startOneShotTimeoutTask(uint64_t timeout_ms){
    std::lock_guard<std::mutex> lock(p_mutex);
    oneshot_timout_in_ms = timeout_ms;
    if(!oneshot_timout_in_ms || !oneshot_callback)
        return -1;
    esp_timer_stop(oneshot_timhandle);
    return (esp_timer_start_once(oneshot_timhandle,oneshot_timout_in_ms*1000ULL) == ESP_OK)?0:-1; 
}

int8_t TimerEvent::stopOneShotTimeoutTask(){
    std::lock_guard<std::mutex> lock(p_mutex);
    if(!oneshot_timout_in_ms || !oneshot_callback)
        return -1;
    return (esp_timer_stop(oneshot_timhandle) == ESP_OK)?0:-1;
}

void TimerEvent::oneShotTimerCallback(void *arg){
    TimerEvent *self = static_cast<TimerEvent*>(arg);
    // Invoke user callback; no lock to avoid deadlock if callback calls registerTask/start
    if (self) {
        // Invoke user callback outside mutex to avoid deadlocks
        Callback cb_copy;
        {
            std::lock_guard<std::mutex> lock(self->p_mutex);
            cb_copy = self->oneshot_callback;
        }
        if (cb_copy) {
            cb_copy();
        }
    }
}

int8_t TimerEvent::registerPeriodicTimeoutTask(Callback cb){
    std::lock_guard<std::mutex> lock(p_mutex);
    
    periodic_callback = std::move(cb);

    if(!periodic_timhandle){
        //Periodic Timer Decalaration
        esp_timer_create_args_t per_timerArgs = { };
        per_timerArgs.callback = &TimerEvent::periodicTimerCallback;
        per_timerArgs.arg = this;
        per_timerArgs.name = "perEvTimer";
        if(esp_timer_create(&per_timerArgs,&periodic_timhandle) != ESP_OK){
            periodic_timhandle = nullptr;
            return -1;
        }
    }
    return 0;  
}

int8_t TimerEvent::startPeriodicTimeoutTask(uint64_t timeout_ms){
    std::lock_guard<std::mutex> lock(p_mutex);
    periodic_timout_in_ms = timeout_ms;
    if(!periodic_timout_in_ms || !periodic_callback)
        return -1;
    esp_timer_stop(periodic_timhandle);
    return (esp_timer_start_periodic(periodic_timhandle,periodic_timout_in_ms*1000ULL) == ESP_OK)?0:-1; 
}

int8_t TimerEvent::stopPeriodicTimeoutTask(){
    std::lock_guard<std::mutex> lock(p_mutex);
    if(!periodic_timout_in_ms || !periodic_callback)
        return -1;
    return (esp_timer_stop(periodic_timhandle) == ESP_OK)?0:-1;
}

void TimerEvent::periodicTimerCallback(void *arg){
    TimerEvent *self = static_cast<TimerEvent*>(arg);
    // Invoke user callback; no lock to avoid deadlock if callback calls registerTask/start
    if (self) {
        // Invoke user callback outside mutex to avoid deadlocks
        Callback cb_copy;
        {
            std::lock_guard<std::mutex> lock(self->p_mutex);
            cb_copy = self->periodic_callback;
        }
        if (cb_copy) {
            cb_copy();
        }
    }
}