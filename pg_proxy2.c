/*************************************************************************
 *
 * pg_proxy2.c - PostgreSQL proxy
 *
 * by Yanlin Liu and Yuntong Dai, May 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pg_proxy2.h"
#include <sys/socket.h>
#include <sys/select.h>
#include <string.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <errno.h>
#include <signal.h>
#define OPT_STR "h:l:p:c:"
#define MAX_BUF 2480



void usage (char *prog, char *msg) {
	fprintf (stderr,
		"usage: %s [-l localport] [-h server] [-p server port] [-c config file]\n", prog);
}


int main (int argc, char *argv[]) {
    char c;
	int localport = atoi (PG_DEF_PORT);
	char *remoteport = PG_DEF_PORT;
	char *remotehost = DEFAULT_HOST;
	char *filename = DEFAULT_CONFIG;

	while ((c = getopt (argc, argv, OPT_STR)) != EOF) {
		switch (c) {
			case 'h' : remotehost = optarg;
				break;
			case 'l' : localport = atoi (optarg);
				break;
			case 'p' : remoteport = optarg;
				break;
      case 'c' : filename = optarg;
  			 break;
			default : usage (argv[0], "");
				return -1;
		}
	}
	 int sockfd;
	 int sockfds;
	 struct sockaddr_in self;
	 struct addrinfo hints, *res, *ai;
	 int n;
	 if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	 {
			 perror("Socket");
			 printf("%s\n","socket" );
			 exit(errno);
	 }

	 /*Config IPv6 addrinfo*/
	 bzero(&hints, sizeof(hints));
	 hints.ai_family = AF_INET6;
	 hints.ai_socktype = SOCK_STREAM;
	 if((n = getaddrinfo(remotehost,remoteport, &hints, &res)))
	 {
		 error (1, 0, "getaddrinfo: %s", gai_strerror (n));
	 }

	 bzero(&self, sizeof(self));
	 self.sin_family = PF_INET;
     self.sin_port = htons(localport);
	 self.sin_addr.s_addr = 0 ;

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

	 while (1) {
      if ( (sockfds = socket(res->ai_family,res->ai_socktype,res->ai_protocol)) < 0 )
  	  {
  			perror("socket");
  			exit(errno);
  	  }
      int iscon;
      int clientfd;
	  struct sockaddr_in client_addr;
	  int addrlen=sizeof(client_addr);

      clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &addrlen);

		  char clientip[INET_ADDRSTRLEN];
      //transfer client ip address from numbers to string
      inet_ntop(AF_INET, &client_addr.sin_addr, clientip, INET_ADDRSTRLEN);


	  if((iscon = connect(sockfds, res->ai_addr, res->ai_addrlen))==-1)
		{
			perror("connect");
			exit(1);
		}

	  int checkuser = 0;

      pid = fork();


      if (pid == -1)
            exit(errno);
 // the "child" process begins            
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

			  char username[10];
			  int i;
			  // the package which contains user's information can present user name from the 13th character
			  for (i = 13; i < ret; i++)
			  {
				 if(recvbuf[i] == 0x00){
				   username[i-13] = '\0';
				   break;
			      }
				  username[i-13] = recvbuf[i];
			  }
							
			  if (checkuser == 0) {

				  FILE *file = fopen(filename, "r");
				  char *line = malloc(200);
				  char *name;
		          char *ip ;
				  int n = 0;
				  const char s[3] = ": #";
				  if(file == NULL)
				  {
					perror("Error openning file");
					exit(1);
				  }
				  int ismatch = 1;
				  // read the config file and check the ip address is matched or not
				  while (fgets(line, 200, file) != NULL)
				  {
					name = strtok(line,s);
					ip = strtok(NULL,s);
			    	char newname[10];
					strcpy(newname, name);
					if (strcmp(username,newname) == 0) {
						ismatch = 1;
						char checkip[INET_ADDRSTRLEN];
 						char enterip[INET_ADDRSTRLEN];
                        strcpy(checkip, ip);
						strcpy(enterip,clientip);
 						//if the ip address from the the config file contains '/', there will be a number to record 
                    int sizecheck = sizeof(checkip)/sizeof(checkip[0]);
 					char mask[2];
 					int i;
 					for (i = 0;i<sizecheck;i++){
 					   if(checkip[i] == '/'){
 					     int c;
 					     for (c = 0;c<2;c++){
 							mask[c] = checkip[i+c+1];
 		     		     }
 					     checkip[i] = '\0';
 					     break;
 					   }
 					}
 					int bit = 32 - atoi(mask);
 					if (bit == 32) {
 						bit = 0;
 					}
 					// level is to decide the length of the user ip address which need to be checked
 					int level;
 					if(bit < 8){
 					    level = 3;
 					}else if(bit<16){
 						level = 2;
 						bit = bit -16;
 					}else if(bit<24){
 						level = 1;
 						bit = bit -24;
 					}
                   //check the ip address
 					int j1 = 0;
 					int j2 = 0;
 					for(i = 0; i<=level;i++){
 						int ch = 0;
 						int en = 0;
 					    char check[4];
 						char enter[4];
 						int j;
                        for (j = 0;j<4;j++){
 							if (checkip[j]=='.') {
 								j1 = j1+j+1;
 								break;
 							}
 					   check[j] = checkip[j+j1];
 					   }
                       for (j = 0;j<4;j++){
 							if (checkip[j]=='.') {
 								j2 = j2+j+1;
 								break;
 							}
 							enter[j] = enterip[j+j2];
 						}
                        ch = atoi(check);
 						en = atoi(enter);
                        if (i == 0 && ch ==0) {
							ismatch = 1;
                  	        break;
                        }
 						if(i == level){
							int enbit = en>>bit;
							int chbit = ch>>bit;
 							if(enbit != chbit){
											// printf("%d\n",enbit );
											// printf("%d\n", chbit);
 								ismatch = 0;
 								break;
 							}
 						}else{
 							if (en != ch) {
 							    ismatch = 0;
 								break;
 							}
 						}
 								}
 						if (ismatch == 1) {
                            checkuser = 1;
							break;
 						}
				      }
				    }
					free(line);
					fclose(file);
				    printf("%d\n", ismatch);
				    if (ismatch == 0) {
						 exit(0);
					}
                 }
              write(sockfds,recvbuf,ret);
              memset(serbuf,0,sizeof(serbuf));
              newret = read(sockfds,serbuf,sizeof(serbuf));
              write(clientfd,serbuf,newret);
            }

            exit(0);
        }
      else
        {
            close(clientfd);
            close(sockfds);

        }
        close(clientfd);
        close(sockfds);
      }
	 printf ("%s - listening on TCP port %d and will connect to %s:%s\n",argv[0], localport, remotehost, remoteport);
 	return 0;
 }
