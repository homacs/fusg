/*
 * conf.c
 *
 *  Created on: 6 Mar 2020
 *      Author: homac
 */



#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>

#include "fusg/conf.h"
#include "fusg/err.h"
#include "fusg/logging.h"


static const char* file;
static int linenum = 0;


// FIXME: check all PATH_MAX and FILE_MAX buffers for one-off errors!
// FIXME: decide whether to resolve paths (absolute and canonical) or not




void fusg_config_init(fusg_conf_t* conf)
{
	strcpy(conf->fusgd_log, FUSG_LOGPATH_DEFAULT);
	strcpy(conf->fusgd_trace, FUSG_TRACEPATH_DEFAULT);
	strcpy(conf->db_path, FUSG_DBPATH_DEFAULT);
}


void conf_error(const char* fmt, ...)
{
	va_list ap;
	const size_t size = PATH_MAX*2;
	char msg[size];

    va_start(ap, fmt);
	vsnprintf(msg, size, fmt, ap);
    va_end(ap);

	log_error("%s:%d: %s", file, linenum, msg);
}


void skip_white(char** line)
{
	char* p = *line;
	for (;isspace(*p); p++);
	*line = p;
}

const char* token_string(char** line)
{
	char* p = *line;
	char del = *p;
	p++;
	char* s = p;
	for(; *p && *p != del; p++)
	{
		if (*p == '\\')
		{
			// skip escaped character
			p++;
		}
	}
	if (*p == '\0')
	{
		conf_error("missing delimiter '%c'", del);
	}
	else
	{
		*p = '\0';
		*line = p+1;
	}
	return s;
}


const char* token(char** line)
{
	const char *s = *line;
	char *p = *line;

	if (*p == '"' || *p == '\'')
	{
		s = token_string(line);
	}
	else
	{
		for(; *p && !isspace(*p); p++);
		if (p == s)
		{
			return NULL;
		}
		if (*p != '\0')
		{
			// not EOF
			*p = 0;
			p++;
		}
		*line = p;
	}
	return s;
}

const char* line_end(char** line)
{
	const char* s = *line;
	char* p = *line;
	for (; *p && *p != '\n'; p++)
	{
		if (*p == '#')
		{
			break;
		}
	}
	if (p == s)
	{
		s = NULL; // line end is empty or a comment
	}
	else
	{
		*p = '\0'; // terminate line end to show error
	}
	return s;
}



int consume(char c, char** line)
{
	if (**line == c)
	{
		*line += 1;
		return 1;
	}
	return 0;
}


int property(fusg_conf_t* conf, const char* name, const char* value)
{
	int rc = 0;
	if (!strcmp(name, "db_path"))
	{
		snprintf(conf->db_path, PATH_MAX, "%s", value);
	}
	else if (!strcmp(name, "fusgd_log"))
	{
		snprintf(conf->fusgd_log, PATH_MAX, "%s", value);
	}
	else if (!strcmp(name, "fusgd_trace"))
	{
		snprintf(conf->fusgd_trace, PATH_MAX, "%s", value);
	}
	else
	{
		conf_error("unknown config property '%s'", name);
		rc = -1;
	}
	return rc;
}


int parse_line(fusg_conf_t* conf, char* line)
{
	skip_white(&line);

	if (!line[0] || line[0] == '#')
	{
		// ignore comment or empty lines
		return 0;
	}
	else
	{
		const char* name = token(&line);
		if (!name) return -1;
		skip_white(&line);
		if (!consume('=', &line)) return -1;
		skip_white(&line);
		const char* value = token(&line);
		if (!value)
		{
			conf_error("missing value for property %s", name);
			return -1;
		}

		skip_white(&line);
		const char* remains = line_end(&line);
		if (remains)
		{
			conf_error("found garbage at end of line: '%s'", remains);
			return -1;
		}

		return property(conf, name, value);
	}
}


int fusg_conf_read(fusg_conf_t* conf, const char* path)
{
	int rc = ERR_NONE;
	fusg_config_init(conf);

	file = path;
	FILE* stream = fopen(path, "r");
	if (!stream) return ERR_CONF;

	size_t bufsize = 2048;
	char buf[bufsize];
	char* line;
	for (line =	fgets(buf, bufsize, stream); line ; line = fgets(buf, bufsize, stream))
	{
		if (0 != parse_line(conf, line))
		{
			rc = ERR_CONF;
			break;
		}
		linenum++;
	}
	if (stream) fclose(stream);
	return rc;
}
