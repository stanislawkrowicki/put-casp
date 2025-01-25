#define _GNU_SOURCE // see man sigaction(2), feature_test_macros(7)

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>

#include "inf160133_160232_types.h"

uint16_t G_client_id;
int G_system_queue;
int G_notification_type;

uint16_t login(int client_id)
{
    struct system_message msg;

    msg.mtype = get_system_type(DISPATCHER_ID, CLI2DISP_LOGIN);

    msg.payload.number = client_id;
    if (msgsnd(G_system_queue, &msg, sizeof(msg.payload), 0) == -1)
    {
        perror("login msgsnd error");
        exit(EXIT_FAILURE);
    }

    // Login response
    ssize_t msg_size;
    struct system_message response;
    system_type ok_id = get_system_type(client_id, DISP2CLI_LOGIN_OK);
    system_type failed_id = get_system_type(client_id, DISP2CLI_LOGIN_FAILED);

    while (1)
    {
        msg_size = msgrcv(G_system_queue, &response, sizeof(msg.payload), ok_id, IPC_NOWAIT);
        if (msg_size != -1)
        {
            printf("Login accepted\n");
            return client_id;
        }
        else if (errno != ENOMSG)
        {
            perror("msgrcv error login_ok");
            return 0;
        }

        msg_size = msgrcv(G_system_queue, &response, sizeof(msg.payload), failed_id, IPC_NOWAIT);
        if (msg_size != -1)
        {
            int corrected_id;
            printf("Client under this login exists. Please try different id number\n");
            printf("ID: ");
            scanf("%d", &corrected_id);
            return login(corrected_id);
        }
        else if (errno != ENOMSG)
        {
            perror("msgrcv error login_failed");
            return 0;
        }
    }
}

void listen_to_notification(uint32_t notification_type)
{
    int message_queue = msgget(C_NOTIFICATION_QUEUE_ID, 0600 | IPC_CREAT);

    struct system_message notification;

    printf("Listening to notification %d\n", notification_type);

    printf("Use Ctrl + \\ to unsubscribe from notification\n");

    while (1)
    {
        msgrcv(message_queue, &notification, sizeof(notification.payload), get_system_type(G_client_id, notification_type), 0);
        printf("%s\n", notification.payload.text);
    }
}

void notification_request(struct system_message fetch_response)
{
    int t;
    scanf("%d", &t);
    int available = fetch_response.payload.numbers[t] == 1;

    while (t <= 0 || t > MAX_NOTIFICATION || !available)
    {
        printf("Incorrect type specified. Try again.\n");
        scanf("%d", &t);

        available = fetch_response.payload.numbers[t] == 1;
    }

    G_notification_type = t;

    // Notification request
    struct system_message sub_msg;

    sub_msg.mtype = get_system_type(DISPATCHER_ID, CLI2DISP_SUBSCRIBE);

    sub_msg.payload.numbers[0] = G_client_id;
    sub_msg.payload.numbers[1] = G_notification_type;
    if (msgsnd(G_system_queue, &sub_msg, sizeof(sub_msg.payload), 0) == -1)
    {
        perror("msgsnd error");
        exit(EXIT_FAILURE);
    }

    listen_to_notification(G_notification_type);
}

void logout()
{
    struct system_message logout;
    logout.mtype = get_system_type(DISPATCHER_ID, CLI2DISP_LOGOUT);
    logout.payload.number = G_client_id;
    if (msgsnd(G_system_queue, &logout, sizeof(logout.payload), 0) == -1)
    {
        perror("logout msgsnd error");
    }
    else
    {
        printf("\nLogging out...\n");
    }
    sleep(1);
    exit(0);
}

void fetch()
{
    struct system_message fetch;
    fetch.mtype = get_system_type(DISPATCHER_ID, CLI2DISP_FETCH);
    fetch.payload.number = G_client_id;
    if (msgsnd(G_system_queue, &fetch, sizeof(fetch.payload), 0) == -1)
    {
        perror("fetch msgsnd error");
    }

    ssize_t msg_size;
    struct system_message fetch_response;
    system_type fetch_response_type = get_system_type(G_client_id, DISP2CLI_AVAILABLE_TYPES);

    while (1)
    {
        uint8_t has_notifications = 0;

        msg_size = msgrcv(G_system_queue, &fetch_response, sizeof(fetch_response.payload), fetch_response_type, IPC_NOWAIT);
        if (msg_size != -1)
        {
            for (int i = 0; i <= MAX_NOTIFICATION; i++)
            {
                if (fetch_response.payload.numbers[i] == 1)
                {
                    printf("Type %d is available\n", i);
                    has_notifications = 1;
                }
            }

            if (!has_notifications)
            {
                printf("There are no notifications to receive!\n");
                logout();
            }
            break;
        }
        else if (errno != ENOMSG)
        {
            perror("msgrcv error on producer system queue\n");
        }
    }
    notification_request(fetch_response);
}

void unsubscribe()
{
    struct system_message unsub;
    unsub.mtype = get_system_type(DISPATCHER_ID, CLI2DISP_UNSUBSCRIBE);
    unsub.payload.numbers[0] = G_client_id;
    unsub.payload.numbers[1] = G_notification_type;
    if (msgsnd(G_system_queue, &unsub, sizeof(unsub.payload), 0) == -1)
    {
        perror("unsubscribe msgsnd error");
    }
    else
    {
        printf("\nNotification unsubscribed\n");
        fetch();
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("You need to provide your ID as an argument\n");
        exit(EXIT_FAILURE);
    }

    int client_id = atoi(argv[1]);
    while (client_id <= 0 || client_id > MAX_ID)
    {
        printf("Incorrect id!\n");
        printf("Client id must be between 1 and %d\n", MAX_ID);
        scanf("%d", &client_id);
    }

    G_system_queue = msgget(C_SYSTEM_QUEUE_ID, 0600 | IPC_CREAT);

    G_client_id = login(client_id);

    struct sigaction logout_sa;
    memset(&logout_sa, 0, sizeof(logout_sa));
    logout_sa.sa_handler = logout;
    logout_sa.sa_flags = SA_NOMASK;

    if (sigaction(SIGINT, &logout_sa, NULL) == -1)
    {
        perror("logout sigaction");
    }

    struct sigaction unsubscribe_sa;
    memset(&unsubscribe_sa, 0, sizeof(unsubscribe_sa));
    unsubscribe_sa.sa_handler = unsubscribe;
    unsubscribe_sa.sa_flags = SA_NOMASK;

    if (sigaction(SIGQUIT, &unsubscribe_sa, NULL) == -1)
    {
        perror("unsubscribe sigaction");
    }

    printf("Use 'Ctrl + C' to logout\n");

    fetch();
    return 0;
}