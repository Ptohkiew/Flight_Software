// sender
#include <csp/csp_debug.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>
#include <csp/csp.h>
#include <csp/drivers/usart.h>
#include <csp/drivers/can_socketcan.h>
#include <csp/interfaces/csp_if_kiss.h>
#include <csp/interfaces/csp_if_zmqhub.h>
#include <stdio.h>
#include "/home/pi3/libcsp/test/message.h"
#include <termios.h>

/* This function must be provided in arch specific way */
int router_start(void);

/* Server port, the port the server listens on for incoming connections from the client. */
#define SERVER_PORT 15

/* Commandline options */
static uint8_t server_address = 1;
static uint8_t client_address = 1;

static unsigned int successful_ping = 0;
uint16_t addr = 10;  // Address สำหรับ interface นี้ (คุณสามารถเปลี่ยนตามที่ต้องการ)

Message send_msg = {0};
Message receive_msg = {0};

void server(void) {

	csp_print("\n-- Server task started --\n");

	/* Create socket with no specific socket options, e.g. accepts CRC32, HMAC, etc. if enabled during compilation */
	csp_socket_t sock = {0};

	/* Bind socket to all ports, e.g. all incoming connections will be handled here */
	csp_bind(&sock, CSP_ANY);

	/* Create a backlog of 10 connections, i.e. up to 10 new connections can be queued */
	csp_listen(&sock, 10);

	/* Wait for connections and then process packets on the connection */
	while (1) {
		csp_conn_t * conn2;

		/* Wait for a new connection, 10000 mS timeout */
		if ((conn2 = csp_accept(&sock, 10000)) == NULL) {
			csp_print("Connection timeout\n");
			continue;
		}
		csp_packet_t * packet2;
		while ((packet2 = csp_read(conn2, 50)) != NULL) {
			switch (csp_conn_dport(conn2)) {
				default:
					/* Process packet here */
					if (packet2->length > sizeof(receive_msg)) {
						csp_print("Packet size too large for receive_msg buffer\n");
						return;
					}
					memcpy(&receive_msg, packet2->data, packet2->length);  // if change to send_msg can use
																		   //          csp_print("Receive Type ID : %u\n", receive_msg.type);
																		   //          csp_print("Receive ModuleID : %u\n", receive_msg.mdid);
																		   //          csp_print("Receive TelemetryID : %u\n", receive_msg.req_id);
																		   //          csp_print("Receive Value : %u\n", receive_msg.val);
																		   //          csp_print("------------------------------\n\n");
					if (receive_msg.type == TC_RETURN) {
						if (receive_msg.mdid == 1 && receive_msg.req_id == 1) {
							printf("Type : %u\n", receive_msg.type);
							printf("Module ID : %u\n", receive_msg.mdid);
							printf("Telecommand ID : %u\n", receive_msg.req_id);
							printf("---- Reboot ----\n");
							printf("-------------------------------------------\n");
							continue;
						} else if (send_msg.mdid == 1 && send_msg.req_id == 2) {
							printf("Type : %u\n", receive_msg.type);
							printf("Module ID : %u\n", receive_msg.mdid);
							printf("Telecommand ID : %u\n", receive_msg.req_id);
							printf("---- Shutdown ----\n");
							printf("-------------------------------------------\n");
							continue;

						} else if (send_msg.mdid == 1 && send_msg.req_id == 3) {
							printf("Type : %u\n", receive_msg.type);
							printf("Module ID : %u\n", receive_msg.mdid);
							printf("Telecommand ID : %u\n", receive_msg.req_id);
							printf("---- Cancel ----\n");
							printf("-------------------------------------------\n");
							continue;
						} else if (send_msg.mdid == 1 && send_msg.req_id == 4) {
							printf("Type : %u\n", receive_msg.type);
							printf("Module ID : %u\n", receive_msg.mdid);
							printf("Telecommand ID : %u\n", receive_msg.req_id);
							printf("---- New log ----\n");
							printf("-------------------------------------------\n");
							continue;
						} else if (send_msg.mdid == 1 && send_msg.req_id == 5) {
							printf("Type : %u\n", receive_msg.type);
							printf("Module ID : %u\n", receive_msg.mdid);
							printf("Telecommand ID : %u\n", receive_msg.req_id);
							printf("---- Edit number of file ----\n");
							printf("-------------------------------------------\n");
							continue;
						} else if (send_msg.mdid == 1 && send_msg.req_id == 6) {
							printf("Type : %u\n", receive_msg.type);
							printf("Module ID : %u\n", receive_msg.mdid);
							printf("Telecommand ID : %u\n", receive_msg.req_id);
							printf("---- Edit size of file ----\n");
							printf("-------------------------------------------\n");
							continue;
						} else if (send_msg.mdid == 1 && send_msg.req_id == 7) {
							printf("Type : %u\n", receive_msg.type);
							printf("Module ID : %u\n", receive_msg.mdid);
							printf("Telecommand ID : %u\n", receive_msg.req_id);
							printf("---- Edit period ----\n");
							printf("-------------------------------------------\n");
							continue;
						}
					}
					if (receive_msg.type == TM_RETURN) {
						if (send_msg.mdid == 1 && send_msg.req_id == 1) {
							printf("Type : %u\n", receive_msg.type);
							printf("Module ID : %u\n", receive_msg.mdid);
							printf("Telemetry ID : %u\n", receive_msg.req_id);
							printf("Telemetry Parameter : %u\n", receive_msg.param);
							printf("CPU Temperature: %d C\n", receive_msg.val);
							float cpu_temp = receive_msg.val;
							cpu_temp *= 0.001;
							printf("CPU Temp: %.3f C\n", cpu_temp);
							printf("-------------------------------------------\n");
							receive_msg.type = TM_REQUEST;
						}

						else if (send_msg.mdid == 1 && send_msg.req_id == 2) {
							printf("Type : %u\n", receive_msg.type);
							printf("Module ID : %u\n", receive_msg.mdid);
							printf("Telemetry ID : %u\n", receive_msg.req_id);
							printf("Telemetry Parameter : %u\n", receive_msg.param);
							printf("Total space: %u MB\n", receive_msg.val);
							printf("-------------------------------------------\n");
						} else if (receive_msg.mdid == 1 && send_msg.req_id == 3) {
							printf("Type : %u\n", receive_msg.type);
							printf("Module ID : %u\n", receive_msg.mdid);
							printf("Telemetry ID : %u\n", receive_msg.req_id);
							printf("Telemetry Parameter : %u\n", receive_msg.param);
							printf("Used space: %u MB\n", receive_msg.val);
							printf("-------------------------------------------\n");
						} else if (receive_msg.mdid == 1 && send_msg.req_id == 4) {
							printf("Type : %u\n", receive_msg.type);
							printf("Module ID : %u\n", receive_msg.mdid);
							printf("Telemetry ID : %u\n", receive_msg.req_id);
							printf("Telemetry Parameter : %u\n", receive_msg.param);
							printf("Available space: %u MB\n", receive_msg.val);
							printf("-------------------------------------------\n");
						} else if (receive_msg.mdid == 1 && send_msg.req_id == 5) {
							uint8_t ip_address[4];
							printf("Type : %u\n", receive_msg.type);
							printf("Module ID : %u\n", receive_msg.mdid);
							printf("Telemetry ID : %u\n", receive_msg.req_id);
							printf("Telemetry Parameter : %u\n", receive_msg.param);
							ip_address[0] = (receive_msg.val >> 24);
							ip_address[1] = (receive_msg.val >> 16);
							ip_address[2] = (receive_msg.val >> 8);
							ip_address[3] = receive_msg.val;
							printf("IP Address : %hhu.%hhu.%hhu.%hhu\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);

							printf("-------------------------------------------\n");
						} else if (receive_msg.mdid == 1 && send_msg.req_id == 6) {
							printf("Type : %u\n", receive_msg.type);
							printf("Module ID : %u\n", receive_msg.mdid);
							printf("Telemetry ID : %u\n", receive_msg.req_id);
							printf("Telemetry Parameter : %u\n", receive_msg.param);
							float cpu_usage = receive_msg.val;
							cpu_usage *= 0.01;
							printf("Cpu usage : %.2f\n", cpu_usage);
							printf("-------------------------------------------\n");
						} else if (receive_msg.mdid == 1 && send_msg.req_id == 7) {
							printf("Type : %u\n", receive_msg.type);
							printf("Module ID : %u\n", receive_msg.mdid);
							printf("Telemetry ID : %u\n", receive_msg.req_id);
							printf("Telemetry Parameter : %u\n", receive_msg.param);
							float cpu_peak = receive_msg.val;
							cpu_peak *= 0.01;
							printf("Cpu peak : %.2f\n", cpu_peak);
							printf("-------------------------------------------\n");
						} else if (receive_msg.mdid == 1 && send_msg.req_id == 9) {
							uint16_t ram[2];
							printf("Type : %u\n", receive_msg.type);
							printf("Module ID : %u\n", receive_msg.mdid);
							printf("Telemetry ID : %u\n", receive_msg.req_id);
							printf("Telemetry Parameter : %u\n", receive_msg.param);
							ram[0] = (receive_msg.val >> 16);
							ram[1] = receive_msg.val;
							printf("Ram usage : %u/%u MB\n", ram[1], ram[0]);
							printf("-------------------------------------------\n");
						} else if (receive_msg.mdid == 1 && send_msg.req_id == 10) {
							printf("Type : %u\n", receive_msg.type);
							printf("Module ID : %u\n", receive_msg.mdid);
							printf("Telemetry ID : %u\n", receive_msg.req_id);
							printf("Telemetry Parameter : %u\n", receive_msg.param);
							printf("Ram peak : %u MB\n", receive_msg.val);
							printf("-------------------------------------------\n");
						} else if (receive_msg.mdid == 1 && send_msg.req_id == 14) {
							printf("Type : %u\n", receive_msg.type);
							printf("Module ID : %u\n", receive_msg.mdid);
							printf("Telemetry ID : %u\n", receive_msg.req_id);
							printf("Telemetry Parameter : %u\n", receive_msg.param);
							printf("Shutdown Time : %u\n", receive_msg.val);
							printf("-------------------------------------------\n");
						} else if (receive_msg.mdid == 1 && send_msg.req_id == 15) {
							printf("Type : %u\n", receive_msg.type);
							printf("Module ID : %u\n", receive_msg.mdid);
							printf("Telemetry ID : %u\n", receive_msg.req_id);
							printf("Telemetry Parameter : %u\n", receive_msg.param);
							printf("Remaining shutdown time : %u\n", receive_msg.val);
							printf("-------------------------------------------\n");
						} else if (receive_msg.mdid == 1 && send_msg.req_id == 16) {
							printf("Type : %u\n", receive_msg.type);
							printf("Module ID : %u\n", receive_msg.mdid);
							printf("Telemetry ID : %u\n", receive_msg.req_id);
							printf("Telemetry Parameter : %u\n", receive_msg.param);
							printf("Shutdown type : %u\n", receive_msg.val);
							printf("-------------------------------------------\n");
						} else if (receive_msg.mdid == 1 && send_msg.req_id == 17) {
							printf("Type : %u\n", receive_msg.type);
							printf("Module ID : %u\n", receive_msg.mdid);
							printf("Telemetry ID : %u\n", receive_msg.req_id);
							printf("Telemetry Parameter : %u\n", receive_msg.param);
							printf("OBC Time : %u\n", receive_msg.val);
							printf("-------------------------------------------\n");
						}
					} else {
						printf("No Type\n");
						printf("-------------------------------------------\n");
					}
					csp_buffer_free(packet2);
					break;
			}
		}

		/* Close current connection */
		csp_close(conn2);
		break;
	}

	return;
}

static int csp_pthread_create(void * (*routine)(void *)) {
	pthread_attr_t attributes;
	pthread_t handle;
	int ret;

	if (pthread_attr_init(&attributes) != 0) {
		return CSP_ERR_NOMEM;
	}
	/* no need to join with thread to free its resources */
	pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);

	ret = pthread_create(&handle, &attributes, routine, NULL);
	pthread_attr_destroy(&attributes);

	if (ret != 0) {
		return ret;
	}

	return CSP_ERR_NONE;
}

