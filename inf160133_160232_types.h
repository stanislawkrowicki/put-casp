#ifndef _TYPES_H
#define _TYPES_H

#include <stdint.h>

#define MAX_MESSAGE_SIZE 256
/*
    Maximum producer/client ID
*/
#define MAX_ID 30
#define MAX_NOTIFICATION 30

extern const uint16_t P_SYSTEM_QUEUE_ID;
extern const uint16_t C_SYSTEM_QUEUE_ID;
extern const uint16_t P_NOTIFICATION_QUEUE_ID;
extern const uint16_t C_NOTIFICATION_QUEUE_ID;

/*
    ID of the dispatcher for use with system_type
*/
extern const uint16_t DISPATCHER_ID;

/*
    Maximum value for message type
*/
extern const uint32_t MAX_TYPE;

/*
    Producer -> Dispatcher message types
*/
enum EProd2DispSystemMessageType
{
    PROD2DISP_LOGIN = 1,
    PROD2DISP_REGISTER_MESSAGE = 2
};

/*
    Dispatcher -> Producer message types
*/
enum EDisp2ProdSystemMessageType
{
    DISP2PROD_LOGIN_OK = 1,
    DISP2PROD_LOGIN_FAILED = 2
};

/*
    Client -> Dispatcher message types
*/
enum ECli2DispSystemMessageType
{
    CLI2DISP_LOGIN = 1,
    CLI2DISP_FETCH = 2,
    CLI2DISP_SUBSCRIBE = 3,
    CLI2DISP_UNSUBSCRIBE = 4,
    CLI2DISP_LOGOUT = 5
};

/*
    Dispatcher -> Client message types
*/
enum EDisp2CliSystemMessageType
{
    DISP2CLI_LOGIN_OK = 1,
    DISP2CLI_LOGIN_FAILED = 2,
    DISP2CLI_AVAILABLE_TYPES = 3,
    DISP2_CLI_NEW_TYPE = 4
};

/*
    Dispatcher -> Producer/Client identification type
*/
#define system_type int64_t

/*
    Function to create a system_type from id and type
*/
system_type
get_system_type(uint16_t id, uint32_t type);

/*
    Function to get target ID from a system_type
*/
uint16_t get_id(system_type st);

/*
    Function to get message type from a system_type
*/
uint32_t get_type(system_type st);

/*
    System message structure
*/
struct system_message
{
    long mtype;
    union
    {
        char text[MAX_MESSAGE_SIZE];
        unsigned int number;
        uint32_t numbers[MAX_MESSAGE_SIZE / sizeof(uint32_t)];
    } payload;
};

#endif