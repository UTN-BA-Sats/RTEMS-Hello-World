/*
 * Button-triggered Hello World with UDP Network Support for RTEMS
 */
#include <rtems.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <rtems/version.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <rtems/bsd/bsd.h>

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
#define UDP_TARGET_IP   "192.168.0.104"
#define UDP_TARGET_PORT 5000

static int udp_socket = -1;
static struct sockaddr_in target_addr;
static bool network_ready = false;

static void setup_udp_socket(void)
{
  int ret;
  
  printf("Setting up UDP socket...\n");
  fflush(stdout);
  
  /* Create UDP socket - will be called after BSD init completes */
  udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (udp_socket < 0) {
    printf("Socket creation failed, errno: %d\n", errno);
    fflush(stdout);
    return;
  }
  
  printf("Socket created: %d\n", udp_socket);
  fflush(stdout);
  
  /* Set up target address */
  memset(&target_addr, 0, sizeof(target_addr));
  target_addr.sin_family = AF_INET;
  target_addr.sin_port = htons(UDP_TARGET_PORT);
  ret = inet_aton(UDP_TARGET_IP, &target_addr.sin_addr);
  if (ret == 0) {
    printf("Invalid IP address\n");
    close(udp_socket);
    udp_socket = -1;
    fflush(stdout);
    return;
  }
  
  network_ready = true;
  printf("UDP socket ready: %s:%d\n", UDP_TARGET_IP, UDP_TARGET_PORT);
  fflush(stdout);
}

static void send_udp_message(const char *message)
{
  if (!network_ready || udp_socket < 0) {
    printf("[Console] %s\n", message);
    fflush(stdout);
    return;
  }
  
  ssize_t ret = sendto(udp_socket, message, strlen(message), 0,
                       (struct sockaddr *)&target_addr, sizeof(target_addr));
  if (ret < 0) {
    printf("[UDP Error] errno: %d\n", errno);
  } else {
    printf("[UDP Sent] %ld bytes\n", ret);
  }
  fflush(stdout);
}

rtems_task Init(
  rtems_task_argument ignored
)
{
  int counter = 0;
  bool last_button_state = false;
  bool current_button_state;
  char message_buffer[128];
  
  printf( "\n\n*** BUTTON-TRIGGERED HELLO WORLD WITH UDP TEST ***\n" );
  printf( "Board: STM32H743ZI Nucleo\n" );
  printf( "RTEMS Version: %s\n\n", rtems_version() );
  fflush(stdout);
  
  /* Setup UDP logging */
  setup_udp_socket();
  
  /* Enable GPIOC clock */
  RCC_AHB4ENR |= RCC_AHB4ENR_GPIOCEN;
  
  /* Configure PC13 as input (clear both mode bits) */
  GPIOC_MODER &= ~(0x3 << (BUTTON_PIN * 2));
  
  /* Configure PC13 with no pull-up/pull-down (button has external pull-up) */
  GPIOC_PUPDR &= ~(0x3 << (BUTTON_PIN * 2));
  
  printf( "GPIO configured, waiting for button press...\n" );
  fflush(stdout);
  
  while (1) {
    /* Read button state (active low, so invert) */
    current_button_state = !(GPIOC_IDR & (1 << BUTTON_PIN));
    
    /* Detect button press (transition from not pressed to pressed) */
    if (current_button_state && !last_button_state) {
      snprintf(message_buffer, sizeof(message_buffer),
               "Hello World #%d from Nucleo STM32H743ZI", counter);
      printf("Button pressed! Sending: %s\n", message_buffer);
      send_udp_message(message_buffer);
      counter++;
    }
    
    last_button_state = current_button_state;
    
    /* Debounce delay - 50ms */
    rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(50));
  }
  
  /* Never reached */
  exit( 0 );
}