static void * task_router(void * param) {

	/* Here there be routing */
	while (1) {
		csp_route_work();
	}

	return NULL;
}

int router_start(void) {
	return csp_pthread_create(task_router);
}

void packet_to_hex(const unsigned char * packet, size_t packet_size, char * hex_string) {
	packet_size = packet_size + sizeof(csp_id_t);

	for (size_t i = 0; i < packet_size; i++) {
		sprintf(hex_string + (i * 2), "%02X", packet[i]);
	}
}

// void send_csp_packet_via_lora(const unsigned char *packet, size_t packet_size) {
//     // Calculate packet size (header + data)
//     packet_size = packet_size + sizeof(csp_id_t);

//     // Show the serialized packet before sending (Header + Data)
//     printf("\nSerialized CSP Packet (Hex): ");
//     for (size_t i = 0; i < packet_size; i++) {
//         printf("%02X", ((uint8_t *)packet)[i]);
//     }
//     printf("\n");

//     // Send the serialized data via LoRa (assuming LoRa is already initialized)
//     // LoRa.beginPacket();
//     // LoRa.write((uint8_t *)packet, packet_size);  // Send the full packet
//     // LoRa.endPacket();
// }

void convert_hex_to_packet(const char * hex_string, unsigned char * packet, size_t * packet_size) {
	size_t len = strlen(hex_string);
	size_t packet_idx = 0;

	// Iterate over the hex string in 8-character (32-bit) chunks
	for (size_t i = 0; i < len; i += 8) {
		char temp[5] = {0};  // Store 4 hex characters (16 bits) + null terminator

		// Copy only the first 4 hex characters (16 bits) and skip the next 4 (padding)
		strncpy(temp, &hex_string[i], 4);

		// Convert the hex string to an integer value and store it in the packet array
		packet[packet_idx++] = (unsigned char)strtol(temp, NULL, 16);
	}

	// Set the packet size based on the actual number of bytes filled
	*packet_size = packet_idx;
}

