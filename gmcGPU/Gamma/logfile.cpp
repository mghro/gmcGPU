/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "logfile.h"

#define LOGFILE_ECHO_ON 1  // 1 = yes

FILE* logfile_fp = 0;

bool logfile_success_var =true;

bool logfile_success()  {return logfile_success_var;}


void logfile_cout(int a) {logfile_printf("%d", a);}
void logfile_cout(unsigned int a) {logfile_printf("%u", a);}
void logfile_cout(unsigned long int a) {logfile_printf("%u", a);}
void logfile_cout(float a) {logfile_printf("%f", a);}
void logfile_cout(double a) {logfile_printf("%f", a);}
void logfile_cout(char a[]) {logfile_printf("%s", a);}
void logfile_cout(char a) {logfile_printf("%c", a);}
void logfile_cout(const char a[]) {logfile_printf("%s", a);}


void logfile_test_only_clear_success(){  logfile_success_var =true;}


void
logfile_open (const char* logfile_fn)
{
    if (!logfile_fn[0]) return;
    if (!(logfile_fp)) {
	logfile_fp = fopen (logfile_fn, "w");
	if (!logfile_fp) {
	  std::cout << "\nERROR: Failed to open " << logfile_fn<<std::endl;
	  logfile_success_var = false;
	  //	  exit (EXIT_FAILURE); 
	}
    } else { 	/* Already open? */
      std::cout << "\nERROR: Logfile re-opened " << logfile_fn<<std::endl;
      logfile_success_var = false;
    }
}

void
logfile_close (void)
{
    if (logfile_fp) {
	fclose (logfile_fp);
	logfile_fp = 0;
    }
}

void
logfile_printf (const char* fmt, ...)
{
    /* Write to console */
    if (LOGFILE_ECHO_ON) {
        va_list argptr;
        va_start (argptr, fmt);
	vprintf (fmt, argptr);
	fflush (stdout);
        va_end (argptr);
    }

    if (logfile_fp) {
        va_list argptr;
        va_start (argptr, fmt);
        vfprintf (logfile_fp, fmt, argptr);
        fflush (logfile_fp);
	va_end (argptr);
    }
}

void
logfile_warn (const char* fmt, ...)
{

    /* Write to console */
    if (LOGFILE_ECHO_ON) {
        va_list argptr;
        va_start (argptr, fmt);
	printf ("WARNING: ");
	vprintf (fmt, argptr);
	fflush (stdout);
        va_end (argptr);
    }

    if (logfile_fp) {
        va_list argptr;
        va_start (argptr, fmt);
	fprintf (logfile_fp, "WARNING: ");
        vfprintf (logfile_fp, fmt, argptr);
        fflush (logfile_fp);
	va_end (argptr);
    }
}

void
logfile_error (const char* fmt, ...)
{
  logfile_success_var = false;
    /* Write to console */
    if (LOGFILE_ECHO_ON) {
        va_list argptr;
        va_start (argptr, fmt);
	printf ("ERROR: ");
	vprintf (fmt, argptr);
	fflush (stdout);
        va_end (argptr);
    }

    if (logfile_fp) {
        va_list argptr;
        va_start (argptr, fmt);
	fprintf (logfile_fp, "ERROR: ");
        vfprintf (logfile_fp, fmt, argptr);
        fflush (logfile_fp);
	va_end (argptr);
    }
}

