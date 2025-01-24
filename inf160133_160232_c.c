#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>

#include "inf160133_160232_types.h"

void login(int client_id, int queue)
{

    struct system_message msg;

    msg.mtype = get_system_type(DISPATCHER_ID, CLI2DISP_LOGIN);

    msg.payload.number = client_id;
    printf("Message ID: %ld\n", msg.mtype);
    if (msgsnd(queue, &msg, sizeof(msg.payload), 0) == -1)
    {
        perror("msgsnd error");
        exit(1);
    }
    else
    {
        printf("Message sent!\n");
    }

    // Login response
    ssize_t msg_size;
    struct system_message response;
    system_type ok_id = get_system_type(client_id, DISP2CLI_LOGIN_OK);
    system_type failed_id = get_system_type(client_id, DISP2CLI_LOGIN_FAILED);

    while (1)
    {
        msg_size = msgrcv(queue, &response, sizeof(msg.payload), ok_id, IPC_NOWAIT);
        if (msg_size != -1)
        {
            printf("Login accepted\n");
            break;
        }
        else if (errno != ENOMSG)
        {
            perror("msgrcv error on producer system queue\n");
        }

        msg_size = msgrcv(queue, &response, sizeof(msg.payload), failed_id, IPC_NOWAIT);
        if (msg_size != -1)
        {
            int corrected_id;
            printf("Client under this login exists. Please try different id number\n");
            scanf("%d", &corrected_id);
            client_id = corrected_id;
            login(client_id, queue);
            break;
        }
        else if (errno != ENOMSG)
        {
            perror("msgrcv error on producer system queue\n");
        }
    }
}

void logout(int client_id, int queue){
    struct system_message logout;

    logout.mtype = get_system_type(DISPATCHER_ID, CLI2DISP_LOGOUT);

    logout.payload.number = client_id;
    printf("Message ID: %ld\n", logout.mtype);
    if (msgsnd(queue, &logout, sizeof(logout.payload), 0) == -1)
    {
        perror("msgsnd error");
        exit(1);
    }
    else
    {
        printf("Message sent!\n");
    }

}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("You need to provide your ID\n");
        return 1;
    }
    
    int client_id = atoi(argv[1]);
    while (client_id <= 0)
    {
        int id;
        printf("Incorrect id!\n");
        printf("Client id must be higher than 0\n");
        scanf("%d", &id);
        client_id = id;
    }
    int queue = msgget(C_SYSTEM_QUEUE_ID, 0600 | IPC_CREAT);
    login(client_id, queue);

    // Fetch
    struct system_message fetch;
    fetch.mtype = get_system_type(DISPATCHER_ID, CLI2DISP_FETCH);
    fetch.payload.number = client_id;
    if (msgsnd(queue, &fetch, sizeof(fetch.payload), 0) == -1)
    {
        printf("msgsnd error\n");
    }
    else
    {
        printf("Fetch done\n");
    }

    ssize_t msg_size;
    struct system_message fetch_response;
    system_type fetch_response_type = get_system_type(client_id, DISP2CLI_AVAILABLE_TYPES);

    while (1)
    {
        uint8_t has_notifications = 0;

        msg_size = msgrcv(queue, &fetch_response, sizeof(fetch_response.payload), fetch_response_type, IPC_NOWAIT);
        if (msg_size != -1)
        {
            for (int i = 0; i <= MAX_NOTIFICATION; i++)
            {
                if (fetch_response.payload.numbers[i] == 1)
                {
                    printf("Type %d available\n", i);
                    has_notifications = 1;
                }
            }

            if (!has_notifications)
            {
                printf("There are no notifications to receive!\n");
                exit(1);
            }
            break;
        }
        else if (errno != ENOMSG)
        {
            perror("msgrcv error on producer system queue\n");
        }
    }
    // Choosing notification type
    int notification_type, t;
    scanf("%d", &t);
    int available;
    if (fetch_response.payload.numbers[t] == 1)
    {
        available = 1;
    }
    else
    {
        available = 0;
    }
    while (t <= 0 || t > MAX_NOTIFICATION || available == 0)
    {
        printf("Incorrect type of message required\n Try again\n");
        scanf("%d", &t);
        if (fetch_response.payload.numbers[t] == 1)
        {
            available = 1;
        }
    }
    notification_type = t;

    // Notification request
    struct system_message msg2;

    msg2.mtype = get_system_type(DISPATCHER_ID, CLI2DISP_SUBSCRIBE);

    msg2.payload.numbers[0] = client_id;
    msg2.payload.numbers[1] = notification_type;
    printf("You are %d and want to receive: %d\n", client_id, notification_type);
    if (msgsnd(queue, &msg2, sizeof(msg2.payload), 0) == -1)
    {
        perror("msgsnd error");
        exit(1);
    }
    else
    {
        printf("Subscribed!\n");
    }

    int message_queue = msgget(C_NOTIFICATION_QUEUE_ID, 0600 | IPC_CREAT);

    struct system_message notification;

    printf("Listening to notification %d\n", notification_type);

    while (1)
    {
        msgrcv(message_queue, &notification, sizeof(notification.payload), get_system_type(client_id, notification_type), 0);
        printf("%s\n", notification.payload.text);
    }

    return 0;
}