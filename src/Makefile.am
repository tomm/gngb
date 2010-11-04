bin_PROGRAMS = gngb gngb_dbg

gngb_SOURCES = cpu.c cpu.h emu.c emu.h frame_skip.c frame_skip.h global.h interrupt.c interrupt.h main.c memory.c memory.h message.c message.h rom.c rom.h serial.c  serial.h sgb.c sgb.h sound.c sound.h tiny_font.h vram.c vram.h video_yuy2.c video_yuv.c video_yuv.h video_yv12.c video_std.c video_gl.c video_gl.h video_std.h menu.h menu.c menu_image.h fileio.h fileio.c save.h save.c @UNZIP_SRC@ 

gngb_dbg_SOURCES = cpu.c cpu.h emu.c emu.h frame_skip.c frame_skip.h global.h interrupt.c interrupt.h  memory.c memory.h message.c message.h rom.c rom.h serial.c  serial.h sgb.c sgb.h sound.c sound.h tiny_font.h vram.c vram.h video_yuy2.c video_yuv.c video_yuv.h video_yv12.c video_std.c video_gl.c video_gl.h video_std.h menu.h menu.c menu_image.h fileio.h fileio.c save.h save.c  @UNZIP_SRC@ 

gngb_dbg_LDADD = debuger/cmd.o debuger/op.o debuger/cmd_code.o debuger/cmd_cpu.o debuger/cmd_lcd.o debuger/args.o debuger/main.o @LIBOBJS@ -lreadline

gngb_LDADD = @LIBOBJS@
EXTRA_DIST = getopt.h

debuger/cmd.o : debuger/cmd.c
	$(COMPILE) -c  debuger/cmd.c -o debuger/cmd.o

debuger/op.o : debuger/op.c
	$(COMPILE) -c  debuger/op.c -o debuger/op.o

debuger/cmd_code.o : debuger/cmd_code.c
	$(COMPILE) -c  debuger/cmd_code.c -o debuger/cmd_code.o

debuger/cmd_cpu.o : debuger/cmd_cpu.c
	$(COMPILE) -c  debuger/cmd_cpu.c -o debuger/cmd_cpu.o

debuger/cmd_lcd.o : debuger/cmd_lcd.c
	$(COMPILE) -c  debuger/cmd_lcd.c -o debuger/cmd_lcd.o

debuger/args.o : debuger/args.c
	$(COMPILE) -c  debuger/args.c -o debuger/args.o

debuger/main.o : debuger/main.c
	$(COMPILE) -c  debuger/main.c -o debuger/main.o




