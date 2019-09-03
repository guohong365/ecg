#pragma once
#include "wfdblib.h"
#include <string>
#include <filesystem>
#include <fstream>
#include <stdarg.h>
#include <cstdarg>
#ifndef VERSION
#define VERSION "VERSION not defined"
#endif

#ifndef LDFLAGS
#define LDFLAGS "LDFLAGS not defined"
#endif

#ifndef CFLAGS
#define CFLAGS  "CFLAGS not defined"
#endif

#define DSEP	'/'
#define PSEP	';'
#define AB	"ab"
#define RB	"rb"
#define WB	"wb"

namespace wfdb {
	struct _path_component {
		std::string prefix;
		int type;		/* WFDB_LOCAL or WFDB_NET */
	};
	class Context
	{
	public:
		void reset()
		{
			_path = _pathInit;
		}
		const char* path() const
		{
			return _path.c_str();
		}
		void path(const char* path)
		{
			if (path == nullptr && (path = getenv("WFDB")) == nullptr) path = DEFWFDB;
			_parsePath(path);
			_path = path;
			_export_config();
		}
		bool verbose() const
		{
			return _verbose;
		}
		void verbose(bool verbose)
		{
			_verbose = verbose;
		}
		static const char* version()
		{
			return VERSION;
		}
		static const char* defaultPath()
		{
			return DEFWFDB;
		}
		static const char* defaultCalibration()
		{
			return DEFWFDBCAL;
		}
		static Context * get(const char* paths = nullptr)
		{
			return new Context(paths);
		}
		std::vector<_path_component> &pathList()
		{
			return _path_list;
		}
		/* wfdb_addtopath adds the path component of its string argument (i.e.
everything except the file name itself) to the WFDB path, inserting it
there if it is not already in the path.  If the first component of the WFDB
path is '.' (the current directory), the new component is moved to the second
position; otherwise, it is moved to the first position.

wfdb_open calls this function whenever it finds and opens a file.

Since the files comprising a given record are most often kept in the
same directory, this strategy improves the likelihood that subsequent
files to be opened will be found in the first or second location wfdb_open
checks.

If the current directory (.) is at the head of the WFDB path, it remains there,
so that wfdb_open will continue to find the user's own files in preference to
like-named files elsewhere in the path.  If this behavior is not desired, the
current directory should not be specified initially as the first component of
the WFDB path.
 */

		void addPath(const char *s)
		{
			const char *p;


			if (s == nullptr || *s == '\0') return;

			/* Start at the end of the string and search backwards for a directory
			   separator (accept any of the possible separators). */
			for (p = s + strlen(s) - 1; p >= s &&
				*p != '/' && *p != '\\' && *p != ':'; p--)
				;

			/* A path component specifying the root directory must be treated as a
			   special case;  normally the trailing directory separator is not
			   included in the path component, but in this case there is nothing
			   else to include. */
			if (p == s && (*p == '/' || *p == '\\' || *p == ':')) p++;

			if (p < s) return;		/* argument did not contain a path component */

			const std::string in(s, p - s);
			std::vector<_path_component>::iterator c0 = _path_list.begin();
			while (c0 != _path_list.end()) {
				if (c0->prefix == in) {
					if (in == ".") return;
					break;
				}
				++c0;
			}
			_path_component c1;
			if (c0 == _path_list.end()) {
				/* path component of s not in WFDB path -- make a new node for it */
				c1.prefix = in;
				c1.type = (c1.prefix.find("://") != std::string::npos) ? WFDB_NET : WFDB_LOCAL;
			}
			if (_path_list[0].prefix == ".")
			{
				_path_list.insert(_path_list.begin() + 1, c1);
			}
			else
			{
				_path_list.insert(_path_list.begin(), c1);
			}
		}
		/* The wfdb_error function handles error messages, normally by printing them
on the standard error output.  Its arguments can be any set of arguments which
would be legal for printf, i.e., the first one is a format string, and any
additional arguments are values to be filled into the '%*' placeholders
in the format string.  It can be silenced by invoking wfdbquiet(), or
re-enabled by invoking wfdbverbose().

The function wfdberror (without the underscore) returns the most recent error
message passed to wfdb_error (even if output was suppressed by wfdbquiet).
This feature permits programs to handle errors somewhat more flexibly (in
windowing environments, for example, where using the standard error output may
be inappropriate).
*/


