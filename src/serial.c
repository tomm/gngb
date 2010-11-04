/*  gngb, a game boy color emulator
 *  Copyright (C) 2001 Peponas Thomas & Peponas Mathieu
 * 
 *  This program is free software; you can redistribute it and/or modify  
 *  it under the terms of the GNU General Public License as published by   
 *  the Free Software Foundation; either version 2 of the License, or    
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */

#include <config.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#else
#include <Winsock2.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif

#include <fcntl.h>
#include <signal.h>
#include <SDL.h>
#include <SDL_thread.h>
#include "serial.h"
#include "interrupt.h"
#include "memory.h"
#include "emu.h"

#define TCPPORT 15781

#ifndef WIN32
#define SOCKET int
#endif

SDL_Thread *thread;
int thread_fun(void *data);

SOCKET dest_socket=-1;
SOCKET listen_socket=-1;

void gngb_closesocket(SOCKET s){
#ifdef WIN32
  closesocket(s);
#else
  close(s);
#endif
}

void gbserial_close(void) {
  if (dest_socket>0) gngb_closesocket(dest_socket);
  SDL_KillThread(thread);
}

void gbserial_signal_recv(int signum) {
  fd_set rfds;
  struct timeval tv;
  int retval;

  tv.tv_sec = 0;
  tv.tv_usec = 0;

  FD_ZERO(&rfds);
  FD_SET(dest_socket, &rfds);
  
  retval = select(dest_socket+1, &rfds, NULL, NULL,&tv);
  if (retval) {
    gbserial.ready2read=1;
    printf("Something to read\n");
    //    read(dest_socket,&gbserial.b,1);
    return;
  } 
  gbserial.ready2read=0;
  return;
}

void gbserial_init(int server_side,char *servername) {
  SOCKET s,n;
  struct sockaddr_in saddr;
  struct hostent *host;
  int len;
  int flags;

#ifdef WIN32
  int e;
  WORD ver;
  WSADATA wsaData;
  ver = MAKEWORD(2,2);
  e = WSAStartup(ver,&wsaData);
  if(e!=0) {
    fprintf(stderr,"Error while trying to initiialize socket system\n");
    exit(1);
  }
#endif

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
      gngb_closesocket(s);
      exit(1);
    }
    
    if (listen(s,1)<0) {
      fprintf(stderr,"Error while listen on port %i\n",TCPPORT);
      gngb_closesocket(s);
      exit(1);
    }

    if ((n=accept(s,(struct sockaddr *)&saddr,&len))<0) {
      gngb_closesocket(s);
      exit(1);
    }
    
    dest_socket=n;
    
  } else {  /* client side */
    
    if ((host=gethostbyname(servername))==NULL) {
      fprintf(stderr,"Error unknown host %s.\n",servername);
      gngb_closesocket(s);
      exit(1);
    }

    memset(&saddr,0,len);
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(TCPPORT);
    memcpy(&saddr.sin_addr,host->h_addr_list[0],host->h_length);

    if (connect(s,(struct sockaddr *)&saddr,len)<0) {
      gngb_closesocket(s);
      exit(1);
    } 

    dest_socket=s;
    printf("Connection established\n");
  }

  /*#ifndef WIN32
    fcntl(dest_socket,F_SETFL,O_NONBLOCK);
    #else
    ioctlsocket(dest_socket,FIONBIO,&len);
    #endif
  */
  //  thread=SDL_CreateThread(thread_fun,NULL);  

  gbserial.cycle_todo=0;
  gbserial.byte_wait=0;
  gbserial.check=0;
  gbserial.wait=0;

  fcntl(dest_socket, F_SETOWN, (int) getpid());
  flags = fcntl(dest_socket, F_GETFL);
  flags |= O_NONBLOCK|O_ASYNC;
  fcntl(dest_socket,F_SETFL,flags);

  if ((signal(SIGIO,gbserial_signal_recv))==SIG_ERR) {
    printf("Heu ya une erreur\n");
  }
  gbserial.ready2read=0;
}

void gbserial_send(Uint8 b) {
#ifndef WIN32
  Uint16 d;
  Uint8 n;
  /* We send the byte */
  write(dest_socket,&b,sizeof(Uint8));
  /* Read The return data */
  //  n=read(dest_socket,&d,sizeof(Uint16));
   


#else
  char c[2];
  int e;
  c[0] = b;
  e = send(dest_socket,c,1,0);
#endif
}

Sint8 gbserial_receive(void) {
#ifndef WIN32
  Sint8 b;
  static Uint8 c[1000];

  if ((b=read(dest_socket,c,1000))<=0) return -1;
  else SB=c;
  return 1;
#else
  char c[1000];
  int e;
  e = recv(dest_socket,c,1000,0);
  if(e>0) {
    return c[e-1];
  } else {
    return -1;
  }
#endif
}

Uint8 gbserial_check2(void) {
#ifndef WIN32
  Uint8 b;
  Uint8 c;
  //  printf("Check\n");
  SB=0xFF;
  if ((b=read(dest_socket,&c,1))<=0) return 0;
  //  printf("Check Ok %02x\n",c);
  if (c!=0xFF) set_interrupt(SERIAL_INT);
  SB=c;
  return 1;
#else
  Uint8 c;
  int e;
  SB=0xFF;
  if ((e = recv(dest_socket,&c,1,0))<=0) return 0;
  SB=c;
  return 1;
#endif
}

Uint8 gbserial_wait_data(void) {
  Uint8 b;
  Uint8 c;
  
  if ((b=read(dest_socket,&c,1))<=0) return 0;
  write(dest_socket,SB,1);
  set_interrupt(SERIAL_INT);
  SB=c;
  return 1;
}

int thread_fun(void *data) {
  Sint8 n;
  while(!conf.gb_done) {
    if (!gbserial.byte_wait && (n=gbserial_receive())>=0) {
      gbserial.b=n;
      gbserial.byte_wait=1;
      /*if (!(SC&0x01)) send_byte(SB);
	SB=(Uint8)n;
	SC&=0x7f;
	set_interrupt(SERIAL_INT);*/
    } 
  }
  return 0;
}

/* New GbSerial */

/* gbserial_check: Return 1 if data can be read */
char gbserial_check(void) {
  fd_set rfds;
  struct timeval tv;
  int retval;

  tv.tv_sec = 0;
  tv.tv_usec = 0;

  FD_ZERO(&rfds);
  FD_SET(dest_socket, &rfds);
  
  retval = select(dest_socket+1, &rfds, NULL, NULL,&tv);
  if (retval) return 1;
  return 0;
}

/* Gbserial_read: Read a byte on the serial 
   This is a block function */
Uint8 gbserial_read(void) {
  Uint8 b;

  gbserial.ready2read=0;
  if ((read(dest_socket,&b,2))<=0) return 0xFF;
  return b;
}

/* Gbserial_write: Write a byte on the serial 
   c: Command
   b: data
*/
void gbserial_write(Uint8 b) {
  write(dest_socket,&b,1);
}




