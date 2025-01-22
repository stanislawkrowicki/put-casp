#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#include "inf160133_160232_types.h"

int initialize_client_system_queue()
{
    return msgget(C_SYSTEM_QUEUE_ID, 0600 | IPC_CREAT);
}

int initialize_producer_system_queue()
{
    return msgget(P_SYSTEM_QUEUE_ID, 0600 | IPC_CREAT);
}

void handle_producer_system_message(struct system_message msg)
{
    printf("Hello from handle producer!\n");
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
    wait_for_messages(producer_system_queue, client_system_queue);
}