		const char* error()
		{
			if (!_errorFlag)
				wfdb_asprintf(&_errorMessage,
					"WFDB library version %d.%d.%d (%s).\n",
					WFDB_MAJOR, WFDB_MINOR, WFDB_RELEASE, __DATE__);
			if (_errorMessage)
				return (_errorMessage);
			return ("WFDB: cannot allocate memory for error message");
		}

		void error(const char *format, ...)
		{
			va_list arguments;
			_errorFlag = 1;
			va_start(arguments, format);
			wfdb_vasprintf(&_errorMessage, format, arguments);
			va_end(arguments);

			/* standard variant: use stderr output */
			if (verbose()) {
				(void)fprintf(stderr, "%s", error());
				(void)fflush(stderr);
			}
		}

	private:
		std::vector<_path_component> _path_list;
		int _errorFlag;
		char *_errorMessage;

		explicit Context(const char * paths = nullptr)
		{
			char *p = getenv("WFDB");
			if (p == nullptr) {
				_pathInit = DEFWFDB;
			}
			else if (*p == '@') {
				_pathInit = _getiwfdb(p);
			}
			else {
				_pathInit = p;
			}
			path(paths);
		}

		/* _export_config is invoked from setwfdb to place the configuration
		 * variables into the environment if possible. */
		void _export_config()
		{
			std::string env;

			env = "WFDB=" + _path;
			_putenv(env.c_str());

			if (getenv("WFDBCAL") == nullptr) {
				env = "WFDBCAL=";
				env += DEFWFDBCAL;
				_putenv(env.c_str());
			}
			if (getenv("WFDBANNSORT") == nullptr) {
				env = "WFDBANNSORT=" + std::to_string(DEFWFDBANNSORT == 0 ? 0 : 1);
				_putenv(env.c_str());
			}
			if (getenv("WFDBGVMODE") == nullptr) {
				env = "WFDBGVMODE=" + std::to_string(DEFWFDBGVMODE == 0 ? 0 : 1);
				_putenv(env.c_str());
			}
		}


		/* _getiwfdb reads a new value for WFDB from the file named by the second
		 * through last characters of its input argument.  If that value begins with '@',
		 * this procedure is repeated, with nesting up to ten levels.
		 * Note that the input file must be local (it is accessed using the standard C I/O
		 * functions rather than their wfdb_* counterparts).  This limitation is
		 * intentional, since the alternative (to allow remote files to determine the
		 * contents of the WFDB path) seems an unnecessary security risk. */
		std::string _getiwfdb(const char *p)
		{
			std::ifstream stream;
			std::string filename;

			for (int i = 0; i < 10 && *p == '@'; i++) {
				try {
					stream.open(p + 1, std::ios::in);
					stream >> filename;
					p = filename.c_str();
				}
				catch (...) {
					return "";
				}
			}
			if (*p == '@') {
				error("getwfdb: files nested too deeply\n");
				p = "";
			}
			return (p);
		}
		
		/* _parsePath constructs a linked list of path components by splitting
		 * its string input (usually the value of WFDB). */
		int _parsePath(const char *paths)
		{
			_path_list.clear();

			/* Do nothing else if no path string was supplied. */
			if (paths == nullptr) return (0);

			const char* p = paths;

			/* Now construct the wfdb_path_list from the contents of p. */
			while (*p) {
				/* Find the beginning of the next component (skip whitespace). */
				while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
					p++;
				const char* start = p--;
				int current_type = WFDB_LOCAL;
				/* Find the end of the current component. */
				do {
					if (*p == ':' && *(p + 1) == '/'  && *(p + 2) == '/') current_type = WFDB_NET;
					else if (*p == ';' /* definitely a component delimiter */
						|| *p == ' '
						|| *p == '\t'
						|| *p == '\n'
						|| *p == '\r'
						|| *p == '\0') break;
					p++;
				} while (true);				

				/* current component begins at p, ends at q-1 */
				_path_component c1;
				c1.prefix = std::string(start, p - start);
				c1.type = current_type;
				_path_list.push_back(c1);
				if (*p) p++;
			}
			return (0);
		}


