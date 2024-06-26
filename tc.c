#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // for sleep(), usleep()
#include <pthread.h> // the header file for the pthread lib
#include <fcntl.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <string.h>
#include <stdint.h> 
#include <sys/resource.h> 
#include <time.h>
#include "message.h"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/mman.h>

int timestop = 0;
pthread_mutex_t timestop_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t reboot_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t param_mutex = PTHREAD_MUTEX_INITIALIZER;

FILE *fp;
char path[1035];
Message struct_type = {0};
Message struct_to_send = {0};
Message struct_to_receive = {0}; 

#define SHM_NAME_REBOOT "/reboot_status_shm"
#define SHM_NAME_SHUTDOWN "/shutdown_time_shm"
#define SHM_SIZE sizeof(uint32_t)

int *reboot_status = 0;
uint32_t *shutdown_time = 0;

void setup_shared_memory() {
    // ตั้งค่าหน่วยความจำที่ใช้ร่วมกันสำหรับ reboot_status
    int shm_fd_reboot = shm_open(SHM_NAME_REBOOT, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (shm_fd_reboot == -1) {
        perror("shm_open reboot");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd_reboot, SHM_SIZE) == -1) {
        perror("ftruncate reboot");
        exit(EXIT_FAILURE);
    }

    reboot_status = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_reboot, 0);
    if (reboot_status == MAP_FAILED) {
        perror("mmap reboot");
        exit(EXIT_FAILURE);
    }
    *reboot_status = 0;
    
    close(shm_fd_reboot);

    int shm_fd_shutdown = shm_open(SHM_NAME_SHUTDOWN, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (shm_fd_shutdown == -1) {
        perror("shm_open shutdown");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd_shutdown, SHM_SIZE) == -1) {
        perror("ftruncate shutdown");
        exit(EXIT_FAILURE);
    }

    shutdown_time = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_shutdown, 0);
    if (shutdown_time == MAP_FAILED) {
        perror("mmap shutdown");
        exit(EXIT_FAILURE);
    }
    *shutdown_time = 0;
    
    close(shm_fd_shutdown);
}

void* OBCReboot(void* arg) {
    uint32_t utime = (uint32_t)time(NULL);
    printf("Current time: %u\n", utime); 

    struct_to_send.val = utime + struct_to_send.param;
    printf("Predict time: %u\n", struct_to_send.val);  
    *shutdown_time = struct_to_send.val;
    while (1) {
        for (int i = 0; i <= struct_to_send.param; i++) {
            uint32_t ftime = (uint32_t)time(NULL);
            printf("Time: %u\n", ftime);  
            sleep(1);
            pthread_mutex_lock(&timestop_mutex);
            if (timestop == 1) {
                pthread_mutex_unlock(&timestop_mutex); 
                printf("Cancel\n");
                break;
            }
            pthread_mutex_unlock(&timestop_mutex);
        }
        pthread_mutex_lock(&reboot_mutex);
        *reboot_status = STANDBY;
        pthread_mutex_unlock(&reboot_mutex);
        *shutdown_time = 0;
        printf("Reboot done!\n");
        printf("Reboot Status: %d\n", *reboot_status);
        break;
    } 
    printf("-------------------------------------------\n");
    pthread_exit(NULL); // Exit thread when done
}

void CancelReboot(){
    pthread_mutex_lock(&timestop_mutex);
    timestop = 1;
    pthread_mutex_unlock(&timestop_mutex);
}

void* OBCShutdown(void* arg) {
    uint32_t utime = (uint32_t)time(NULL);
    printf("Current time: %u\n", utime); 
 

    struct_to_send.val = utime + struct_to_send.param;
    printf("Predict time: %u\n", struct_to_send.val);  
    *shutdown_time = struct_to_send.val;

    while (1) {
        for (int i = 0; i <= struct_to_send.param; i++) {
            uint32_t ftime = (uint32_t)time(NULL);
            printf("Time: %u\n", ftime);  
            sleep(1);
            pthread_mutex_lock(&timestop_mutex);
            if (timestop == 1) {
                pthread_mutex_unlock(&timestop_mutex);
                printf("Cancel\n");
                break;
            }
            pthread_mutex_unlock(&timestop_mutex);
        }
        pthread_mutex_lock(&reboot_mutex);
        *reboot_status = STANDBY;
        pthread_mutex_unlock(&reboot_mutex);
        *shutdown_time = 0;
         pthread_mutex_unlock(&param_mutex);
         struct_to_send.param = 0;
         pthread_mutex_lock(&param_mutex);
        printf("Shutdown done!\n");
        printf("Reboot Status: %d\n", *reboot_status);

        
        break;
    } 
    printf("-------------------------------------------\n");
    pthread_exit(NULL); // Exit thread when done
}

