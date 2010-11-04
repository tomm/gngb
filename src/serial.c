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

#ifndef GNGB_WIN32
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
#include <SDL.h>
#include <SDL_thread.h>
#include "serial.h"
#include "interrupt.h"
#include "memory.h"
#include "emu.h"

#define TCPPORT 15781

#ifndef GNGB_WIN32
#define SOCKET int
#endif

SDL_Thread *thread;
int thread_fun(void *data);

SOCKET dest_socket=-1;
SOCKET listen_socket=-1;

void gngb_closesocket(SOCKET s){
#ifdef GNGB_WIN32
  closesocket(s);
#else
  close(s);
#endif
}

void gbserial_close(void) {
  if (dest_socket>0) gngb_closesocket(dest_socket);
  SDL_KillThread(thread);
}

void gbserial_init(int server_side,char *servername) {
  SOCKET s,n;
  struct sockaddr_in saddr;
  struct hostent *host;
  int len;

#ifdef GNGB_WIN32
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

  /*#ifndef GNGB_WIN32
  fcntl(dest_socket,F_SETFL,O_NONBLOCK);
#else
  ioctlsocket(dest_socket,FIONBIO,&len);
  #endif*/
  thread=SDL_CreateThread(thread_fun,NULL);  

  gbserial.cycle_todo=0;
  gbserial.byte_wait=0;
}

void send_byte(Uint8 b) {
#ifndef GNGB_WIN32
  write(dest_socket,&b,sizeof(Uint8));
#else
  char c[2];
  int e;
  c[0] = b;
  e = send(dest_socket,c,1,0);
#endif
}

Sint16 recept_byte(void) {
#ifndef GNGB_WIN32
  Sint8 b;
  static Uint8 c[1000];

  if ((b=read(dest_socket,c,1000))<=0) return -1;
  
  //b=c;
  /*while(read(dest_socket,&c,sizeof(Uint8))>0) b=c;    */
  
  return c[b-1];

  return b;
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

int thread_fun(void *data) {
  Sint8 n;
 
  while(!conf.gb_done) {
    if (!gbserial.byte_wait && (n=recept_byte())>=0) {
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






