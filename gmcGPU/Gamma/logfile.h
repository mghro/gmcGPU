/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#ifndef _logfile_h_
#define _logfile_h_




void logfile_open (const char* logfile_fn);
void logfile_close (void);
void logfile_printf (const char* fmt, ...);
void logfile_warn(const char* fmt, ...);
void logfile_error(const char* fmt, ...); //sets the success variable
bool logfile_success();  //gets the success variable

void logfile_cout(int a);
void logfile_cout(unsigned int a);
void logfile_cout(unsigned long int a);
void logfile_cout(float a);
void logfile_cout(double a);
void logfile_cout(char a[]);
void logfile_cout(const char a[]);
void logfile_cout(char a);


void logfile_test_only_clear_success();

#endif
