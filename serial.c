#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <fcntl.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include "serial.h"
#include "interrupt.h"
#include "memory.h"
#include "emu.h"

#define TCPPORT 15781

int f(void *data);

int dest_socket=-1;
int listen_socket=-1;

void gbserial_close(void) {
  if (dest_socket>0) close(dest_socket);
  if (listen_socket>0) close(listen_socket);
}

void gbserial_init(int server_side,char *servername) {
  int s;
  struct sockaddr_in saddr;
  struct hostent *host;
  int n,len;

  serial_cycle_todo=0;
  gblisten=0;

  if ((s=socket(AF_INET,SOCK_STREAM,0))<0) {
    fprintf(stderr,"Error while trying to initilialize socket\n");
    exit(1);
  }

  len=sizeof(struct sockaddr_in);

  if (server_side) {
    memset(&saddr,0,len);
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(TCPPORT);
    /*n=INADDR_ANY;
      memcpy(&saddr.sin_addr,&n,sizeof(n));*/
    saddr.sin_addr.s_addr=INADDR_ANY;

    if (bind(s,(struct sockaddr *)&saddr,len)<0) {
      fprintf(stderr,"Error while trying to bind\n");
      close(s);
      exit(1);
    }
    
    if (listen(s,1)<0) {
      fprintf(stderr,"Error while listen on port %i\n",TCPPORT);
      close(s);
      exit(1);
    }

    if ((n=accept(s,(struct sockaddr *)&saddr,&len))<0) {
      close(s);
      exit(1);
    }
    
    dest_socket=n;
    listen_socket=s;

  } else {  /* client side */
    
    if ((host=gethostbyname(servername))==NULL) {
      fprintf(stderr,"Error unknown host %s.\n",servername);
      close(s);
      exit(1);
    }

    memset(&saddr,0,len);
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(TCPPORT);
    memcpy(&saddr.sin_addr,host->h_addr_list[0],host->h_length);

    if (connect(s,(struct sockaddr *)&saddr,len)<0) {
      close(s);
      exit(1);
    } 

    dest_socket=s;
    printf("Connection established\n");
  }

  fcntl(dest_socket,F_SETFL,O_NONBLOCK);

  //  SDL_CreateThread(f,NULL);
  
}

void send_byte(UINT8 b) {
  write(dest_socket,&b,sizeof(UINT8));
}

INT16 recept_byte(void) {
  INT8 b;
  static UINT8 c[1000];

  if ((b=read(dest_socket,c,1000))<=0) return -1;
  
  //b=c;
  /*while(read(dest_socket,&c,sizeof(UINT8))>0) b=c;    */
  
  return c[b-1];

  return b;
}

int f(void *data) {
  int n;
  printf("toto\n");
  while(!conf.gb_done) {
    if ((n=recept_byte())>=0) {
      if (!(SC&0x01)) send_byte(SB);
      SB=(UINT8)n;
      SC&=0x7f;
      set_interrupt(SERIAL_INT);
    }
  }
  return 0;
}






