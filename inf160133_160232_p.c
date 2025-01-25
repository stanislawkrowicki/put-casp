#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "inf160133_160232_types.h"

int G_system_queue;
int G_id;
int G_producing_type;

void produce_messages()
{
    int message_queue = msgget(P_NOTIFICATION_QUEUE_ID, 0600 | IPC_CREAT);

    char text[MAX_MESSAGE_SIZE];

    while (1)
    {
        printf("Enter notification content or EXIT to exit: ");
        scanf("%s", text);

        if (strcmp(text, "EXIT") == 0)
        {
            exit(EXIT_SUCCESS);
        }

        struct system_message msg;
        msg.mtype = get_system_type(G_id, G_producing_type);
        strcpy(msg.payload.text, text);

        if (msgsnd(message_queue, &msg, sizeof(msg.payload), 0) == -1)
        {
            perror("Error while sending notification");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("Notification sent!\n");
        }
    }
}

void login_ok()
{
    printf("Login accepted.\nYour ID is %d and you are producing %d.\n", G_id, G_producing_type);
    produce_messages();
}

void login_failed()
{
    printf("Login failed. Producer already exists or there are too many producers.\n");
    exit(EXIT_FAILURE);
}

void wait_for_login_response()
{
    system_type ok_id = get_system_type(G_id, DISP2PROD_LOGIN_OK);
    system_type failed_id = get_system_type(G_id, DISP2PROD_LOGIN_FAILED);

    struct system_message response;

    while (1)
    {
        ssize_t msg_size = msgrcv(G_system_queue, &response, sizeof(response.payload), ok_id, IPC_NOWAIT);
        if (msg_size != -1)
        {
            login_ok();
            break;
        }
        else if (errno != ENOMSG)
        {
            perror("msgrcv error while waiting for login response\n");
        }

        msg_size = msgrcv(G_system_queue, &response, sizeof(response.payload), failed_id, IPC_NOWAIT);
        if (msg_size != -1)
        {
            login_failed();
            break;
        }
        else if (errno != ENOMSG)
        {
            perror("msgrcv error while waiting for login response\n");
        }

        usleep(100);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Provide your ID and type you are going to produce as arguments.\n");
        return 1;
    }

    G_id = atoi(argv[1]);
    G_producing_type = atoi(argv[2]);

    if (G_id < 1 || G_id > MAX_ID)
    {
        printf("Your id must be between 1 and %d\n", MAX_ID);
        return 1;
    }

    if (G_producing_type < 1 || G_producing_type > MAX_NOTIFICATION)
    {
        printf("Message type must be between 1 and %d.\n", MAX_NOTIFICATION);
        return 1;
    }

    G_system_queue = msgget(P_SYSTEM_QUEUE_ID, 0600 | IPC_CREAT);

    struct system_message msg;

    msg.mtype = get_system_type(DISPATCHER_ID, PROD2DISP_LOGIN);

    msg.payload.numbers[0] = G_id;             // Producer ID
    msg.payload.numbers[1] = G_producing_type; // Type to listen to

    if (msgsnd(G_system_queue, &msg, sizeof(msg.payload), 0) == -1)
    {
        perror("msgsnd error");
        exit(EXIT_FAILURE);
    }
#ifdef DEBUG
    else
    {
        printf("Login request sent!\n");
    }
#endif

    wait_for_login_response();

    return 0;
}