#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>

#include "inf160133_160232_types.h"

void login(int client_id, int queue){  

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

    //Login response
    ssize_t msg_size;
    struct system_message response;
    system_type ok_id = get_system_type(client_id,DISP2CLI_LOGIN_OK);
    system_type failed_id = get_system_type(client_id,DISP2CLI_LOGIN_FAILED);
    
    while(1){
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
            login(client_id,queue);
            break;
        }
        else if (errno != ENOMSG)
        {
            perror("msgrcv error on producer system queue\n");
        }
    }
}

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
    int queue = msgget(C_SYSTEM_QUEUE_ID, 0600 | IPC_CREAT);
    login(client_id,queue);

    //Fetch
    struct system_message fetch;
    fetch.mtype = get_system_type(DISPATCHER_ID, CLI2DISP_FETCH);
    fetch.payload.number = client_id; 
    if(msgsnd(queue,&fetch,sizeof(fetch.payload),0)==-1){
        printf("msgsnd error\n");
    }
    else{
        printf("Fetch done\n");
    }

    ssize_t msg_size;
    struct system_message fetch_response;
    system_type fetch_response_type = get_system_type(client_id, DISP2CLI_AVAILABLE_TYPES);
    
    while(1){
    msg_size = msgrcv(queue, &fetch_response, sizeof(fetch_response.payload), fetch_response_type, IPC_NOWAIT);
        if (msg_size != -1)
        {
            printf("Got here\n");
            for(int i=1;i<11;i++){
                if(fetch_response.payload.numbers[i]==1){
                    printf("Type %d available",i);
                }
            }
            break;
        }
        else if (errno != ENOMSG)
        {
            perror("msgrcv error on producer system queue\n");
        }
    }
    //Choosing notification type
    // printf("What types do you want to receive:(for now only choose one)\n 1 - ALERT RCB\n 2 - WEATHER NOTIFICATION\n 3 - POLITICAL NEWS \n");
    // int not_type, t;
    // scanf("%d",&t);
    // while(t<=0||t>3){
    //     printf("Incorrect type of message required\n Try again\n");
    //     scanf("%d",&t);
    // }
    // not_type = t;

    // //Notification request
    // struct system_message msg2;

    // msg2.mtype = get_system_type(DISPATCHER_ID, CLI2DISP_SUBSCRIBE);

    // msg2.payload.numbers[0] = client_id;
    // msg2.payload.numbers[1] = not_type;
    // printf("You want to receive: %ld\n", msg2.mtype);
    // if (msgsnd(queue, &msg2, sizeof(msg2.payload), 0) == -1)
    // {
    //     perror("msgsnd error");
    //     exit(1);
    // }
    // else
    // {
    //     printf("Message sent!\n");
    // }

    return 0;
}