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


# configuration
# JOYSTICK = 1


DEBUG = -g 
OPT = -O3  -mpentiumpro  -Wno-unused -funroll-loops -fstrength-reduce -ffast-math -malign-functions=2   -malign-jumps=2 -malign-loops=2 -fomit-frame-pointer -Wall -g
OBJ = cpu.o memory.o vram.o interrupt.o rom.o sound.o frame_skip.o 


CFLAGS = $(OPT) -D_REENTRANT
LIBS = -lSDL -lpthread  

ifdef JOYSTICK
CFLAGS += -DLINUX_JOYSTICK 
OBJ += joystick.o 
endif

DEP = global.h 

all: gngb

# GNGB

cpu.o : cpu.c cpu.h memory.h rom.h $(DEP)
	gcc -c $(CFLAGS) cpu.c 

memory.o : memory.c memory.h rom.h cpu.h vram.h joystick.h interrupt.h sound.h $(DEP)
	gcc -c $(CFLAGS) memory.c

interrupt.o : interrupt.c interrupt.h memory.h cpu.h vram.h $(DEP)
	gcc -c $(CFLAGS) interrupt.c

rom.o : rom.c rom.h memory.h cpu.h $(DEP)
	gcc -c $(CFLAGS) rom.c

vram.o : vram.c vram.h memory.h rom.h $(DEP)
	gcc -c $(CFLAGS) vram.c

sound.o : sound.c sound.h memory.h cpu.h $(DEP)
	gcc -c $(CFLAGS) sound.c

frame_skip.o : frame_skip.c frame_skip.h  $(DEP)
	gcc -c $(CFLAGS) frame_skip.c

joystick.o : joystick.h joystick.c
	gcc -c $(CFLAGS) joystick.c

main.o : main.c rom.h memory.h cpu.h vram.h interrupt.h sound.h $(DEP)
	gcc -c $(CFLAGS) main.c

gngb_debug.o : gngb_debug.c rom.h memory.h cpu.h vram.h interrupt.h $(DEP)
	gcc -c $(CFLAGS) gngb_debug.c

# PROGRAMME

gngb_debug : $(OBJ) gngb_debug.o
	gcc $(CFLAGS) $(OBJ) gngb_debug.o $(LIBS) -lreadline -lncurses -o gngb_debug

gngb : $(OBJ) main.o
	gcc $(CFLAGS) $(OBJ)  main.o $(LIBS) -o gngb