		std::string _path;
		std::string _pathInit;
		bool _verbose;
	};
	class LocalFile
	{
		Context* _context;
	public:
		explicit LocalFile(Context * context): _context(context), irec{}
		{
		}

		std::string wfdb_filename;

		/* wfdbfile returns the pathname or URL of a WFDB file. */
		const char* wfdbfile(char *s, char *record)
		{
			WFDB_FILE *ifile;

			if (s == nullptr && record == nullptr)
				return wfdb_filename.c_str();

			/* Remove trailing .hea, if any, from record name. */
			wfdb_striphea(record);

			if ((ifile = wfdb_open(s, record, WFDB_READ))) {
				(void)wfdb_fclose(ifile);
				return (wfdb_filename.c_str());
			}
			return (nullptr);
		}

		/* The next four functions read and write integers in PDP-11 format, which is
		common to both MIT and AHA database files.  The purpose is to achieve
		interchangeability of binary database files between machines which may use
		different byte layouts.  The routines below are machine-independent; in some
		cases (notably on the PDP-11 itself), taking advantage of the native byte
		layout can improve the speed.  For 16-bit integers, the low (least significant)
		byte is written (read) before the high byte; 32-bit integers are represented as
		two 16-bit integers, but the high 16 bits are written (read) before the low 16
		bits. These functions, in common with other WFDB library functions, assume that
		a byte is 8 bits, a "short" is 16 bits, an "int" is at least 16 bits, and a
		"long" is at least 32 bits.  The last two assumptions are valid for ANSI C
		compilers, and for almost all older C compilers as well.  If a "short" is not
		16 bits, it may be necessary to rewrite wfdb_g16() to obtain proper sign
		extension. */

		/* read a 16-bit integer in PDP-11 format */
		int wfdb_g16(WFDB_FILE *fp)
		{
			const int x = wfdb_getc(fp);
			return int(short((wfdb_getc(fp) << 8) | (x & 0xff)));
		}

		/* read a 32-bit integer in PDP-11 format */
		long wfdb_g32(WFDB_FILE *fp)
		{
			const long x = wfdb_g16(fp);
			const long y = wfdb_g16(fp);
			return ((x << 16) | (y & 0xffff));
		}

		/* write a 16-bit integer in PDP-11 format */
		void wfdb_p16(unsigned int x, WFDB_FILE *fp)
		{
			(void)wfdb_putc(static_cast<char>(x), fp);
			(void)wfdb_putc(static_cast<char>(x >> 8), fp);
		}

		/* write a 32-bit integer in PDP-11 format */
		void wfdb_p32(long x, WFDB_FILE *fp)
		{
			wfdb_p16(static_cast<unsigned int>(x >> 16), fp);
			wfdb_p16(static_cast<unsigned int>(x), fp);
		}




		

		/* wfdb_vasprintf formats a string in the same manner as vsprintf, and
		allocates a new buffer that is sufficiently large to hold the result.
		The original buffer, if any, is freed afterwards (meaning that, unlike
		vsprintf, it is permissible to use the original buffer as a '%s'
		argument.) */
		int wfdb_vasprintf(char **buffer, const char *format, va_list arguments)
		{
			int length;
			va_list arguments2;

			/* make an initial guess at how large the buffer should be */
			int bufsize = 2 * strlen(format) + 1;

			char* oldbuffer = *buffer;
			*buffer = nullptr;

			while (1) {
				/* do not use SALLOC; avoid recursive calls to wfdb_error */
				if (*buffer)
					free(*buffer);
				*buffer =new char[bufsize];
				if (!(*buffer)) {
					if (wfdb_me_fatal()) {
						fprintf(stderr, "WFDB: out of memory\n");
						exit(1);
					}
					length = 0;
					break;
				}

				va_copy(arguments2, arguments);
				length = _vsnprintf(*buffer, bufsize, format, arguments2);
				va_end(arguments2);

				/* some pre-standard versions of 'vsnprintf' return -1 if the
				   formatted string does not fit in the buffer; in that case,
				   try again with a larger buffer */
				if (length < 0)
					bufsize *= 2;
				/* standard 'vsnprintf' returns the actual length of the
				   formatted string */
				else if (length >= bufsize)
					bufsize = length + 1;
				else
					break;
			}

			if (oldbuffer)
				free(oldbuffer);
			return (length);
		}

