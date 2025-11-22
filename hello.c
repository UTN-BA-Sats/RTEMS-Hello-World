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

/* Network includes - only if enabled */
#define ENABLE_NETWORK 1

#if ENABLE_NETWORK
#include <rtems/bsd/bsd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Include our network configuration */
#include "network-config.h"
#endif

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

/* Network configuration - disable if causing issues */
#define UDP_SERVER_IP   "192.168.0.104"
#define UDP_SERVER_PORT 5000

#if ENABLE_NETWORK
static int udp_socket = -1;
static struct sockaddr_in server_addr;
#endif

static void send_udp_message(const char *message)
{
#if ENABLE_NETWORK
  if (udp_socket < 0) return;
  sendto(udp_socket, message, strlen(message), 0,
         (struct sockaddr *)&server_addr, sizeof(server_addr));
#endif
}

static void print_and_send(const char *message)
{
  printf("%s", message);
  send_udp_message(message);
}

#if ENABLE_NETWORK
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

/* Alternative lightweight network initialization for cases where full libbsd fails */
static rtems_status_code init_libbsd_with_retry(void)
{
  rtems_status_code sc;
  int retry_count = 0;
  rtems_time_of_day tod;
  
  printf("Attempting to initialize libbsd...\n");
  printf("DEBUG: This requires RTEMS kernel built with --enable-posix --disable-networking\n");
  printf("DEBUG: Checking clock system...\n");
  fflush(stdout);
  
  /* Check if clock is working */
  sc = rtems_clock_get_tod(&tod);
  if (sc != RTEMS_SUCCESSFUL) {
    printf("WARNING: Clock system not ready (status=%d)\n", sc);
  } else {
    printf("DEBUG: Clock OK (%04d-%02d-%02d %02d:%02d:%02d)\n",
           tod.year, tod.month, tod.day, tod.hour, tod.minute, tod.second);
  }
  
  printf("DEBUG: Checking libbsd availability...\n");
  fflush(stdout);
  
  for (retry_count = 0; retry_count < 5; retry_count++) {
    printf("DEBUG: Calling rtems_bsd_initialize() (attempt %d)...\n", retry_count + 1);
    fflush(stdout);
    
    sc = rtems_bsd_initialize();
    
    if (sc == RTEMS_SUCCESSFUL) {
      printf("libbsd initialized successfully on attempt %d\n", retry_count + 1);
      return RTEMS_SUCCESSFUL;
    }
    
    printf("libbsd initialization attempt %d failed (status=%d: 0x%x)\n", 
           retry_count + 1, sc, sc);
    fflush(stdout);
    
    if (retry_count < 4) {
      printf("Retrying in 2 seconds...\n");
      fflush(stdout);
      rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(2000));
    }
  }
  
  printf("\n=== LIBBSD INITIALIZATION FAILED ===\n");
  printf("Status code: %d (0x%x)\n", sc, sc);
  printf("\nROOT CAUSE:\n");
  printf("The RTEMS BSP was likely built WITHOUT explicit --enable-libbsd flag.\n");
  printf("\nYour Docker build uses 'waf bspdefaults' which may not enable libbsd.\n");
  printf("\nSOLUTION:\n");
  printf("Rebuild RTEMS 6.1 with these exact flags:\n");
  printf("  ./waf configure \\\n");
  printf("    --prefix=/opt/rtems/6.1 \\\n");
  printf("    --rtems-bsps=arm/nucleo-h743zi \\\n");
  printf("    --rtems-tools=/opt/rtems/6.1 \\\n");
  printf("    --enable-posix \\\n");
  printf("    --disable-networking \\\n");
  printf("    --enable-libbsd\n");
  printf("\nOr update Docker build script with:\n");
  printf("  RUN ./waf configure \\\n");
  printf("    ... existing args ...\\\n");
  printf("    --enable-libbsd\n");
  printf("\nWithout libbsd, UDP networking is unavailable.\n");
  printf("Button functionality will work normally.\n\n");
  fflush(stdout);
  
  return sc;
}
#endif

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
  
#if ENABLE_NETWORK
  printf("Initializing network stack...\n");
  sc = init_libbsd_with_retry();
  if (sc != RTEMS_SUCCESSFUL) {
    printf("ERROR: Failed to initialize libbsd: %d\n", sc);
    printf("Continuing without network...\n\n");
    printf("RECOMMENDATION:\n");
    printf("The error suggests the RTEMS BSP was not built with libbsd support.\n");
    printf("Try rebuilding RTEMS 6 kernel with:\n");
    printf("  ./configure --enable-posix --disable-networking \\\n");
    printf("              --enable-rtemsbsp=arm/nucleo-h743zi\n");
    printf("Or use a pre-built BSP known to include libbsd.\n\n");
  } else {
    printf("Network stack initialized successfully\n");
    
    /* Wait for network stack to be ready */
    printf("Waiting for network stack to stabilize...\n");
    rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(2000));
    
    /* Configure the network interface with our static IP */
    configure_network_static();

    rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(1000));
    
    if (init_udp() == 0) {
      printf("UDP ready to %s:%d\n\n", UDP_SERVER_IP, UDP_SERVER_PORT);
    } else {
      printf("UDP initialization failed, continuing without UDP...\n\n");
    }
  }
#else
  printf("Network stack support disabled\n\n");
#endif
  
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

#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS 32
#define CONFIGURE_MAXIMUM_TASKS 16
#define CONFIGURE_MAXIMUM_SEMAPHORES 32
#define CONFIGURE_MAXIMUM_TIMERS 16
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES 8

#define CONFIGURE_UNIFIED_WORK_AREAS
#define CONFIGURE_UNIFIED_WORK_AREA_SIZE (256 * 1024)

#define CONFIGURE_INIT_TASK_STACK_SIZE (16 * 1024)
#define CONFIGURE_MINIMUM_TASK_STACK_SIZE (2 * 1024)

/* Request the standard network stack configuration symbols */
#define RTEMS_BSD_CONFIG_INCLUDE_COMMON_SYMBOLS

/* Allocate more mbufs for network operations - error 13 might indicate insufficient memory */
#define RTEMS_BSD_CONFIG_DOMAIN_PAGE_MBUFS_SIZE (4 * 1024 * 1024)
#define RTEMS_BSD_CONFIG_MBUFS 2000
#define RTEMS_BSD_CONFIG_CLUSTERS 2000

/* Include the BSP-specific network driver configuration */
#define RTEMS_BSD_CONFIG_BSP_CONFIG

/* Initialize the libbsd configuration */
#define RTEMS_BSD_CONFIG_INIT

#include <machine/rtems-bsd-config.h>

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#define CONFIGURE_INIT_TASK_ENTRY_POINT Init
#define CONFIGURE_INIT_TASK_NAME rtems_build_name('U', 'I', ' ', ' ')

#define CONFIGURE_INIT
#include <rtems/confdefs.h>