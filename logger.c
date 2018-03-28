#include "logger.h"

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

FILE *g_logger_file = NULL;

bool setup_logger(const char *filename) {
  int fd = -1;
  struct stat s;
  if(!filename) {
    return false;
  }

  fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if(fd < 0) {
    perror("Could not open log file\n");
    return false;
  }

  // make sure the file is not a symlink, for security purposes
  if(fstat(fd, &s) < 0) {
    perror("Could not stat file\n");
    close(fd);
    return false;
  }

  if((s.st_mode & S_IFLNK) == S_IFLNK) {
    perror("Log file is a symlink\n");
    return false;
  }

  g_logger_file = fdopen(fd, "a");
  if(!g_logger_file) {
    close(fd);
    return false;
  }

  return true;
}

void log_msg_lineinfo(const char *fmt, const char *file, int lineno, ...) {
  va_list v;
  time_t t;
  char buf[32]; // buffer for time string

  if(!g_logger_file) {
    return;
  }

  time(&t);
  if(ctime_r(&t, buf)) {
    char *newline = strchr(buf, '\n');
    // get rid of the annoying newline from ctime()
    if(newline) {
      *newline = '\0';
    }
  }

  fprintf(g_logger_file, "%s: %s %d: ", buf, file, lineno);
  va_start(v, lineno);
  vfprintf(g_logger_file, fmt, v);
  if(fmt && !strchr(fmt, '\n')) {
    fputc('\n', g_logger_file);
  }
}

void cleanup_logger() {
  if(g_logger_file) {
    fclose(g_logger_file);
  }
}
