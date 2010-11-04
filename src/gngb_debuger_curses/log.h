#ifndef LOG_H
#define LOG_H

char active_log;

extern UINT16 nb_nop;

void open_log(void);
void close_log(void);
void put_log(const char *format,...);
void put_log_message(const char *mes);

#endif
