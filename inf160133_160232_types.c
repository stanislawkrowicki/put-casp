#include "inf160133_160232_types.h"

const uint16_t P_SYSTEM_QUEUE_ID = 1;
const uint16_t C_SYSTEM_QUEUE_ID = 2;
const uint16_t P_NOTIFICATION_QUEUE_ID = 11;
const uint16_t C_NOTIFICATION_QUEUE_ID = 12;
const int SUBSCRIPTIONS_SHM_KEY = 0x1354;
const uint16_t DISPATCHER_ID = 0;
const uint32_t MAX_TYPE = UINT32_MAX;

system_type get_system_type(uint16_t id, uint32_t type)
{
    int64_t id_shifted = id;
    id_shifted <<= 32;
    return id_shifted | type;
}

uint16_t get_id(system_type st)
{
    return st >> 32;
}

uint32_t get_type(system_type st)
{
    return st & 0xFFFFFFFF;
}