////TC FUNCTION 
void* tc(void* arg) {
    mqd_t mqdes_receive_tc = mq_open("/mq_tc", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes);
    if (mqdes_receive_tc == -1) {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    mqd_t mqdes_send_tc = mq_open("/mq_tc", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes);
    if (mqdes_send_tc == -1) {
        perror("mq_open");
        exit(EXIT_FAILURE);  
    } 
 
    while (1) {
        if (mq_receive(mqdes_receive_tc, (char *)&struct_to_receive, sizeof(struct_to_receive), NULL) == -1) {
            perror("mq_receive");
            exit(EXIT_FAILURE);  
        }
        
        printf("%d\n", *reboot_status);
        printf("Module ID from sender: %u\n", struct_to_receive.mdid);
        printf("Telemetry ID: %u\n", struct_to_receive.req_id);   
        struct_to_send.mdid = struct_to_receive.mdid;
        struct_to_send.req_id = struct_to_receive.req_id;
        struct_to_send.type = struct_to_receive.type; 
        struct_to_send.val = struct_to_receive.val;
        

        printf("Type: %u\n", struct_to_send.type); 
        printf("Module ID: %u\n", struct_to_send.mdid);
        printf("Telemetry ID: %u\n", struct_to_send.req_id);
        printf("Parameter: %u\n", struct_to_send.param);  
        if (*reboot_status == STANDBY){
            if (struct_to_send.type == TC_REQUEST) {
                struct_to_send.type = TC_RETURN;
            }
            struct_to_send.val = *reboot_status;
            printf("Reboot Status1: %d\n", struct_to_send.val);
            if (mq_send(mqdes_send_tc, (char *)&struct_to_send, sizeof(struct_to_send), 1) == -1) {
                perror("mq_send");
                exit(EXIT_FAILURE); 
            }      
            if (struct_to_send.mdid == 1) {
                if (struct_to_send.req_id == 1 && *reboot_status == STANDBY) {    
                    pthread_mutex_lock(&timestop_mutex);
                    timestop = 0; 
                    pthread_mutex_unlock(&timestop_mutex); 
                    
                    pthread_mutex_lock(&reboot_mutex);
                    *reboot_status = REBOOT;
                    pthread_mutex_unlock(&reboot_mutex);
                      
                    pthread_t obc_reboot_thread;
                    pthread_create(&obc_reboot_thread, NULL, OBCReboot, NULL);
                    pthread_mutex_unlock(&param_mutex);
                    struct_to_send.param = struct_to_receive.param;
                    pthread_mutex_lock(&param_mutex);
                    pthread_detach(obc_reboot_thread);                 
                }

                else if (struct_to_send.req_id == 2 && *reboot_status == STANDBY) {
                      printf("Hello");    
                    pthread_mutex_lock(&timestop_mutex);
                    timestop = 0; 
                    pthread_mutex_unlock(&timestop_mutex); 
                    
                    pthread_mutex_lock(&reboot_mutex);
                    *reboot_status = SHUTDOWN;
                    pthread_mutex_unlock(&reboot_mutex);
                      
                    pthread_t obc_shutdown_thread;
                    pthread_create(&obc_shutdown_thread, NULL, OBCShutdown, NULL);
                    pthread_mutex_unlock(&param_mutex);
                    struct_to_send.param = struct_to_receive.param;
                    pthread_mutex_lock(&param_mutex);
                    pthread_detach(obc_shutdown_thread);                 
                }
                else { 
                  struct_to_send.val = 1; 
                  printf("Don't have Shutdown Command\n");
                }
                
            }
                                  
        }
        else if (*reboot_status == REBOOT){
             if (struct_to_send.type == TC_REQUEST) {
              struct_to_send.type = TC_RETURN;
            }
            struct_to_send.val = *reboot_status;
            printf("Reboot Status: %d\n", struct_to_send.val);
            if (mq_send(mqdes_send_tc, (char *)&struct_to_send, sizeof(struct_to_send), 1) == -1) {
                perror("mq_send");
                exit(EXIT_FAILURE); 
            }
            else if (struct_to_send.req_id == 1 && *reboot_status == REBOOT) {
                    struct_to_send.val = 1;
                    printf("Reboot in progress\n");
                    
                }
            else if (struct_to_send.req_id == 2 && *reboot_status == REBOOT) {
                    struct_to_send.val = 1;
                    printf("Reboot in progress\n");
                    
                }
            else if (struct_to_send.req_id == 3) {
                  CancelReboot();
                  pthread_mutex_lock(&reboot_mutex);
                  *reboot_status = 0;
                  pthread_mutex_unlock(&reboot_mutex);
                  if (struct_to_send.type == TC_REQUEST) {
                      struct_to_send.type = TC_RETURN;
                } 
                    
            }  
        } 
        
        else if (*reboot_status == SHUTDOWN){
             if (struct_to_send.type == TC_REQUEST) {
              struct_to_send.type = TC_RETURN;
            }
            struct_to_send.val = *reboot_status;
            printf("Reboot Status: %d\n", struct_to_send.val);
            if (mq_send(mqdes_send_tc, (char *)&struct_to_send, sizeof(struct_to_send), 1) == -1) {
                perror("mq_send");
                exit(EXIT_FAILURE); 
            }
            else if (struct_to_send.req_id == 1 && *reboot_status == SHUTDOWN) {
                    struct_to_send.val = 1;
                    printf("Shutdown in progress\n");
                    
                }
            else if (struct_to_send.req_id == 2 && *reboot_status == SHUTDOWN) {
                    struct_to_send.val = 1;
                    printf("Shutdown in progress\n");
                    
                }
            else if (struct_to_send.req_id == 3) {
                  CancelReboot();
                  pthread_mutex_lock(&reboot_mutex);
                  *reboot_status = STANDBY;
                  pthread_mutex_unlock(&reboot_mutex);
                  if (struct_to_send.type == TC_REQUEST) {
                      struct_to_send.type = TC_RETURN;
                } 
                    
            }  
        } 

        printf("--Respond Success!--\n");  
    }

    mq_close(mqdes_receive_tc);
    mq_unlink("/mq_tc");  

    return NULL;
}

int main(int argc, char *argv[]) {
    setup_shared_memory();

    pthread_t tc_thread;

    pthread_create(&tc_thread, NULL, tc, NULL);
    pthread_join(tc_thread, NULL);

    return 0;
}