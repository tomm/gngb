CFLAGS=@CFLAGS@ @DEBUG_CFLAGS@
LIBS=@LIBS@ @DEBUG_LIBS@

bin_PROGRAMS = gngb_dbg

gngb_dbg_SOURCES = break.c break.h code_info.c cpu_info.c debuger.c debuger.h debuger_wid.h io_info.c main.c mem_info.c menu.c msg_dbg.c vram_info.c op.c op.h int_info.c

if NATIVE_GETOPT_LONG

gngb_dbg_LDADD = memory_dbg.o cpu_dbg.o vram_dbg.o interrupt_dbg.o message_dbg.o sgb_dbg.o sound_dbg.o rom_dbg.o frame_skip_dbg.o emu_dbg.o serial_dbg.o video_std_dbg.o video_yuy2_dbg.o video_yv12_dbg.o video_gl_dbg.o video_yuv_dbg.o menu_dbg.o 

else

gngb_dbg_LDADD = memory_dbg.o cpu_dbg.o vram_dbg.o interrupt_dbg.o message_dbg.o sgb_dbg.o sound_dbg.o rom_dbg.o frame_skip_dbg.o emu_dbg.o serial_dbg.o video_std_dbg.o video_yuy2_dbg.o video_yv12_dbg.o video_gl_dbg.o video_yuv_dbg.o menu_dbg.o getopt_dbg.o getopt1_dbg.o

endif

%_dbg.o : ../%.c
	$(COMPILE) -c $<  -o $@