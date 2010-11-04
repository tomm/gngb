# gngb, a game boy color emulator
# Copyright (C) 2001 Peponas Thomas & Peponas Mathieu
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

CC = gcc
INCDIRS = -I. -I/usr/include 
LIBDIRS =  -L/usr/X11R6/lib 
DEBUG = -g 
PROFILE = -pg -O3
OPT = -O3  -mpentiumpro  -Wno-unused -funroll-loops -fstrength-reduce -ffast-math -malign-functions=2   -malign-jumps=2 -malign-loops=2 -fomit-frame-pointer -Wall -g

DOBJ = memory_dbg.o vram_dbg.o interrupt_dbg.o cpu_dbg.o rom_dbg.o sound_dbg.o serial_dbg.o frame_skip_dbg.o gngb_debuger.o log.o op.o cpu_info.o mem_info.o code_info.o io_info.o

DGTKOBJ = memory_dbg.o vram_dbg.o interrupt_dbg.o cpu_dbg.o rom_dbg.o sound_dbg.o serial_dbg.o frame_skip_dbg.o gtk_debuger.o log.o op.o gtk_cpu_info.o gtk_mem_info.o gtk_code_info.o

CFLAGS = $(OPT) -D_REENTRANT -DSDL_GL
LIBS = -lSDL -lpthread -lGL

all : gngb_debuger 

# GNGB

%_dbg.o : ../%.c
	$(CC) -c $(CFLAGS) -DDEBUG $(INCDIRS)  $< -o $@

%.o : %.c 
	$(CC) -c $(CFLAGS) -DDEBUG $(INCDIRS)  $< -o $@

gtk_%.o : %.c 
	$(CC) -c $(CFLAGS) -DDEBUG -DGTK_DEBUGER $(INCDIRS)  $< -o $@

../cpu.c : ../cpu.h ../memory.h ../rom.h 
../memory.c : ../memory.h ../rom.h ../cpu.h ../vram.h ../interrupt.h ../sound.h 
../interrupt.c : ../interrupt.h ../memory.h ../cpu.h ../vram.h 
../rom.c : ../rom.h ../memory.h ../cpu.h 
../vram.c : ../vram.h ../memory.h ../rom.h 
../sound.c : ../sound.h ../memory.h ../cpu.h 
../serial.c : ../serial.h
../frame_skip.c : ../frame_skip.h  

gngb_debuger.c : ../rom.h ../memory.h ../cpu.h ../vram.h ../interrupt.h 
log.c : log.h
op.c : op.h ../memory.h
cpu_info.c : cpu_info.h
mem_info.c : mem_info.h
code_info.c : code_info.h

# PROGRAMME

gngb_debuger: $(DOBJ)
	gcc $(CFLAGS) $(DOBJ) $(LIBS) -lreadline -lncurses -o gngb_debuger

clean : 
	rm -f *.o *~ 


