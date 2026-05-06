/*!
    \file    os_gdwifi.c
    \brief   OS specific functions for GDWIFI system

    \version 2023-07-20, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2023, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include "includes.h"

#include "os.h"
#include "trace.h"
#include "systime.h"
#include <stdarg.h>
#include "trng.h"
#include "common.h"

void os_sleep(os_time_t sec, os_time_t usec)
{
	uint32_t delay;
	delay = (sec * 1000) + (usec / 1000);
	sys_ms_sleep(delay);
}


int os_get_time(struct os_time *t)
{
	uint32_t sec, usec;
	int ret;

	ret = get_time(SINCE_EPOCH, &sec, &usec);
	t->sec = sec;
	t->usec = usec;
	return ret;
}


int os_get_reltime(struct os_reltime *t)
{
	uint32_t sec, usec;
	int ret;

	ret = get_time(SINCE_BOOT, &sec, &usec);
	t->sec = sec;
	t->usec = usec;
	return ret;
}


int os_mktime(int year, int month, int day, int hour, int min, int sec,
	      os_time_t *t)
{
    int leap_years = 0, i = 0, time_zone = 8;
    unsigned int days = 0, resultValue = 0;
    int year_t = year - 1;
    const int month_days[] = {31,28,31,30,31,30,31,31,30,31,30,31};

    leap_years = (year_t / 4) - (year_t / 100);
    leap_years += (year_t / 400);
    days = year_t * 365 + leap_years;

    for (i = 0; i < month - 1; i++) {
        days += month_days[i];
    }

    if (month > 2) {
        if (((((year) % 4) == 0) && (((year) % 100) != 0)) || (((year) % 400) == 0))
        days ++;
    }

    days = days + day - 1;

    resultValue = (days - 719162) * 24 * 3600;
    resultValue += ((hour) * 3600 + (min) * 60 + sec);
    resultValue -= ((unsigned int)time_zone) * 3600;

    *t = (os_time_t)resultValue;

    return 0;
}

int os_gmtime(os_time_t t, struct os_tm *tm)
{
	wpa_printf(MSG_INFO, "TODO: os_gmtime");
	return -1;
}


int os_daemonize(const char *pid_file)
{
	wpa_printf(MSG_INFO, "TODO: os_daemonize");
	return -1;
}


void os_daemonize_terminate(const char *pid_file)
{
	wpa_printf(MSG_INFO, "TODO: os_daemonize_terminate");
}

/* TODO: Check this is good enough */
int os_get_random(unsigned char *buf, size_t len)
{
#ifdef CONFIG_NO_RANDOM_POOL
	random_get(buf, len);
	return 0;
#else
	size_t i;
	for (i = 0; i < len ; i++) {
		buf[i] = co_rand_byte();
	}
	return 0;
#endif
}


unsigned long os_random(void)
{
#ifdef CONFIG_NO_RANDOM_POOL
	unsigned long random_byte = 0;
	random_get((unsigned char *)&random_byte, sizeof(unsigned long));
	return random_byte;
#else
	return co_rand_word();
#endif
}


char * os_rel2abs_path(const char *rel_path)
{
	wpa_printf(MSG_INFO, "TODO: os_rel2abs_path");
	return NULL; /* strdup(rel_path) can be used here */
}


int os_program_init(void)
{
	wpa_printf(MSG_INFO, "TODO: os_program_init");
	return 0;
}


void os_program_deinit(void)
{
	wpa_printf(MSG_INFO, "TODO: os_program_deinit");
}


int os_setenv(const char *name, const char *value, int overwrite)
{
	wpa_printf(MSG_INFO, "TODO: os_setenv");
	return -1;
}

int os_unsetenv(const char *name)
{
	wpa_printf(MSG_INFO, "TODO: os_unsetenv");
	return -1;
}


char * os_readfile(const char *name, size_t *len)
{
#ifdef CFG_8021x_EAP_TLS
	return wifi_wpa_sta_read_eap_tls_files(name, len);
#else
	wpa_printf(MSG_INFO, "TODO: os_readfile");
	return NULL;
#endif
}


int os_fdatasync(FILE *stream)
{
	wpa_printf(MSG_INFO, "TODO: os_fdatasync");
	return 0;
}


void * os_zalloc(size_t size)
{
	void * ptr = os_malloc(size);
	if (ptr) {
		os_memset(ptr, 0, size);
	}
	return ptr;
}


