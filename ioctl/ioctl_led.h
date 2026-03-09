#ifndef _LED_IOCTL_H
#define _LED_IOCTL_H

#include <linux/ioctl.h>

#define GPIO_SELECT_LED  _IOW('l', 1, int)
#define GPIO_SET_LED     _IOW('l', 2, int)
#define GPIO_GET_LED     _IOR('l', 3, int)

#endif
