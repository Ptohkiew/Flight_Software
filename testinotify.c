#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <ctype.h>


#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

int ptime, type_id, module_id, ttc_id, parameter;
int prev_ptime = 0;  // Variable to hold the previous ptime

void* timesend(void* arg) {
    int target_time = *(int*)arg; // Retrieve the time sent in the argument
    
    uint32_t obc_time, cur_time, f_time;
    obc_time = (uint32_t)time(NULL);

    while (1) {
        //if (target_time > 1700000000) {
            obc_time = (uint32_t)time(NULL);
            f_time = target_time - obc_time;
            cur_time = target_time - f_time;
        //} 
//        else {
//            f_time = target_time - obc_time;
//            cur_time = target_time - f_time;
//            printf("Target Relative Time: %u\n", target_time);
//        }
        sleep(1);
        if (cur_time == target_time) {
            printf("Current time: %u\n", cur_time);
            break;
        }
    }
    printf("Finish time : %d\n", target_time);
    printf("-------------------------------------------\n");
    pthread_exit(NULL); // Exit thread when done
}

void read_file_content(const char *filename) {
    char buffer[1024];
    ssize_t bytes_read;
    int line_count = 0;  // Line counter

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return;
    }

    printf("Content of %s:\n", filename);
    uint32_t obc_time = (uint32_t)time(NULL);
    printf("OBC time : %u\n", obc_time);

    // Read file line by line
    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate the buffer

        // Pointer to tokenize the buffer
        char *token = strtok(buffer, "\n");

        // Process each line
        while (token != NULL) {
            line_count++;

            // Skip the first 4 lines
            if (line_count <= 4) {
                token = strtok(NULL, "\n");
                continue;
            }

            // Check if the line is not a comment
            if (token[0] != '#') {
                if (token[0] == '+' && isdigit(token[1])) {
                    // Use sscanf to parse data from token (each line)
                    sscanf(token, "%d\t%d\t%d\t%d\t%d", &ptime, &type_id, &module_id, &ttc_id, &parameter);
                    // Print parsed values
                    printf("Time: %d\n", ptime);
                    printf("Type ID: %d\n", type_id);
                    printf("Module ID: %d\n", module_id);
                    printf("TTC ID: %d\n", ttc_id);
                    printf("Parameter: %d\n", parameter);
                    printf("-------------------------------------------\n");
                    
                    if (prev_ptime == 0){
                        prev_ptime += obc_time;
                    }
                    ptime += prev_ptime; // Add previous time to relative time
                    pthread_t timesend_thread;
                    int *ptime_copy = malloc(sizeof(int));
                    *ptime_copy = ptime; // Copy ptime to avoid race conditions
                    pthread_create(&timesend_thread, NULL, timesend, ptime_copy);
    
                    prev_ptime = ptime; // Update prev_ptime after creating the thread
                }
                else if(token[0] != '+'){
                    // Use sscanf to parse data from token (each line)
                    sscanf(token, "%d\t%d\t%d\t%d\t%d", &ptime, &type_id, &module_id, &ttc_id, &parameter);
                    // Print parsed values
                    printf("Time: %d\n", ptime);
                    printf("Type ID: %d\n", type_id);
                    printf("Module ID: %d\n", module_id);
                    printf("TTC ID: %d\n", ttc_id);
                    printf("Parameter: %d\n", parameter);
                    printf("-------------------------------------------\n");
    
                    pthread_t timesend_thread;
                    int *ptime_copy = malloc(sizeof(int));                    
                    *ptime_copy = ptime; // Copy ptime to avoid race conditions
                    pthread_create(&timesend_thread, NULL, timesend, ptime_copy);
    
                    prev_ptime = ptime; // Update prev_ptime after creating the thread
                }
                else { 
                    printf("Reject Syntax at line %d: %s\n", line_count, token);
                    close(fd);
                    const char *newExtension = ".man";
                    char newFilename[256]; 
                    char *dot;
                
                    strcpy(newFilename, filename);
                
                    dot = strrchr(newFilename, '.');
                
                    if (dot != NULL) {
                        *dot = '\0';
                    }
                
                    // Rename
                    strcat(newFilename, newExtension);
                
                    if (rename(filename, newFilename) == 0) {
                        printf("New file name: %s\n", newFilename);
                    } else {
                        perror("Failed to rename");
                    }
                
                    printf("\n");
                    return;
                }
                
            }
            // Get the next line 
            token = strtok(NULL, "\n");
        }
    }

    if (bytes_read == -1) {
        perror("read");
    }
    close(fd);
    const char *newExtension = ".md";
    char newFilename[256];
    char *dot;

    strcpy(newFilename, filename);

    dot = strrchr(newFilename, '.');

    if (dot != NULL) {
        *dot = '\0';
    }

    // Rename
    strcat(newFilename, newExtension);

    if (rename(filename, newFilename) == 0) {
        printf("New file name: %s\n", newFilename);
    } else {
        perror("Failed to rename");
    }

    printf("\n");
}

