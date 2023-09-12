//
// Created by root on 9/11/23.
//
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

_Atomic volatile int ascTasks = 0;
_Atomic volatile int descTasks = 0;

sem_t finalizeSem;

sem_t *clientSem;
sem_t *serverSem;

pthread_mutex_t serverPerforming;

pthread_t p1, p2, p3;

typedef struct {
    _Bool isAsc;
    int numberCount;
} task_info_t;

void *command(void *arg) {
    char command[5];
    while (1) {
        scanf("%s", command);
        if (strcmp(command, "quit") == 0)
            sem_post(&finalizeSem);
        else if (strcmp(command, "stat") == 0)
            printf("Current stats: %d %d\n", ascTasks, descTasks);
        else
            printf("Invalid command");
    }
}

void *finalize(void *arg) {
    sem_wait(&finalizeSem);
    pthread_cancel(p1);

    pthread_mutex_lock(&serverPerforming);
    pthread_cancel(p3);
    pthread_mutex_unlock(&serverPerforming);

    return NULL;
}

int fd;
task_info_t *taskInfo;

_Noreturn void *routine(void *arg) {
    fd = shm_open("shared_mem", O_CREAT | O_RDWR, 0666);
    ftruncate(fd, sizeof(task_info_t));
    taskInfo = mmap(NULL, sizeof(task_info_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    while (1) {
        sem_wait(clientSem);
        pthread_mutex_lock(&serverPerforming);
        int fdArr = shm_open("array", O_RDWR, 0777);
        int *arr = mmap(NULL, sizeof(int) * taskInfo->numberCount, PROT_READ | PROT_WRITE, MAP_SHARED, fdArr, 0);

        for (int i = 0; i < taskInfo->numberCount - 1; i++) {
            for (int j = 0; j < taskInfo->numberCount - 1; j++) {
                if (arr[j] > arr[j + 1]) {
                    int buf = arr[j];
                    arr[j] = arr[j + 1];
                    arr[j + 1] = buf;
                }
            }
        }

        sem_post(serverSem);

        close(fdArr);
        munmap(arr, sizeof(int) * taskInfo->numberCount);
        pthread_mutex_unlock(&serverPerforming);
    }

}

int main(void) {
    sem_init(&finalizeSem, 0, 0);
    pthread_mutex_init(&serverPerforming, NULL);

    serverSem = sem_open("serverSem", O_CREAT, 0777, 0);
    clientSem = sem_open("clientSem", O_CREAT, 0777, 0);

    pthread_create(&p1, NULL, command, NULL);
    pthread_create(&p2, NULL, finalize, NULL);
    pthread_create(&p3, NULL, routine, NULL);
    pthread_join(p2, NULL);

    munmap(taskInfo, sizeof(task_info_t));
    close(fd);
    shm_unlink("shared_mem");

    sem_close(serverSem);
    sem_close(clientSem);
    sem_destroy(&finalizeSem);
    pthread_mutex_destroy(&serverPerforming);

    return 0;
}
