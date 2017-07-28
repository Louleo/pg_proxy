/*************************************************************************
 *
 * pg_proxy.c - PostgreSQL proxy
 *
 * by Robert (bob) Edwards, April 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pg_proxy.h"
#include <sys/socket.h>
#include <sys/select.h>
#include <string.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <errno.h>
#include <signal.h>
#include <postgresql/libpq-fe.h>
#define OPT_STR "h:l:p:"
#define MAX_BUF 1024



void usage (char *prog, char *msg) {
	fprintf (stderr,
		"usage: %s [-l localport] [-h server] [-p server port]\n", prog);
}


/*Check if the ip is within the allowed range*/
int checkip(char *ip, char *clientip){
	int ismatch = 1;
	char checkip[INET_ADDRSTRLEN];
	char enterip[INET_ADDRSTRLEN];
	strcpy(checkip, ip);
	strcpy(enterip,clientip);
 //check the slash;

 int sizecheck = sizeof(checkip)/sizeof(checkip[0]);
 char mask[2];
 int i;
 for (i = 0;i<sizecheck;i++){
	 if(checkip[i] == '/'){
		 int c;
		 for (c = 0;c<2;c++){
			 mask[c] = checkip[i+c];
		 }
		 checkip[i] = '\0';
		 break;
	 }
 }
 int bit = 32 - atoi(mask);
 if (bit == 32) {
	 bit = 0;
 }

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

 //compare the ip
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
	 printf("%d\n",ch );
	 en = atoi(enter);
	 printf("%d\n",en );
	 if (i == 0 && ch ==0) {
		 break;
	 }
	 if(i == level){
		 if(en>>bit != ch>>bit){
			 ismatch = 0;
			 //break;
		 }
	 }else{
		 if (en != ch) {
			 ismatch = 0;
			 //break;
		 }
	 }
 }
  printf("CheckipResult  %d\n",ismatch );
	return ismatch;
}

/*Query the database and check the access*/
int CheckAccess(char *username, char *clientip)
{
	/*connect information*/
	const char *conninfo = "host=gnosia.anu.edu.au port=5432 user=networks dbname=networks password=networks";
	PGconn *conn;
	PGresult *res;
	int i;
	int checkuser = 1;

	char rawquery[100] = "select from_addr from useraccess where uid = '";
	char *rawqueryend = "';";
	char *query = strcat(strcat(rawquery,username),rawqueryend);

	/*connect database*/
	conn = PQconnectdb(conninfo);

	/*Check connection status*/
	if (PQstatus(conn) != CONNECTION_OK)
  {
      fprintf(stderr, "Connection to database failed: %s",PQerrorMessage(conn));
      PQfinish(conn);
			exit(1);
  }

	/*Query*/
	res = PQexec(conn, query);

	/*Check result status*/
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
  {
      fprintf(stderr, "RETRIEVE ip failed: %s", PQerrorMessage(conn));
      PQclear(res);
			PQfinish(conn);
			exit(1);
  }

	/*retrieve ip from result and check*/
	for (i=0;i<PQntuples(res);i++)
	{
		char *ip = PQgetvalue(res,i,0);
		if((checkuser = checkip(ip, clientip))== 1) break;
	}
	printf("checkAccessResult %d\n",checkuser );
	return checkuser;

}


int main (int argc, char *argv[]) {
  signal(SIGCHLD, SIG_IGN);
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


	 int sockfd;
	 int sockfds;
	 struct sockaddr_in self;
   struct hostent        *hs;
	 struct addrinfo hints, *res, *ai;
	 int n;
	 /*---Open socket for streaming---*/
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
  			perror("Socket");
  			exit(errno);
  	  }
      int iscon;
      int clientfd;
		  struct sockaddr_in client_addr;
		  int addrlen=sizeof(client_addr);

      clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &addrlen);

		  char clientip[INET_ADDRSTRLEN];

      inet_ntop(AF_INET, &client_addr.sin_addr, clientip, INET_ADDRSTRLEN);

			//void *clip = &client_addr.sin_addr;

			printf("%s\n", clientip );

			if((iscon = connect(sockfds, res->ai_addr, res->ai_addrlen))==-1)
			{
				perror("connect");
				exit(1);
			}

			int checkuser = 0;

      pid = fork();


      if (pid == -1)
            exit(errno);
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

								/*Retrieve username from psql request*/
								char isuser[4];
								char username[10];
								int i;
								for (i = 8; i < ret; i++)
								{
									if(recvbuf[i]==0x00){
										isuser[i-8] = '\0';
										break;
									}
									isuser[i-8] = recvbuf[i];
								}
								printf("User: %s\n", isuser );
								if(strcmp(isuser, "user")==0)
								{
									for (i = 13; i < ret; i++)
									{
										if(recvbuf[i] == 0x00){
											username[i-13] = '\0';
											break;
										}
										username[i-13] = recvbuf[i];
									}
									printf("The username%s\n", username);

						if (checkuser == 0)
						{
							checkuser = CheckAccess(username, clientip);
							printf("FinalCheckResult  %d\n", checkuser );

							if (checkuser == 0) {
							 /*if access denied terminate the child process*/
 						 	 exit(0);
 						 }
						}
					}
						/*Access allowed send request to the real server*/
            write(sockfds,recvbuf,ret);
            memset(serbuf,0,sizeof(serbuf));
						/*Receive response and forward to the client*/
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
	 printf ("%s - listening on TCP port %d and will connect to %s:%s\n",
 		argv[0], localport, remotehost, remoteport);
 	return 0;
 }
