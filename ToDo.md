Add your content here.  Format your content with:
  * Joystick Patch needs to be reviewed/applied, see http://m.peponas.free.fr/gngb/download/memory.c.diff (news itemhttp://m.peponas.free.fr/gngb/news.html)
  * build problem: If HAVE\_GETOPT\_LONG is not defined, a local getopt.h is included BUT there is no local  getopt.c
  * Review http://code.google.com/r/clach04-gngb-dingoo/ - has a potential solution to getopt issue above, and also has an option to disable serial port. Incomplete, currently not handled with `#ifdef`