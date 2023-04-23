#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>

#define SHM_SIZE 1024
#define SEM_NAME "/sem_name"

int main(int argc, char *argv[]) {
    int shmid, i, j, n, count, first, second, help, remainder;
    char *s, *input, *output;
    srand(time(NULL));
    count = rand() % 3; // Получаем количество процессов
    sem_t *sem;
    key_t key = 12345;
    if ((shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666)) < 0) { // Разделяемая память
        printf("failed shmget");
        exit(1);
    }
    if ((s = shmat(shmid, NULL, 0)) == (char *) -1) {
        printf("failed shmat");
        exit(1);
    }
    sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0644, 1); // Создание именованного семафора
    if (sem == SEM_FAILED) {
        printf("failed sem_open");
        exit(1);
    }
    FILE *file = fopen(argv[1], "r"); // Чтение тестового файла
    if (file == NULL) {
        printf("Ошибка чтения тестового файла");
        perror("fopen");
        exit(1);
    }
    fseek(file, 0L, SEEK_END);
    n = ftell(file);
    fseek(file, 0L, SEEK_SET);
    input = (char *) malloc(n + 1);
    fread(input, 1, n, file);
    fclose(file);
    input[n] = '\0';
    help = n / count;
    remainder = n % count;
    for (i = 0; i < count; i++) {
        first = i * help;
        second = first + help;
        if (i == count - 1) {
            second += remainder;
        }
        sem_wait(sem);
        memcpy(s, input + first, second - first); // Запись кусочка текста в память
        sem_post(sem);
        pid_t pid = fork();
        if (pid < 0) {
            exit(1);
        } else if (pid == 0) {
            sem_wait(sem);
            char *text_chunk = (char *) malloc(second - first + 1);
            memcpy(text_chunk, s, second - first);
            text_chunk[second - first] = '\0';
            sem_post(sem);
            // Шифруем текст
            for (j = 0; j < second - first; j++) {
                text_chunk[j] = text_chunk[j] + 2; // само шифрование
            }
            sem_wait(sem);
            memcpy(s, text_chunk, second - first); // Запись зашифрованного кусочка текста в память
            sem_post(sem);
            free(text_chunk);
            exit(0);
        }
    }
    for (i = 0; i < count; i++) {
        wait(NULL); // ждем, пока все процессы доделают свою работу
    }
    output = (char *) malloc(n + 1);
    output[n] = '\0';
    for (i = 0; i < count; i++) {
        first = i * help;
        second = first + help;
        if (i == count - 1) {
            second += remainder;
        }
        sem_wait(sem);
        memcpy(output + first, s, second - first);
        sem_post(sem);
    }
    file = fopen(argv[2], "w"); // запись результата
    if (file == NULL) {
        printf("Ошибка чтения файла-результата");
        perror("fopen");
        exit(1);
    }
    fwrite(output, 1, n, file);
    fclose(file);
    free(input);
    free(output);
    shmdt(s);
    shmctl(shmid, IPC_RMID, NULL);
    sem_unlink(SEM_NAME);

    return 0;
}