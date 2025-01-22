#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>

#include "inf160133_160232_types.h"

int main(int argc, char *argv[])
{
    int client_id = atoi(argv[1]);
    while (client_id<=0)
    {
        int id;
        printf("Incorrect id!\n");
        printf("Client id must be higher than 0\n");
        scanf("%d",&id);
        client_id =id;
    }
    printf("Correct id\n");
    int queue = msgget(C_SYSTEM_QUEUE_ID, 0600 | IPC_CREAT);  

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

    struct system_message response;
    system_type expected_ok = get_system_type(client_id, DISP2CLI_LOGIN_OK);
    system_type expected_failed = get_system_type(client_id, DISP2CLI_LOGIN_FAILED);

    ssize_t received_size = msgrcv(queue, &response, sizeof(response.payload), client_id, 0);

    if (received_size == -1)
    {
        perror("Error receiving response");
        exit(1);
    }

    if (response.mtype == expected_ok)
    {
        printf("Login successful for client ID: %d\n", client_id);
    }
    else if (response.mtype == expected_failed)
    {
        printf("Login failed for client ID: %d. Client under this id already exists. PLease try different id\n", client_id);
    }

    printf("What types do you want to receive:\n 1 - ALERT RCB\n 2 - WEATHER NOTIFICATION\n 3 - POLITICAL NEWS \n");
    int not_type, t;
    scanf("%d",&t);
    while(t<=0||t>3){
        printf("Incorrect type of message required\n Try again\n");
        scanf("%d",&t);
    }
    not_type = t;
    printf("You want to receive type %d message (for now only choose one)\n",not_type);

    //Notification request
    struct system_message msg2;

    msg2.mtype = get_system_type(DISPATCHER_ID, CLI2DISP_SUBSCRIBE);

    msg2.payload.numbers[0] = client_id;
    msg2.payload.numbers[1] = not_type;
    printf("Message ID: %ld\n", msg2.mtype);
    if (msgsnd(queue, &msg2, sizeof(msg2.payload), 0) == -1)
    {
        perror("msgsnd error");
        exit(1);
    }
    else
    {
        printf("Message sent!\n");
    }

    return 0;
}