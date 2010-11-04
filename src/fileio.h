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

#ifndef FILEIO_H
#define FILEIO_H

#include <sys/stat.h>
#include <sys/types.h>

#define UNKNOW_FILE_TYPE -1
#define ZIP_ARCH_FILE_TYPE 1
#define GZIP_FILE_TYPE 2
#define BZIP_FILE_TYPE 3
#define GB_ROM_FILE_TYPE 4
#define NORMAL_FILE_TYPE 5

typedef struct _gngb_file {
  char type;
  char *stream;
}GNGB_FILE;

GNGB_FILE *gngb_file_open(char *filename,char *mode,char type);
int gngb_file_close(GNGB_FILE *f);
int gngb_file_read(void *ptr, size_t size, size_t nmemb,GNGB_FILE *f);
int gngb_file_write(const void *ptr, size_t size, size_t nmemb,GNGB_FILE *f);
int gngb_file_seek(GNGB_FILE *f, long offset, int whence);
int gngb_file_eof(GNGB_FILE *f);

#endif

