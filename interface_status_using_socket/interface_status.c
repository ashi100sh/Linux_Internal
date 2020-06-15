#include <stdio.h>        // printf
#include <string.h>       // strncpy
#include <sys/socket.h>   // AF_INET
#include <sys/ioctl.h>    // SIOCGIFFLAGS
#include <errno.h>        // errno
#include <netinet/in.h>   // IPPROTO_IP
#include <net/if.h>       // IFF_*, ifreq
#include <unistd.h>       // close
#include <stdlib.h>       // exit

#define ERROR(fmt, ...) do { printf(fmt, __VA_ARGS__); } while(0)

int checkInterfaceStatus(char *ifname) {
    int socId = -1;
    int rv = -1;
    struct ifreq if_req;
    
    socId = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if(socId < 0)
        ERROR("Socket failed. Errno = %d\n", errno);

    (void)strncpy(if_req.ifr_name, ifname, sizeof(if_req.ifr_name));
    rv = ioctl(socId, SIOCGIFFLAGS, &if_req);
    close(socId);

    if(rv == -1) {
        ERROR("Ioctl failed. Errno = %d\n", errno);
        ERROR("Interface: %s is not available\n", ifname);
        exit(1);
    }

    return (if_req.ifr_flags & IFF_UP) && (if_req.ifr_flags & IFF_RUNNING);
}


int main(int argc, char *argv[]) {
    int status=0;
    if(argc==1) {
        printf("Usage: %s <interface name>\n", argv[0]);
        exit(1);
    }
    
    status=checkInterfaceStatus(argv[1]);
    printf("Status of Interface:%s ==> %d (%s)\n", argv[1], status, status?"UP":"DOWN");
    return 0;
}
