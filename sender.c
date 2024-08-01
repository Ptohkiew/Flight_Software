//SENDER
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
  
Message struct_type = {0};
Message struct_to_send = {0}; // Initialize to 0
Message struct_to_receive = {0}; // Initialize to 0

void* sender(void*arg)
{
    mqd_t mqdes_send = mq_open("/mq_dispatch", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes);
    if (mqdes_send == -1) {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    mqd_t mqdes_receive = mq_open("/mq_dispatch", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes);
    if (mqdes_receive == -1) {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }
    
    while(1)
    {
        int type, mdid, req_id;
        do {
            printf("Enter Type ID : "); 
            if (scanf("%d", &type) != 1) {
                printf("Invalid input.\n");
                while (getchar() != '\n'); // Clear the input buffer
                continue; 
            }
            
            printf("Enter MD ID : ");
            if (scanf("%d", &mdid) != 1) {
                printf("Invalid input.\n");
                while (getchar() != '\n'); 
                continue; 
            }
            
            printf("Enter TTC ID : ");
            if (scanf("%d", &req_id) != 1) {
                printf("Invalid input.\n");
                while (getchar() != '\n'); 
                continue; 
            }
            
            while (getchar() != '\n'); 
    
            if (type < 0 || mdid < 0 || req_id < 0) {
                printf("Invalid input.\n");
            }
              
        } while (type < 0 || mdid < 0 || req_id < 0);
        
        struct_to_send.type = (unsigned char)type;
        struct_to_send.mdid = (unsigned char)mdid;
        struct_to_send.req_id = (unsigned char)req_id;
        
        if(struct_to_send.type == TC_REQUEST && struct_to_send.mdid == 1 && struct_to_send.req_id == 1 || struct_to_send.type == TC_REQUEST && struct_to_send.mdid == 1 && struct_to_send.req_id == 2){
            int delay;
            do {
                printf("Enter delay in seconds : ");
                scanf("%d", &delay);
                while (getchar() != '\n');
                if (delay < 0) {
                    printf("Invalid input\n");
                }
            } while (delay < 0);
            struct_to_send.param = (unsigned char)delay;
        }

        if(struct_to_send.type == TC_REQUEST && struct_to_send.mdid == 1 && struct_to_send.req_id == 5){
            int num_file;
            do {
                printf("Enter number of file in folder : ");
                scanf("%d", &num_file);
                while (getchar() != '\n');
                if (num_file < 0) {
                    printf("Invalid input\n");
                }
            } while (num_file < 0);
            printf("%d\n", num_file);
            struct_to_send.param = (unsigned int)num_file;
            printf("%u\n", struct_to_send.param);
        }
        if(struct_to_send.type == TC_REQUEST && struct_to_send.mdid == 1 && struct_to_send.req_id == 6){
            int file_size;
            do {
                printf("Enter size of file : ");
                scanf("%d", &file_size);
                while (getchar() != '\n');
                if (file_size < 0) { 
                    printf("Invalid input\n");
                }
            } while (file_size < 0);
            struct_to_send.param = (unsigned int)file_size;
        }
        
        if(struct_to_send.type == TC_REQUEST && struct_to_send.mdid == 1 && struct_to_send.req_id == 7){
            int period;
            do {
                printf("Enter period : ");
                scanf("%d", &period);
                while (getchar() != '\n');
                if (period < 0) {
                    printf("Invalid input\n");
                }
            } while (period < 0);
            struct_to_send.param = (unsigned int)period;
        }
        
        if (mq_send(mqdes_send, (char *)&struct_to_send, sizeof(struct_to_send), 1) == -1) {
            perror("mq_send"); 
            exit(EXIT_FAILURE);
        }
        
        if(struct_to_send.type == TM_REQUEST   && struct_to_send.mdid != 0 || struct_to_send.type == TC_REQUEST && struct_to_send.mdid != 0) {
            printf("\n-- Wait for respond --\n");
          
        }
        if (mq_receive(mqdes_receive, (char *)&struct_to_receive, sizeof(struct_to_receive), NULL) == -1) {
              perror("mq_receive");
              mq_close(mqdes_receive); 
              pthread_exit(NULL);  
        }
///////////////////////////////////////////////////// TC //////////////////////////////////////////////
        /*if(struct_to_send.type == TC_REQUEST){
          if (mq_receive(mqdes_receive, (char *)&struct_to_receive, sizeof(struct_to_receive), NULL) == -1) {
              perror("mq_receive");
              mq_close(mqdes_receive); 
              pthread_exit(NULL);  
          }*/
          if (struct_to_receive.type == TC_RETURN ){
              if(struct_to_receive.mdid == 1 && struct_to_receive.req_id == 1)
              {
                printf("Type : %u\n", struct_to_receive.type);
                printf("Module ID : %u\n", struct_to_receive.mdid);
                printf("Telemetry ID : %u\n", struct_to_receive.req_id);
                printf("---- Reboot ----\n");                  
                printf("-------------------------------------------\n");
                continue;
              } 
              else if(struct_to_send.mdid == 1 && struct_to_send.req_id == 2)
              {
                printf("Type : %u\n", struct_to_receive.type); 
                printf("Module ID : %u\n", struct_to_receive.mdid);
                printf("Telemetry ID : %u\n", struct_to_receive.req_id);  
                printf("---- Shutdown ----\n");                  
                printf("-------------------------------------------\n");
                continue;
                
              }
              else if(struct_to_send.mdid == 1 && struct_to_send.req_id == 3)
              {
                printf("Type : %u\n", struct_to_receive.type); 
                printf("Module ID : %u\n", struct_to_receive.mdid);
                printf("Telemetry ID : %u\n", struct_to_receive.req_id);
                printf("---- Cancel ----\n");                  
                printf("-------------------------------------------\n");
                continue;
              }
              else if(struct_to_send.mdid == 1 && struct_to_send.req_id == 4)
              {
                printf("Type : %u\n", struct_to_receive.type); 
                printf("Module ID : %u\n", struct_to_receive.mdid);
                printf("Telemetry ID : %u\n", struct_to_receive.req_id);
                printf("---- New log ----\n");                  
                printf("-------------------------------------------\n");
                continue;
              }
              else if(struct_to_send.mdid == 1 && struct_to_send.req_id == 5)
              {
                printf("Type : %u\n", struct_to_receive.type); 
                printf("Module ID : %u\n", struct_to_receive.mdid);
                printf("Telemetry ID : %u\n", struct_to_receive.req_id);
                printf("---- Edit number of file ----\n");                  
                printf("-------------------------------------------\n");
                continue;
              }
              else if(struct_to_send.mdid == 1 && struct_to_send.req_id == 6)
              {
                printf("Type : %u\n", struct_to_receive.type); 
                printf("Module ID : %u\n", struct_to_receive.mdid);
                printf("Telemetry ID : %u\n", struct_to_receive.req_id);
                printf("---- Edit size of file ----\n");                  
                printf("-------------------------------------------\n");
                continue;
              }
              else if(struct_to_send.mdid == 1 && struct_to_send.req_id == 7)
              {
                printf("Type : %u\n", struct_to_receive.type); 
                printf("Module ID : %u\n", struct_to_receive.mdid);
                printf("Telemetry ID : %u\n", struct_to_receive.req_id);
                printf("---- Edit period ----\n");                  
                printf("-------------------------------------------\n");
                continue;
              }
              
    //          else if(struct_to_receive.type == TC_RETURN){
    //            if (mq_receive(mqdes_receive, (char *)&struct_to_receive, sizeof(struct_to_receive), NULL) == -1) {
    //                  perror("mq_receive");
    //                  mq_close(mqdes_receive); 
    //                  pthread_exit(NULL);  
    //            }
    //            if(struct_to_receive.mdid == 1 && struct_to_receive.req_id == 1)
    //            {
    //              printf("Type : %u\n", struct_to_receive.type);
    //              printf("Module ID : %u\n", struct_to_receive.mdid);
    //              printf("Telemetry ID : %u\n", struct_to_receive.req_id);
    //              printf("Telemetry Parameter : %u\n", struct_to_receive.param);  
    //              printf("Finished time : %d\n", struct_to_receive.val);                  
    //              printf("-------------------------------------------\n");
    //            }
    //          }
            }
           /* else
            {   
                printf("No Type3\n"); 
                printf("-------------------------------------------\n");
                continue;
            }
           
        //}
        
///////////////////////////////////////////////////// TM //////////////////////////////////////////////       
        
        /*if (struct_to_send.type == TM_REQUEST ){
          if (mq_receive(mqdes_receive, (char *)&struct_to_receive, sizeof(struct_to_receive), NULL) == -1) {
              perror("mq_receive");
              mq_close(mqdes_receive); 
              pthread_exit(NULL);    
          }*/ 
          if(struct_to_receive.type == TM_RETURN){
            if(struct_to_send.mdid == 1 && struct_to_send.req_id == 1)
            {
              printf("Type : %u\n", struct_to_receive.type);
              printf("Module ID : %u\n", struct_to_receive.mdid);
              printf("Telemetry ID : %u\n", struct_to_receive.req_id);
              printf("Telemetry Parameter : %u\n", struct_to_receive.param);
              printf("CPU Temperature: %d C\n", struct_to_receive.val);
              float cpu_temp  = struct_to_receive.val ;
              cpu_temp *= 0.001;             
              printf("CPU Temp: %.3f C\n", cpu_temp);                        
              printf("-------------------------------------------\n"); 
              struct_to_receive.type = TM_REQUEST ;
            }
  
            else if(struct_to_send.mdid == 1 && struct_to_send.req_id == 2)
            {
              printf("Type : %u\n", struct_to_receive.type);
              printf("Module ID : %u\n", struct_to_receive.mdid);
              printf("Telemetry ID : %u\n", struct_to_receive.req_id);
              printf("Telemetry Parameter : %u\n", struct_to_receive.param);
              printf("Total space: %u MB\n", struct_to_receive.val);
              printf("-------------------------------------------\n");
            }
            else if (struct_to_receive.mdid == 1 && struct_to_send.req_id == 3) 
            {
              printf("Type : %u\n", struct_to_receive.type);
              printf("Module ID : %u\n", struct_to_receive.mdid);
              printf("Telemetry ID : %u\n", struct_to_receive.req_id);
              printf("Telemetry Parameter : %u\n", struct_to_receive.param); 
              printf("Used space: %u MB\n", struct_to_receive.val);
              printf("-------------------------------------------\n");
            }
            else if (struct_to_receive.mdid == 1 && struct_to_send.req_id == 4) 
            {
              printf("Type : %u\n", struct_to_receive.type);
              printf("Module ID : %u\n", struct_to_receive.mdid);
              printf("Telemetry ID : %u\n", struct_to_receive.req_id);
              printf("Telemetry Parameter : %u\n", struct_to_receive.param);
              printf("Available space: %u MB\n", struct_to_receive.val);
              printf("-------------------------------------------\n");
            }
            else if (struct_to_receive.mdid == 1 && struct_to_send.req_id == 5) 
            { 
              uint8_t  ip_address[4];
              printf("Type : %u\n", struct_to_receive.type);
              printf("Module ID : %u\n", struct_to_receive.mdid);
              printf("Telemetry ID : %u\n", struct_to_receive.req_id);
              printf("Telemetry Parameter : %u\n", struct_to_receive.param);
              ip_address[0] = (struct_to_receive.val >> 24);
              ip_address[1] = (struct_to_receive.val >> 16);  
              ip_address[2] = (struct_to_receive.val >> 8);
              ip_address[3] = struct_to_receive.val;
              printf("IP Address : %hhu.%hhu.%hhu.%hhu\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
        
              printf("-------------------------------------------\n");
            }
            else if (struct_to_receive.mdid == 1 && struct_to_send.req_id == 6) 
            {
              printf("Type : %u\n", struct_to_receive.type);
              printf("Module ID : %u\n", struct_to_receive.mdid);
              printf("Telemetry ID : %u\n", struct_to_receive.req_id);
              printf("Telemetry Parameter : %u\n", struct_to_receive.param);
              float cpu_usage = struct_to_receive.val;
              cpu_usage *= 0.01;
              printf("Cpu usage : %.2f %\n", cpu_usage);
              printf("-------------------------------------------\n");
            }  
            else if (struct_to_receive.mdid == 1 && struct_to_send.req_id == 7) 
            {
              printf("Type : %u\n", struct_to_receive.type);
              printf("Module ID : %u\n", struct_to_receive.mdid);
              printf("Telemetry ID : %u\n", struct_to_receive.req_id);
              printf("Telemetry Parameter : %u\n", struct_to_receive.param);
              float cpu_peak = struct_to_receive.val;
              cpu_peak *= 0.01;
              printf("Cpu peak : %.2f %\n", cpu_peak);
              printf("-------------------------------------------\n");
            }
            else if (struct_to_receive.mdid == 1 && struct_to_send.req_id == 9) 
            {
              uint16_t  ram[2];
              printf("Type : %u\n", struct_to_receive.type);
              printf("Module ID : %u\n", struct_to_receive.mdid);
              printf("Telemetry ID : %u\n", struct_to_receive.req_id);
              printf("Telemetry Parameter : %u\n", struct_to_receive.param);
              ram[0] = (struct_to_receive.val >> 16);
              ram[1] = struct_to_receive.val;
              printf("Ram usage : %u/%u MB\n", ram[1], ram[0]);
              printf("-------------------------------------------\n");
            }
            else if (struct_to_receive.mdid == 1 && struct_to_send.req_id == 10) 
            {
              printf("Type : %u\n", struct_to_receive.type);
              printf("Module ID : %u\n", struct_to_receive.mdid);
              printf("Telemetry ID : %u\n", struct_to_receive.req_id);
              printf("Telemetry Parameter : %u\n", struct_to_receive.param); 
              printf("Ram peak : %u MB\n", struct_to_receive.val);
              printf("-------------------------------------------\n");
            } 
            else if (struct_to_receive.mdid == 1 && struct_to_send.req_id == 14) 
            {
              printf("Type : %u\n", struct_to_receive.type);
              printf("Module ID : %u\n", struct_to_receive.mdid);
              printf("Telemetry ID : %u\n", struct_to_receive.req_id);
              printf("Telemetry Parameter : %u\n", struct_to_receive.param);
              printf("Shutdown Time : %u\n", struct_to_receive.val);
              printf("-------------------------------------------\n");
            }
            else if (struct_to_receive.mdid == 1 && struct_to_send.req_id == 15) 
            {
              printf("Type : %u\n", struct_to_receive.type);
              printf("Module ID : %u\n", struct_to_receive.mdid); 
              printf("Telemetry ID : %u\n", struct_to_receive.req_id); 
              printf("Telemetry Parameter : %u\n", struct_to_receive.param); 
              printf("Remaining shutdown time : %u\n", struct_to_receive.val);
              printf("-------------------------------------------\n");
            }
            else if (struct_to_receive.mdid == 1 && struct_to_send.req_id == 16) 
            {
              printf("Type : %u\n", struct_to_receive.type);
              printf("Module ID : %u\n", struct_to_receive.mdid);
              printf("Telemetry ID : %u\n", struct_to_receive.req_id);
              printf("Telemetry Parameter : %u\n", struct_to_receive.param);
              printf("Shutdown type : %u\n", struct_to_receive.val);
              printf("-------------------------------------------\n");
            }
            else if (struct_to_receive.mdid == 1 && struct_to_send.req_id == 17) 
            {
              printf("Type : %u\n", struct_to_receive.type);
              printf("Module ID : %u\n", struct_to_receive.mdid);
              printf("Telemetry ID : %u\n", struct_to_receive.req_id);
              printf("Telemetry Parameter : %u\n", struct_to_receive.param);
              printf("OBC Time : %u\n", struct_to_receive.val);
              printf("-------------------------------------------\n");
            }
             
            //code aocs
            /*else if(struct_to_send.mdid == 2 && struct_to_send.req_id == 1)
            {
              printf("Type : %u\n", struct_to_receive.type);
              printf("Module ID : %u\n", struct_to_receive.mdid);
              printf("Telemetry ID : %u\n", struct_to_receive.req_id);
              printf("Telemetry Parameter : %u\n", struct_to_receive.param);
              printf("CPU Temperature: %d C\n", struct_to_receive.val);
              printf("-------------------------------------------\n");
            }
  
            else if(struct_to_send.mdid == 2 && struct_to_send.req_id == 2)
            {
              printf("Type : %u\n", struct_to_receive.type);
              printf("Module ID : %u\n", struct_to_receive.mdid);
              printf("Telemetry ID : %u\n", struct_to_receive.req_id);
              printf("Telemetry Parameter : %u\n", struct_to_receive.param);
              printf("Total space: %u MB\n", struct_to_receive.val); 
              printf("-------------------------------------------\n");
            }
            else if (struct_to_receive.mdid == 2 && struct_to_send.req_id == 3) 
            {
              printf("Type : %u\n", struct_to_receive.type);
              printf("Module ID : %u\n", struct_to_receive.mdid);
              printf("Telemetry ID : %u\n", struct_to_receive.req_id);
              printf("Telemetry Parameter : %u\n", struct_to_receive.param);
              printf("Used space: %u MB\n", struct_to_receive.val);
              printf("-------------------------------------------\n");
            }
             else if (struct_to_receive.mdid == 2 && struct_to_send.req_id == 4) 
            {
              printf("Type : %u\n", struct_to_receive.type);
              printf("Module ID : %u\n", struct_to_receive.mdid);
              printf("Telemetry ID : %u\n", struct_to_receive.req_id);
              printf("Telemetry Parameter : %u\n", struct_to_receive.param);
              printf("Available space: %u MB\n", struct_to_receive.val);
              printf("-------------------------------------------\n");
            }
            */
            
            
            } 
            else 
            { 
             if (mq_send(mqdes_send, (char *)&struct_to_send, sizeof(struct_to_send), 1) == -1) {
              perror("mq_send");
              exit(EXIT_FAILURE); 
              }
              
              if (mq_receive(mqdes_receive, (char *)&struct_to_receive, sizeof(struct_to_receive), NULL) == -1) {
                  perror("mq_receive");
                  mq_close(mqdes_receive);
                  pthread_exit(NULL);
              }
              printf("No Type1\n"); 
              printf("-------------------------------------------\n");  
          } 
        //}
          
//        else
//        {   
//            if (mq_receive(mqdes_receive, (char *)&struct_to_receive, sizeof(struct_to_receive), NULL) == -1) {
//                perror("mq_receive");
//                mq_close(mqdes_receive); 
//                pthread_exit(NULL);
//            }
//            printf("No Type\n"); 
//            printf("-------------------------------------------\n");
//        }
       
    }  
  
    mq_close(mqdes_send);
    
    mq_unlink("/mq_dispatch");
    pthread_exit(NULL); 
}

int main(int argc, char *argv[])
{
    pthread_t send;

    pthread_create(&send, NULL, &sender, NULL);
    pthread_join(send, NULL);

    return 0;
}