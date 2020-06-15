#include <asm/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

int open_netlink() {
    int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    struct sockaddr_nl addr;

    memset((void *)&addr, 0, sizeof(addr));

    if (sock < 0) {
        printf ("Socket Open Error!");
        exit(1);
    }

    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;

    if (bind(sock,(struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf ("Socket bind failed!");
        exit(1);
    }

    return sock;
}

int read_event(int sockint, int (*msg_handler)(struct sockaddr_nl *, struct nlmsghdr *))
{
    int status;
    int ret = 0;
    char buf[4096];
    struct iovec iov = { buf, sizeof buf };
    struct sockaddr_nl snl;
    struct msghdr buf_msg = { (void*)&snl, sizeof snl, &iov, 1, NULL, 0, 0 };
    struct nlmsghdr *msg;

    status = recvmsg(sockint, &buf_msg, 0);
    if(status < 0) {
        /* Socket non-blocking so bail out once we have read everything */
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return ret;
        }

        /* Anything else is an error */
        printf("read_netlink: Error recvmsg: %d\n", status);
        perror("read_netlink: Error: ");
        return status;
    }

    if(status == 0) {
        printf("read_netlink: EOF\n");
    }

    /* We need to handle more than one message per 'recvmsg' */
    for(msg = (struct nlmsghdr *) buf; NLMSG_OK(msg, (unsigned int)status); msg = NLMSG_NEXT(msg, status)) {
        /* Finish reading */
        if (msg->nlmsg_type == NLMSG_DONE) {
            return ret;
        }

        /* Message is some kind of error */
        if (msg->nlmsg_type == NLMSG_ERROR) {
            printf("read_netlink: Message is an error - decode TBD\n");
            return -1; // Error
        }

        /* Call message handler */
        if(msg_handler) {
            ret = (*msg_handler)(&snl, msg);
            if(ret < 0) {
                printf("read_netlink: Message hander error %d\n", ret);
                return ret;
            }
        }
        else {
            printf("read_netlink: Error NULL message handler\n");
            return -1;
        }
    }
    return ret;
}

static int netlink_link_state(struct sockaddr_nl *nl, struct nlmsghdr *msg)
{
    struct ifinfomsg *ifi;
    char ifname[1024];

    ifi = NLMSG_DATA(msg);
    if_indextoname(ifi->ifi_index, ifname);

    printf("netlink_link_state: Link %s %s\n", ifname,((ifi->ifi_flags & IFF_UP)  && (ifi->ifi_flags & IFF_RUNNING))?"Up":"Down");
    return 0;
}

static int msg_handler(struct sockaddr_nl *nl, struct nlmsghdr *msg) {
    switch (msg->nlmsg_type) {
        case RTM_NEWADDR:
            printf("msg_handler: RTM_NEWADDR\n");
            break;
        case RTM_DELADDR:
            printf("msg_handler: RTM_DELADDR\n");
            break;
        case RTM_NEWROUTE:
            printf("msg_handler: RTM_NEWROUTE\n");
            break;
        case RTM_DELROUTE:
            printf("msg_handler: RTM_DELROUTE\n");
            break;
        case RTM_NEWLINK:
            printf("msg_handler: RTM_NEWLINK\n");
            netlink_link_state(nl, msg);
            break;
        case RTM_DELLINK:
            printf("msg_handler: RTM_DELLINK\n");
            break;
        default:
            printf("msg_handler: Unknown netlink nlmsg_type %d\n",
                   msg->nlmsg_type);
            break;
    }
    return 0;
}


int main(int argc, char *argv[]) {
    int nls = open_netlink();
    printf("Started watching:\n");
    if (nls<0) {
        printf("Open Error!");
    }

    while (1) {
        read_event(nls, msg_handler);
    }
    return 0;
}
