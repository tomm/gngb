#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "serial.h"

#define TCPPORT 9200

void gbserial_init(int server_side) {
  int s;
  struct sockaddr_in saddr;
  int n;
  
  if ((s=socket(AF_INET,SOCK_STREAM,0))<0) {
    fprintf(stderr,"Error while trying to initilialize socket\n");
    exit(1);
  }

  if (server_side) {
    memset(&saddr,0,sizeof(struct sockaddr_in));
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(TCPPORT);
    /*n=INADDR_ANY;
      memcpy(&saddr.sin_addr,&n,sizeof(n));*/
    saddr.sin_addr.s_addr=INADDR_ANY;

    if (bind(sock_d,(struct sockaddr *)&sock_name,sock_len)<0) {
      fprintf(stderr,"Error while trying to bind\n");
      close(s);
      exit(1);
    }
    

    
  } else {  /* client side */

  }
}

void send_byte(UINT8 b) {

}

UINT8 recept_byte(void) {

}
