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
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <errno.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif


#include <stdlib.h>
#include <string.h>
#include "fileio.h"

#ifdef HAVE_LIBZ 
#include "unzip.h"
#include <zlib.h>
#endif

#ifdef HAVE_LIBBZ2
#include <bzlib.h>
#endif


char get_type_file(char *filename) {
  FILE *file_tmp;
  char str[10];
  int i;
  struct {
    char *sign;
    int len;
    char type;
  } tab_sign[]={
    {"PK\003\004",4,ZIP_ARCH_FILE_TYPE},
    {"\037\213",2,GZIP_FILE_TYPE},
    {"BZh",3,BZIP_FILE_TYPE},
    {NULL,0,UNKNOW_FILE_TYPE}};

  if (!(file_tmp=fopen(filename,"rb"))) return UNKNOW_FILE_TYPE;
  fread(str,1,10,file_tmp);
  fclose(file_tmp);

  for(i=0;tab_sign[i].sign;i++) {
    if (!memcmp(str,tab_sign[i].sign,tab_sign[i].len)) return tab_sign[i].type;
  }

  return NORMAL_FILE_TYPE;
}

GNGB_FILE *gngb_file_open(char *filename,char *mode,char type) {
  GNGB_FILE *f=(GNGB_FILE *)malloc(sizeof(GNGB_FILE));
  if (!f) return NULL;

  if (type!=UNKNOW_FILE_TYPE) f->type=type;
  else f->type=get_type_file(filename);

  switch(f->type) {
  case GZIP_FILE_TYPE:
#ifdef HAVE_LIBZ
    f->stream=(char *)gzopen(filename,mode);
#else
    fprintf(stderr,"Gzip file unsupported\n");
    return NULL;
#endif
    break;

  case ZIP_ARCH_FILE_TYPE:
#ifdef HAVE_LIBZ
    f->stream=(char *)unzOpen(filename);
#else
    fprintf(stderr,"Zip file unsupported\n");
    return NULL;
#endif
    break;

  case BZIP_FILE_TYPE:
#ifdef HAVE_LIBBZ2
    f->stream=(char *)BZ2_bzopen(filename,mode);
    break;
#else
    fprintf(stderr,"Bzip2 file unsupported\n");
    return NULL;   
#endif
    break;

  case NORMAL_FILE_TYPE:
  case GB_ROM_FILE_TYPE:
    f->stream=(char *)fopen(filename,mode);
    break;
  default:
    return NULL;
  }
  
  if (!f->stream) {
    free(f);
    return NULL;
  }
  return f;
}

int gngb_file_close(GNGB_FILE *f) {

  switch(f->type) {
  case NORMAL_FILE_TYPE:
  case GB_ROM_FILE_TYPE:return fclose((FILE *)f->stream);

  case GZIP_FILE_TYPE:
#ifdef HAVE_LIBZ
    return gzclose((gzFile)f->stream);
#else
    fprintf(stderr,"Gzip file unsupported\n");
    return -1; 
#endif

  case ZIP_ARCH_FILE_TYPE:
#ifdef HAVE_LIBZ
    unzCloseCurrentFile((unzFile)f->stream);
    return unzClose((unzFile)f->stream);
#else
    fprintf(stderr,"Zip file unsupported\n");
    return -1; 
#endif

  case BZIP_FILE_TYPE:
#ifdef HAVE_LIBBZ2
    BZ2_bzclose((BZFILE *)f->stream);
    return 0;
#else
    fprintf(stderr,"Bzip2 file unsupported\n");
    return -1;   
#endif
    break;
  default:return -1;
  }

}

int gngb_file_read(void *ptr, size_t size, size_t nmemb,GNGB_FILE *f) {
  switch(f->type) {
  case NORMAL_FILE_TYPE:
  case GB_ROM_FILE_TYPE:return fread(ptr,size,nmemb,(FILE *)f->stream);

  case GZIP_FILE_TYPE:
#ifdef HAVE_LIBZ  
    return gzread((gzFile)f->stream,ptr,size*nmemb);
#else
    fprintf(stderr,"Gzip file unsupported\n");
    return -1;  
#endif

  case ZIP_ARCH_FILE_TYPE:
#ifdef HAVE_LIBZ 
    return unzReadCurrentFile((unzFile)f->stream,ptr,size*nmemb);
#else
    fprintf(stderr,"Zip file unsupported\n");
    return -1;  
#endif

  case BZIP_FILE_TYPE:
#ifdef HAVE_LIBBZ2
    return BZ2_bzread((BZFILE *)f->stream,ptr,size*nmemb);
#else
    fprintf(stderr,"Bzip2 file unsupported\n");
    return -1;   
#endif
    break;
    
  default:return -1;
  }  
}