char last_created_file[PATH_MAX] = "";
int last_plan_num = -1;
int last_plan_date = -1;

void handle_event(struct inotify_event *i, const char *watched_dir) {
    char full_path[PATH_MAX];
    snprintf(full_path, PATH_MAX, "%s/%s", watched_dir, i->name);

    if (i->mask & IN_CREATE) {
        // Check if the file path is the same as the last created file
//        if (strcmp(full_path, last_created_file) == 0) {
//            printf("File %s was created again. Skipping.\n", full_path);
//            return;
//        }

        printf("The file %s was created.\n", full_path);
        sleep(1); // Wait for a second to ensure the file is written
        FILE *file = fopen(full_path, "r");

        if (file == NULL) {
            perror("Failed to open file");
            return;
        }

        char line[256];
        int target_line = 1;
        int data_line = 0;
        int plan_num, plan_date;

        // Read the first non-comment line
        while (fgets(line, sizeof(line), file)) {
            // Remove newline character
            line[strcspn(line, "\n")] = '\0';

            // Skip comment lines
            if (line[0] == '#') {
                continue;
            }

            // Increment data line count
            data_line++;

            // Extract plan_num and plan_date from target line
            if (data_line == target_line) {
                // Use sscanf to parse data from line
                sscanf(line, "%d,%d", &plan_num, &plan_date);
                printf("Plan Number: %d\n", plan_num);
                printf("Plan Date: %d\n", plan_date);
                printf("Last Plan Number: %d\n", last_plan_num);
                printf("Last Plan Date: %d\n", last_plan_date);

                // Check if plan_num or plan_date has changed
                if (plan_date <= last_plan_date || plan_num <= last_plan_num) {
                    printf("Reject plan.\n");
                    //fclose(file);
                    const char *newExtension = ".man";
                    char newFilename[256]; 
                    char *dot;
                
                    strcpy(newFilename, full_path);
                
                    dot = strrchr(newFilename, '.');
                
                    if (dot != NULL) {
                        *dot = '\0';
                    }
                
                    // Rename
                    strcat(newFilename, newExtension);
                
                    if (rename(full_path, newFilename) == 0) {
                        printf("New file name: %s\n", newFilename);
                    } else {
                        perror("Failed to rename");
                    }
                
                    printf("\n");
                }
                else if (plan_num != last_plan_num || plan_date != last_plan_date) {
                    // Update last created file and its content
                    strcpy(last_created_file, full_path); 
                    last_plan_num = plan_num;
                    last_plan_date = plan_date;
                    read_file_content(full_path);
                }
                else {
                    printf("Reject plan.\n");
                } 

                break;
            }
        }
        fclose(file);
    }
    if (i->mask & IN_DELETE) {
        printf("The file %s was deleted.\n", full_path);
    }
    if (i->mask & IN_MODIFY) {
        printf("The file %s was modified.\n", full_path);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s PATH\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int inotify_fd = inotify_init();
    if (inotify_fd == -1) {
        perror("inotify_init");
        exit(EXIT_FAILURE);
    }

    int wd = inotify_add_watch(inotify_fd, argv[1], IN_CREATE | IN_DELETE | IN_MODIFY);
    if (wd == -1) {
        perror("inotify_add_watch");
        close(inotify_fd);
        exit(EXIT_FAILURE); 
    }

    char buf[BUF_LEN] __attribute__ ((aligned(8)));
    ssize_t num_read;
    struct inotify_event *event;

    printf("Starting to monitor changes in %s...\n", argv[1]);

    while (1) {
        num_read = read(inotify_fd, buf, BUF_LEN);
        if (num_read == 0) {
            fprintf(stderr, "read() from inotify fd returned 0!\n");
            exit(EXIT_FAILURE);
        }

        if (num_read == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        for (char *ptr = buf; ptr < buf + num_read;) {
            event = (struct inotify_event *)ptr;
            handle_event(event, argv[1]);
            ptr += sizeof(struct inotify_event) + event->len;
        }
    }

    close(inotify_fd);
    return 0;
}
