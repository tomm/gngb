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

GL_MODE = 1

CC = gcc
INCDIRS = -I. -I/usr/include 
LIBDIRS =  -L/usr/X11R6/lib 
DEBUG = -g 
PROFILE = -pg -O3
OPT = -O3  -mpentiumpro  -Wno-unused -funroll-loops -fstrength-reduce -ffast-math -malign-functions=2   -malign-jumps=2 -malign-loops=2 -fomit-frame-pointer -Wall -g
OBJ = memory.o vram.o interrupt.o  cpu.o rom.o sound.o serial.o frame_skip.o main.o message.o emu.o sgb.o
DOBJ = debuger/memory.o debuger/vram.o debuger/interrupt.o debuger/cpu.o debuger/rom.o debuger/sound.o debuger/serial.o debuger/frame_skip.o debuger/log.o debuger/debuger.o

ifdef GL_MODE
CFLAGS = $(OPT) `sdl-config --cflags`  -DSDL_GL
LIBS = `sdl-config --libs` -lpthread -lGL
else
CFLAGS = $(OPT) `sdl-config --cflags`
LIBS = `sdl-config --libs` -lpthread 
endif

all : gngb

# GNGB

%.o : %.c
	$(CC) -c $(CFLAGS) $(INCDIRS)  $< -o $@

#TODO clean depend
cpu.c : cpu.h memory.h rom.h interrupt.h serial.h frame_skip.h global.h
memory.c : memory.h rom.h cpu.h vram.h interrupt.h sound.h serial.h frame_skip.h global.h
interrupt.c :interrupt.h memory.h cpu.h vram.h global.h 
rom.c : rom.h memory.h cpu.h frame_skip.h vram.h global.h
vram.c : vram.h memory.h rom.h interrupt.h global.h
sound.c : sound.h memory.h cpu.h interrupt.h global.h
serial.c : serial.h memory.h global.h
frame_skip.c : frame_skip.h  
message.c : message.h
emu.c : emu.h
main.c : rom.h memory.h cpu.h vram.h interrupt.h sound.h frame_skip.h global.h serial.h
sgb.c : sgb.h memory.h

# PROGRAMME

gngb : $(OBJ) 
	gcc $(CFLAGS) $(OBJ) $(LIBS) -o gngb

clean : 
	rm -f *.o *~


