#ifndef KERNEL_LOG_H
#define KERNEL_LOG_H

void klog_init(void);
void klog_putc(char ch);
void klog_write(const char *text);
void klog_writeln(const char *text);

#endif