#ifdef OS_NO_C_LIB_DEFINES
void * os_malloc(size_t size)
{
	return sys_malloc(size);
}

void * os_memdup(const void *src, size_t len)
{
    void *r = os_malloc(len);

    if (r)
        os_memcpy(r, src, len);
    return r;
}

void * os_realloc(void *ptr, size_t size)
{
#ifdef CONFIG_GDWIFI
	return sys_realloc(ptr, size);
#else
	void *res;

	if (!ptr)
		return os_malloc(size);

	if (!size) {
		os_free(ptr);
		return NULL;
	}

	res = os_malloc(size);

#if WIFI_PLF == GD32VW55X
	if (res) {
		os_memcpy(res, ptr, size);
		os_free(ptr);
	}
#else
	if (size)
		os_memcpy(res, ptr, size);
	os_free(ptr);
#endif

	return res;
#endif
}


void os_free(void *ptr)
{
	if (ptr == NULL)
		return;
	sys_mfree(ptr);
}


void * os_memcpy(void *dest, const void *src, size_t n)
{
	return memcpy(dest, src, n);
}


void * os_memmove(void *dest, const void *src, size_t n)
{
	return memmove(dest, src, n);
}


void * os_memset(void *s, int c, size_t n)
{
	return memset(s, c, n);
}


int os_memcmp(const void *s1, const void *s2, size_t n)
{
#ifdef CONFIG_GDWIFI
	return sys_memcmp(s1, s2, n);
#else
	const uint8_t *ptr1=s1, *ptr2=s2;

	while(n) {
		int diff = (*ptr1++ - *ptr2++);
		if (diff)
			return diff;
		n--;
	}

	return 0;
#endif
}


char * os_strdup(const char *s)
{
	char *res;
	size_t len = os_strlen(s) + 1;
	res = os_malloc(len);
	if (res)
		os_memcpy(res, s, len);

	return res;
}


size_t os_strlen(const char *s)
{
	return strlen(s);
}


int os_strcasecmp(const char *s1, const char *s2)
{
	/*
	 * Ignoring case is not required for main functionality, so just use
	 * the case sensitive version of the function.
	 */
	return os_strcmp(s1, s2);
}


int os_strncasecmp(const char *s1, const char *s2, size_t n)
{
	/*
	 * Ignoring case is not required for main functionality, so just use
	 * the case sensitive version of the function.
	 */
	return os_strncmp(s1, s2, n);
}


char * os_strchr(const char *s, int c)
{
	return strchr(s, c);
}


char * os_strrchr(const char *s, int c)
{
	return strrchr(s, c);
}


int os_strcmp(const char *s1, const char *s2)
{
	return strcmp(s1, s2);
}


int os_strncmp(const char *s1, const char *s2, size_t n)
{
	return strncmp(s1, s2, n);
}


char * os_strncpy(char *dest, const char *src, size_t n)
{
	return strncpy(dest, src, n);
}


size_t os_strlcpy(char *dest, const char *src, size_t size)
{
	const char *s = src;
	size_t left = size;

	if (left) {
		/* Copy string up to the maximum size of the dest buffer */
		while (--left != 0) {
			if ((*dest++ = *s++) == '\0')
				break;
		}
	}

	if (left == 0) {
		/* Not enough room for the string; force NUL-termination */
		if (size != 0)
			*dest = '\0';
		while (*s++)
			; /* determine total src string length */
	}

	return s - src - 1;
}


int os_memcmp_const(const void *a, const void *b, size_t len)
{
	const uint8_t *aa = a;
	const uint8_t *bb = b;
	size_t i;
	uint8_t res;

	for (res = 0, i = 0; i < len; i++)
		res |= aa[i] ^ bb[i];

	return res;
}

char * os_strstr(const char *haystack, const char *needle)
{
	return strstr(haystack, needle);
}



int os_snprintf(char *str, size_t size, const char *format, ...)
{
	int ret;
	va_list args;
	va_start(args, format);
	ret = dbg_vsnprintf(str, size, format, args);
	va_end(args);

	return ret;
}
#endif /* OS_NO_C_LIB_DEFINES */


int os_exec(const char *program, const char *arg, int wait_completion)
{
	wpa_printf(MSG_INFO, "TODO: os_exec");
	return -1;
}
