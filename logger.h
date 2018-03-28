#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdio.h>
#include <stdbool.h>

extern FILE *g_logger_file;

bool setup_logger(const char *filename);

void log_msg_lineinfo(const char *fmt, const char *file, int lineno, ...);
#define LOG_MSG(fmt, ...) log_msg_lineinfo(fmt, __FILE__, __LINE__, ## __VA_ARGS__)

void cleanup_logger();

#endif // __LOGGER_H__
