#define _GNU_SOURCE // see man sigaction(2), feature_test_macros(7)

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
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
int *G_notification_type;
uint32_t *G_available_types;
uint8_t G_listen = 0;

uint16_t login(int client_id)
{
    struct message_event msg;

    msg.mtype = get_message_type(DISPATCHER_ID, CLI2DISP_LOGIN);

    msg.payload.number = client_id;
    if (msgsnd(G_system_queue, &msg, sizeof(msg.payload), 0) == -1)
    {
        perror("login msgsnd error");
        exit(EXIT_FAILURE);
    }

    // Login response
    ssize_t msg_size;
    struct message_event response;
    message_type ok_id = get_message_type(client_id, DISP2CLI_LOGIN_OK);
    message_type failed_id = get_message_type(client_id, DISP2CLI_LOGIN_FAILED);

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

void listen_to_new_types()
{
    struct message_event event;
    message_type mtype = get_message_type(G_client_id, DISP2_CLI_NEW_TYPE);

    while (1)
    {
        if (msgrcv(G_system_queue, &event, sizeof(event.payload), mtype, 0) != -1)
        {
            G_available_types[event.payload.number] = 1;
            if (*G_notification_type == -1)
                printf("Type %d is available\n", event.payload.number);
            else
                printf("Type %d just got registered. Unsubscribe from current type using "
                       "Ctrl + \\ to subscribe to the new type.\n",
                       event.payload.number);
        }
    }
}

void listen_to_notification(uint32_t notification_type)
{
    int message_queue = msgget(C_NOTIFICATION_QUEUE_ID, 0600 | IPC_CREAT);

    struct message_event notification;

    printf("Listening to notification %d\n", notification_type);

    printf("Use Ctrl + \\ to unsubscribe from notification\n");

    while (G_listen)
    {
        msgrcv(message_queue, &notification, sizeof(notification.payload), get_message_type(G_client_id, notification_type), 0);
        printf("%s\n", notification.payload.text);
    }
}

void notification_request()
{
    int t;
    scanf("%d", &t);
    int available = G_available_types[t] == 1;

    while (t <= 0 || t > MAX_TYPE || !available)
    {
        printf("Incorrect type specified. Try again.\n");
        scanf("%d", &t);

        available = G_available_types[t] == 1;
    }

    *G_notification_type = t;

    // Notification request
    struct message_event sub_msg;

    sub_msg.mtype = get_message_type(DISPATCHER_ID, CLI2DISP_SUBSCRIBE);

    sub_msg.payload.numbers[0] = G_client_id;
    sub_msg.payload.numbers[1] = *G_notification_type;
    if (msgsnd(G_system_queue, &sub_msg, sizeof(sub_msg.payload), 0) == -1)
    {
        perror("msgsnd error");
        exit(EXIT_FAILURE);
    }

    G_listen = 1;
    listen_to_notification(*G_notification_type);
}

void logout()
{
    struct message_event logout;
    logout.mtype = get_message_type(DISPATCHER_ID, CLI2DISP_LOGOUT);
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
    struct message_event fetch;
    fetch.mtype = get_message_type(DISPATCHER_ID, CLI2DISP_FETCH);
    fetch.payload.number = G_client_id;
    if (msgsnd(G_system_queue, &fetch, sizeof(fetch.payload), 0) == -1)
    {
        perror("fetch msgsnd error");
    }

    ssize_t msg_size;
    struct message_event fetch_response;
    message_type fetch_response_type = get_message_type(G_client_id, DISP2CLI_AVAILABLE_TYPES);

    while (1)
    {
        uint8_t has_notifications = 0;

        msg_size = msgrcv(G_system_queue, &fetch_response, sizeof(fetch_response.payload), fetch_response_type, IPC_NOWAIT);
        if (msg_size != -1)
        {
            for (int i = 0; i <= MAX_TYPE; i++)
            {
                if (fetch_response.payload.numbers[i] == 1)
                {
                    printf("Type %d is available\n", i);
                    G_available_types[i] = 1;
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

    notification_request();
}

void unsubscribe()
{
    struct message_event unsub;
    unsub.mtype = get_message_type(DISPATCHER_ID, CLI2DISP_UNSUBSCRIBE);
    unsub.payload.numbers[0] = G_client_id;
    unsub.payload.numbers[1] = *G_notification_type;
    if (msgsnd(G_system_queue, &unsub, sizeof(unsub.payload), 0) == -1)
    {
        perror("unsubscribe msgsnd error");
    }
    else
    {
        printf("\nNotification unsubscribed\n");
        G_listen = 0;
        *G_notification_type = -1;
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

    int shmid = shmget(AVAILABLE_TYPES_SHM_KEY, sizeof(uint32_t) * (MAX_TYPE + 1), 0600 | IPC_CREAT);
    G_available_types = (uint32_t *)shmat(shmid, NULL, 0);

    if (G_available_types == (uint32_t *)-1)
    {
        perror("G_available_types shmat failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i <= MAX_TYPE; ++i)
    {
        G_available_types[i] = 0;
    }

    int mtshmid = shmget(NOTIFICATION_TYPE_SHM_KEY, sizeof(int), 0600 | IPC_CREAT);
    G_notification_type = (int *)shmat(mtshmid, NULL, 0);

    if (G_notification_type == (int *)-1)
    {
        perror("G_notification_type shmat failed");
        exit(EXIT_FAILURE);
    }

    *G_notification_type = -1; // -1 means not set yet

    G_system_queue = msgget(C_SYSTEM_QUEUE_ID, 0600 | IPC_CREAT);

    G_client_id = login(client_id);

    pid_t cpid = fork();

    if (cpid != 0)
    {
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
    }
    else
    {
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        listen_to_new_types();
    }

    return 0;
}