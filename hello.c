/*
 * BUTTON-TRIGGERED HELLO WORLD WITH UDP TEST (RTEMS 6, STM32H743ZI, LIBBSD-READY)
 * With error reporting for rtems_bsd_initialize()
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
#include <machine/rtems-bsd-commands.h> // Optional: for rtems_bsd_command_ifconfig()

#define UDP_TARGET_IP   "192.168.0.104"
#define UDP_TARGET_PORT 5000

/* STM32H743 RCC / GPIO Addresses */
#define RCC_BASE        0x58024400
#define RCC_AHB4ENR     (*(volatile uint32_t *)(RCC_BASE + 0xE0))
#define RCC_AHB4ENR_GPIOCEN  (1 << 2)
#define GPIOC_BASE      0x58020800
#define GPIOC_MODER     (*(volatile uint32_t *)(GPIOC_BASE + 0x00))
#define GPIOC_PUPDR     (*(volatile uint32_t *)(GPIOC_BASE + 0x0C))
#define GPIOC_IDR       (*(volatile uint32_t *)(GPIOC_BASE + 0x10))
#define BUTTON_PIN      13

static int udp_socket = -1;
static struct sockaddr_in target_addr;
static bool udp_ready = false;

/* 
 * 1. Force the linker to include the stmac driver symbols.
 *    This macro creates a reference used by the LibBSD initialization.
 */
SYSINIT_DRIVER_REFERENCE(stmac, nexus);

static void setup_udp_socket(void)
{
    int ret;
    printf("[UDP] Starting UDP socket setup...\n");
    printf("[UDP] Creating DGRAM socket...\n");
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        printf("[UDP] ERROR: Socket creation failed with errno=%d\n", errno);
        perror("socket");
        return;
    }
    printf("[UDP] Socket created successfully (fd=%d)\n", udp_socket);
    
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(UDP_TARGET_PORT);
    printf("[UDP] Parsing target IP: %s\n", UDP_TARGET_IP);
    ret = inet_aton(UDP_TARGET_IP, &target_addr.sin_addr);
    if (ret == 0) {
        printf("[UDP] ERROR: Invalid target IP address '%s'\n", UDP_TARGET_IP);
        close(udp_socket);
        udp_socket = -1;
        return;
    }
    printf("[UDP] Target address parsed: %s:%d\n", UDP_TARGET_IP, UDP_TARGET_PORT);
    
    udp_ready = true;
    printf("[UDP] Socket ready and configured!\n");
}

static void send_udp_message(const char *message)
{
    if (!udp_ready || udp_socket < 0) {
        printf("[UDP] WARNING: Socket not ready, using console fallback\n");
        printf("[Console fallback] %s\n", message);
        return;
    }
    
    size_t msg_len = strlen(message);
    printf("[UDP] Sending %zu bytes to %s:%d\n", msg_len, UDP_TARGET_IP, UDP_TARGET_PORT);
    
    ssize_t ret = sendto(udp_socket, message, msg_len, 0,
                         (struct sockaddr *)&target_addr, sizeof(target_addr));
    if (ret < 0) {
        printf("[UDP] ERROR: sendto() failed with errno=%d\n", errno);
        perror("sendto");
    } else if (ret != (ssize_t)msg_len) {
        printf("[UDP] WARNING: Sent %ld bytes, expected %zu\n", ret, msg_len);
    } else {
        printf("[UDP] SUCCESS: Message sent (%ld bytes)\n", ret);
    }
}

rtems_task Init(
    rtems_task_argument ignored
)
{
    int counter = 0;
    bool last_button_state = false;
    bool current_button_state;
    char message_buffer[128];
    rtems_status_code sc;

    printf("\n*** BUTTON-TRIGGERED HELLO WORLD WITH UDP TEST ***\n");
    printf("Board: STM32H743ZI Nucleo\n");
    printf("RTEMS Version: %s\n\n", rtems_version());

    /* 1. LibBSD initialization happens automatically when CONFIGURE_BSD_INIT_TASK_STACK_SIZE is set */
    printf("[INIT] Waiting for BSD stack initialization...\n");
    printf("[INIT] Sleeping for 2 seconds to allow BSD task to initialize...\n");
    rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(2000));
    
    printf("[INIT] BSD stack should be ready now.\n\n");

    /* 2. Configure network interface using ifconfig command */
    printf("[NETWORK] Bringing up network interface stmac0 with DHCP...\n");
    printf("[NETWORK] Executing: ifconfig stmac0 inet dhcp up\n");
    char *ifconfig_argv[] = {
        "ifconfig",
        "stmac0",
        "inet",
        "dhcp",
        "up",
        NULL
    };
    int ifconfig_result = rtems_bsd_command_ifconfig(5, ifconfig_argv);
    printf("[NETWORK] ifconfig returned: %d\n\n", ifconfig_result);

    /* 3. Wait for DHCP to complete */
    printf("[NETWORK] Waiting 10 seconds for DHCP address assignment...\n");
    for (int i = 0; i < 10; i++) {
        printf("[NETWORK] %ds...\n", 10 - i);
        rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(1000));
    }

    /* 4. Print final network config (diagnostic) */
    printf("[NETWORK] Final network configuration:\n");
    rtems_bsd_command_ifconfig(0, NULL);
    printf("\n");

    /* 5. Setup UDP */
    printf("[INIT] Setting up UDP communication...\n");
    setup_udp_socket();
    printf("[INIT] UDP setup complete.\n\n");

    /* 6. GPIO Setup */
    printf("[GPIO] Initializing GPIO for button...\n");
    printf("[GPIO] Enabling GPIOC clock...\n");
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOCEN;
    printf("[GPIO] Configuring PC%d as input...\n", BUTTON_PIN);
    GPIOC_MODER &= ~(0x3 << (BUTTON_PIN * 2));
    GPIOC_PUPDR &= ~(0x3 << (BUTTON_PIN * 2));
    printf("[GPIO] GPIO configuration complete.\n\n");
    printf("[MAIN] Ready! Waiting for button press on PC%d...\n", BUTTON_PIN);

    printf("[MAIN] Starting main event loop...\n");
    while (1) {
        current_button_state = !(GPIOC_IDR & (1 << BUTTON_PIN));
        if (current_button_state && !last_button_state) {
            printf("[BUTTON] Button pressed (count: %d)!\n", counter);
            snprintf(message_buffer, sizeof(message_buffer),
                     "Hello World #%d from Nucleo STM32H743ZI", counter);
            printf("[BUTTON] Composed message: %s\n", message_buffer);
            send_udp_message(message_buffer);
            counter++;
            printf("[BUTTON] Message sent, waiting for button release...\n\n");
        }
        last_button_state = current_button_state;
        rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(50)); /* debounce */
    }
    exit(0);
}
