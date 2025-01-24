#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#include "inf160133_160232_types.h"

uint16_t CONNECTED_PRODUCERS[MAX_ID + 1];          // type of messages sent if producer connected, 0 otherwise
uint16_t CONNECTED_CLIENTS[MAX_ID + 1];            // 1 if client connected, 0 otherwise
int AVAILABLE_NOTIFICATIONS[MAX_NOTIFICATION + 1]; // 0 if type not available 1 if available

void get_available_notifications(uint32_t *types, int *len)
{
    int curr = 0;

    for (int i = 0; i <= MAX_ID; ++i)
    {
        if (CONNECTED_PRODUCERS[i] > 0)
        {
            types[curr] = CONNECTED_PRODUCERS[i];
            curr++;
        }
    }

    *len = curr + 1;
}

void handle_producer_login(struct system_message msg, int producer_system_queue)
{
#ifdef DEBUG
    printf("Got LOGIN event from producer\n");
#endif

    struct system_message response;
    response.payload.number = 0;

    uint16_t producer_id = msg.payload.numbers[0];

    if (producer_id < 1 || producer_id > MAX_ID)
    {
        printf("ERROR: TRIED TO REGISTER PRODUCER WITH INVALID ID: %d\n", producer_id);
        return;
    }

    system_type login_ok_type = get_system_type(producer_id, DISP2PROD_LOGIN_OK);
    system_type login_failed_type = get_system_type(producer_id, DISP2PROD_LOGIN_FAILED);

    unsigned int type = msg.payload.numbers[1];

    if (type < 1 || type > MAX_TYPE)
    {
        printf("ERROR: PRODUCER %d TRIED TO REGISTER INVALID TYPE: %d\n", producer_id, type);
        response.mtype = login_failed_type;
        msgsnd(producer_system_queue, &response, sizeof(response.payload.number), 0);
        return;
    }

    if (CONNECTED_PRODUCERS[producer_id] != 0)
    {
        response.mtype = login_failed_type;
#ifdef DEBUG
        printf("Producer %d already exists\n", producer_id);
#endif
    }
    else
    {
        CONNECTED_PRODUCERS[producer_id] = type;
        AVAILABLE_NOTIFICATIONS[type] = 1;

        response.mtype = login_ok_type;
#ifdef DEBUG
        printf("Producer %d registered successfully, listens to type %d\n", producer_id, type);
#endif
    }

    msgsnd(producer_system_queue, &response, sizeof(response.payload.number), 0);
}

void handle_producer_system_message(struct system_message msg, int producer_system_queue)
{
    uint16_t producer_id = get_id(msg.mtype);
    uint32_t mtype = get_type(msg.mtype);

    switch (mtype)
    {
    case PROD2DISP_LOGIN:
        handle_producer_login(msg, producer_system_queue);
        break;
    default:
        printf("Unknown system message type from ID: %d: %d", producer_id, mtype);
    }
}

void handle_client_login(struct system_message msg, int client_system_queue)
{
#ifdef DEBUG
    printf("Got LOGIN event from client\n");
#endif

    struct system_message response;
    response.payload.number = 0;

    uint16_t client_id = msg.payload.number;

    if (client_id < 1 || client_id > MAX_ID)
    {
        printf("ERROR: TRIED TO REGISTER CLIENT WITH INVALID ID: %d\n", client_id);
        return;
    }

    system_type login_ok_type = get_system_type(client_id, DISP2CLI_LOGIN_OK);
    system_type login_failed_type = get_system_type(client_id, DISP2CLI_LOGIN_FAILED);

    if (CONNECTED_CLIENTS[client_id] != 0)
    {
        response.mtype = login_failed_type;

#ifdef DEBUG
        printf("Client %d already exists\n", client_id);
#endif
    }
    else
    {
        CONNECTED_CLIENTS[client_id] = 1; // 1 for client id taken

        response.mtype = login_ok_type;
#ifdef DEBUG
        printf("Client %d registered successfully\n", client_id);
#endif
    }

    if (msgsnd(client_system_queue, &response, sizeof(response.payload.number), 0) == -1)
    {
        printf("Response failed\n");
    }
    else
    {
        printf("Response sent\n");
    }
}

