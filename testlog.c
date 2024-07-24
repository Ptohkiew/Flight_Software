#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

FILE *fp;
char path[1035];
uint32_t obc_time;
int i;
uint32_t period = 10;
char housekpfile[30];
// ฟังก์ชันสำหรับการบันทึก log
void log_data(const char *message) {
    FILE *logfile = fopen(housekpfile, "a");
    if (logfile == NULL) {
        fprintf(stderr, "Error opening log file\n");
        return;
    }

    obc_time = (uint32_t)time(NULL);
    // บันทึกเวลาปัจจุบันและข้อความ log
    fprintf(logfile, "[%u] %s\n", obc_time,message); 
    fclose(logfile);
}


void collect_and_log_data() {
    int temp; 
    uint32_t remain_mem;
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

    fp = popen("df -BM /dev/mmcblk0p2 | awk 'NR==2 {print $4}'", "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        exit(1);
    }
    if (fgets(path, sizeof(path)-1, fp) != NULL) {
        sscanf(path, "%uM", &remain_mem);     
    } 
    pclose(fp);

//    fp = popen("sar -r 1 1 | awk '$3 ~ /^[0-9]+(\\.[0-9]+)?$/ {print $3}' | sort -k1n | head -n 1", "r");
//    if (fp == NULL) {
//        printf("Failed to run command\n");
//        exit(EXIT_FAILURE);
//    }
//    uint32_t ram_avail = 1;
//    uint32_t ram_total = 427072;
//    while (fgets(path, sizeof(path)-1, fp) != NULL) {
//        sscanf(path, "%u", &ram_avail);
//        //ram_usage = (ram_total - ram_avail)/1024;     
//    } 
//    pclose(fp); 
    uint32_t ram_usage;
    fp = popen("free -m | awk 'NR==2 {print $3}' | sort -k1n | head -n 1", "r");
    if (fgets(path, sizeof(path)-1, fp) != NULL) {
        sscanf(path, "%uM", &ram_usage);     
    } 
    pclose(fp);
    
    uint32_t ram_avail;
    uint32_t ram_total = 417;
    uint32_t ram_peak;
    fp = popen("sar -r | awk '$3 ~ /^[0-9]+(\\.[0-9]+)?$/ {print $3}' | sort -k1n | head -n 1", "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        exit(EXIT_FAILURE);
    } 

    while (fgets(path, sizeof(path)-1, fp) != NULL) {
        sscanf(path, "%u", &ram_avail);
        ram_peak = ram_total - (ram_avail/1024);   
    } 
    pclose(fp);

    char log_message[100];
    snprintf(log_message, sizeof(log_message), "Temperature: %d, Remain memory: %u, Ram usage: %u, Ram peak: %u", temp, remain_mem, ram_usage, ram_peak);

    // บันทึกข้อมูลลงใน log
    log_data(log_message);
}


int main() {  
    while(1){
      for(i = 0;i<5;i++){     
        sprintf(housekpfile, "Housekeeping_log_%d.txt", i); 
        FILE *logfile = fopen(housekpfile, "w");
        fprintf(logfile, "Housekeeping Log\n");
        obc_time = (uint32_t)time(NULL);
        fprintf(logfile, "Start Time : %u\n", obc_time);
        uint32_t end_time = obc_time + 30;  
        fprintf(logfile, "End Time : %u\n", end_time);
        fprintf(logfile, "Period : %u\n", period);
        fprintf(logfile, "\n#Channel Information\n");
        fprintf(logfile, "1 ,0, 1, 1, \"CPU_Temperature\", 0, 0.001, 0, ?C\n");
        fprintf(logfile, "1 ,0, 1, 4, \"Memory_Remain\", 0, 1, 0, MB\n");
        fprintf(logfile, "1 ,0, 1, 9, \"RAM_Usage\", 0, 1, 0, MB\n");
        fprintf(logfile, "1 ,0, 1, 10, \"RAM_Peak\", 0, 1, 0, MB\n");
        fprintf(logfile, "\n#Data\n");
        fclose(logfile); 
    
        while (1) {
            collect_and_log_data();
            obc_time = (uint32_t)time(NULL); 
            if(end_time <= obc_time){ 
                sleep(10); 
                break;
            }
            sleep(10); 
        }
      }
    }
    return 0;
}
