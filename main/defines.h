#ifndef DEFINES_H_
#define DEFINES_H_

#define  MAX_IP_SIZE       39
#define  MAX_URL_SIZE      64

enum class BTCharType : uint8_t {CHAR_UNKNWN,CHAR_1,CHAR_2};

struct DeviceCommand{
    BTCharType char_type{BTCharType::CHAR_UNKNWN};
    char value[64];
    uint64_t time;
    uint8_t status;
};

#endif /* !DEFINES_H_ */
