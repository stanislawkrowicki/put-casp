#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "inf160133_160232_types.h"

int G_queue;
int G_id;
int G_producing_type;

void produce_messages()
{
    int message_queue = msgget(P_NOTIFICATION_QUEUE_ID, 0600 | IPC_CREAT);

    char text[MAX_MESSAGE_SIZE];

    while (1)
    {
        printf("Podaj treść wiadomości lub EXIT by wyjść: ");
        scanf("%s", text);

        if (strcmp(text, "EXIT") == 0)
        {
            exit(1);
        }

        struct system_message msg;
        msg.mtype = get_system_type(G_id, G_producing_type);
        strcpy(msg.payload.text, text);

        if (msgsnd(message_queue, &msg, sizeof(msg.payload), 0) == -1)
        {
            perror("Error while sending notification");
            exit(1);
        }
        else
        {
            printf("Notification sent!\n");
        }
    }
}

void login_ok()
{
    printf("Login accepted\n");
    produce_messages();
}

void login_failed()
{
    printf("Login failed. Producer already exists or there are too many producers.\n");
}

void wait_for_login_response()
{
    system_type ok_id = get_system_type(G_id, DISP2PROD_LOGIN_OK);
    system_type failed_id = get_system_type(G_id, DISP2PROD_LOGIN_FAILED);

    struct system_message response;

    while (1)
    {
        ssize_t msg_size = msgrcv(G_queue, &response, sizeof(response.payload), ok_id, IPC_NOWAIT);
        if (msg_size != -1)
        {
            login_ok();
            break;
        }
        else if (errno != ENOMSG)
        {
            perror("msgrcv error while waiting for login response\n");
        }

        msg_size = msgrcv(G_queue, &response, sizeof(response.payload), failed_id, IPC_NOWAIT);
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
        printf("Podaj ID i typ jaki będziesz produkować.");
        return 1;
    }

    G_id = atoi(argv[1]);
    G_producing_type = atoi(argv[2]);

    if (G_id < 1 || G_id > MAX_ID)
    {
        printf("ID musi być pomiędzy 1 a %d\n", MAX_ID);
        return 1;
    }

    if (G_producing_type < 1 || G_producing_type > MAX_NOTIFICATION)
    {
        printf("Typ wiadomości musi być pomiędzy 1 a %d.", MAX_NOTIFICATION);
        return 1;
    }

    G_queue = msgget(P_SYSTEM_QUEUE_ID, 0600 | IPC_CREAT);

    struct system_message msg;

    msg.mtype = get_system_type(DISPATCHER_ID, PROD2DISP_LOGIN);

    msg.payload.numbers[0] = G_id;             // Producer ID
    msg.payload.numbers[1] = G_producing_type; // Type to listen to

    if (msgsnd(G_queue, &msg, sizeof(msg.payload), 0) == -1)
    {
        perror("msgsnd error");
        exit(1);
    }
    else
    {
        printf("Login request sent!\n");
    }

    wait_for_login_response();

    return 0;
}