CFLAGS=@CFLAGS@ @DEBUG_CFLAGS@
LIBS=@LIBS@ @DEBUG_LIBS@

bin_PROGRAMS = gngb_debug


gngb_debug_SOURCES = debuger.h debuger.c code_info.c cpu_info.c mem_info.c io_info.c vram_info.c msg_info.c main.c log.c log.h op.c op.h
gngb_debug_LDADD = memory_dbg.o cpu_dbg.o vram_dbg.o interrupt_dbg.o message_dbg.o sgb_dbg.o sound_dbg.o rom_dbg.o frame_skip_dbg.o emu_dbg.o serial_dbg.o

%_dbg.o : ../%.c
	$(COMPILE) -c $<  -o $@
