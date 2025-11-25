/*
 * RTEMS configuration with libbsd network support
 * This configuration file sets up the basic RTEMS system and BSD network stack.
 */
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER

#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_UNIFIED_WORK_AREAS

/* Increase workspace size for BSD stack and networking */
#define CONFIGURE_EXECUTIVE_RAM_SIZE (16 * 1024 * 1024)
#define CONFIGURE_MEMORY_PER_TASK_FOR_DYNAMIC_ALLOCATION (64 * 1024)

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

/* Increase stack size for safety */
#define CONFIGURE_MINIMUM_TASK_STACK_SIZE (16 * 1024)
#define CONFIGURE_INIT_TASK_STACK_SIZE (32 * 1024)

/* Enable libbsd network stack - proper config */
#define CONFIGURE_BSD_INIT_TASK_STACK_SIZE (64 * 1024)
#define CONFIGURE_BSD_INIT_TASK_PRIORITY 50

#define CONFIGURE_INIT

#include <rtems/confdefs.h>
#include <bsp.h>
#include <rtems/bsd/bsd.h>

