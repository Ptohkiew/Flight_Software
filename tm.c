//TM
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


int sent = 0;
int temp;
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

    // ตั้งค่าหน่วยความจำที่ใช้ร่วมกันสำหรับ shutdown_time
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
    *reboot_status = 0;
    close(shm_fd_shutdown);
}

void tm_cpu_temp() {
    fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (fp == NULL) {
        perror("Failed to open temperature file");
        exit(1);
    }

    if (fscanf(fp, "%d", &temp) != 1) {
        perror("Failed to read temperature");
        fclose(fp);
        exit(1);
    } 
    fclose(fp);

    
    struct_to_send.val = temp;
}

void tm_maxmem() {
    fp = popen("df -BM /dev/mmcblk0p2 | awk 'NR==2 {print $2}'", "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        exit(1);
    }
    if (fgets(path, sizeof(path)-1, fp) != NULL) {
        sscanf(path, "%uM", &struct_to_send.val);     
    } 
    pclose(fp);
}

void tm_usagemem() {
    fp = popen("df -BM /dev/mmcblk0p2 | awk 'NR==2 {print $3}'", "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        exit(1);
    }
    if (fgets(path, sizeof(path)-1, fp) != NULL) {
        sscanf(path, "%uM", &struct_to_send.val);     
    } 
    pclose(fp);
}

void tm_remainmem() {
    fp = popen("df -BM /dev/mmcblk0p2 | awk 'NR==2 {print $4}'", "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        exit(1);
    }
    if (fgets(path, sizeof(path)-1, fp) != NULL) {
        sscanf(path, "%uM", &struct_to_send.val);     
    } 
    pclose(fp);
}


void tm_IP_Address()
{
    uint8_t  ip_address[4];
    struct ifaddrs *ifaddr, *ifa;
    struct_to_send.val = 0;
    int n = 24;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET)
            continue;

        if (strcmp(ifa->ifa_name, "lo") == 0)
            continue;

        struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
  
        ip_address[0] = addr->sin_addr.s_addr & 0xFF;
        ip_address[1] = ((addr->sin_addr.s_addr) >> 8) & 0xFF;
        ip_address[2] = ((addr->sin_addr.s_addr) >> 16) & 0xFF;
        ip_address[3] = ((addr->sin_addr.s_addr) >> 24) & 0xFF;
        printf("%u",ip_address[0]);
        
        for(int i=0; i<4; i++)
        {
           struct_to_send.val = struct_to_send.val | (ip_address[i] << n) ;
           n = n-8;
        }
    } 
    freeifaddrs(ifaddr);    
}

void tm_Shutdown_Time(){
    printf("Shutdown Time : %u\n", *shutdown_time);
    if (*shutdown_time <= 0){
        *shutdown_time = 0;
    }
    printf("Shutdown Time : %u\n", *shutdown_time);
    struct_to_send.val = *shutdown_time;
} 

void tm_ShutRemain_Time(){
    uint32_t cur_time = (uint32_t)time(NULL);
    printf("Current time: %u\n", cur_time); 
    printf("Shutdown Time : %u\n", *shutdown_time);
    uint32_t remain_time = *shutdown_time - cur_time;
    if (*shutdown_time <= 0){
        remain_time = 0;
    }
    printf("Remain Time : %u\n", remain_time);
    struct_to_send.val = remain_time;
}

void tm_Shutdown_Type(){
    printf("Type of shutdown: %u \n", *reboot_status);
    if (*reboot_status == 0){ 
        printf("Standby Mode\n");
    }
    else if (*reboot_status == 1){ 
        printf("Reboot Mode\n");
    }
    else if (*reboot_status == 2){ 
        printf("Shutdown Mode\n");
    }
}

void tm_OBC_Time(){
    uint32_t obc_time = (uint32_t)time(NULL);
    printf("OBC time: %u\n", obc_time); 

    printf("Remain Time : %u\n", obc_time);
    struct_to_send.val = obc_time;
}

