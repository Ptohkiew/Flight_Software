#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <mqueue.h>
#include <pthread.h>
#include "message.h"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/mman.h>


 
#define SHM_SIZE sizeof(uint32_t)
#define SHM_NAME_LOG "/log_shm"

Message log_send = {0};
Message log_receive = {0};

FILE *fp;
char path[1035];
uint32_t obc_time;
char housekpfile[30];
uint32_t end_time;
int temp, re_log = 0;
uint32_t remain_mem;
uint32_t ram_usage; 
uint32_t ram_peak;
uint32_t MAX_FILE_SIZE = 0; 

uint32_t num_file = 0;
uint32_t file_size = 0;
uint32_t period = 0;


pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
int *log_status = 0;


void setup_shared_memory() {
    int shm_fd_log = shm_open(SHM_NAME_LOG, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (shm_fd_log == -1) {
        perror("shm_open log");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd_log, SHM_SIZE) == -1) {
        perror("ftruncate shutdown");
        exit(EXIT_FAILURE);
    }
    

    log_status = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_log, 0);
    if (log_status == MAP_FAILED) {
        perror("mmap log");
        exit(EXIT_FAILURE);
    }
    if (log_status == NULL) {
        fprintf(stderr, "log_status is NULL\n");
        exit(EXIT_FAILURE);
    }

    *log_status = 0;
    
    close(shm_fd_log);
}


void log_data(const char *message) {
    FILE *logfile2 = fopen(housekpfile, "a");
    if (logfile2 == NULL) {
        fprintf(stderr, "Error opening log file\n");
        return;
    }

    obc_time = (uint32_t)time(NULL);
    fprintf(logfile2, "%u, %s\n", obc_time, message);
    printf("Data written successfully.\n");
    fclose(logfile2);

    
}

void collect_and_log_data() {
    char log_message[100];
    snprintf(log_message, sizeof(log_message), "%d, %u, %u, %u",
             temp, remain_mem, ram_usage, ram_peak);

    log_data(log_message);
}

void read_config(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Could not open config file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    
    while (fgets(line, sizeof(line), file)) {
        char key[256];
        uint32_t value;
        if (sscanf(line, "%255[^=]=%u", key, &value) == 2) {
            if (strcmp(key, "num_file") == 0) {
                num_file = value;
            } else if (strcmp(key, "file_size") == 0) {
                file_size = value;
            } else if (strcmp(key, "period") == 0) {
                period = value;
            }
        }
    }

    fclose(file);
}


int main() {
    setup_shared_memory();
    read_config("log.conf");
    while (1) {
        read_config("log.conf");
        int num, type, mdid, req_id;
        char buffer[256];
        for (int i = 0; i < num_file; i++) {
            read_config("log.conf");                              
            sprintf(housekpfile, "hkp_log_%d.txt", i);

            FILE *logfile1 = fopen(housekpfile, "w");
            if (logfile1 == NULL) {
                perror("fopen");
                exit(EXIT_FAILURE); 
            }
            
            fprintf(logfile1, "Housekeeping Log\n");
            obc_time = (uint32_t)time(NULL);
            fprintf(logfile1, "Start Time : %u\n", obc_time);
            fprintf(logfile1, "End Time : %10u\n", end_time); 
            fprintf(logfile1, "Period : %u\n", period);
            fprintf(logfile1, "\n#Channel Information\n");
            fprintf(logfile1, "1 ,0, 1, 1, \"CPU_Temperature\", 0, 0.001, 0, ?C\n");
            fprintf(logfile1, "2 ,0, 1, 4, \"Memory_Remain\", 0, 1, 0, MB\n");
            fprintf(logfile1, "3 ,0, 1, 9, \"RAM_Usage\", 0, 1, 0, MB\n");
            fprintf(logfile1, "4 ,0, 1, 10, \"RAM_Peak\", 0, 1, 0, MB\n");
            fprintf(logfile1, "\n#Data\n");
            fclose(logfile1);
 
            while (1) {   
                read_config("log.conf");
                pthread_mutex_lock(&log_mutex);
                if (*log_status == 1) {
                    printf("Log Status : %d\n", *log_status);
                    pthread_mutex_unlock(&log_mutex);
                    pthread_mutex_lock(&log_mutex);
                    *log_status = 0;
                    pthread_mutex_unlock(&log_mutex);
                    printf("1 MB.\n");
                    end_time = (uint32_t)time(NULL);
                    logfile1 = fopen(housekpfile, "r+");
                    fseek(logfile1, 41, SEEK_SET);
                    fprintf(logfile1, "End Time : %10u\n", end_time);
                    fclose(logfile1);
                    break;
                }
                pthread_mutex_unlock(&log_mutex);   
                       
                struct stat file_info; 
                if (stat(housekpfile, &file_info) == -1) {
                    perror("stat"); 
                    exit(EXIT_FAILURE);
                }

                if (file_info.st_size > file_size) {
                    printf("1 MB.\n");
                    end_time = (uint32_t)time(NULL);
                    logfile1 = fopen(housekpfile, "r+");
                    fseek(logfile1, 41, SEEK_SET); 
                    fprintf(logfile1, "End Time : %10u\n", end_time);
                    fclose(logfile1);
                    break;
                }
                

                logfile1 = fopen(housekpfile, "r");
                if (logfile1 == NULL) { 
                    perror("fopen");
                    exit(EXIT_FAILURE);
                }

                while (fgets(buffer, sizeof(buffer), logfile1) != NULL) {
                    read_config("log.conf");
                    if (sscanf(buffer, "%d ,%d, %d, %d", &num, &type, &mdid, &req_id) == 4) {
                        log_send.type = type;
                        log_send.mdid = mdid;
                        log_send.req_id = req_id;
                        printf("type = %u\n", log_send.type);
                        printf("mdid = %u\n", log_send.mdid);
                        printf("req = %u\n", log_send.req_id);

                        mqd_t mqdes_send = mq_open("/mq_dispatch", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes);
                        if (mqdes_send == -1) {
                            perror("mq_open");
                            exit(EXIT_FAILURE);
                        }

                        mqd_t mqdes_receive = mq_open("/mq_dispatch", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes);
                        if (mqdes_receive == -1) {
                            perror("mq_open");
                            mq_close(mqdes_send);
                            exit(EXIT_FAILURE);
                        }
 
                        if (mq_send(mqdes_send, (char *)&log_send, sizeof(log_send), 1) == -1) {
                            perror("mq_send");
                            mq_close(mqdes_send);
                            mq_close(mqdes_receive);
                            exit(EXIT_FAILURE);
                        }

                        if (mq_receive(mqdes_receive, (char *)&log_receive, sizeof(log_receive), NULL) == -1) {
                            perror("mq_receive");
                            mq_close(mqdes_receive);
                            mq_close(mqdes_send);
                            exit(EXIT_FAILURE);
                        }

                        if (log_send.req_id == 1) {
                            temp = log_receive.val;
                        } else if (log_send.req_id == 4) {
                            remain_mem = log_receive.val;
                        } else if (log_send.req_id == 9) {
                            ram_usage = log_receive.val;
                        } else if (log_send.req_id == 10) {
                            ram_peak = log_receive.val;
                        }

                        if (num == 4 && type == 0 && mdid == 1 && req_id == 10) {
                            break;
                        }

                        mq_close(mqdes_send);
                        mq_close(mqdes_receive);
                    }
                }
                fclose(logfile1);

                collect_and_log_data();
                sleep(period);
            }
        }
    }
    return 0;
}
