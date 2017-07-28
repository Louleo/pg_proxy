/*************************************************************************
 *
 * pg_proxy.c - PostgreSQL proxy
 *
 * by Robert (bob) Edwards, April 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <string.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <string.h>

#include "pg_proxy.h"

#define OPT_STR "h:l:p:"
#define MAX_BUF 1024

void usage (char *prog, char *msg) {
	fprintf (stderr,
		"usage: %s [-l localport] [-h server] [-p server port] [-c config file]\n", prog);
}

/*Connect to the real server*/
int connectServer(int sock_ser_fd,char *remotehost, char *remoteport){
	struct addrinfo hints, *res, *ai;
	int n;

	bzero (&hints, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;

	if((n = getaddrinfo(remotehost,remoteport, &hints, &res))){
		error (1, 0, "getaddrinfo: %s", gai_strerror (n));
	}

	if((sock_ser_fd = socket(res->ai_family,res->ai_socktype,res->ai_protocol)) == -1)
	{
		perror("socket");
		exit(1);
	}

	if(connect(sock_ser_fd, res->ai_addr, res->ai_addrlen) == -1)
  {
		perror("connect");
		exit(1);
	}

	return sock_ser_fd;

}


int main (int argc, char *argv[]) {
	char c;
	int localport = atoi (PG_DEF_PORT);
	char *remoteport = PG_DEF_PORT;
	char *remotehost = DEFAULT_HOST;

	while ((c = getopt (argc, argv, OPT_STR)) != EOF) {
		switch (c) {
			case 'h' : remotehost = optarg;
				break;
			case 'l' : localport = atoi (optarg);
				break;
			case 'p' : remoteport = optarg;
				break;
			default : usage (argv[0], "");
				return -1;
		}
	}

		int sock_cli_fd, sock_ser_fd;
    struct sockaddr_in addr;
    fd_set mset;
    int max_fd;
		int isConn;// check connection: 0:disconnected / 1:connected

    /*Make a connection*/
		sock_ser_fd = connectServer(sock_ser_fd,remotehost,remoteport);
		isConn = 1;

    sock_cli_fd = socket (AF_INET, SOCK_STREAM, 0);
    if (sock_cli_fd < 0) {
      	perror ("socket");
      	exit (EXIT_FAILURE);
    }

    memset (&addr, 0 , sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons (localport);
    addr.sin_addr.s_addr = 0;

    if (bind (sock_cli_fd, (struct sockaddr *) &addr, sizeof (addr))<0) {
      	perror("bind");
      	exit(EXIT_FAILURE);
    }

    if(listen(sock_cli_fd, 5) < 0) {
      	perror("listen");
      	exit(EXIT_FAILURE);
    }

    FD_ZERO (&mset);
    FD_SET(sock_cli_fd, &mset);
    max_fd = sock_cli_fd;

		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;

    while(1){

        fd_set rset;
        int retval;
        int fd;
      	memcpy(&rset, &mset, sizeof(fd_set));

        /*Select to manipulate multiple clients*/
      	retval = select (max_fd + 1, &rset, NULL, NULL, &tv);
      	if (retval <= 0) continue;
      	for (fd = 0; fd <= max_fd; fd++){
         	if(FD_ISSET (fd,&rset)) {
           		if(fd == sock_cli_fd)
           		{
           			printf("listen fd:%d\n", fd);
             		int nsock = accept (sock_cli_fd, NULL, NULL);
             		FD_SET (nsock, &mset);
             		if(nsock > max_fd) max_fd = nsock;
             		printf("new connection on %d\n", nsock);
          		}
          		else
          		{
           			char buf[MAX_BUF + 1];
           			char rebuf[MAX_BUF + 1];
           			int nread;
           			int nreadbk;

								if(isConn == 0)
								{
									sock_ser_fd = connectServer(sock_ser_fd,remotehost,remoteport);
									isConn = 1;
								}

           			nread = read(fd, buf, MAX_BUF);
								printf("get info %s / %d from connection %d \n", buf , nread, fd);

								/*If client terminate the connection, terminate the connection to server as well*/
           			if(nread == 0)
           			{
           				close (fd);
           				FD_CLR(fd,&mset);
           				printf("Disconnected to client on %d\n", fd);
									close (sock_ser_fd);
									printf("Disconnected to server on %d\n", fd);
									isConn = 0;
           			}
           			else
           			{// forward the data between client and server
           				write(sock_ser_fd, buf,nread);
           				nreadbk = read(sock_ser_fd,rebuf,MAX_BUF);
           				printf("get info %s from PostgreSQL %d \n", rebuf, nreadbk);
           				write(fd, rebuf, nreadbk);
           			}
           		}
         	}
      	}
    }

	//end

	printf ("%s - listening on TCP port %d and will connect to %s:%s\n",
		argv[0], localport, remotehost, remoteport);
	return 1;
}
