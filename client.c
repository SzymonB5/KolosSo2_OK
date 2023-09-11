//
// Created by root on 9/11/23.
//

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>


typedef struct {
    _Bool isAsc;
    int numberCount;
} task_info_t;

sem_t *clientSem;
sem_t *serverSem;

int main(void) {
    serverSem = sem_open("serverSem", O_CREAT, 0777, 0);
    clientSem = sem_open("clientSem", O_CREAT, 0777, 0);

    FILE *fp = fopen("file.txt", "r");
    int numbCount = 0;
    int buf;
    while (1) {
        if (fscanf(fp, "%d", &buf) != 1) {
            break;
        }
        numbCount++;
        if (fgetc(fp) == EOF) {
            break;
        }
    }

    printf("%d\n", numbCount);
    fclose(fp);
    int *arr = calloc(numbCount, sizeof(int));
    if (!arr) {
        return 1;
    }

    FILE *fr = fopen("file.txt", "r");

    for (int i = 0; i < numbCount; i++) {
        fscanf(fr, "%d", arr + i);
        fgetc(fr);
    }
    fclose(fr);

    task_info_t *taskInfo;
    int *sharedArr;

    int fdArr = shm_open("array", O_CREAT | O_RDWR, 0777);
    ftruncate(fdArr, (__off_t) numbCount * (long) sizeof(int));
    sharedArr = mmap(NULL, (__off_t) numbCount * (long) sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fdArr, 0);
    memcpy(sharedArr, arr, (__off_t) numbCount * (long) sizeof(int));

    puts("-------------");
    int fd = shm_open("shared_mem", O_RDWR, 0777);
    taskInfo = mmap(NULL, sizeof(task_info_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    taskInfo->isAsc = 1;
    taskInfo->numberCount = numbCount;

    sem_post(clientSem);

    sem_wait(serverSem);

    for (int i = 0; i < numbCount; i++)
        printf("%d ", sharedArr[i]);

    free(arr);

    munmap(taskInfo, (long) numbCount * (long) sizeof(int));
    munmap(sharedArr, sizeof(task_info_t));

    close(fd);
    close(fdArr);
    shm_unlink("array");
    return 0;
}
