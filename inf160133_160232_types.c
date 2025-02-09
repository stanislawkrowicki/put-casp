#include "inf160133_160232_types.h"

const uint16_t P_SYSTEM_QUEUE_ID = 1;
const uint16_t C_SYSTEM_QUEUE_ID = 2;
const uint16_t P_NOTIFICATION_QUEUE_ID = 11;
const uint16_t C_NOTIFICATION_QUEUE_ID = 12;
const int SUBSCRIPTIONS_SHM_KEY = 0x2222;
const int AVAILABLE_TYPES_SHM_KEY = 0x3333;
const int NOTIFICATION_TYPE_SHM_KEY = 0x444;
const uint16_t DISPATCHER_ID = 0;

message_type get_message_type(uint16_t id, uint32_t type)
{
    int64_t id_shifted = id;
    id_shifted <<= 32;
    return id_shifted | type;
}

uint16_t get_id(message_type st)
{
    return st >> 32;
}

uint32_t get_type(message_type st)
{
    return st & 0xFFFFFFFF;
}