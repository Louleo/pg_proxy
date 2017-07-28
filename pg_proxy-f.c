/*************************************************************************
 *
 * pg_proxy.c - PostgreSQL proxy
 *
 * by Yanlin Liu and Yuntong Dai, May 02016
 *
 * This c file is used to test the proxy which applies the fork() method.
 *
 * This version of proxy can only run in IPV4 environment.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pg_proxy-f.h"
#include <sys/socket.h>
#include <sys/select.h>
#include <string.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <errno.h>
#include<signal.h>
#define OPT_STR "h:l:p:"
#define MAX_BUF 2480

void usage (char *prog, char *msg) {
	fprintf (stderr,
		"usage: %s [-l localport] [-h server] [-p server port]\n", prog);
}

int main (int argc, char *argv[]) {
    char c;
	int localport = PG_DEF_PORT;
	int remoteport = PG_DEF_PORT;
	char *remotehost = DEFAULT_HOST;

	while ((c = getopt (argc, argv, OPT_STR)) != EOF) {
		switch (c) {
			case 'h' : remotehost = optarg;
				break;
			case 'l' : localport = atoi (optarg);
				break;
			case 'p' : remoteport = atoi (optarg);
				break;
			default : usage (argv[0], "");
				return -1;
		}
	}

	 int sockfd; // listen sock_fd (to connect with clients)
	 int sockfds;// connect sock_fd (to connect with servers)
	 // get the ip addresses of the server and the machine which is running the proxy
	 struct sockaddr_in dest;
	 struct sockaddr_in self;
     struct hostent        *hs;
	 if ( (hs = gethostbyname(remotehost) ) == NULL ) {
			 exit(1); /* error */
	 }
	 /*---Open socket for streaming---*/
	 if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	 {
			 perror("Socket");
			 printf("%s\n","socket" );
			 exit(errno);
	 }

	bzero(&dest, sizeof(dest));
	dest.sin_family = PF_INET;
	dest.sin_port = htons(remoteport);
	bzero(&self, sizeof(self));
	self.sin_family = PF_INET;
	self.sin_port = htons(localport);
	self.sin_addr.s_addr = 0 ;
	memcpy(&dest.sin_addr, hs->h_addr_list[0], hs->h_length);
    bzero(&(dest.sin_zero),8);
    
   // bind and listen
   
   if ( bind(sockfd, (struct sockaddr*)&self, sizeof(self)) != 0 )
	 {
		 perror("socket--bind");
		 exit(errno);
	 }


	 if ( listen(sockfd, 5) != 0 )
		{
		perror("socket--listen");
		printf("%s\n","listen" );
		exit(errno);
		}

     pid_t pid;

     //

	 while (1) {
      if ( (sockfds = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
  	  {
  			perror("Socket");
  			exit(errno);
  	  }
      int iscon;
      int clientfd;

	  struct sockaddr_in client_addr;
      int addrlen=sizeof(client_addr);

      clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &addrlen);

      iscon = connect(sockfds, (struct sockaddr*)&dest, sizeof(dest));

      pid = fork();

      if (pid == -1)
            exit(errno);
      // "child" process
      if (pid == 0)
        {
            int is = 0;

            int connectstate = 0;
            char recvbuf[MAX_BUF];
            char serbuf[MAX_BUF];

            while (1) {
              int newret;
              memset(recvbuf, 0, sizeof(recvbuf));

              int ret = read(clientfd, recvbuf, sizeof(recvbuf));
              if (ret == 0)
                  {

                    close(clientfd);
                    close(sockfds);
                    break;

                  }
                  else if (ret == -1)
                  {
                      printf("read error");
                      exit(errno);
                  }
              // send information to server and get information from server then send information to client 
              write(sockfds,recvbuf,ret);
              memset(serbuf,0,sizeof(serbuf));
              newret = read(sockfds,serbuf,sizeof(serbuf));
              write(clientfd,serbuf,newret);

            }

            exit(0);
        }
      // "father" process
      else
        {
            wait(0);
            close(clientfd);
            close(sockfds);

        }
        close(clientfd);
        close(sockfds);
      }
	 printf ("%s - listening on TCP port %d and will connect to %s:%d\n",
 		argv[0], localport, remotehost, remoteport);
 	return 0;
 }
