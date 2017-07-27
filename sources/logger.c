
/*
 * Odissey.
 *
 * Advanced PostgreSQL connection pooler.
*/

#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <time.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <machinarium.h>
#include <shapito.h>

#include "sources/macro.h"
#include "sources/pid.h"
#include "sources/id.h"
#include "sources/log_file.h"
#include "sources/log_system.h"
#include "sources/logger.h"

typedef struct {
	od_logsystem_prio_t syslog_prio;
	char *ident;
	char *ident_short;
} od_logger_ident_t;

static od_logger_ident_t od_logger_ident_tab[] =
{
	[OD_LOG]              = { OD_LOGSYSTEM_INFO,  "info",          NULL   },
	[OD_LOG_ERROR]        = { OD_LOGSYSTEM_ERROR, "error",        "error" },
	[OD_LOG_CLIENT]       = { OD_LOGSYSTEM_INFO,  "client_info",   NULL   },
	[OD_LOG_CLIENT_ERROR] = { OD_LOGSYSTEM_ERROR, "client_error", "error" },
	[OD_LOG_CLIENT_DEBUG] = { OD_LOGSYSTEM_DEBUG, "client_debug", "debug" },
	[OD_LOG_SERVER]       = { OD_LOGSYSTEM_INFO,  "server_info",   NULL   },
	[OD_LOG_SERVER_ERROR] = { OD_LOGSYSTEM_ERROR, "server_error", "error" },
	[OD_LOG_SERVER_DEBUG] = { OD_LOGSYSTEM_DEBUG, "server_debug", "debug" }
};

static inline void
od_logger_write(od_logger_t *logger, od_logger_ident_t *ident,
                char *buf, int buf_len)
{
	od_logfile_write(&logger->log, buf, buf_len);
	od_logsystem(&logger->log_system, ident->syslog_prio, buf, buf_len);
	if (logger->log_stdout) {
		write(0, buf, buf_len);
	}
}

static void
od_logger_text(od_logger_t *logger,
               od_logger_event_t event,
               od_id_t *id,
               char *context,
               char *fmt, va_list args)
{
	char buf[512];
	int  buf_len;

	/* pid */
	buf_len = snprintf(buf, sizeof(buf), "%s ", logger->pid->pid_sz);

	/* timestamp */
	struct timeval tv;
	gettimeofday(&tv, NULL);
	buf_len += strftime(buf + buf_len, sizeof(buf) - buf_len, "%d %b %H:%M:%S.",
	                    localtime(&tv.tv_sec));
	buf_len += snprintf(buf + buf_len, sizeof(buf) - buf_len, "%03d  ",
	                    (signed)tv.tv_usec / 1000);

	/* ident */
	od_logger_ident_t *ident;
	ident = &od_logger_ident_tab[event];
	if (ident->ident_short) {
		buf_len += snprintf(buf + buf_len, sizeof(buf) - buf_len, "%s: ",
		                    ident->ident_short);
	}

	/* id */
	if (id) {
		buf_len += snprintf(buf + buf_len, sizeof(buf) - buf_len, "%s%.*s: ",
		                    id->id_prefix,
		                    (signed)sizeof(id->id), id->id);
	}

	/* context */
	if (context) {
		buf_len += snprintf(buf + buf_len, sizeof(buf) - buf_len, "(%s) ",
		                    context);
	}

	/* message */
	buf_len += vsnprintf(buf + buf_len, sizeof(buf) - buf_len, fmt, args);
	buf_len += snprintf(buf + buf_len, sizeof(buf) - buf_len, "\n");

	/* write log message */
	od_logger_write(logger, ident, buf, buf_len);
}

static void
od_logger_tskv(od_logger_t *logger,
               od_logger_event_t event,
               od_id_t *id,
               char *context,
               char *fmt, va_list args)
{
	(void)logger;
	(void)event;
	(void)id;
	(void)context;
	(void)fmt;
	(void)args;
}

void od_logger_init(od_logger_t *logger, od_pid_t *pid)
{
	logger->pid = pid;
	logger->log_debug = 0;
	logger->log_stdout = 0;
	logger->function = od_logger_text;
	od_logfile_init(&logger->log);
	od_logsystem_init(&logger->log_system);
}

void od_logger_set_debug(od_logger_t *logger, int enable)
{
	logger->log_debug = enable;
}

void od_logger_set_stdout(od_logger_t *logger, int enable)
{
	logger->log_stdout = enable;
}

int od_logger_open(od_logger_t *logger, char *path)
{
	return od_logfile_open(&logger->log, path);
}

int od_logger_open_syslog(od_logger_t *logger, char *ident, char *facility)
{
	return od_logsystem_open(&logger->log_system, ident, facility);
}

void od_logger_set_tskv(od_logger_t *logger)
{
	logger->function = od_logger_tskv;
}

void od_logger_close(od_logger_t *logger)
{
	od_logfile_close(&logger->log);
	od_logsystem_close(&logger->log_system);
}
