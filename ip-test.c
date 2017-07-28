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
#include<signal.h>
#define OPT_STR "h:l:p:"
#define MAX_BUF 2480

int main(int argc, char const *argv[]) {
  int localport = PG_DEF_PORT;
	int remoteport = PG_DEF_PORT;
	char *remotehost = DEFAULT_HOST;
  struct sockaddr_in dest;
  struct hostent        *hs;
  if ( (hs = gethostbyname(remotehost) ) == NULL ) {
      exit(1); /* error */
  }
  bzero(&dest, sizeof(dest));
	dest.sin_family = PF_INET;
	dest.sin_port = htons(remoteport);
  memcpy(&dest.sin_addr, hs->h_addr_list[0], hs->h_length);
  char str[INET_ADDRSTRLEN];
  inet_ntop( AF_INET, &dest.sin_addr, str, INET_ADDRSTRLEN );
  printf("%s\n",str );

  printf("%d\n", strcmp("150.203.164.18",str));


  char recvbuf[10];
  recvbuf[0] = 0x62;
  recvbuf[1] = 0x6F;
  recvbuf[2] = 0x62;
  recvbuf[3] = '\0';
  char clientip[] = "192.168.1.1";

  int i;
  char *buf_str = (char*)malloc(2*3);
  char *buf_ptr;
  for (i = 0; i < 3; i++)
  {
    //buf_ptr += sprintf(buf_ptr,"%02X",recvbuf[i]);
    buf_ptr = buf_ptr + recvbuf[i];
  }
    char *buf = "626F63";
    printf("%d\n", strcmp("bob",recvbuf));
    //puts(buf_ptr);

  return 0;
}
