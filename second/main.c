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
#include <unistd.h>


#define SIZE 1024

// шифрование
void cipher(char* text, int start, int end) {
    for(int i = start; i < end; i++) {
        text[i] = text[i] + 2;
    }
}

int main(int argc, char *argv[]) {
    char buffer;
    int fd, first, second, pid1, pid2;
    semt sem1, sem2;
    FILE *file = fopen(argv[1], "r"); // Чтение тестового файла
    if (file == NULL) {
        printf("Ошибка чтения тестового файла");
        perror("fopen");
        exit(1);
    }
    fgets(buffer, SIZE, file);
    fclose(file);
    fd = shmopen("/shared_memory", OCREAT | ORDWR, 0666); // Создание памяти и семофоров
    ftruncate(fd, SIZE);
    char* text = mmap(NULL, SIZE, PROTREAD | PROTWRITE, MAPSHARED, fd, 0);
    seminit(&sem1, 1, 1);
    seminit(&sem2, 1, 1);
    strncpy(text, buffer, SIZE); // Копирование текста в память
    pid1 = fork();
    if(pid1 == 0) { // работа процессов-шифровальщиков
        while(1) {
            semwait(&sem1);
            first = strlen(text) / 2;
            second = strlen(text);
            if(first >= second) {
                sempost(&sem1);
                break;
            }
            cipher(text, first, second);
            sempost(&sem1);
        }
        exit(0);
    } else {
        pid2 = fork();
        if(pid2 == 0) {
            while(1) {
                semwait(&sem2);
                first = 0;
                second = strlen(text) / 2;
                if(first >= second) {
                    sempost(&sem2);
                    break;
                }
                cipher(text, first, second);
                sempost(&sem2);
            }
            exit(0);
        } else {
            while(1) {
                semwait(&sem1);
                first = 0;
                second = strlen(text) / 2;
                if(first >= second) {
                    sempost(&sem1);
                    break;
                }
                cipher(text, first, second);
                sempost(&sem1);
                semwait(&sem2);
                first = strlen(text) / 2;
                second = strlen(text);
                if(first >= second) {
                    sempost(&sem2);
                    break;
                }
                cipher(text, first, second);
                sempost(&sem2);
            }

            // Вывод зашифрованного текста и запись его в файл
            file = fopen(argv[2], "w"); // запись результата
            if (file == NULL) {
                printf("Ошибка чтения файла-результата");
                perror("fopen");
                exit(1);
            }
            fprintf(file, "%s", text);
            fclose(file);
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
            semdestroy(&sem1);
            semdestroy(&sem2);
            munmap(text, SIZE);
            shmunlink("/shared_memory");
        }
    }

    return 0;
}