// Response to fetch by a client
void handle_client_fetch(struct system_message msg, int client_system_queue)
{
#ifdef DEBUG
    printf("Got FETCH event from client\n");
#endif
    struct system_message fetch_response;
    int client_id = msg.payload.number;
    fetch_response.mtype = get_system_type(client_id, DISP2CLI_AVAILABLE_TYPES);

    for (size_t i = 0; i <= MAX_NOTIFICATION; i++)
    {
        fetch_response.payload.numbers[i] = AVAILABLE_NOTIFICATIONS[i];
    }

    if (msgsnd(client_system_queue, &fetch_response, sizeof(fetch_response.payload), 0) == -1)
    {
        printf("Sending available types failed\n");
    }
    else
    {
        printf("Sent notification array\n");
    }
}

void handle_client_system_notification_request(struct system_message msg, int client_system_queue)
{
#ifdef DEBUG
    printf("Got REQUEST event from client\n");
#endif

    uint16_t client_id = msg.payload.numbers[0];
    uint32_t notification_requested = msg.payload.numbers[1];

    CONNECTED_CLIENTS[client_id] = notification_requested;
#ifdef DEBUG
    printf("Clients notification request accepted\n");
#endif
}

void handle_client_system_message(struct system_message msg, int client_system_queue)
{
    uint16_t client_id = get_id(msg.mtype);
    uint32_t mtype = get_type(msg.mtype);

    switch (mtype)
    {
    case CLI2DISP_LOGIN:
        handle_client_login(msg, client_system_queue);
        break;
    case CLI2DISP_SUBSCRIBE:
        handle_client_system_notification_request(msg, client_system_queue);
        break;
    case CLI2DISP_FETCH:
        handle_client_fetch(msg, client_system_queue);
        break;
    default:
        printf("Unknown system message type from ID: %d: %d", client_id, mtype);
    }
}

void wait_for_system_messages(int producer_system_queue, int client_system_queue)
{
    ssize_t msg_size;
    struct system_message msg;

    // Get all messages destined for dispatcher
    long for_dispatcher_type_range = -get_system_type(DISPATCHER_ID, MAX_TYPE);

    while (1)
    {
        uint8_t received = 0;

        msg_size = msgrcv(producer_system_queue, &msg, sizeof(msg.payload), for_dispatcher_type_range, IPC_NOWAIT);
        if (msg_size != -1)
        {
#ifdef DEBUG
            printf("Received message on producer system queue!\n");
#endif
            handle_producer_system_message(msg, producer_system_queue);
            received = 1;
        }
        else if (errno != ENOMSG)
        {
            perror("msgrcv error on producer system queue");
        }

        msg_size = msgrcv(client_system_queue, &msg, sizeof(msg.payload), for_dispatcher_type_range, IPC_NOWAIT);
        if (msg_size != -1)
        {
#ifdef DEBUG
            printf("Received message on client system queue!\n");
#endif
            handle_client_system_message(msg, client_system_queue);
            received = 1;
        }
        else if (errno != ENOMSG)
        {
            perror("msgrcv error on client system queue");
        }

        if (!received)
            usleep(10000); // 10 millis
    }
}

void wait_for_notifications(int producer_queue, int client_queue)
{
    // ssize_t msg_size;
    struct system_message msg;

    printf("Listening!\n");
    while (1)
    {
        msgrcv(producer_queue, &msg, sizeof(msg.payload), 0, 0);

        if (errno)
            printf("Error while waiting for notifications");

        printf("Got message of type %ld with content %s\n", msg.mtype, msg.payload.text);
    }
}

int main()
{
    int client_system_queue = msgget(C_SYSTEM_QUEUE_ID, 0600 | IPC_CREAT);
    int producer_system_queue = msgget(P_SYSTEM_QUEUE_ID, 0600 | IPC_CREAT);

    int producer_message_queue = msgget(P_NOTIFICATION_QUEUE_ID, 0600 | IPC_CREAT);
    int client_message_queue = msgget(C_NOTIFICATION_QUEUE_ID, 0600 | IPC_CREAT);

    if (client_system_queue == -1 || producer_system_queue == -1)
    {
        perror("Error while creating system queues");
        exit(EXIT_FAILURE);
    }

    printf("System queues created successfully\n");

    for (int i = 0; i <= MAX_ID; ++i)
    {
        CONNECTED_PRODUCERS[i] = 0;
        CONNECTED_CLIENTS[i] = 0;
    }

    for (int i = 0; i < MAX_NOTIFICATION; ++i)
    {
        AVAILABLE_NOTIFICATIONS[i] = 0;
    }

    if (fork() == 0)
        wait_for_notifications(producer_message_queue, client_message_queue);
    else
        wait_for_system_messages(producer_system_queue, client_system_queue);
}