/*
 * Button-triggered Hello World with UDP Network Support for RTEMS
 * Enhanced with detailed UART logging for debugging
 */
#include <rtems.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <rtems/version.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <rtems/bsd/bsd.h>

#define RCC_BASE        0x58024400
#define RCC_AHB4ENR     (*(volatile uint32_t *)(RCC_BASE + 0xE0))
#define RCC_AHB4ENR_GPIOCEN  (1 << 2)

#define GPIOC_BASE   0x58020800
#define GPIOC_MODER  (*(volatile uint32_t *)(GPIOC_BASE + 0x00))
#define GPIOC_PUPDR  (*(volatile uint32_t *)(GPIOC_BASE + 0x0C))
#define GPIOC_IDR    (*(volatile uint32_t *)(GPIOC_BASE + 0x10))

#define BUTTON_PIN   13

#define UDP_TARGET_IP   "192.168.0.104"
#define UDP_TARGET_PORT 5000

static int udp_socket = -1;
static struct sockaddr_in target_addr;
static bool network_ready = false;

static void log_uart(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  printf("[LOG] ");
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
}

static void setup_udp_socket(void)
{
  int ret;

  log_uart("Starting UDP socket setup...");

  udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (udp_socket < 0) {
    log_uart("Socket creation failed. errno=%d (%s)", errno, strerror(errno));
    return;
  }
  log_uart("Socket created with fd=%d", udp_socket);

  memset(&target_addr, 0, sizeof(target_addr));
  target_addr.sin_family = AF_INET;
  target_addr.sin_port = htons(UDP_TARGET_PORT);
  ret = inet_aton(UDP_TARGET_IP, &target_addr.sin_addr);
  if (ret == 0) {
    log_uart("Invalid UDP target IP address: %s", UDP_TARGET_IP);
    close(udp_socket);
    udp_socket = -1;
    return;
  }

  network_ready = true;
  log_uart("UDP socket ready for target %s:%d", UDP_TARGET_IP, UDP_TARGET_PORT);
}

static void send_udp_message(const char *message)
{
  if (!network_ready || udp_socket < 0) {
    log_uart("[Console] %s", message);
    return;
  }

  ssize_t ret = sendto(udp_socket, message, strlen(message), 0,
                       (struct sockaddr *)&target_addr, sizeof(target_addr));
  if (ret < 0) {
    log_uart("[UDP ERROR] sendto() failed errno=%d (%s)", errno, strerror(errno));
  } else {
    log_uart("[UDP SENT] %ld bytes: %s", ret, message);
  }
}

rtems_task Init(
  rtems_task_argument unused
)
{
  int counter = 0;
  bool last_button_state = false;
  bool current_button_state;
  char message_buffer[128];
  rtems_status_code sc;

  log_uart("\n\n*** BUTTON-TRIGGERED HELLO WORLD WITH UDP TEST ***");
  log_uart("Board: STM32H743ZI Nucleo");
  log_uart("RTEMS Version: %s", rtems_version());

  log_uart("Enabling GPIOC clock...");
  RCC_AHB4ENR |= RCC_AHB4ENR_GPIOCEN;

  log_uart("Configuring PC13 as input, no pull-up/pull-down...");
  GPIOC_MODER &= ~(0x3 << (BUTTON_PIN * 2));  // Input mode
  GPIOC_PUPDR &= ~(0x3 << (BUTTON_PIN * 2));  // No pull-up/pull-down

  log_uart("Delaying 100ms to allow clock stabilization...");
  rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(100));

  log_uart("Starting libbsd initialization...");
  sc = rtems_bsd_initialize();
  if (sc != RTEMS_SUCCESSFUL) {
    log_uart("rtems_bsd_initialize() failed with status: %d", sc);
  } else {
    log_uart("libbsd initialized successfully.");
  }

  sc = rtems_bsd_ifconfig_lo0();
  if (sc != RTEMS_SUCCESSFUL) {
    log_uart("Failed to configure loopback interface, status: %d", sc);
  } else {
    log_uart("Loopback interface configured.");
  }

  log_uart("Delaying 5 seconds to ensure libbsd network ready...");
  rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(5000));

  setup_udp_socket();

  log_uart("Starting main loop, waiting for button press...");

  while (1) {
    current_button_state = !(GPIOC_IDR & (1 << BUTTON_PIN));

    if (current_button_state && !last_button_state) {
      snprintf(message_buffer, sizeof(message_buffer),
               "Hello World #%d from Nucleo STM32H743ZI", counter);
      log_uart("Button pressed! Sending UDP message: %s", message_buffer);
      send_udp_message(message_buffer);
      counter++;
    }

    last_button_state = current_button_state;

    rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(50));
  }

  // Never reached
  exit(0);
}