void reverse_byte_order(char * hex_chunk) {
	for (int i = 0; i < 4; i++) {
		// Swap the pairs of characters (e.g. "D2" <-> "04")
		char temp1 = hex_chunk[i * 2];
		char temp2 = hex_chunk[i * 2 + 1];
		hex_chunk[i * 2] = hex_chunk[(3 - i) * 2];
		hex_chunk[i * 2 + 1] = hex_chunk[(3 - i) * 2 + 1];
		hex_chunk[(3 - i) * 2] = temp1;
		hex_chunk[(3 - i) * 2 + 1] = temp2;
	}
}

unsigned int convert_hex_to_decimal_little_endian(const char * hex_chunk) {
	// Ensure that the input is 8 characters (32 bits)
	if (strlen(hex_chunk) != 8) {
		fprintf(stderr, "Invalid hex chunk length. Expected 8 characters (32 bits).\n");
		return 0;
	}

	// Create an array to hold the bytes in little-endian order
	char little_endian[9] = {0};  // 8 characters + null terminator

	// Rearrange the hex string from little-endian order
	little_endian[0] = hex_chunk[6];
	little_endian[1] = hex_chunk[7];
	little_endian[2] = hex_chunk[4];
	little_endian[3] = hex_chunk[5];
	little_endian[4] = hex_chunk[2];
	little_endian[5] = hex_chunk[3];
	little_endian[6] = hex_chunk[0];
	little_endian[7] = hex_chunk[1];

	// Convert the rearranged hex string to an unsigned integer
	return (unsigned int)strtoul(little_endian, NULL, 16);
}

