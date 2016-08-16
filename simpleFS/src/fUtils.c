
#include "fUtils.h"

static struct sockaddr_in sin;
extern int s;
extern char dev_name[10];

int atoi(const char *nptr)
{
    int c;              /* current char */
    int total;         /* current total */
    int sign;           /* if '-', then negative, otherwise positive */

    /* skip whitespace */
    while (isspace((int)(unsigned char)*nptr))
        ++nptr;

    c = (int)(unsigned char)*nptr++;
    sign = c;           /* save sign indication */
    if (c == '-' || c == '+')
        c = (int)(unsigned char)*nptr++;    /* skip sign */

    total = 0;

    while (isdigit(c)) {
        total = 10 * total + (c - '0');     /* accumulate digit */
        c = (int)(unsigned char)*nptr++;    /* get next char */
    }

    if (sign == '-')
        return -total;
    else
        return total;   /* return result, negated if necessary */
}

int isspace(int x)
{
    if (x == ' ' || x == '\t' || x == '\n' || x == '\f' || x == '\b' || x == '\r')
	return 1;
    else  
	return 0;
}

int isdigit(int x)
{
    if (x <= '9' && x >= '0')         
	return 1;
    else 
	return 0;
}

int get_local_ip(char *ip, char *dev_name)
{
    struct ifaddrs *ifAddrStruct;
    void *tmpAddrPtr = NULL;

    getifaddrs(&ifAddrStruct);
    while (ifAddrStruct != NULL) {
	if (ifAddrStruct -> ifa_addr -> sa_family == AF_INET) {
	    tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct -> ifa_addr) -> sin_addr;
	    inet_ntop(AF_INET, tmpAddrPtr, ip, INET_ADDRSTRLEN);

	    if (ifAddrStruct -> ifa_name == dev_name) {
		printf("%s IP Address:%s\n", ifAddrStruct -> ifa_name, ip);
		break;
	    }	    
	}

	ifAddrStruct = ifAddrStruct -> ifa_next;
    }

    //free ifaddrs
    freeifaddrs(ifAddrStruct);

    return 0;
}

int my_inet_aton(const char *cp, u_int32_t * addr)
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

int createFrag(char *file_id, char *frag_id)
{
    char dir[10];
    // if /myfs does not exist, create the directory...
    sprintf(dir, "/myfs");
    if(access(dir, 0) == -1) {
        if (mkdir(dir, 0777) == -1) {
	    printf("Error: cannot create the /myfs directory\n");
	    exit(1);
	}
	FILE *f;
	f = fopen("/myfs/a", "w+");
    }

    // prepare the file-frag name...
    char fileId[10];
    char fragId[10];
    char name[20];
    sprintf(fileId, file_id);
    sprintf(fragId, frag_id);
    sprintf(name, "/myfs/");
    strcat(name, fileId);
    strcat(name, "-");
    strcat(name, fragId);
    printf("created file name: %s\n", name);

    FILE *fp;
    fp = fopen(name, "w+");
    if(fp == NULL) {
	printf("Error: cannot create fragment\n");
	exit(1);
    }
    fclose(fp);
    return 0;
}

int init_sock()
{
    // create a socket and set its dst address...
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
	printf("Error creating socket\n");
	exit(1);
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    sin.sin_port = htons(CACHEPORT);

    // enable broadcast...
    int broadcastEnable = 1;
    int ret = setsockopt(s, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
    if (ret < 0) {
	printf("Broadcast setting failed\n");
	exit(1);
    }

    if (setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, dev_name, sizeof(dev_name)) != 0) {
	printf("Cannot bind to device %s\n", dev_name);
	exit(1);
    }

    return 0;
}

int broadcast(char *data, int datalen)
{
    if (sendto(s, data, datalen, 0, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
	perror("sendto");
	exit(1);
    }

    return 0;
}

int gen_flreq(u_int32_t src_ip, u_int32_t file_id, u_int32_t seq)
{
    // prepare the file request...
    fileReq *new_flreq;
    new_flreq = (fileReq *)malloc(sizeof(fileReq));
    if (new_flreq == NULL) {
	printf("Error getting memory for new file request\n");
	exit(1);
    }

    u_int32_t broadcast_ip;	
    char s_broadcast_ip[20];
    sprintf(s_broadcast_ip, "255.255.255.255");
    my_inet_aton(s_broadcast_ip, &broadcast_ip);

    new_flreq -> type = FILE_REQ_MESSAGE;
    new_flreq -> src_ip = src_ip;
    new_flreq -> dst_ip = broadcast_ip;
    new_flreq -> file_id = htonl(file_id);
    new_flreq -> seq = htonl(seq);
    new_flreq -> metric = htonl(0);

    char *message = (char *)new_flreq;
    int len = sizeof(fileReq);

    broadcast(message, len);

    return 0;
}


