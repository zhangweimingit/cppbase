#include <cassert>

#include "base/utils/ik_logger.h"

thread_local const pid_t g_tid = ::syscall(SYS_gettid);

static const char *g_log_level_str[] = {
	"TRACE",
	"DEBUG",
	"INFO",
	"WARN",
	"ERROR",
	"DEAD",
};


/* cli color */
#define CLI_DEFAULT_COLOR   "\033[0m"
#define CLI_FGROUND_RED     "\033[0;31m"
#define CLI_FGROUND_GREEN   "\033[0;32m"
#define CLI_FGROUND_YELLOW  "\033[0;33m"
#define CLI_FGROUND_BLUE    "\033[0;34m"
#define CLI_FGROUND_PURPLE  "\033[0;35m"
#define CLI_FGROUND_WHITE   "\033[0;37m"

static const char *g_log_color[] = {
	CLI_FGROUND_BLUE, // trace
	CLI_FGROUND_WHITE, // debug
	CLI_FGROUND_GREEN, // info
	CLI_FGROUND_YELLOW, // warn
	CLI_FGROUND_RED, // error
	CLI_FGROUND_PURPLE, //dead
};
#define DEFAULT_STYLE	CLI_FGROUND_BLUE


class Logger
{
	public:
		Logger();
		~Logger();
		int init(const char *dir, const char *file, uint8_t level, uint8_t days, uint32_t size);

		void dump(const char *title, const void *buffer, int32_t len);

		void logging(int level, const char *format, va_list ap) {
			static_assert(sizeof(g_log_level_str)/sizeof(g_log_level_str[0]) == D_LOG_NR,
				"Need to modify log level str");
			static_assert(sizeof(g_log_color)/sizeof(g_log_color[0]) == D_LOG_NR,
				"Need to modify log color");
			assert(level < D_LOG_NR);
			
			write_log(level, g_log_level_str[level], g_log_color[level], format, ap);
 		}

		void set_level(uint8_t level) { log_level_ = level; }
		int get_level() { return log_level_; }
	private:
		enum {
			LOGGER_DIR_MAX_LEN = 128,
			LOGGER_FILE_MAX_LEN = 32,
			LOGGER_FULL_PATH_LEN = (LOGGER_DIR_MAX_LEN+LOGGER_FILE_MAX_LEN),
		};
		char log_dir_[LOGGER_DIR_MAX_LEN];
		char log_file_[LOGGER_FILE_MAX_LEN];
		char log_full_[LOGGER_FULL_PATH_LEN];
		uint32_t log_size_;
		uint16_t log_idx_;
		uint8_t log_level_;
		uint8_t log_days_;
		FILE *fp_;
		pthread_mutex_t locker_;

		void write_log(uint8_t level, const char* flag, const char* style, const char* format, va_list ap);
		int check_dir();
		int check_size(struct tm *now);
		void delete_old();
		static void *timer(void *arg);
};

Logger g_logger;

Logger::Logger()
{
	memset(&log_dir_, 0, sizeof(log_dir_));
	memset(&log_file_, 0, sizeof(log_file_));
	memset(&log_full_, 0, sizeof(log_full_));
	log_level_ = D_DBUG;
	log_days_ = 7;
	log_size_ = 5120;
	log_idx_ = 1;
	pthread_mutex_init(&locker_, NULL);
}

Logger::~Logger()
{
	if (fp_) {
		fclose(fp_);
		fp_ = NULL;
	}
	pthread_mutex_destroy(&locker_);
}

int Logger::init(const char *dir, const char *file, uint8_t level, uint8_t days, uint32_t size)
{
	if(strlen(dir) + 1 > sizeof(log_dir_)) {
		printf(" [%s] length overflow\n", dir);
		return -1;
	}
	if(strlen(file) + 1 > sizeof(log_file_)) {
		printf(" [%s] length overflow\n", file);
		return -1;
	}
	strcpy(log_dir_, dir);
	strcpy(log_file_, file);

	if (check_dir() != 0)
		return -1;

	log_level_ = level;
	log_days_ = days;
	log_size_ = size;

	sprintf(log_full_, "%s/%s.log", dir, file);
	fp_ = fopen(log_full_, "a");

	pthread_t tid;
	if (pthread_create(&tid, NULL, timer, (void*)this) != 0)
		return -1;
	pthread_detach(tid); 

	return 0;
}