		/* wfdb_asprintf formats a string in the same manner as sprintf, and
		allocates a new buffer that is sufficiently large to hold the
		result. */
		int wfdb_asprintf(char **buffer, const char *format, ...)
		{
			va_list arguments;
			int length;

			va_start(arguments, format);
			length = wfdb_vasprintf(buffer, format, arguments);
			va_end(arguments);

			return (length);
		}


		/* The wfdb_fprintf function handles all formatted output to files.  It is
		used in the same way as the standard fprintf function, except that its first
		argument is a pointer to a WFDB_FILE rather than a FILE. */

		int wfdb_fprintf(WFDB_FILE *wp, const char *format, ...)
		{
			va_list args;

			va_start(args, format);
			int ret = vfprintf(wp->fp, format, args);
			va_end(args);
			return (ret);
		}

		std::string spr1(const char* RECORD, const char* TYPE) {
			return ((*TYPE == '\0') ? RECORD : std::string(RECORD) + "." + TYPE);
		}
		std::string spr2(const char* RECORD, const char* TYPE) {
			return ((*TYPE == '\0') ? std::string(RECORD) + "." : std::string(RECORD) + "." + TYPE);
		}


		char irec[WFDB_MAXRNL + 1]; /* current record name, set by wfdb_setirec */

		/* wfdb_open is used by other WFDB library functions to open a database file
		for reading or writing.  wfdb_open accepts two string arguments and an integer
		argument.  The first string specifies the file type ("hea", "atr", etc.),
		and the second specifies the record name.  The integer argument (mode) is
		either WFDB_READ or WFDB_WRITE.  Note that a function which calls wfdb_open
		does not need to know the filename itself; thus all system-specific details of
		file naming conventions can be hidden in wfdb_open.  If the first argument is
		"-", or if the first argument is "hea" and the second is "-", wfdb_open
		returns a file pointer to the standard input or output as appropriate.  If
		either of the string arguments is null or empty, wfdb_open takes the other as
		the file name.  Otherwise, it constructs the file name by concatenating the
		string arguments with a "." between them.  If the file is to be opened for
		reading, wfdb_open searches for it in the list of directories obtained from
		getwfdb(); output files are normally created in the current directory.  By
		prefixing the record argument with appropriate path specifications, files can
		be opened in any directory, provided that the WFDB path includes a null
		(empty) component.

		Beginning with version 10.0.1, the WFDB library accepts whitespace (space, tab,
		or newline characters) as path component separators under any OS.  Multiple
		consecutive whitespace characters are treated as a single path component
		separator.  Use a '.' to specify the current directory as a path component when
		using whitespace as a path component separator.

		If the WFDB path includes components of the forms 'http://somewhere.net/mydata'
		or 'ftp://somewhere.else/yourdata', the sequence '://' is explicitly recognized
		as part of a URL prefix (under any OS), and the ':' and '/' characters within
		the '://' are not interpreted further.  Note that the MS-DOS '\' is *not*
		acceptable as an alternative to '/' in a URL prefix.  To make WFDB paths
		containing URL prefixes more easily (human) readable, use whitespace for path
		component separators.

		WFDB file names are usually formed by concatenating the record name, a ".", and
		the file type, using the spr1 macro (above).  If an input file name, as
		constructed by spr1, does not match that of an existing file, wfdb_open uses
		spr2 to construct an alternate file name.  In this form, the file type is
		truncated to no more than 3 characters (as MS-DOS does).  When searching for
		input files, wfdb_open tries both forms with each component of the WFDB path
		before going on to the next path component.

		If the record name is empty, wfdb_open swaps the record name and the type
		string.  If the type string (after swapping, if necessary) is empty, spr1 uses
		the record name as the literal file name, and spr2 uses the record name with an
		appended "." as the file name.

		In environments in which ISO 9660 version numbers are visible in CD-ROM file
		names, define the symbol FIXISOCD.  This causes spr2 to append the
		characters ";1" (the version number for an ordinary file) to the file names
		it generates.  This feature is needed in order to read post-1992 CD-ROMs
		with pre-5.0 versions of the Macintosh "ISO 9660 File Access" software,
		with some versions of HP-UX, and possibly in other environments as well.

		Pre-10.0.1 versions of this library that were compiled for environments other
		than MS-DOS used file names in the format TYPE.RECORD.  This file name format
		is no longer supported. */

