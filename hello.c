/*
 * Button-triggered Hello World with UDP for RTEMS
 */
#include <rtems.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <rtems/version.h>

/* Network includes */
#include <rtems/bsd/bsd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* STM32H743 RCC */
#define RCC_BASE        0x58024400
#define RCC_AHB4ENR     (*(volatile uint32_t *)(RCC_BASE + 0xE0))
#define RCC_AHB4ENR_GPIOCEN  (1 << 2)

/* STM32H743 GPIO Port C */
#define GPIOC_BASE   0x58020800
#define GPIOC_MODER  (*(volatile uint32_t *)(GPIOC_BASE + 0x00))
#define GPIOC_PUPDR  (*(volatile uint32_t *)(GPIOC_BASE + 0x0C))
#define GPIOC_IDR    (*(volatile uint32_t *)(GPIOC_BASE + 0x10))

#define BUTTON_PIN   13

/* UDP Configuration */
#define UDP_SERVER_IP   "192.168.1.100"
#define UDP_SERVER_PORT 5000

static int udp_socket = -1;
static struct sockaddr_in server_addr;

static void send_udp_message(const char *message)
{
  if (udp_socket < 0) return;
  sendto(udp_socket, message, strlen(message), 0,
         (struct sockaddr *)&server_addr, sizeof(server_addr));
}

static void print_and_send(const char *message)
{
  printf("%s", message);
  send_udp_message(message);
}

static int init_udp(void)
{
  udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (udp_socket < 0) {
    printf("ERROR: Failed to create UDP socket (errno=%d)\n", errno);
    return -1;
  }
  
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(UDP_SERVER_PORT);
  
  if (inet_pton(AF_INET, UDP_SERVER_IP, &server_addr.sin_addr) <= 0) {
    printf("ERROR: Invalid server IP address\n");
    close(udp_socket);
    udp_socket = -1;
    return -1;
  }
  
  printf("UDP socket initialized successfully\n");
  return 0;
}

static void Init(rtems_task_argument arg)
{
  int counter = 0;
  bool last_button_state = false;
  bool current_button_state;
  char msg_buffer[128];
  rtems_status_code sc;
  
  printf("\n\n*** BUTTON-TRIGGERED HELLO WORLD TEST ***\n");
  printf("Board: STM32H743ZI Nucleo\n");
  printf("RTEMS Version: %s\n\n", rtems_version());
  
  printf("Initializing network stack...\n");
  sc = rtems_bsd_initialize();
  if (sc != RTEMS_SUCCESSFUL) {
    printf("ERROR: Failed to initialize libbsd: %d\n", sc);
    printf("Continuing without network...\n\n");
  } else {
    printf("Network stack initialized\n");
    rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(500));
    
    if (init_udp() == 0) {
      printf("UDP ready to %s:%d\n\n", UDP_SERVER_IP, UDP_SERVER_PORT);
    }
  }
  
  RCC_AHB4ENR |= RCC_AHB4ENR_GPIOCEN;
  GPIOC_MODER &= ~(0x3 << (BUTTON_PIN * 2));
  GPIOC_PUPDR &= ~(0x3 << (BUTTON_PIN * 2));
  
  printf("GPIO configured, waiting for button press...\n");
  
  while (1) {
    current_button_state = !(GPIOC_IDR & (1 << BUTTON_PIN));
    
    if (current_button_state && !last_button_state) {
      snprintf(msg_buffer, sizeof(msg_buffer), 
               "Button pressed! Hello World #%d\n", counter);
      print_and_send(msg_buffer);
      counter++;
    }
    
    last_button_state = current_button_state;
    rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(50));
  }
  
  if (udp_socket >= 0) close(udp_socket);
  exit(0);
}

/* ============ Configuration Section ============ */

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_LIBBLOCK

#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS 64
#define CONFIGURE_MAXIMUM_TASKS 32
#define CONFIGURE_MAXIMUM_SEMAPHORES 64
#define CONFIGURE_MAXIMUM_TIMERS 32
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES 16

#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_UNIFIED_WORK_AREAS

#define CONFIGURE_INIT_TASK_STACK_SIZE (64 * 1024)
#define CONFIGURE_MINIMUM_TASK_STACK_SIZE (8 * 1024)

/*
         *** FATAL ***
fatal source: 0 (INTERNAL_ERROR_CORE)
fatal code: 2 (INTERNAL_ERROR_TOO_LITTLE_WORKSPACE)
RTEMS version: 6.0.0.0a46769ba42d3476b0f37a85db49b3276658d293
RTEMS tools: 13.3.0 20240521 (RTEMS 6, RSB b1aec32059aa0e86385ff75ec01daf93713fa382-modified, Newlib 1b3dcfd)

#define CONFIGURE_EXECUTIVE_RAM_SIZE (16 * 1024 * 1024)
*/ 

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#define CONFIGURE_INIT_TASK_ENTRY_POINT Init
#define CONFIGURE_INIT_TASK_NAME rtems_build_name('U', 'I', ' ', ' ')

#define CONFIGURE_INIT
#include <rtems/confdefs.h>

/* LibBSD Configuration - BSP config only, NO INIT */
#define RTEMS_BSD_CONFIG_BSP_CONFIG
#include <machine/rtems-bsd-config.h>
