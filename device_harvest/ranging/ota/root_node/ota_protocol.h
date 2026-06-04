#pragma once
#include <stdint.h>

#define OTA_REQ     1
#define OTA_READY   2
#define OTA_DATA    3
#define OTA_ACK     4
#define OTA_END     5
#define OTA_NOTIFY  6
#define OTA_CANCEL  7

#define OTA_CHUNK 1024
#define FW_VERSION 1

#pragma pack(push,1)

typedef struct
{
    uint8_t type;
    uint16_t version;
} ota_req_t;

typedef struct
{
    uint8_t type;
    uint16_t seq;
    uint16_t len;
} ota_packet_t;

#pragma pack(pop)
