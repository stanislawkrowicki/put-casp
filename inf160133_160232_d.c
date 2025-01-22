#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#include "inf160133_160232_types.h"

uint16_t CONNECTED_PRODUCERS[MAX_ID + 1];
uint16_t CONNECTED_CLIENTS[MAX_ID + 1];

int initialize_client_system_queue()
{
    return msgget(C_SYSTEM_QUEUE_ID, 0600 | IPC_CREAT);
}

int initialize_producer_system_queue()
{
    return msgget(P_SYSTEM_QUEUE_ID, 0600 | IPC_CREAT);
}

void handle_producer_login(struct system_message msg)
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
        msgsnd(P_SYSTEM_QUEUE_ID, &response, sizeof(response.payload.number), 0);
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

        response.mtype = login_ok_type;
#ifdef DEBUG
        printf("Producer %d registered successfully, listens to type %d\n", producer_id, type);
#endif
    }

    msgsnd(P_SYSTEM_QUEUE_ID, &response, sizeof(response.payload.number), 0);
}

void handle_producer_system_message(struct system_message msg)
{
    uint16_t producer_id = get_id(msg.mtype);
    uint32_t mtype = get_type(msg.mtype);

    switch (mtype)
    {
    case PROD2DISP_LOGIN:
        handle_producer_login(msg);
        break;
    default:
        printf("Unknown system message type from ID: %d: %d", producer_id, mtype);
    }
}

void handle_client_system_message(struct system_message msg)
{
    printf("Hello from handle client!\n");
}

void wait_for_messages(int producer_system_queue, int client_system_queue)
{
    ssize_t msg_size;
    struct system_message msg;

    // Get all messages with type lower or equal to DISPATCHER_ID, so destined to dispatcher
    // Conversion to long is necessary because of unexpected behavior when negating uint32_t
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
            handle_producer_system_message(msg);
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
            handle_client_system_message(msg);
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

int main()
{
    int client_system_queue = initialize_client_system_queue();
    int producer_system_queue = initialize_producer_system_queue();

    if (client_system_queue == -1 || producer_system_queue == -1)
    {
        perror("Error while creating system queues");
        exit(EXIT_FAILURE);
    }

    printf("System queues created successfully\n");

    for (int i = 0; i <= MAX_ID; ++i)
    {
        CONNECTED_PRODUCERS[i] = 0;
    }

    wait_for_messages(producer_system_queue, client_system_queue);
}