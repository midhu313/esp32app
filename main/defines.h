#ifndef DEFINES_H_
#define DEFINES_H_

#define  MAX_IP_SIZE       39
#define  MAX_URL_SIZE      64

#define CMD_ID_SET_WIFI_PARAMS 1
#define CMD_ID_GET_WIFI_PARAMS 2
#define CMD_ID_SET_MQTT_PARAMS 3
#define CMD_ID_GET_MQTT_PARAMS 4
#define CMD_ID_REBOOT_DEVICE   5
#define CMD_ID_RESTORE_CONFIGS 6

#define MQTT_TOPIC_MAX_LENGTH   64
#define MQTT_PAYLOAD_MAX_LENGTH 256


enum class CmdSource  : uint8_t {NA,BT,MQTT};
enum class BTCharType : uint8_t {CHAR_UNKNWN,CHAR_1,CHAR_2};
enum class CmdType    : uint8_t {NA,SET,GET};
enum class ConnState : uint8_t{Unknown,Disconnected,Connected,NotConfigured};

struct DeviceCommand{
    CmdSource source{CmdSource::NA};
    BTCharType char_type{BTCharType::CHAR_UNKNWN};
    CmdType type{CmdType::NA};
    char mqtt_topic[MQTT_TOPIC_MAX_LENGTH];
    char value[MQTT_PAYLOAD_MAX_LENGTH];
    uint64_t time;
    uint8_t status;
    int command;
};

#endif /* !DEFINES_H_ */
