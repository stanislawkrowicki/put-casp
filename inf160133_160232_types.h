#ifndef _TYPES_H
#define _TYPES_H

#include <stdint.h>

#define MAX_MESSAGE_SIZE 1024

/*
    Producer -> Dispatcher message types
*/
enum EProd2DispSystemMessageType
{
    LOGIN,
    REGISTER_MESSAGE
};

/*
    Dispatcher -> Producer message types
*/
enum EDisp2ProdSystemMessageType
{
    LOGIN_OK,
    LOGIN_FAILED
};

/*
    Client -> Dispatcher message types
*/
enum ECli2DispSystemMessageType
{
    LOGIN,
    FETCH,
    SUBSCRIBE,
    UNSUBSCRIBE
};

/*
    Dispatcher -> Client message types
*/
enum EDisp2CliSystemMessageType
{
    LOGIN_OK,
    LOGIN_FAILED,
    AVAILABLE_TYPES,
    NEW_TYPE
};

/*
    Dispatcher -> Producer/Client identification type
*/
#define system_type uint32_t

/*
    Function to create a system_type from id and type
*/
system_type get_system_type(uint16_t id, uint16_t type);

/*
    Function to get target ID from a system_type
*/
uint16_t get_id(system_type st);

/*
    Function to get message type from a system_type
*/
uint16_t get_type(system_type st);

/*
    System message structure
*/
struct system_message
{
    long mtype;
    union
    {
        char text[MAX_MESSAGE_SIZE];
        uint16_t numbers[MAX_MESSAGE_SIZE / sizeof(uint16_t)];
    } payload;
};

#endif