int gngb_file_write(const void *ptr, size_t size, size_t nmemb,GNGB_FILE *f) {
  switch(f->type) {
  case NORMAL_FILE_TYPE:
  case GB_ROM_FILE_TYPE:
    return fwrite(ptr,size,nmemb,(FILE *)(f->stream));

  case GZIP_FILE_TYPE:    
#ifdef HAVE_LIBZ
    return gzwrite((gzFile)f->stream,ptr,size*nmemb);
#else
    fprintf(stderr,"Gzip file unsupported\n");
    return -1;  
#endif

  case ZIP_ARCH_FILE_TYPE:
#ifdef HAVE_LIBZ  
    fprintf(stderr,"Write for Unzip file unsupported\n");
    return -1;  
#else
    fprintf(stderr,"Zip file unsupported\n");
    return -1;  
#endif

  case BZIP_FILE_TYPE:
#ifdef HAVE_LIBBZ2
    return BZ2_bzwrite((BZFILE *)f->stream,ptr,size*nmemb);
#else
    fprintf(stderr,"Bzip2 file unsupported\n");
    return -1;   
#endif

  default:return -1;
  }  
}

int gngb_file_seek(GNGB_FILE *f, long offset, int whence) {
  switch(f->type) {
  case NORMAL_FILE_TYPE:
  case GB_ROM_FILE_TYPE:return fseek((FILE *)f->stream,offset,whence);

  case BZIP_FILE_TYPE:
#ifdef HAVE_LIBBZ2
    fprintf(stderr,"fseek for Bzip file: unsuported\n");
    return -1;
#else
    fprintf(stderr,"Bzip2 file unsupported\n");
    return -1;   
#endif


  case GZIP_FILE_TYPE:
#ifdef HAVE_LIBZ
    fprintf(stderr,"fseek for Gzip file: unsuported\n");
    return -1;
#else
    fprintf(stderr,"Gzip file unsupported\n");
    return -1;   
#endif 
    
  case ZIP_ARCH_FILE_TYPE:
#ifdef HAVE_LIBZ
    fprintf(stderr,"fseek for Zip file unsupported\n");
    return -1;
#else
    fprintf(stderr,"Zip file unsupported\n");
    return -1;   
#endif 
  
  default:return -1;
  }  
}

int gngb_file_eof(GNGB_FILE *f) {
  switch(f->type) {
  case NORMAL_FILE_TYPE:
  case GB_ROM_FILE_TYPE:return feof((FILE *)f->stream);

  case GZIP_FILE_TYPE:
#ifdef HAVE_LIBZ
    fprintf(stderr,"feof for Gzip file: unsuported\n");
    return -1;
#else
    fprintf(stderr,"Gzip file unsupported\n");
    return -1;   
#endif

  case ZIP_ARCH_FILE_TYPE:
#ifdef HAVE_LIBZ
    fprintf(stderr,"feof for Unzip file unsupported\n");
    return -1;  
#else
    fprintf(stderr,"Zip file unsupported\n");
    return -1;   
#endif
    
  case BZIP_FILE_TYPE:
#ifdef HAVE_LIBBZ2
    fprintf(stderr,"feof for Bzip file: unsuported\n");
    return -1;
#else
    fprintf(stderr,"Bzip2 file unsupported\n");
    return -1;   
#endif

    
  default:return -1;
  }    
}

#ifdef HAVE_LIBZ

int zip_file_open_next_rom(unzFile file) {
  char header[0x200];

  unzGoToFirstFile(file);
  do {
    unzOpenCurrentFile(file);
    unzReadCurrentFile(file,header,0x200);
    unzCloseCurrentFile(file);
#ifdef WORD_BIGENDIAN
    if ((*((unsigned long *)(header+0x104)))==0xCEED6666) {
#else
    if ((*((unsigned long *)(header+0x104)))==0x6666EDCE) {
#endif
      /* Gameboy Roms */
      unzOpenCurrentFile(file);
      return 0;
    }
  }while(unzGoToNextFile(file)!=UNZ_END_OF_LIST_OF_FILE);
  printf("titi\n");
  return -1;
}

#endif