////TM FUNCTION
void* tm(void* arg) {
    mqd_t mqdes_receive = mq_open("/mq_tm", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes);
    
    if (mqdes_receive == -1) {
        perror("mq_open");
        exit(EXIT_FAILURE); 
    }

    mqd_t mqdes_send = mq_open("/mq_tm", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes);
    if (mqdes_send == -1) { 
        perror("mq_open");
        exit(EXIT_FAILURE); 
    }
    
   while (1) {
        
        if (mq_receive(mqdes_receive, (char *)&struct_to_receive, sizeof(struct_to_receive), NULL) == -1) {
            perror("mq_receive");
            exit(EXIT_FAILURE); 
        } 
        printf("Module ID from sender : %u\n", struct_to_receive.mdid);
        printf("Telemetry ID : %u\n", struct_to_receive.req_id);   
        
        
        struct_to_send.mdid = struct_to_receive.mdid;
        struct_to_send.req_id = struct_to_receive.req_id;
        struct_to_send.param = 0; 
        struct_to_send.type = struct_to_receive.type;
        struct_to_send.val = struct_to_receive.val;
        
        
        printf("Type : %u\n", struct_to_send.type);
        printf("Module ID : %u\n", struct_to_send.mdid);
        printf("Telemetry ID : %u\n", struct_to_send.req_id);
        if (struct_to_send.type == 0 && struct_to_send.mdid == 1 && struct_to_send.req_id == 1) {
            tm_cpu_temp();
            printf("Telemetry Parameter : %u\n", struct_to_send.param);
            printf("CPU Temperature: %d C\n", struct_to_send.val);
        }        
        else if (struct_to_send.type == 0 && struct_to_send.mdid == 1 && struct_to_send.req_id == 2) {
            tm_maxmem();
            printf("Total space: %u MB\n", struct_to_send.val);
        }
        else if (struct_to_send.type == 0 && struct_to_send.mdid == 1 && struct_to_send.req_id == 3) {
            tm_usagemem();
            printf("Used space: %u MB\n", struct_to_send.val);
        }
        else if (struct_to_send.type == 0 && struct_to_send.mdid == 1 && struct_to_send.req_id == 4) {
            tm_remainmem();
            printf("Available space: %u MB\n", struct_to_send.val);
        }
        else if (struct_to_send.type == 0 && struct_to_send.mdid == 1 && struct_to_send.req_id == 5) {
            tm_IP_Address();
            printf("IP: %u \n", struct_to_send.val);
        }
        else if (struct_to_send.type == 0 && struct_to_send.mdid == 1 && struct_to_send.req_id == 14) {
            tm_Shutdown_Time();
            printf("Shutdown time : %u \n", struct_to_send.val);
        }
        else if (struct_to_send.type == 0 && struct_to_send.mdid == 1 && struct_to_send.req_id == 15) {
            tm_ShutRemain_Time();
            printf("Remain time : %u \n", struct_to_send.val);
        }
        else if (struct_to_send.type == 0 && struct_to_send.mdid == 1 && struct_to_send.req_id == 16) {
            tm_Shutdown_Type();
            printf("Type of shutdown: %u \n", *reboot_status);
            struct_to_send.val = *reboot_status;
        }
        else if (struct_to_send.type == 0 && struct_to_send.mdid == 1 && struct_to_send.req_id == 17) {
            tm_OBC_Time();
            printf("OBC Time: %u \n", struct_to_send.val);
        }
//        else {
//            struct_to_send.type = 1;  
//            if (mq_send(mqdes_send, (char *)&struct_to_send, sizeof(struct_to_send), 1) == -1) {
//              perror("mq_send"); 
//              exit(EXIT_FAILURE);   
//             }   
//            printf("--Exit--\n");
//            mq_close(mqdes_send); // Close message queue
//            mq_unlink("/mq_tm");
//            pthread_exit(NULL); // Exit thread when the message is 0 0
//            break; // Exit the loop after pthread_exit
//        }  
        
        if (struct_to_send.type == 0){
          struct_to_send.type = 1;
        }     
        if (mq_send(mqdes_send, (char *)&struct_to_send, sizeof(struct_to_send), 1) == -1) {
              perror("mq_send");
              exit(EXIT_FAILURE);
        } 
        printf("--Respond Success!--\n");                         
        
     
    }
    
    mq_close(mqdes_receive);
    mq_unlink("/mq_tm");  
    return NULL;
} 


int main(int argc, char *argv[]) {
    setup_shared_memory();
    
    pthread_t tm_thread;

    pthread_create(&tm_thread, NULL, tm, NULL);    
    pthread_join(tm_thread, NULL);  // Wait for the thread to finish

    return 0;
}
