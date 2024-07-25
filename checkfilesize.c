#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define MAX_FILE_SIZE 204800  // ��Ҵ�٧�ش�ͧ��� (�亵�)

FILE *fp;
char path[1035];
uint32_t obc_time;
uint32_t period = 2;
char housekpfile[30];
uint32_t end_time;

void log_data(const char *message) {
    FILE *logfile2 = fopen(housekpfile, "a");
    if (logfile2 == NULL) {
        fprintf(stderr, "Error opening log file\n");
        return;
    }

    obc_time = (uint32_t)time(NULL);
    // �ѹ�֡���һѨ�غѹ��Т�ͤ��� log
    fprintf(logfile2, "%u, %s\n", obc_time, message);
    printf("Data written successfully.\n");
    fclose(logfile2);
}

// �ѧ��ѹ����Ѻ����红�������кѹ�֡
void collect_and_log_data() {
    int temp;
    uint32_t remain_mem;
    uint32_t ram_usage;
    uint32_t ram_peak;

    // �红����Ũҡ���
    FILE *fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
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
        perror("Failed to run command");
        exit(1);
    }
    char path[1035];
    if (fgets(path, sizeof(path) - 1, fp) != NULL) {
        sscanf(path, "%uM", &remain_mem);
    }
    pclose(fp);

    fp = popen("free -m | awk 'NR==2 {print $3}'", "r");
    if (fgets(path, sizeof(path) - 1, fp) != NULL) {
        sscanf(path, "%uM", &ram_usage);
    }
    pclose(fp);

    uint32_t ram_avail;
    uint32_t ram_total = 417;
    fp = popen("sar -r | awk '$3 ~ /^[0-9]+(\\.[0-9]+)?$/ {print $3}' | sort -k1n | head -n 1", "r");
    if (fp == NULL) {
        perror("Failed to run command");
        exit(EXIT_FAILURE);
    }
    while (fgets(path, sizeof(path) - 1, fp) != NULL) {
        sscanf(path, "%u", &ram_avail);
        ram_peak = ram_total - (ram_avail / 1024);
    }
    pclose(fp);

    // ���ҧ��ͤ��� log
    char log_message[100];
    snprintf(log_message, sizeof(log_message), "%d, %u, %u, %u",
             temp, remain_mem, ram_usage, ram_peak);

    // �ѹ�֡������ŧ����
    log_data(log_message);
}

int main() {
    while (1) {
        for (int i = 0; i < 5; i++) {
            sprintf(housekpfile, "hkp_log_%d.txt", i);

            // ���ҧ�����¹��Ǣ�ͧ͢��� log
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
            fprintf(logfile1, "1 ,0, 1, 4, \"Memory_Remain\", 0, 1, 0, MB\n");
            fprintf(logfile1, "1 ,0, 1, 9, \"RAM_Usage\", 0, 1, 0, MB\n");
            fprintf(logfile1, "1 ,0, 1, 10, \"RAM_Peak\", 0, 1, 0, MB\n");
            fprintf(logfile1, "\n#Data\n");

            fclose(logfile1);  // Close after writing the header

            while (1) {
                struct stat file_info;
                if (stat(housekpfile, &file_info) == -1) {
                    perror("stat");
                    exit(EXIT_FAILURE);
                }

                if (file_info.st_size > MAX_FILE_SIZE) {
                    printf("1 MB.\n");
                    end_time = (uint32_t)time(NULL);
                    FILE *logfile1 = fopen(housekpfile, "r+");
                    fseek(logfile1, 41, SEEK_SET); 
                    fprintf(logfile1, "End Time : %10u\n", end_time);
                    fclose(logfile1); 
                    break; // ��ش����红����Ŷ�Ң�Ҵ����Թ��Ҵ�٧�ش
                }
                collect_and_log_data();

                sleep(period);  
            } 
        }
    }
    return 0;
}
