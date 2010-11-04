#ifndef LOG_H
#define LOG_H

char active_log;

void open_log(void);
void close_log(void);
void put_log(const char *format,...);

#endif
