
#include "cUtils.h"

u_int64_t getcurrtime()
{
    struct timeval tv;
    u_int64_t result;

    do_gettimeofday(&tv);

    //This is a fix for an error that occurs on ARM Linux Kernels because they do 64bits differently
    //Thanks to S. Peter Li for coming up with this fix!

    result = (u_int64_t) tv.tv_usec;
    do_div(result, 1000);
    return ((u_int64_t) tv.tv_sec) * 1000 + result;
}

char *inet_ntoa(u_int32_t ina)
{
    static char buf[4 * sizeof "123"];
    unsigned char *ucp = (unsigned char *) &ina;
    sprintf(buf, "%d.%d.%d.%d", ucp[0] & 0xff, ucp[1] & 0xff, ucp[2] & 0xff, ucp[3] & 0xff);
    return buf;
}

int inet_aton(const char *cp, u_int32_t *addr)
{
    unsigned int val;
    int base, n;
    char c;
    u_int parts[4];
    u_int *pp = parts;

    for (;;)
    {
        //Collect number up to ``.''. Values are specified as for C:
        // 0x=hex, 0=octal, other=decimal.

        val = 0;
        base = 10;

        if (*cp == '0')
        {
            if (*++cp == 'x' || *cp == 'X')
                base = 16, cp++;
            else
                base = 8;
        }

        while ((c = *cp) != '\0')
        {
            if (isascii(c) && isdigit(c))
            {
                val = (val * base) + (c - '0');
                cp++;
                continue;

            }
            if (base == 16 && isascii(c) && isxdigit(c))
            {
                val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
                cp++;
                continue;
            }
            break;
        }

        if (*cp == '.')
        {
            // Internet format: a.b.c.d a.b.c       (with c treated as
            // 16-bits) a.b         (with b treated as 24 bits)
            if (pp >= parts + 3 || val > 0xff)
                return (0);
            *pp++ = val, cp++;
        }
        else
            break;
    }

    // Check for trailing characters.

    if (*cp && (!isascii(*cp) || !isspace(*cp)))
        return (0);

    // Concoct the address according to the number of parts specified.

    n = pp - parts + 1;
    switch (n)
    {

        case 1:                    // a -- 32 bits
            break;

        case 2:                    //a.b -- 8.24 bits
            if (val > 0xffffff)
                return (0);
            val |= parts[0] << 24;
            break;

        case 3:                    //a.b.c -- 8.8.16 bits
            if (val > 0xffff)
                return (0);
            val |= (parts[0] << 24) | (parts[1] << 16);
            break;

        case 4:                    // a.b.c.d -- 8.8.8.8 bits
            if (val > 0xff)
                return (0);
            val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
            break;
    }

    if (addr)
        *addr = htonl(val);
    return (1);
}

int my_isspace(int x)
{
    if (x == ' ' || x == '\t' || x == '\n' || x == '\f' || x == '\b' || x == '\r')
	return 1;
    else  
	return 0;
}

int my_isdigit(int x)
{
    if (x <= '9' && x >= '0')         
	return 1;
    else 
	return 0;
}

int atoi(const char *nptr)
{
    int c;              /* current char */
    int total;         /* current total */
    int sign;           /* if '-', then negative, otherwise positive */

    /* skip whitespace */
    while (my_isspace((int)(unsigned char)*nptr) == 1)
        ++nptr;

    c = (int)(unsigned char)*nptr++;
    sign = c;           /* save sign indication */
    if (c == '-' || c == '+')
        c = (int)(unsigned char)*nptr++;    /* skip sign */

    total = 0;

    while (my_isdigit(c) == 1) {
        total = 10 * total + (c - '0');     /* accumulate digit */
        c = (int)(unsigned char)*nptr++;    /* get next char */
    }

    if (sign == '-')
        return -total;
    else
        return total;   /* return result, negated if necessary */
}

char *strtok_r(char *s, const char *delim, char **save_ptr) {     
    char *token;     
     
    if (s == NULL) s = *save_ptr;     
     
    /* Scan leading delimiters.  */     
    s += strspn(s, delim);     
    if (*s == '\0')      
        return NULL;     
     
    /* Find the end of the token.  */     
    token = s;     
    s = strpbrk(token, delim);     
    if (s == NULL)     
        /* This token finishes the string.  */     
        *save_ptr = strchr(token, '\0');     
    else {     
        /* Terminate the token and make *SAVE_PTR point past it.  */     
        *s = '\0';     
        *save_ptr = s + 1;     
    }     
     
    return token;     
}