		WFDB_FILE *wfdb_open(const char *type, const char *record, const int mode)
		{
			char *q, *buf = nullptr;
			std::string r;
			WFDB_FILE *ifile;

			/* If the type (s) is empty, replace it with an empty string so that
			   strcmp(s, ...) will not segfault. */
			if (type == nullptr) type = "";

			/* If the record name is empty, use s as the record name and replace s
			   with an empty string. */
			if (record == nullptr || *record == '\0') {
				if (*type){
					record = type; 
					type = "";
				}
				else return (nullptr);	/* failure -- both components are empty */
			}

			/* Check to see if standard input or output is requested. */
			if (strcmp(record, "-") == 0)
			{
				if (mode == WFDB_READ) {
					static WFDB_FILE wfdb_stdin;

					wfdb_stdin.type = WFDB_LOCAL;
					wfdb_stdin.fp = stdin;
					return (&wfdb_stdin);
				}
				static WFDB_FILE wfdb_stdout;

				wfdb_stdout.type = WFDB_LOCAL;
				wfdb_stdout.fp = stdout;
				return (&wfdb_stdout);
			}

			/* If the record name ends with '/', expand it by adding another copy of
			   the final element (e.g., 'abc/123/' becomes 'abc/123/123'). */
			std::experimental::filesystem::path path(record);
			if(*(path.generic_string().rbegin())=='/')
			{
				r = path.append(path.parent_path().filename()).generic_string();
			} 
			else
			{
				r = path.generic_string();
			}			

			/* If the file is to be opened for output, use the current directory.
			   An output file can be opened in another directory if the path to
			   that directory is the first part of 'record'. */
			wfdb_filename = spr1(r.c_str(), type);
			if (mode == WFDB_WRITE) {
				return (wfdb_fopen(wfdb_filename.c_str(), WB));
			}
			if (mode == WFDB_APPEND) {
				return (wfdb_fopen(wfdb_filename.c_str(), AB));
			}

			/* If the filename begins with 'http://' or 'https://', it's a URL.  In
			   this case, don't search the WFDB path, but add its parent directory
			   to the path if the file can be read. */
			if (r.compare(0, 7,"http://") == 0 || r.compare(0, 8, "https://") == 0) {
				if ((ifile = wfdb_fopen(wfdb_filename.c_str(), RB)) != nullptr) {
					/* Found it! Add its path info to the WFDB path. */
					_context->addPath(wfdb_filename.c_str());
					return (ifile);
				}
			}

			for (std::vector<_path_component>::iterator it = _context->pathList().begin(); it!=_context->pathList().end(); ++it)
			{
				struct _path_component c0 = *it;
			
				std::string long_filename;
				std::string buf;
				const char* wfdb = c0.prefix.c_str();
				while (*wfdb) {
					if (*wfdb == '%') {
						/* Perform substitutions in the WFDB path where '%' is found */
						wfdb++;
						if (*wfdb == 'r') {
							/* '%r' -> record name */
							buf += irec;
							wfdb++;
						}
						else if ('1' <= *wfdb && *wfdb <= '9' && *(wfdb + 1) == 'r') {
							/* '%Nr' -> first N characters of record name */
							const int n = *wfdb - '0';
							buf.append(irec, n);
							wfdb += 2;
						}
						else {
							/* '%X' -> X, if X is neither 'r', nor a non-zero digit followed by 'r' */
							buf.append(1, *wfdb);
							*wfdb++;
						}
					}
					else {
						buf.append(1, *wfdb);
						*wfdb++;
					}
				}
				/* Unless the WFDB component was empty, or it ended with a directory
				   separator, append a directory separator to wfdb_filename;  then
				   append the record and type components.  Note that names of remote
				   files (URLs) are always constructed using '/' separators, even if
				   the native directory separator is '\' (MS-DOS) or ':' (Macintosh).
				*/
				if (!buf.empty() &&
					c0.type == WFDB_NET && 
					*buf.rbegin()!= DSEP && (c0.type == WFDB_NET || *buf.rbegin()!= ':'))
				{
					buf.append(1, DSEP);
				}
				buf += r;

				wfdb_filename= spr1(buf.c_str(), type);
				if ((ifile = wfdb_fopen(wfdb_filename.c_str(), RB)) != NULL) {
					/* Found it! Add its path info to the WFDB path. */
					_context->addPath(wfdb_filename.c_str());
					return (ifile);
				}
				/* Not found -- try again, using an alternate form of the name,
				   provided that that form is distinct. */
				long_filename = wfdb_filename;
				
				wfdb_filename= spr2(buf.c_str(), type);
				if (wfdb_filename!=long_filename &&
					(ifile = wfdb_fopen(wfdb_filename.c_str(), RB)) != nullptr) {
					_context->addPath(wfdb_filename.c_str());
					return (ifile);
				}
			}
			return (nullptr);
		}

