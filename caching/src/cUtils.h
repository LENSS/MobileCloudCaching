
#ifndef CUTILS_H
#define CUTILS_H

#include <asm/byteorder.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/ctype.h>
#include <asm/div64.h>

u_int64_t getcurrtime();
char *inet_ntoa(u_int32_t ina);
int inet_aton(const char *cp, u_int32_t * addr);
int atoi(const char *nptr);
char *strtok_r(char *s, const char *delim, char **save_ptr);

#endif
