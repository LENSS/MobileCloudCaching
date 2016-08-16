#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "sfs.h"
#include "fUtils.h"

char dev_name[10];
u_int32_t my_ip;
int s;

int main(int argc, char **argv)
{
    if (argc < 2) {
	printf("Error: not enough input argument\n");
	exit(1);
    }

    sprintf(dev_name, "wlan0");

    char s_my_ip[20];
    get_local_ip(s_my_ip, dev_name);  // get the ip address for wlan0...
    my_inet_aton(s_my_ip, &my_ip);	
    
    char *command1 = "create";
    char *command2 = "retrieve";
    char *command3 = "test1";
    char *command4 = "test2";
    char *command5 = "test3";

    if (strcmp(argv[1], command1) == 0) {
	if (argc < 4) {
	    printf("Error: not enough input argument\n");
	    exit(1);
	}
	createFrag(argv[2], argv[3]);
	return 0;
    }

    else if (strcmp(argv[1], command3) == 0) {
	if (strcmp(s_my_ip, "192.168.0.10") == 0) {
	    createFrag("1", "1");
	    createFrag("2", "1");
	    createFrag("3", "1");
	}
	else if (strcmp(s_my_ip, "192.168.0.20") == 0) {
	    createFrag("4", "1");
	    createFrag("5", "1");
	    createFrag("1", "2");
	}
	else if (strcmp(s_my_ip, "192.168.0.30") == 0) {
	    createFrag("2", "2");
	    createFrag("3", "2");
	    createFrag("4", "2");
	}
	else if (strcmp(s_my_ip, "192.168.0.40") == 0) {
	    createFrag("5", "2");
	    createFrag("1", "3");
	    createFrag("2", "3");
	}
	else if (strcmp(s_my_ip, "192.168.0.60") == 0) {
	    createFrag("3", "3");
	    createFrag("4", "3");
	    createFrag("5", "3");
	}
	else if (strcmp(s_my_ip, "192.168.0.70") == 0) {
	    createFrag("1", "4");
	    createFrag("2", "4");
	    createFrag("3", "4");
	}
	else if (strcmp(s_my_ip, "192.168.0.80") == 0) {
	    createFrag("4", "4");
	    createFrag("5", "4");
	    createFrag("1", "5");
	}
	else if (strcmp(s_my_ip, "192.168.0.90") == 0) {
	    createFrag("2", "5");
	    createFrag("3", "5");
	    createFrag("4", "5");
	    createFrag("5", "5");
	}

	printf("Creating Fragments Done!\n");
    }

    init_sock();

    if (strcmp(argv[1], command2) == 0) {
	if (argc < 4) {
	    printf("Error: not enough input argument\n");
	    exit(1);
	}

	u_int32_t fileId = (u_int32_t)atoi(argv[2]);
	u_int32_t seq = (u_int32_t)atoi(argv[3]);
	
	// broadcast the file request...
	gen_flreq(my_ip, fileId, seq);
    }

    else if (strcmp(argv[1], command4) == 0) {
	int total = 120;
	int i;
	int fileList[11] = {1, 3, 1, 2, 5, 4, 1, 2, 3, 1, 2};
	for (i = 0; i < total; i++) {
	    gen_flreq(my_ip, fileList[i % 11], i);
	    sleep(30);
	}
    }

    else if (strcmp(argv[1], command5) == 0) {
	int total = 60;
	int i;
	int fileList[11] = {1, 3, 1, 2, 5, 4, 1, 2, 3, 1, 2};
	for (i = 0; i < total; i++) {
	    gen_flreq(my_ip, fileList[i % 11], i);
	    sleep(30);
	}
    }

    close(s);
    return 0;
}