		/* wfdb_checkname checks record and annotator names -- they must not be empty,
		   and they must contain only letters, digits, hyphens, tildes, underscores, and
		   directory separators. */

		int wfdb_checkname(char *p, char *s)
		{
			do {
				if (('0' <= *p && *p <= '9') || *p == '_' || *p == '~' || *p == '-' ||
					*p == DSEP ||
#ifdef MSDOS
					*p == ':' || *p == '/' ||
#endif
					('a' <= *p && *p <= 'z') || ('A' <= *p && *p <= 'Z'))
					p++;
				else {
					_context->error("init: illegal character %d in %s name\n", *p, s);
					return (-1);
				}
			} while (*p);
			return (0);
		}

		/* wfdb_setirec saves the current record name (its argument) in irec (defined
		above) to be substituted for '%r' in the WFDB path by wfdb_open as necessary.
		wfdb_setirec is invoked by isigopen (except when isigopen is invoked
		recursively to open a segment within a multi-segment record) and by annopen
		(when it is about to open a file for input). */

		void wfdb_setirec(const char *p)
		{
			const char *r;
			int len;

			for (r = p; *r; r++)
				if (*r == DSEP) p = r + 1;	/* strip off any path information */
#ifdef MSDOS
				else if (*r == ':') p = r + 1;
#endif
			len = strlen(p);
			if (len > WFDB_MAXRNL)
				len = WFDB_MAXRNL;
			if (strcmp(p, "-")) {       /* don't record '-' (stdin) as record name */
				strncpy(irec, p, len);
				irec[len] = '\0';
			}
		}

		char *wfdb_getirec(void)
		{
			return (*irec ? irec : NULL);
		}

		/* Remove trailing '.hea' from a record name, if present. */
		void wfdb_striphea(char *p)
		{
			if (p) {
				int len = strlen(p);

				if (len > 4 && strcmp(p + len - 4, ".hea") == 0)
					p[len - 4] = '\0';
			}
		}


