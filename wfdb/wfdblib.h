#pragma once
#include "wfdb.h"


#ifndef WFDB_NETFILES
# define DEFWFDB	". /usr/local/database"
#else
# define DEFWFDB ". /usr/local/database http://physionet.org/physiobank/database"
#endif

/* Mac OS 9 and earlier, only:  The value of DEFWFDB given below specifies
   that the WFDB path is to be read from the file udb/dbpath.mac on the third
   edition of the MIT-BIH Arrhythmia Database CD-ROM (which has a volume name
   of `MITADB3');  you may prefer to use a file on a writable disk for this
   purpose, to make reconfiguration possible.  See getwfdb() in wfdbio.c for
   further information.
  
   If the version of "ISO 9660 File Access" in the "System:Extensions" folder
   is earlier than 5.0, either update your system software (recommended) or
   define FIXISOCD. */

#ifdef MAC
/* #define FIXISOCD */
# ifdef FIXISOCD
#  define DEFWFDB	"@MITADB3:UDB:DBPATH.MAC;1"
# else
#  define DEFWFDB	"@MITADB3:UDB:DBPATH.MAC"
#  define __STDC__
# endif
#endif

/* DEFWFDBCAL is the name of the default WFDB calibration file, used if the
   WFDBCAL environment variable is not set.  This name need not include path
   information if the calibration file is located in a directory included in
   the WFDB path.  The value given below is the name of the standard
   calibration file supplied on the various CD-ROM databases.  DEFWFDBCAL may
   be NULL if you prefer not to have a default calibration file.  See calopen()
   in calib.c for further information. */
#define DEFWFDBCAL "wfdbcal"

/* WFDB applications may write annotations out-of-order, but in almost all
   cases, they expect that annotations they read must be in order.  The
   environment variable WFDBANNSORT specifies if wfdbquit() should attempt to
   sort annotations in any output annotation files before closing them (it
   does this if WFDBANNSORT is non-zero, or if WFDBANNSORT is not set, and
   DEFWFDBANNSORT is non-zero).  Sorting is done by invoking 'sortann' (see
   ../app/sortann.c) as a separate process;  since this cannot be done from
   an MS-Windows DLL, sorting is disabled by default in this case. */
#if defined(_WINDLL)
#define DEFWFDBANNSORT 0
#else
#define DEFWFDBANNSORT 1
#endif

/* When reading multifrequency records, getvec() can operate in two modes:
   WFDB_LOWRES (returning one sample per signal per frame), or WFDB_HIGHRES
   (returning each sample of any oversampled signals, and duplicating samples
   of other signals as necessary).  If the operating mode is not selected by
   invoking setgvmode(), the value of the environment variable WFDBGVMODE
   determines the mode (0: WFDB_LOWRES, 1: WFDB_HIGHRES);  if WFDBGVMODE
   is not set, the value of DEFWFDBMODE determines the mode. */
#define DEFWFDBGVMODE WFDB_LOWRES

/* putenv() is available in POSIX, SVID, and BSD Unices and in MS-DOS and
   32-bit MS Windows, but not under 16-bit MS Windows or under MacOS.  If it is
   available, getwfdb() (in wfdbio.c) detects when the environment variables
   WFDB, WFDBCAL, WFDBANNSORT, and WFDBGVMODE are not set, and sets them
   according to DEFWFDB, DEFWFDBCAL, DEFWFDBANNSORT, and DEFWFDBGVMODE as
   needed using putenv().  This feature is useful mainly for programs such as
   WAVE, where these variables are set interactively and it is useful to show
   their default values to the user; setwfdb() and getwfdb() do not depend on
   it.
*/
#if !defined(_WIN16) && !defined(MAC)
#define HAS_PUTENV
#endif

#ifndef FILE
#include <stdio.h>
/* stdin/stdout may not be defined in some environments (e.g., for MS Windows
   DLLs).  Defining them as NULL here allows the WFDB library to be compiled in
   such environments (it does not allow use of stdin/stdout when the operating
   environment does not provide them, however). */
#ifndef stdin
#define stdin NULL
#endif
#ifndef stdout
#define stdout NULL
#endif
#endif

#ifndef TRUE
#define TRUE 1 
#endif 
#ifndef FALSE
#define FALSE 0
#endif 

/* Structures used by internal WFDB library functions only */
struct WFDB_FILE {
  FILE *fp;
  struct netfile *netfp;
  int type;
};

/* Values for WFDB_FILE 'type' field */
#define WFDB_LOCAL	0	/* a local file, read via C standard I/O */
#define WFDB_NET	1	/* a remote file, read via libwww */

/* Composite data types */
typedef struct netfile netfile;
typedef struct WFDB_FILE WFDB_FILE;


#ifdef _WINDOWS
#ifndef _WIN32	/* these definitions are needed for 16-bit MS Windows only */
#define strcat _fstrcat
#define strchr _fstrchr
#define strcmp _fstrcmp
#define strcpy _fstrcpy
#define strlen _fstrlen
#define strncmp _fstrncmp
#define strtok _fstrtok
#endif
#endif


/* Define function prototypes for ANSI C, MS Windows C, and C++ compilers */
#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus) || defined(_WINDOWS)
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/* These functions are defined in wfdbio.c */
extern int wfdb_fclose(WFDB_FILE *fp);
extern WFDB_FILE *wfdb_open(const char *file_type, const char *record, int mode);
extern int wfdb_checkname(char *name, char *description);
extern void wfdb_striphea(char *record);
extern int wfdb_g16(WFDB_FILE *fp);
extern long wfdb_g32(WFDB_FILE *fp);
extern void wfdb_p16(unsigned int x, WFDB_FILE *fp);
extern void wfdb_p32(long x, WFDB_FILE *fp);
extern int wfdb_parse_path(char *wfdb_path);
extern void wfdb_addtopath(char *pathname);
#if __GNUC__ >= 3
__attribute__((__format__(__printf__, 2, 3)))
#endif
extern int wfdb_asprintf(char **buffer, const char *format, ...);
extern WFDB_FILE *wfdb_fopen(char *fname, const char *mode);
#if __GNUC__ >= 3
__attribute__((__format__(__printf__, 2, 3)))
#endif
extern int wfdb_fprintf(WFDB_FILE *fp, const char *format, ...);
extern void wfdb_setirec(const char *record_name);
extern char *wfdb_getirec(void);

extern void wfdb_clearerr(WFDB_FILE *fp);
extern int wfdb_feof(WFDB_FILE *fp);
extern int wfdb_ferror(WFDB_FILE *fp);
extern int wfdb_fflush(WFDB_FILE *fp);
extern char *wfdb_fgets(char *s, int size, WFDB_FILE *fp);
extern size_t wfdb_fread(void *ptr, size_t size, size_t nmemb, WFDB_FILE *fp);
extern int wfdb_fseek(WFDB_FILE *fp, long offset, int whence);
extern long wfdb_ftell(WFDB_FILE *fp);
extern size_t wfdb_fwrite(void *ptr, size_t size, size_t nmemb, WFDB_FILE *fp);
extern int wfdb_getc(WFDB_FILE *fp);
extern int wfdb_putc(int c, WFDB_FILE *fp);

/* These functions are defined in signal.c */
extern void wfdb_sampquit(void);
extern void wfdb_sigclose(void);
extern void wfdb_osflush(void);
extern void wfdb_freeinfo(void);
extern void wfdb_oinfoclose(void);

/* These functions are defined in annot.c */
extern void wfdb_anclose(void);
extern void wfdb_oaflush(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
