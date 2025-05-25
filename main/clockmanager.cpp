#include "includes.h"

static const char * tag = "Clock";

ClockManager ClockManager::s_instance;

ClockManager::ClockManager(){

}

ClockManager::~ClockManager(){
    
}

ClockManager* ClockManager::getInstance(){
    return &s_instance;
}

uint64_t ClockManager::get_current_time_in_ms(){
    struct timeval tv;
	gettimeofday(&tv, NULL);
	uint64_t timenow=(uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
	return timenow;
}

void ClockManager::set_current_time(uint64_t timestamp,char* timezone){
    struct timeval tv = {
		.tv_sec = (time_t)(timestamp), // Convert milliseconds to seconds
		.tv_usec =(suseconds_t) (0) // Convert remaining milliseconds to microseconds
	};
	// Set system time
	ESP_LOGI(tag, "Setting system time to %" PRIu64, timestamp);
	settimeofday(&tv, NULL);
	// Print current time
	setenv("TZ",(const char *)timezone,1);
	tzset();
	time_t now;
	struct tm timeinfo;
	time(&now);
	localtime_r(&now, &timeinfo);
	char strftime_buf[64];
	strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
	ESP_LOGI(tag, "The current time is: %s", strftime_buf);
}

uint8_t ClockManager::sync_time_from_ntp_server(char * timezone){
    ESP_LOGI(tag, "Initializing SNTP TimeZone:%s","GMT-4");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0,(const char *)"pool.ntp.org");
    esp_sntp_init();
    time_t now = 0;
	struct tm timeinfo={0,0,0,0,0,0,0,0,0};
	int retry = 0;
    while(esp_sntp_get_sync_status()==SNTP_SYNC_STATUS_RESET && ++retry < 10){
        ESP_LOGI(tag,"Waiting for system time to be set..(%d/10)",retry);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    if(retry>=9){
		esp_sntp_stop();
        ESP_LOGI(tag,"Failed to update time");
    	return -1;
	}
    char strftime_buf[64];
	setenv("TZ","GMT-4",1);
	tzset();
	time(&now);
	localtime_r(&now, &timeinfo);
	strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
	ESP_LOGI(tag, "The current date/time is: %s", strftime_buf);
	esp_sntp_stop();
    return 0;
}

