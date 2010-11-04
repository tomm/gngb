CFLAGS=@CFLAGS@ @DEBUG_CFLAGS@
LIBS=@LIBS@ @DEBUG_LIBS@

bin_PROGRAMS = gngb_dbg

gngb_dbg_SOURCES = break.c break.h code_info.c cpu_info.c debuger.c debuger.h debuger_wid.h io_info.c main.c mem_info.c menu.c msg_dbg.c op.c op.h 
gngb_dbg_LDADD = memory_dbg.o cpu_dbg.o vram_dbg.o interrupt_dbg.o message_dbg.o sgb_dbg.o sound_dbg.o rom_dbg.o frame_skip_dbg.o emu_dbg.o serial_dbg.o

%_dbg.o : ../%.c
	$(COMPILE) -c $<  -o $@