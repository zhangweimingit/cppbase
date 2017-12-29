/*
 * Created by zyzhou on 4/24/2017
 * Copyright © 2012 - 2017 iKuai. All Rights Reserved.
 * Revision History:
 *  - 04/24/2017 by s.c init
 *  - 06/09/2017 by s.c hpp issue
 *  - 06/13/2017 by s.c fixed issue 
 *      # namespace pollution: split hpp
 *  - 06/16/2017 by s.c added issue
 *      # LOG_DUMP
 */

#ifndef IK_LOGGER_HPP_
#define IK_LOGGER_HPP_

#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <pthread.h>    // not c++11 <thread>
#include <signal.h>
#include <stdarg.h>     // va_start ...
#include <stdint.h>     // int8_t ...
#include <stdlib.h>     // exit ...
#include <string.h>
#include <stdio.h>      // FILE fprintf ...
#include <sys/syscall.h>// gettid
#include <sys/stat.h>   // mkdir ...
#include <sys/time.h>
#include <sys/types.h>  // opendir ...
#include <time.h>
#include <unistd.h>     // getpid() ...

/* log level */
enum {
	D_TRAC = 0,
	D_DBUG,
	D_INFO,
	D_WARN,
	D_ERRO,
	D_DEAD,

	D_LOG_NR
};

/*
Size: KB
*/
extern int log_init(const char* dir, const char* file, uint8_t level = D_INFO, uint8_t days = 7, uint32_t size = 5120);
extern void reset_log_level(const char * level);

#define __FILENAME__ (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)
#define __FILE_LINE_FUNC__  __FILENAME__, __LINE__, __FUNCTION__
#define LOG_DEAD(format, ...) log_base(D_DEAD, " %s +%d %s | " format, __FILE_LINE_FUNC__, ##__VA_ARGS__)
#define LOG_ERRO(format, ...) log_base(D_ERRO, " %s +%d %s | " format, __FILE_LINE_FUNC__, ##__VA_ARGS__)
#define LOG_WARN(format, ...) log_base(D_WARN, " %s +%d %s | " format, __FILE_LINE_FUNC__, ##__VA_ARGS__)
#define LOG_INFO(format, ...) log_base(D_INFO, " %s +%d %s | " format, __FILE_LINE_FUNC__, ##__VA_ARGS__)
#define LOG_TRAC(format, ...) log_base(D_TRAC, " %s +%d %s | " format, __FILE_LINE_FUNC__, ##__VA_ARGS__)
#define LOG_DBUG(format, ...) log_base(D_DBUG, " %s +%d %s | " format, __FILE_LINE_FUNC__, ##__VA_ARGS__)

#define LOG_DUMP(title, buffer, len) { LOG_DBUG(); log_dump(title, buffer, len); }


extern thread_local const pid_t g_tid;

extern void sig_handle(int sig);
extern void log_dump(const char *title, const void *buffer, int32_t len);
extern void log_base(int level, const char *fmt, ...);


#endif // IK_LOGGER_HPP_