		/* WFDB file I/O functions

		The WFDB library normally reads and writes local files.  If libcurl
		(http://curl.haxx.se/) is available, the WFDB library can also read files from
		any accessible World Wide Web (HTTP) or FTP server.  (Writing files to a remote
		WWW or FTP server may be supported in the future.)

		If you do not wish to allow access to remote files, or if libcurl is not
		available, simply define the symbol WFDB_NETFILES as 0 when compiling the WFDB
		library.  If the symbol WFDB_NETFILES is zero, wfdblib.h defines wfdb_fread as
		fread, wfdb_fwrite as fwrite, etc.;  thus in this case, the I/O is performed
		using the standard C I/O functions, and the function definitions in the next
		section are not compiled.  This behavior exactly mimics that of versions of the
		WFDB library earlier than version 10.0.1 (which did not support remote file
		access), with no additional run-time overhead.

		If WFDB_NETFILES is non-zero, however, these functions are compiled.  The
		WFDB_FILE pointers that are among the arguments to these functions point to
		objects that may contain either (local) FILE handles or (remote) NETFILE
		handles, depending on the value of the 'type' member of the WFDB_FILE object.
		All access to local files is handled by passing the 'fp' member of the
		WFDB_FILE object to the appropriate standard C I/O function.  Access to remote
		files via http or ftp is handled by passing the 'netfp' member of the WFDB_FILE
		object to the appropriate libcurl function(s).

		In order to read remote files, the WFDB environment variable should include
		one or more components that specify http:// or ftp:// URL prefixes.  These
		components are concatenated with WFDB file names to obtain complete URLs.  For
		example, if the value of WFDB is
		  /usr/local/database http://dilbert.bigu.edu/wfdb /cdrom/database
		then an attempt to read the header file for record xyz would look first for
		/usr/local/database/xyz.hea, then http://dilbert.bigu.edu/wfdb/xyz.hea, and
		finally /cdrom/database/xyz.hea.  The second and later possibilities would
		be checked only if the file had not been found already.  As a practical matter,
		it would be best in almost all cases to search all available local file systems
		before looking on remote http or ftp servers, but the WFDB library allows you
		to set the search order in any way you wish, as in this example.
		*/



		/* The definition of nf_vfprintf (which is a stub) has been moved;  it is
		   now just before wfdb_fprintf, which refers to it.  There is no completely
		   portable way to make a forward reference to a static (local) function. */

		void wfdb_clearerr(WFDB_FILE *wp)
		{
				clearerr(wp->fp);
		}

		int wfdb_feof(WFDB_FILE *wp)
		{
			return (feof(wp->fp));
		}

		int wfdb_ferror(WFDB_FILE *wp)
		{
			return (ferror(wp->fp));
		}

		int wfdb_fflush(WFDB_FILE *wp)
		{
			if (wp == nullptr) {	/* flush all WFDB_FILEs */
				return (fflush(nullptr));
			}
			return (fflush(wp->fp));
		}

		char* wfdb_fgets(char *s, int size, WFDB_FILE *wp)
		{
			return (fgets(s, size, wp->fp));
		}

		size_t wfdb_fread(void *ptr, size_t size, size_t nmemb, WFDB_FILE *wp)
		{
			return (fread(ptr, size, nmemb, wp->fp));
		}

		int wfdb_fseek(WFDB_FILE *wp, long int offset, int whence)
		{
			return(fseek(wp->fp, offset, whence));
		}

		long wfdb_ftell(WFDB_FILE *wp)
		{
			return (ftell(wp->fp));
		}

		size_t wfdb_fwrite(void *ptr, size_t size, size_t nmemb, WFDB_FILE *wp)
		{
			return (fwrite(ptr, size, nmemb, wp->fp));
		}

		int wfdb_getc(WFDB_FILE *wp)
		{
			return (getc(wp->fp));
		}

		int wfdb_putc(int c, WFDB_FILE *wp)
		{
			return (putc(c, wp->fp));
		}

		int wfdb_fclose(WFDB_FILE *wp)
		{
			const int status = fclose(wp->fp);
			if (wp->fp != stdin)
				delete wp;
			return (status);
		}

		WFDB_FILE *wfdb_fopen(const char *fname, const char *mode)
		{
			if (fname == nullptr || *fname=='\0' || strstr(fname, ".."))
				return (nullptr);
			const char* p =strrchr(fname, '/');

			WFDB_FILE* wp = new WFDB_FILE;
			fopen_s(&wp->fp, fname, mode);
			if (wp->fp) {
				wp->type = WFDB_LOCAL;
				return (wp);
			}
			if (strcmp(mode, WB) == 0 || strcmp(mode, AB) == 0) {
				std::experimental::filesystem::path path(fname);
				if(create_directories(path.parent_path())){
					fopen_s(&wp->fp, fname, mode);
					if (wp->fp) {
						wp->type = WFDB_LOCAL;
						return (wp);
					}
				}
			}
			delete wp;
			return (nullptr);
		}
	};
}
