#ifndef INCLUDES_H_
#define INCLUDES_H_

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <list>
#include <queue>
#include <deque>
#include <algorithm>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <regex>

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <functional>
#include <assert.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "nimble/ble.h"
#include "modlog/modlog.h"
#include "esp_peripheral.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "services/ans/ble_svc_ans.h"
#include "console/console.h"
#include "esp_timer.h"

#include "esp_sntp.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_app_desc.h"
#include "esp_ota_ops.h"
#include "cJSON.h"
#include "mqtt_client.h"

#include "defines.h"
#include "clockmanager.h"
#include "timerevent.h"
#include "configmanager.h"
#include "wifimanager.h"
#include "taskmanager.h"
#include "cmdmanager.h"
#include "blemanager.h"
#include "mqttclient.h"

#endif /* !INCLUDES_H_ */