void convert_hex_string_to_decimal(const char * hex_string, csp_packet_t * packet) {
	size_t len = strlen(hex_string);
	size_t chunk_count = 0;  // Counter for the number of 32-bit chunks processed
	size_t packet_idx = 0;

	// Iterate over the hex string in 8-character (32-bit) chunks
	for (size_t i = 0; i < len && chunk_count < 3; i += 8) {
		char temp[9] = {0};  // Buffer for 8 hex characters + null terminator

		// Copy 8 hex characters (32 bits) from the hex string
		strncpy(temp, &hex_string[i], 8);

		// Convert the hex chunk to decimal considering little-endian order
		unsigned int decimal_value = convert_hex_to_decimal_little_endian(temp);

		// Print the decimal value
		printf("32-bit chunk: %s -> Decimal: %u\n", temp, decimal_value);

		memcpy(&packet->data[packet_idx], &decimal_value, sizeof(unsigned int));
		packet_idx += sizeof(unsigned int);  // ขยับไปยังตำแหน่งถัดไปใน packet

		chunk_count++;  // Increment the chunk counter
	}
	printf("packet : %u\n", packet->data);
}

// void convert_hex_to_decimal_little_endian(const char *hex_string) {
//     size_t len = strlen(hex_string);

//     // Iterate over the hex string in 8-character (32-bit) chunks
//     for (size_t i = 0; i < len; i += 8) {
//         char temp[9] = {0};  // Store 8 hex characters (32 bits) + null terminator

//         // Copy 8 hex characters (32 bits) from the hex string
//         strncpy(temp, &hex_string[i], 8);

//         // Reverse the byte order for little-endian interpretation
//         reverse_byte_order(temp);

//         // Convert the 32-bit hex chunk to an unsigned integer
//         unsigned int decimal_value = (unsigned int)strtoul(temp, NULL, 16);

