/*
 * RTEMS configuration with libbsd network support
 * This configuration file sets up the basic RTEMS system and BSD network stack.
 */
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER


#define CONFIGURE_UNIFIED_WORK_AREAS

/* Increase workspace to 768KB, but still below 1MB */
#define CONFIGURE_EXECUTIVE_RAM_SIZE (768 * 1024)
#define CONFIGURE_MEMORY_PER_TASK_FOR_DYNAMIC_ALLOCATION (8 * 1024)

/* Limit RTEMS objects for minimal footprint */
#define CONFIGURE_MAXIMUM_TASKS             6
#define CONFIGURE_MAXIMUM_SEMAPHORES        8
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES    4
#define CONFIGURE_MAXIMUM_TIMERS            4
#define CONFIGURE_MAXIMUM_PERIODS           2

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

/* Reduce stack sizes further */
#define CONFIGURE_MINIMUM_TASK_STACK_SIZE (2 * 1024)
#define CONFIGURE_INIT_TASK_STACK_SIZE (8 * 1024)

/* BSD stack: reduce further */
#define CONFIGURE_BSD_INIT_TASK_STACK_SIZE (16 * 1024)
#define CONFIGURE_BSD_INIT_TASK_PRIORITY 50

#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS 8

#define CONFIGURE_INIT

#include <rtems/confdefs.h>
#include <bsp.h>
#include <rtems/bsd/bsd.h>

