#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>

#include "inf160133_160232_types.h"

int main()
{
    int queue = msgget(P_SYSTEM_QUEUE_ID, 0600 | IPC_CREAT);

    struct system_message msg;

    msg.mtype = get_system_type(DISPATCHER_ID, PROD2DISP_LOGIN);

    // char text[] = "Hello!";

    msg.payload.numbers[0] = 4;   // Producer ID
    msg.payload.numbers[1] = 420; // Type to listen to

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

    return 0;
}