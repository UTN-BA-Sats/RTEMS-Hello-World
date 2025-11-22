#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

#include <machine/rtems-bsd-commands.h>
#include <rtems/bsd/bsd.h>

/* Network configuration - adjust these for your network */
#define STATIC_IP      "192.168.0.55"
#define NETMASK        "255.255.255.0"
#define GATEWAY        "192.168.0.1"
#define INTERFACE_NAME "stm0"  /* STM32H743ZI Ethernet interface */

static void configure_network_static(void)
{
  int exit_code;
  char *ifconfig_args[] = {
    "ifconfig",
    INTERFACE_NAME,
    "inet",
    STATIC_IP,
    "netmask",
    NETMASK,
    NULL
  };
  
  printf("Configuring network interface %s...\n", INTERFACE_NAME);
  exit_code = rtems_bsd_command_ifconfig(RTEMS_BSD_ARGC(ifconfig_args), ifconfig_args);
  
  if (exit_code != EXIT_SUCCESS) {
    printf("ERROR: Failed to configure interface\n");
  }
  
  /* Add default route */
  char *route_args[] = {
    "route",
    "add",
    "default",
    GATEWAY,
    NULL
  };
  
  exit_code = rtems_bsd_command_route(RTEMS_BSD_ARGC(route_args), route_args);
  
  if (exit_code != EXIT_SUCCESS) {
    printf("ERROR: Failed to add default route\n");
  }
  
  printf("Network configured: IP=%s, Gateway=%s\n", STATIC_IP, GATEWAY);
}

#endif /* NETWORK_CONFIG_H */
