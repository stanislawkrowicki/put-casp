#include "inf160133_160232_types.h"

system_type get_system_type(uint16_t id, uint16_t type)
{
    return id << 16 | type;
}

uint16_t get_id(system_type st)
{
    return st >> 16;
}

uint16_t get_type(system_type st)
{
    return st & 0xFFFF;
}