//         // Print the decimal value
//         printf("32-bit chunk (little-endian): %s -> Decimal: %u\n", temp, decimal_value);
//     }
// }

// void convert_hex_to_decimal(const char *hex_string) {
//     size_t len = strlen(hex_string);

//     // Iterate over the hex string in 8-character (32-bit) chunks
//     for (size_t i = 0; i < len; i += 8) {
//         char temp[9] = {0};  // Store 8 hex characters (32 bits) + null terminator

//         // Copy 8 hex characters (32 bits) from the hex string
//         strncpy(temp, &hex_string[i], 8);

//         // Convert the 32-bit hex chunk to an unsigned integer
//         unsigned int decimal_value = (unsigned int)strtoul(temp, NULL, 16);

//         // Print the decimal value
//         printf("32-bit chunk: %s -> Decimal: %u\n", temp, decimal_value);
//     }
// }

int main(int argc, char * argv[]) {
	int ret = EXIT_SUCCESS;
	int type, mdid, req_id;

	csp_print("Initialising CSP\n");

	/* Init CSP */
	csp_init();

	/* Start router */
	router_start();

	// csp_iface_t * kiss_iface = NULL;
	// csp_usart_conf_t uart_conf = {
	// 	.device = "/dev/serial0",
	// 	.baudrate = 115200, /* supported on all platforms */
	// 	.databits = 8,
	// 	.stopbits = 1,
	// 	.paritysetting = 0,
	// };
	// // open UART and ADD Interface
	// int result = csp_usart_open_and_add_kiss_interface(&uart_conf, CSP_IF_KISS_DEFAULT_NAME, addr, &kiss_iface);  // เพิ่ม addr
	// if (result != CSP_ERR_NONE) {
	// 	printf("Error adding KISS interface: %d\n", result);
	// 	return result;
	// } else {
	// 	printf("KISS interface added successfully\n");
	// }

	// kiss_iface->addr = client_address;
	// kiss_iface->is_default = 1;

	csp_print("Client started\n");

	csp_print("Connection table\r\n");
	csp_conn_print_table();

	csp_print("Interfaces\r\n");
	csp_iflist_print();

	while (1) {
		// int result = csp_ping(server_address, 1000, 100, CSP_O_NONE);
		// if (result < 0) {
		// 	csp_print("Ping failed for address %u: %d\n", server_address, result);
		// } else {
		// 	csp_print("Ping succeeded for address %u: %d [mS]\n", server_address, result);
		// }

		//        csp_reboot(server_address);
		//		    csp_print("reboot system request sent to address: %u\n", server_address);
		/* 1. Connect to host on 'server_address', port SERVER_PORT with regular UDP-like protocol and 1000 ms timeout */
		// csp_conn_t * conn = csp_connect(CSP_PRIO_NORM, server_address, SERVER_PORT, 1000, CSP_O_NONE);
		// if (conn == NULL) {
		// 	/* Connect failed */
		// 	csp_print("Connection failed\n");
		// 	ret = EXIT_FAILURE;
		// 	break;
		// }

		do {
			printf("Enter Type ID : ");
			if (scanf("%d", &type) != 1) {
				printf("Invalid input.\n");
				while (getchar() != '\n');  // Clear the input buffer
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

		send_msg.type = (unsigned int)type;
		send_msg.mdid = (unsigned int)mdid;
		send_msg.req_id = (unsigned int)req_id;

		if (send_msg.type == TC_REQUEST && send_msg.mdid == 1 && send_msg.req_id == 1 || send_msg.type == TC_REQUEST && send_msg.mdid == 1 && send_msg.req_id == 2) {
			int delay;
			do {
				printf("Enter delay in seconds : ");
				scanf("%d", &delay);
				while (getchar() != '\n');
				if (delay < 0) {
					printf("Invalid input\n");
				}
			} while (delay < 0);
			send_msg.param = (unsigned char)delay;
		}

		if (send_msg.type == TC_REQUEST && send_msg.mdid == 1 && send_msg.req_id == 5) {
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
			send_msg.param = (unsigned int)num_file;
			printf("%u\n", send_msg.param);
		}
		if (send_msg.type == TC_REQUEST && send_msg.mdid == 1 && send_msg.req_id == 6) {
			int file_size;
			do {
				printf("Enter size of file : ");
				scanf("%d", &file_size);
				while (getchar() != '\n');
				if (file_size < 0) {
					printf("Invalid input\n");
				}
			} while (file_size < 0);
			send_msg.param = (unsigned int)file_size;
		}

		if (send_msg.type == TC_REQUEST && send_msg.mdid == 1 && send_msg.req_id == 7) {
			int period;
			do {
				printf("Enter period : ");
				scanf("%d", &period);
				while (getchar() != '\n');
				if (period < 0) {
					printf("Invalid input\n");
				}
			} while (period < 0);
			send_msg.param = (unsigned int)period;
		}

		/* 2. Get packet buffer for message/data */
		csp_packet_t * packet = csp_buffer_get(0);
		if (packet == NULL) {
			/* Could not get buffer element */
			csp_print("Failed to get CSP buffer\n");
			ret = EXIT_FAILURE;
			break;
		}

		
		packet->id.pri = CSP_PRIO_HIGH;  // Priority
		packet->id.dst = 2;              // Destination address
		packet->id.src = 1;              // Source address
		packet->id.dport = 5;            // Destination port
		packet->id.sport = 10;           // Source port
										 /* 3. Copy data to packet */
		memcpy(packet->data, &send_msg, sizeof(send_msg));
		//memcmp(packet->id, &packet->id,sizeof(csp_id_t));
		printf("packet : %u\n", packet->data);
		printf("pri: %u\n", packet->id.pri);
		printf("flags: %u\n", packet->id.flags);
		printf("src: %u\n", packet->id.src);
		printf("dst: %u\n", packet->id.dst);
		printf("dport: %u\n", packet->id.dport);
		printf("sport: %u\n", packet->id.sport);

		// Print the size of each field
		// printf("Size of pri: %lu\n", sizeof(id.pri));
		// printf("Size of flags: %lu\n", sizeof(id.flags));
		// printf("Size of src: %lu\n", sizeof(id.src));
		// printf("Size of dst: %lu\n", sizeof(id.dst));
		// printf("Size of dport: %lu\n", sizeof(id.dport));
		// printf("Size of sport: %lu\n", sizeof(id.sport));

		// Print the total size of the struct
		printf("Size of csp_id_t: %lu\n", sizeof(csp_id_t));

		/* 4. Set packet length */
		packet->length = sizeof(send_msg) + sizeof(csp_id_t);

		//  printf(" %06x\n", packet->data[5]);
		//  printf(" %d %d\n", packet->data[4], packet->data[5]);

		// for(int i=0 ;i<sizeof(packet->data);i++){
		//   printf("%03x", packet->data[i]);
		// }

		// Send and display the serialized packet (including header)
		// send_csp_packet_via_lora(packet->data, packet->length);
		// printf("packet : %u\n", packet->data[0]);
		// printf("packet : %u\n", packet->data[1]);
		// printf("packet : %u\n", packet->data[2]);
		char hex_string[packet->length * 2 + 1];  // ต้องรองรับ 2 เท่าของขนาด packet + null terminator
		packet_to_hex(packet->data, packet->length, hex_string);

		printf("Hex String: %s\n", hex_string);
		// convert_hex_string_to_decimal(hex_string, packet);
		// printf("packet : %u\n", packet->data);
		// memcpy(&receive_msg, packet->data, packet->length);
		// csp_print("Type : %u\n", receive_msg.type);
		// csp_print("Receive ModuleID : %u\n", receive_msg.mdid);
		// csp_print("Receive TelemetryID : %u\n", receive_msg.req_id);

		int uart0_filestream = open("/dev/serial0", O_WRONLY | O_NOCTTY);
    
		if (uart0_filestream == -1) {
			printf("Error - Unable to open UART.\n");
			return -1;
		}
		
		struct termios options;
		tcgetattr(uart0_filestream, &options);
		options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
		options.c_iflag = IGNPAR;
		options.c_oflag = 0;
		options.c_lflag = 0;
		tcflush(uart0_filestream, TCIFLUSH);
		tcsetattr(uart0_filestream, TCSANOW, &options);
		size_t hex_length = packet->length * 2+1; // ขนาดของ hex string
		int count = write(uart0_filestream, hex_string, hex_length);
		
		if (count < 0) {
			printf("UART TX error.\n");
		}
		
		if (tcdrain(uart0_filestream) != 0) {
			printf("Error draining UART.\n");
		}

		close(uart0_filestream);
		/* 5. Send packet */
		//csp_send(conn, packet);
		//csp_close(conn);
		server();
	}
	/* Wait for execution to end (ctrl+c) */
	return ret;
}