int Logger::check_dir()
{
	if (!strlen(log_dir_))
		return -1;
	int ret = mkdir(log_dir_, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (ret != 0 && errno != EEXIST) {
		printf("mkdir fail which dir:%s err:%s\n", log_dir_, strerror(errno));
		return -1;
	}
	return 0;
}

int Logger::check_size(struct tm *now)
{
	if (1 == isatty(fileno(fp_)))
		return 0;

	if (ftell(fp_) > log_size_ * 1024) {
		fclose(fp_);
		fp_ = NULL;

		char new_file[128] = {0};
		sprintf(new_file, "%s/%s_%04d%02d%02d_%02d%02d%02d_%03d.log",
				log_dir_,
				log_file_,
				now->tm_year + 1900,
				now->tm_mon + 1,
				now->tm_mday,
				now->tm_hour,
				now->tm_min,
				now->tm_sec,
				log_idx_++);
		if (log_idx_ > 999)
			log_idx_ = 1;
		if (rename(log_full_, new_file))
			return -1;
		fp_ = fopen(log_full_, "a");
	}

	return 0;
}

void Logger::delete_old()
{
	if (log_days_ <= 0)
		return; 

	time_t now;
	struct tm tm;

	now = time(NULL);
	now -= log_days_ * 3600 * 24;
	localtime_r(&now, &tm);

	char pattern[12] = {0};
	sprintf(pattern, "*%04d%02d%02d*", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

	DIR *dp = NULL;
	if (!(dp = opendir(log_dir_))) {
		printf("opendir error which dir:%s err:%s\n", log_dir_, strerror(errno));
		return;
	}
	int ret = 0;
	struct dirent *entry = NULL;
	while ((entry = readdir(dp))) {
		ret = fnmatch(pattern, entry->d_name, FNM_PATHNAME|FNM_PERIOD);
		if ( 0 == ret ){
			char rm_log[128] = {0};
			snprintf(rm_log, sizeof(rm_log), "%s/%s", log_dir_, entry->d_name);
			remove(rm_log);
		} else if ( FNM_NOMATCH == ret )
			continue;
	}
	closedir(dp); 
}

void *Logger::timer(void *arg)
{
	Logger *ptr = (Logger *)arg;
	while(1) {
		ptr->delete_old();
		sleep(24 * 3600);
	}
	return NULL;
}

void Logger::write_log(uint8_t level, const char *flag, const char *style, const char *format, va_list ap)
{
	if (log_level_ > level)
		return;

	pthread_mutex_lock(&locker_);
	if (!fp_)
		fp_ = stdout;

	if (1 == isatty(fileno(fp_)))
		fprintf(fp_, "%s", style);

	struct tm tm_now;
	struct timeval tv_now;
	gettimeofday(&tv_now, NULL);
	localtime_r(&tv_now.tv_sec, &tm_now);

	check_size(&tm_now);

	fprintf(fp_, "%s %04d-%02d-%02d %02d:%02d:%02d.%ld %d %d",
			flag,
			tm_now.tm_year + 1900,
			tm_now.tm_mon + 1,
			tm_now.tm_mday,
			tm_now.tm_hour,
			tm_now.tm_min,
			tm_now.tm_sec,
			tv_now.tv_usec,
			getpid(),
			g_tid);

	va_list ap_t;
	va_copy(ap_t, ap);
	vfprintf(fp_, format, ap_t);
	va_end(ap_t);

	if (1 == isatty(fileno(fp_)))
		fprintf(fp_, "%s", DEFAULT_STYLE);

	fprintf(fp_, "\n");

	fflush(fp_);

	pthread_mutex_unlock(&locker_);
}

void Logger::dump(const char *title, const void *buffer, int32_t len)
{
	const int32_t max_len = 1024 * 1024;
	const uint8_t *pbuf = reinterpret_cast<const uint8_t*>(buffer);
	char hex[max_len] = {0};
	size_t hex_len = 0;
	for (int idx = 0; idx < len; idx++) {
		snprintf(hex + hex_len, sizeof(hex), "%02X ", pbuf[idx]);
		hex_len += 3;
	}

	log_base(D_DBUG, " %s [len:%d, data:%s]", title, len, hex);
}

void sig_handle(int sig)
{ 
	if (g_logger.get_level() <= D_INFO)
		g_logger.set_level(D_DBUG);
	else if (g_logger.get_level() > D_INFO)
		g_logger.set_level(D_INFO);
}

int log_init(const char *dir, const char *file, uint8_t level, uint8_t days, uint32_t size)
{
	if (g_logger.init(dir, file, level, days, size) != 0)
		return -1;
	int ret = 0;
	if (signal(SIGHUP, sig_handle) == SIG_ERR)
		ret = -1;
	// only for a warning
#define SIGHUP
	return ret;
}

void reset_log_level(const char * level)
{
	uint8_t level_value = D_INFO;

	LOG_INFO("Set log level as %s", level);

	if (strcmp("debug", level) == 0) {
		level_value = D_DBUG;
	} else if (strcmp("trace", level) == 0) {
		level_value = D_TRAC;
	} else if (strcmp("info", level) == 0) {
		level_value = D_INFO;
	} else if (strcmp("warn", level) == 0) {
		level_value = D_WARN;
	} else if (strcmp("error", level) == 0) {
		level_value = D_ERRO;
	} else if (strcmp("dead", level) == 0) {
		level_value = D_DEAD;
	}
	g_logger.set_level(level_value);
}

void log_dump(const char *title, const void *buffer, int32_t len)
{
	g_logger.dump(title, buffer, len);
}

void log_base(int level, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	g_logger.logging(level, fmt, ap);
	va_end(ap);

	if (level == D_DEAD) {
		exit(EXIT_FAILURE);
	}
}
