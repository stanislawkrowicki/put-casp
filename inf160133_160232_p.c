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
    if (argc != 3)
    {
        printf("Podaj ID i typ jaki będziesz produkować.");
        return 1;
    }

    int id = atoi(argv[1]);
    int type = atoi(argv[2]);

    if (id < 1 || id > MAX_ID)
    {
        printf("ID musi być pomiędzy 1 a %d\n", MAX_ID);
        return 1;
    }

    if (type < 1 || type > MAX_NOTIFICATION)
    {
        printf("Typ wiadomości musi być pomiędzy 1 a %d.", MAX_NOTIFICATION);
        return 1;
    }

    int queue = msgget(P_SYSTEM_QUEUE_ID, 0600 | IPC_CREAT);

    struct system_message msg;

    msg.mtype = get_system_type(DISPATCHER_ID, PROD2DISP_LOGIN);

    // char text[] = "Hello!";

    msg.payload.numbers[0] = id;   // Producer ID
    msg.payload.numbers[1] = type; // Type to listen to

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