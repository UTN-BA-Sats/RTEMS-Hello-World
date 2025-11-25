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
    printf("Setting up UDP socket...\n");
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        printf("Socket creation failed: errno=%d\n", errno);
        return;
    }
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(UDP_TARGET_PORT);
    ret = inet_aton(UDP_TARGET_IP, &target_addr.sin_addr);
    if (ret == 0) {
        printf("Invalid target IP address\n");
        close(udp_socket);
        udp_socket = -1;
        return;
    }
    udp_ready = true;
    printf("UDP socket ready: %s:%d\n", UDP_TARGET_IP, UDP_TARGET_PORT);
}

static void send_udp_message(const char *message)
{
    if (!udp_ready || udp_socket < 0) {
        printf("[Console fallback] %s\n", message);
        return;
    }
    ssize_t ret = sendto(udp_socket, message, strlen(message), 0,
                         (struct sockaddr *)&target_addr, sizeof(target_addr));
    if (ret < 0) {
        printf("[UDP Error] errno: %d\n", errno);
    } else {
        printf("[UDP Sent] %ld bytes\n", ret);
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

    /* 1. Init LibBSD */
    sc = rtems_bsd_initialize();
    if (sc != RTEMS_SUCCESSFUL) {
        printf("ERROR: rtems_bsd_initialize() failed, status=0x%08x\n", sc);
        /* loop forever -- or rtems_fatal_error_occurred(sc); */
        while (1) { rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(1000)); }
    }

    /* 2. Configure network interface using ifconfig command */
    printf("Bringing up network interface stmac0 with DHCP...\n");
    char *ifconfig_argv[] = {
        "ifconfig",
        "stmac0",
        "inet",
        "dhcp",
        "up",
        NULL
    };
    rtems_bsd_command_ifconfig(5, ifconfig_argv);

    /* 3. Wait for DHCP to complete */
    printf("Waiting 10s for DHCP address...\n");
    rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(10000));

    /* 4. Print final network config (diagnostic) */
    rtems_bsd_command_ifconfig(0, NULL);

    /* 5. Setup UDP */
    setup_udp_socket();

    /* 6. GPIO Setup */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOCEN;
    GPIOC_MODER &= ~(0x3 << (BUTTON_PIN * 2));
    GPIOC_PUPDR &= ~(0x3 << (BUTTON_PIN * 2));
    printf("GPIO configured, waiting for button press...\n");

    while (1) {
        current_button_state = !(GPIOC_IDR & (1 << BUTTON_PIN));
        if (current_button_state && !last_button_state) {
            snprintf(message_buffer, sizeof(message_buffer),
                     "Hello World #%d from Nucleo STM32H743ZI", counter);
            printf("Button pressed! Sending: %s\n", message_buffer);
            send_udp_message(message_buffer);
            counter++;
        }
        last_button_state = current_button_state;
        rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(50)); /* debounce */
    }
    exit(0);
}
