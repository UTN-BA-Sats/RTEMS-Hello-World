/*
 * Button-triggered Hello World with UDP for RTEMS
 */
#include <rtems.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <rtems/version.h>

/* Network includes */
#include <machine/rtems-bsd-commands.h>
#include <rtems/bsd/bsd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>  /* For close() */


/* STM32H743 RCC (Reset and Clock Control) */
#define RCC_BASE        0x58024400
#define RCC_AHB4ENR     (*(volatile uint32_t *)(RCC_BASE + 0xE0))
#define RCC_AHB4ENR_GPIOCEN  (1 << 2)  /* Enable GPIOC clock */

/* STM32H743 GPIO Port C */
#define GPIOC_BASE   0x58020800
#define GPIOC_MODER  (*(volatile uint32_t *)(GPIOC_BASE + 0x00))
#define GPIOC_PUPDR  (*(volatile uint32_t *)(GPIOC_BASE + 0x0C))
#define GPIOC_IDR    (*(volatile uint32_t *)(GPIOC_BASE + 0x10))

/* PC13 is the blue button */
#define BUTTON_PIN   13

/* UDP Configuration */
#define UDP_SERVER_IP   "192.168.1.100"  /* Change to your server IP */
#define UDP_SERVER_PORT 5000              /* Change to your server port */

/* Global UDP socket */
static int udp_socket = -1;
static struct sockaddr_in server_addr;

/* Function to initialize UDP socket */
static int init_udp(void)
{
  /* Create UDP socket */
  udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (udp_socket < 0) {
    printf("ERROR: Failed to create UDP socket\n");
    return -1;
  }
  
  /* Configure server address */
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(UDP_SERVER_PORT);
  
  if (inet_pton(AF_INET, UDP_SERVER_IP, &server_addr.sin_addr) <= 0) {
    printf("ERROR: Invalid server IP address\n");
    close(udp_socket);
    return -1;
  }
  
  printf("UDP socket initialized successfully\n");
  return 0;
}

/* Function to send message via UDP */
static void send_udp_message(const char *message)
{
  if (udp_socket < 0) {
    printf("ERROR: UDP socket not initialized\n");
    return;
  }
  
  ssize_t sent = sendto(udp_socket, message, strlen(message), 0,
                        (struct sockaddr *)&server_addr, sizeof(server_addr));
  
  if (sent < 0) {
    printf("ERROR: Failed to send UDP message\n");
  }
}

/* Helper function to print and send via UDP */
static void print_and_send(const char *message)
{
  printf("%s", message);
  send_udp_message(message);
}

rtems_task Init(
  rtems_task_argument ignored
)
{
  int counter = 0;
  bool last_button_state = false;
  bool current_button_state;
  char msg_buffer[128];
  rtems_status_code sc;
  
  print_and_send("\n\n*** BUTTON-TRIGGERED HELLO WORLD TEST ***\n");
  print_and_send("Press the blue button to print message...\n");
  
  snprintf(msg_buffer, sizeof(msg_buffer), "Board: STM32H743ZI Nucleo\n");
  print_and_send(msg_buffer);
  
  snprintf(msg_buffer, sizeof(msg_buffer), "RTEMS Version: %s\n\n", rtems_version());
  print_and_send(msg_buffer);
  
  /* Initialize libbsd */
  print_and_send("Initializing network stack...\n");
  sc = rtems_bsd_initialize();
  if (sc != RTEMS_SUCCESSFUL) {
    printf("ERROR: Failed to initialize libbsd: %d\n", sc);
    exit(1);
  }
  
  /* Wait for network interface to come up */
  /* You may need to configure your network interface here */
  /* Example: run DHCP or configure static IP */
  print_and_send("Network stack initialized\n");
  
  /* Initialize UDP */
  if (init_udp() != 0) {
    print_and_send("ERROR: Failed to initialize UDP\n");
    exit(1);
  }
  
  /* Enable GPIOC clock */
  RCC_AHB4ENR |= RCC_AHB4ENR_GPIOCEN;
  
  /* Configure PC13 as input (clear both mode bits) */
  GPIOC_MODER &= ~(0x3 << (BUTTON_PIN * 2));
  
  /* Configure PC13 with no pull-up/pull-down (button has external pull-up) */
  GPIOC_PUPDR &= ~(0x3 << (BUTTON_PIN * 2));
  
  print_and_send("GPIO configured, waiting for button press...\n");
  
  while (1) {
    /* Read button state (active low, so invert) */
    current_button_state = !(GPIOC_IDR & (1 << BUTTON_PIN));
    
    /* Detect button press (transition from not pressed to pressed) */
    if (current_button_state && !last_button_state) {
      snprintf(msg_buffer, sizeof(msg_buffer), 
               "Version 12 - Button pressed! Hello World #%d\n", counter);
      print_and_send(msg_buffer);
      counter++;
    }
    
    last_button_state = current_button_state;
    
    /* Debounce delay - 50ms */
    rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(50));
  }
  
  /* Cleanup (never reached) */
  if (udp_socket >= 0) {
    close(udp_socket);
  }
  
  exit(0);
}

/* RTEMS Configuration */
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER

#define CONFIGURE_MAXIMUM_TASKS 32
#define CONFIGURE_MAXIMUM_SEMAPHORES 32
#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS 32

#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_UNIFIED_WORK_AREAS

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INIT

#include <rtems/confdefs.h>
