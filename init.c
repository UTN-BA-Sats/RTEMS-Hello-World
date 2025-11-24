/*
 * RTEMS configuration with libbsd network support
 */
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER

#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_UNIFIED_WORK_AREAS

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

/* Increase stack size for safety */
#define CONFIGURE_MINIMUM_TASK_STACK_SIZE (8 * 1024)

/* Enable libbsd network stack */
#define CONFIGURE_BSD_INIT_TASK_STACK_SIZE (32 * 1024)

#define CONFIGURE_INIT

#include <rtems/confdefs.h>

