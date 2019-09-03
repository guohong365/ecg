﻿#pragma once

#include "wfdblib.h"
#include <limits.h>
#include <time.h>
#include <string>
#include "WFDBFile.h"

namespace wfdb{
	typedef struct _signal_info
	{
		std::string fname;	/* filename of signal file */
		std::string desc;		/* signal description */
		std::string units;	/* physical units (mV unless otherwise specified) */
		GainType gain;	/* gain (ADC units/physical unit, 0: uncalibrated) */
		SampleType initVale; 	/* initial value (that of sample number 0) */
		GroupType group;	/* signal group number */
		int fmt;		/* format (8, 16, etc.) */
		int spf;		/* samples per frame (>1 for oversampled signals) */
		int bsize;		/* block size (for character special files only) */
		int adcres;		/* ADC resolution in bits */
		int adczero;	/* ADC output given 0 VDC input */
		int baseline;	/* ADC output given 0 physical units input */
		long nsamp;		/* number of samples (0: unspecified) */
		int cksum;		/* 16-bit checksum of all samples */
	} SignalInfo;

class SignalOperator
{
	Context* _context;
public:
	/* Shared local data */

	/* These variables are set by readheader, and contain information about the
	   signals described in the most recently opened header file.
	*/
	 unsigned maxhsig;	/* # of hsdata structures pointed to by hsd */
	 WFDB_FILE *hheader;	/* file pointer for header file */
	 struct hsdata {
		WFDB_Siginfo info;		/* info about signal from header */
		long start;			/* signal file byte offset to sample 0 */
		int skew;			/* intersignal skew (in frames) */
	} **hsd;


	/* Variables in this group are also set by readheader, but may be reset (by,
	   e.g., setsampfreq, setbasetime, ...).  These are used by strtim, timstr,
	   etc., for converting among sample intervals, counter values, elapsed times,
	   and absolute times and dates; they are recorded when writing header files
	   using newheader, setheader, and setmsheader.  Changing these variables has
	   no effect on the data read by getframe (or getvec) or on the data written by
	   putvec (although changes will affect what is written to output header files
	   by setheader, etc.).  An application such as xform can use independent
	   sampling frequencies and different base times or dates for input and output
	   signals, but only one set of these parameters is available at any given time
	   for use by the strtim, timstr, etc., conversion functions.
	*/
	 FrequencyType ffreq;	/* frame rate (frames/second) */
	 FrequencyType ifreq;	/* samples/second/signal returned by getvec */
	 FrequencyType sfreq;	/* samples/second/signal read by getvec */
	 FrequencyType cfreq;	/* counter frequency (ticks/second) */
	 long btime;		/* base time (milliseconds since midnight) */
	 DateType bdate;		/* base date (Julian date) */
	 TimeType nsamples;	/* duration of signals (in samples) */
	 double bcount;		/* base count (counter value at sample 0) */
	 long prolog_bytes;	/* length of prolog, as told to wfdbsetstart
					   (used only by setheader, if output signal
					   file(s) are not open) */

					   /* The next set of variables contains information about multi-segment records.
						  The first two of them ('segments' and 'in_msrec') are used primarily as
						  flags to indicate if a record contains multiple segments.  Unless 'in_msrec'
						  is set already, readheader sets 'segments' to the number of segments
						  indicated in the header file it has most recently read (0 for a
						  single-segment record).  If it reads a header file for a multi-segment
						  record, readheader also sets the variables 'msbtime', 'msbdate', and
						  'msnsamples'; allocates and fills 'segarray'; and sets 'segp' and 'segend'.
						  Note that readheader's actions are not restricted to records opened for
						  input.

						  If isigopen finds that 'segments' is non-zero, it sets 'in_msrec' and then
						  invokes readheader again to obtain signal information from the header file
						  for the first segment, which must be a single-segment record (readheader
						  refuses to open a header file for a multi-segment record if 'in_msrec' is
						  set).

						  When creating a header file for a multi-segment record using setmsheader,
						  the variables 'msbtime', 'msbdate', and 'msnsamples' are filled in by
						  setmsheader based on btime and bdate for the first segment, and on the
						  sum of the 'nsamp' fields for all segments.  */
	int segments;		/* number of segments found by readheader() */
	int in_msrec;		/* current input record is: 0: a single-segment
					   record; 1: a multi-segment record */
	long msbtime;		/* base time for multi-segment record */
	DateType msbdate;	/* base date for multi-segment record */
	TimeType msnsamples;	/* duration of multi-segment record */
	WFDB_Seginfo *segarray, *segp, *segend;
	/* beginning, current segment, end pointers */

/* These variables relate to open input signals. */
	unsigned maxisig;	/* max number of input signals */
	unsigned maxigroup;	/* max number of input signal groups */
	unsigned nisig;		/* number of open input signals */
	unsigned nigroup;	/* number of open input signal groups */
	unsigned ispfmax;	/* max number of samples of any open signal
					   per input frame */
	struct isdata {		/* unique for each input signal */
		SignalInfo info;		/* input signal information */
		SampleType samp;		/* most recent sample read */
		int skew;			/* intersignal skew (in frames) */
	} **isd;
	struct igdata {		/* shared by all signals in a group (file) */
		int data;			/* raw data read by r*() */
		int datb;			/* more raw data used for bit-packed formats */
		WFDB_FILE *fp;		/* file pointer for an input signal group */
		long start;			/* signal file byte offset to sample 0 */
		int bsize;			/* if non-zero, all reads from the input file  are in multiples of bsize bytes */
		char *buf;			/* pointer to input buffer */
		char *bp;			/* pointer to next location in buf[] */
		char *be;			/* pointer to input buffer endpoint */
		char count;			/* input counter for bit-packed signal */
		char seek;			/* 0: do not seek on file, 1: seeks permitted */
		int stat;			/* signal file status flag */
	} **igd;
	SampleType *tvector;	/* getvec workspace */
	SampleType *uvector;	/* isgsettime workspace */
	SampleType *vvector;	/* tnextvec workspace */
	int tuvlen;		/* lengths of tvector and uvector in samples */
	TimeType istime;	/* time of next input sample */
	int ibsize;		/* default input buffer size */
	unsigned skewmax;	/* max skew (frames) between any 2 signals */
	SampleType *dsbuf;	/* deskewing buffer */
	int dsbi;		/* index to oldest sample in dsbuf (if < 0,
					   dsbuf does not contain valid data) */
	unsigned dsblen;		/* capacity of dsbuf, in samples */
	unsigned framelen;	/* total number of samples per frame */
	int gvmode = DEFWFDBGVMODE;	/* getvec mode */
	int gvc;			/* getvec sample-within-frame counter */
	int isedf;		/* if non-zero, record is stored as EDF/EDF+ */
	SampleType *sbuf = NULL;	/* buffer used by sample() */
	int sample_vflag;	/* if non-zero, last value returned by sample()
					   was valid */

					   /* These variables relate to output signals. */
	unsigned maxosig;	/* max number of output signals */
	unsigned maxogroup;	/* max number of output signal groups */
	unsigned nosig;		/* number of open output signals */
	unsigned nogroup;	/* number of open output signal groups */
	WFDB_FILE *oheader;	/* file pointer for output header file */
	WFDB_FILE *outinfo;	/* file pointer for output info file */
	struct osdata {		/* unique for each output signal */
		SignalInfo info;		/* output signal information */
		SampleType samp;		/* most recent sample written */
		int skew;			/* skew to be written by setheader() */
	} **osd;
	struct ogdata {		/* shared by all signals in a group (file) */
		int data;			/* raw data to be written by w*() */
		int datb;			/* more raw data used for bit-packed formats */
		WFDB_FILE *fp;		/* file pointer for output signal */
		long start;			/* byte offset to be written by setheader() */
		int bsize;			/* if non-zero, all writes to the output file
					   are in multiples of bsize bytes */
		char *buf;			/* pointer to output buffer */
		char *bp;			/* pointer to next location in buf[]; */
		char *be;			/* pointer to output buffer endpoint */
		char count;		/* output counter for bit-packed signal */
		signed char seek;		/* 1: seek works, -1: seek doesn't work,
					   0: unknown */
		char force_flush;		/* flush even if seek doesn't work */
		char nrewind;		/* number of bytes to seek backwards
					   after flushing */
	} **ogd;
	TimeType ostime;	/* time of next output sample */
	int obsize;		/* default output buffer size */

	/* These variables relate to info strings. */
	char **pinfo;	/* array of info string pointers */
	int nimax;	/* number of info string pointers allocated */
	int ninfo;	/* number of info strings read */

	/* Allocate workspace for up to n input signals. */
	int allocisig(unsigned int n)
	{
		if (maxisig < n) {
			unsigned m = maxisig;
			isd = new isdata*[n];			
			while (m < n) {
				isd[m] = new isdata;
				m++;
			}
			maxisig = n;
		}
		return (maxisig);
	}

	/* Allocate workspace for up to n input signal groups. */
	int allocigroup(unsigned int n)
	{
		if (maxigroup < n) {
			unsigned m = maxigroup;
			igd = new igdata*[n];
			while (m < n) {
				igd[m] = new igdata;
				m++;
			}
			maxigroup = n;
		}
		return (maxigroup);
	}

	/* Allocate workspace for up to n output signals. */
	int allocosig(unsigned int n)
	{
		if (maxosig < n) {
			unsigned m = maxosig;
			osd = new osdata*[n];
			while (m < n) {
				osd[m] = new osdata;
				m++;
			}
			maxosig = n;
		}
		return (maxosig);
	}

	/* Allocate workspace for up to n output signal groups. */
	int allocogroup(unsigned int n)
	{
		if (maxogroup < n) {
			unsigned m = maxogroup;
			ogd = new ogdata*[n];
			while (m < n) {
				ogd[m] = new ogdata;
				m++;
			}
			maxogroup = n;
		}
		return (maxogroup);
	}

	static int isfmt(int f)
	{
		static int fmt_list[WFDB_NFMTS] = WFDB_FMT_LIST;

		for (int i = 0; i < WFDB_NFMTS; i++)
			if (f == fmt_list[i]) return (1);
		return (0);
	}

	int copysi(SignalInfo *to, SignalInfo *from)
	{
		if (to == nullptr || from == nullptr) return (0);
		*to = *from;
		return (1);
	}

	/* Code for handling variable-layout multi-segment records

		The following code (together with minor changes elsewhere in this file)
	   was written to permit reading a record consisting of multiple segments
	   on-the-fly, even if the segments do not contain the same signals or the
	   same number of signals.  If the gain or baseline of any signal changes
	   from segment to segment, the code in this section adjusts for the changes
	   transparently, so that a WFDB application can assume that all signals
	   are always present in the same order, with constant gains and baselines
	   as specified in the .hea file for the first segment.  This .hea file
	   is in the same format as an ordinary .hea file, but if its length is
	   specified as zero samples, it is recognized as a "layout header".  In
	   this case, the signal file name is conventionally given as "~" (although
	   any name is acceptable), and the format is given as 0 (required).

	   The "layout header" contains a signal description line for each signal
	   of interest that appears anywhere in the record.  The gain and baseline
	   specified in the layout header are those that will apply to samples
	   throughout the record (sigmap, below, scales and shifts the raw samples
	   as needed).

	   If a gap occurs between the end of one segment and the beginning of the
	   next, a special "null" segment can be listed in the master .hea file,
	   like this:
		   ~ 4590
	   The segment name, "~", does not correspond to a real .hea file, but the
	   number that follows indicates the length of the gap in sample intervals.
	 */

	int need_sigmap;
	int maxvsig;
	int nvsig;
	int tspf;
	int vspfmax;
	std::vector<isdata*> vsd;
	SampleType *ovec;

	struct SignalMapInfo {
		std::string desc;
		double gain, scale, offset;
		SampleType sample_offset;
		SampleType baseline;
		int index;
		int spf;
	}*smi;

	void sigmap_cleanup()
	{
		need_sigmap = nvsig = tspf = vspfmax = 0;
		SFREE(ovec);
		if (smi) {
			SFREE(smi);
		}

		if (vsd.size()) {
			struct isdata *is;

			while (maxvsig)
				if ((is = vsd[--maxvsig])) {
					delete is;
				}
			vsd.clear();
		}
	}

	int make_vsd()
	{
		if (nvsig != nisig) {
			_context->error("make_vsd: oops! nvsig = %d, nisig = %d\n", nvsig, nisig);
			return (-1);
		}
		if (maxvsig < nvsig) {
			unsigned m = maxvsig;
			while (m < nvsig) {
				vsd.push_back(new isdata);
				m++;
			}
			maxvsig = nvsig;
		}

		for (int i = 0; i < nvsig; i++)
			copysi(&vsd[i]->info, &isd[i]->info);

		return (nvsig);
	}

	int sigmap_init()
	{
		int i;
		int j;
		int k;
		int kmax;
		int s;
		int ivmin;
		int ivmax;
		double ovmin;
		double ovmax;
		SignalMapInfo *ps;

		/* is this the layout segment?  if so, set up output side of map */
		if (in_msrec && ovec == NULL && isd[0]->info.nsamp == 0L) {
			need_sigmap = 1;

			/* The number of virtual signals is the number of signals defined
			   in the layout segment. */
			nvsig = nisig;
			vspfmax = ispfmax;
			for (s = tspf = 0; s < nisig; s++)
				tspf += isd[s]->info.spf;
			smi=new SignalMapInfo[tspf];

			for (i = s = 0; i < nisig; i++) {
				smi[s].desc= isd[i]->info.desc;
				smi[s].gain = isd[i]->info.gain;
				smi[s].baseline = isd[i]->info.baseline;
				k = smi[s].spf = isd[i]->info.spf;
				for (j = 1; j < k; j++)
					smi[s + j] = smi[s];
				s += k;
			}
			ovec= new SampleType[tspf];
			return (make_vsd());
		}
		if (need_sigmap) {	/* set up the input side of the map */
			for (s = 0; s < tspf; s++) {
				smi[s].index = 0;
				smi[s].scale = 0.;
				smi[s].offset = 0.;
				smi[s].sample_offset = WFDB_INVALID_SAMPLE;
			}
			ispfmax = vspfmax;

			if (isd[0]->info.fmt == 0 && nisig == 1)
				return (0);    /* the current segment is a null record */

			for (i = j = 0; i < nisig; j += isd[i++]->info.spf)
				for (s = 0; s < tspf; s += smi[s].spf)
					if (smi[s].desc== isd[i]->info.desc) {
						if ((kmax = smi[s].spf) != isd[i]->info.spf) {
							_context->error("sigmap_init: unexpected spf for signal %d in segment %s\n",i, segp->recname);
							if (kmax > isd[i]->info.spf)
								kmax = isd[i]->info.spf;
						}
						for (k = 0; k < kmax; k++) {
							ps = &smi[s + k];
							ps->index = j + k;
							ps->scale = ps->gain / isd[i]->info.gain;
							if (ps->scale < 1.0)
								_context->error(
									"sigmap_init: loss of precision in signal %d in segment %s\n",
									i, segp->recname);
							ps->offset = ps->baseline -
								ps->scale * isd[i]->info.baseline + 0.5;

							/* If it is possible to add an additional
							   offset such that all possible output values
							   will fit into a positive signed integer, we
							   can use the "fast" case in sigmap, below. */
							switch (isd[i]->info.fmt) {
							case 80: ivmin = -0x80; ivmax = 0x7f; break;
							case 310:
							case 311: ivmin = -0x200; ivmax = 0x1ff; break;
							case 212: ivmin = -0x800; ivmax = 0x7ff; break;
							case 16:
							case 61:
							case 160: ivmin = -0x8000; ivmax = 0x7fff; break;
							case 24: ivmin = -0x800000; ivmax = 0x7fffff; break;
							default:
								ivmin = WFDB_SAMPLE_MIN;
								ivmax = WFDB_SAMPLE_MAX;
								break;
							}
							ovmin = ivmin * ps->scale + ps->offset;
							ovmax = ivmax * ps->scale + ps->offset;
							if (ovmin < ovmax &&
								ovmin >= WFDB_SAMPLE_MIN + 1 &&
								ovmax <= WFDB_SAMPLE_MAX &&
								ovmax - ovmin + 1 < WFDB_SAMPLE_MAX) {
								ps->sample_offset = ovmin - 1;
								ps->offset -= ps->sample_offset;
							}
							else {
								ps->sample_offset = 0;
							}
						}
						break;
					}
		}

		else {	/* normal record, or multisegment record without a dummy
			   header */
			nvsig = nisig;
			return (make_vsd());
		}

		return (0);
	}

	int sigmap(SampleType *vector)
	{
		int i;
		double v;

		for (i = 0; i < tspf; i++)
			ovec[i] = vector[i];

		for (i = 0; i < tspf; i++) {
			if (ovec[smi[i].index] == WFDB_INVALID_SAMPLE)
				vector[i] = WFDB_INVALID_SAMPLE;
			else {
				/* Scale the input sample and round it to the nearest
				   integer.  Halfway cases are always rounded up (10.5 is
				   rounded to 11, but -10.5 is rounded to -10.)  Note that
				   smi[i].offset already includes an extra 0.5, so we
				   simply need to calculate the floor of v. */
				v = ovec[smi[i].index] * smi[i].scale + smi[i].offset;
				if (smi[i].sample_offset) {
					/* Fast case: if we can guarantee that v is always
					   positive and the following calculation cannot
					   overflow, we can avoid additional floating-point
					   operations. */
					vector[i] = (SampleType)v + smi[i].sample_offset;
				}
				else {
					/* Slow case: we need to check bounds and handle
					   negative values. */
					if (v >= 0) {
						if (v <= WFDB_SAMPLE_MAX)
							vector[i] = (SampleType)v;
						else
							vector[i] = WFDB_SAMPLE_MAX;
					}
					else {
						if (v >= WFDB_SAMPLE_MIN) {
							vector[i] = (SampleType)v;
							if (vector[i] > v)
								vector[i]--;
						}
						else {
							vector[i] = WFDB_SAMPLE_MIN;
						}
					}
				}
			}
		}
		return (tspf);
	}

	/* end of code for handling variable-layout records */

	/* get header information from an EDF file */
	int edfparse(WFDB_FILE *ifile)
	{
		static char buf[80], *edf_fname, *p;
		double *pmax, *pmin, spr, baseline;
		int format, i, s, nsig, offset, day, month, year, hour, minute, second;
		long adcrange, *dmax, *dmin, nframes;

		edf_fname = wfdbfile(NULL, NULL);

		/* Read the first 8 bytes and check for the magic string.  (This might
		   accept some non-EDF files.) */
		wfdb_fread(buf, 1, 8, ifile);
		if (strncmp(buf, "0       ", 8) == 0)
			format = 16;	/* EDF or EDF+ */
		else if (strncmp(buf + 1, "BIOSEMI", 7) == 0)
			format = 24;	/* BDF */
		else {
			_context->error("init: '%s' is not EDF or EDF+\n", edf_fname);
			return (-2);
		}

		/* Read the remainder of the fixed-size section of the header. */
		wfdb_fread(buf, 1, 80, ifile);	/* patient ID (ignored) */
		wfdb_fread(buf, 1, 80, ifile);	/* recording ID (ignored) */
		wfdb_fread(buf, 1, 8, ifile);	/* recording date */
		buf[8] = '\0';
		sscanf(buf, "%d%*c%d%*c%d", &day, &month, &year);
		year += 1900;			/* EDF has only two-digit years */
		if (year < 1985) year += 100;	/* fix this before 1/1/2085! */
		wfdb_fread(buf, 1, 8, ifile);	/* recording time */
		sscanf(buf, "%d%*c%d%*c%d", &hour, &minute, &second);
		wfdb_fread(buf, 1, 8, ifile);	/* number of bytes in header */
		sscanf(buf, "%d", &offset);
		wfdb_fread(buf, 1, 44, ifile);	/* free space (ignored) */
		wfdb_fread(buf, 1, 8, ifile);	/* number of frames (EDF blocks) */
		buf[8] = '\0';
		sscanf(buf, "%ld", &nframes);
		nsamples = nframes;
		wfdb_fread(buf, 1, 8, ifile);	/* data record duration (seconds) */
		sscanf(buf, "%lf", &spr);
		if (spr <= 0.0) spr = 1.0;
		wfdb_fread(buf + 4, 1, 4, ifile);	/* number of signals */
		sscanf(buf + 4, "%d", &nsig);

		if (nsig < 1 || (nsig + 1) * 256 != offset) {
			_context->error("init: '%s' is not EDF or EDF+\n", edf_fname);
			return (-2);
		}

		/* Allocate workspace. */
		if (maxhsig < nsig) {
			unsigned m = maxhsig;

			SREALLOC(hsd, nsig, sizeof(struct hsdata *));
			while (m < nsig) {
				SUALLOC(hsd[m], 1, sizeof(struct hsdata));
				m++;
			}
			maxhsig = nsig;
		}
		SUALLOC(dmax, nsig, sizeof(long));
		SUALLOC(dmin, nsig, sizeof(long));
		SUALLOC(pmax, nsig, sizeof(double));
		SUALLOC(pmin, nsig, sizeof(double));

		/* Strip off any path info from the EDF file name. */
		p = edf_fname + strlen(edf_fname) - 4;
		while (--p > edf_fname)
			if (*p == '/') edf_fname = p + 1;

		/* Read the variable-size section of the header. */
		for (s = 0; s < nsig; s++) {
			hsd[s]->start = offset;
			hsd[s]->skew = 0;
			SSTRCPY(hsd[s]->info.fname, edf_fname);
			hsd[s]->info.group = hsd[s]->info.bsize = hsd[s]->info.cksum = 0;
			hsd[s]->info.fmt = format;
			hsd[s]->info.nsamp = nframes;

			wfdb_fread(buf, 1, 16, ifile);	/* signal type */
			buf[16] = ' ';
			for (i = 16; i >= 0 && buf[i] == ' '; i--)
				buf[i] = '\0';
			SSTRCPY(hsd[s]->info.desc, buf);
		}

		for (s = 0; s < nsig; s++)
			wfdb_fread(buf, 1, 80, ifile); /* transducer type (ignored) */

		for (s = 0; s < nsig; s++) {
			wfdb_fread(buf, 1, 8, ifile);	/* signal units */
			for (i = 7; i >= 0 && buf[i] == ' '; i--)
				buf[i] = '\0';
			SSTRCPY(hsd[s]->info.units, buf);
		}

		for (s = 0; s < nsig; s++) {
			wfdb_fread(buf, 1, 8, ifile);	/* physical minimum */
			sscanf(buf, "%lf", &pmin[s]);
		}

		for (s = 0; s < nsig; s++) {
			wfdb_fread(buf, 1, 8, ifile);	/* physical maximum */
			sscanf(buf, "%lf", &pmax[s]);
		}

		for (s = 0; s < nsig; s++) {
			wfdb_fread(buf, 1, 8, ifile);	/* digital minimum */
			sscanf(buf, "%ld", &dmin[s]);
		}

		for (s = 0; s < nsig; s++) {
			wfdb_fread(buf, 1, 8, ifile);	/* digital maximum */
			sscanf(buf, "%ld", &dmax[s]);
			hsd[s]->info.initval = hsd[s]->info.adczero = (dmax[s] + 1 + dmin[s]) / 2;
			adcrange = dmax[s] - dmin[s];
			for (i = 0; adcrange > 1; i++)
				adcrange /= 2;
			hsd[s]->info.adcres = i;
			if (pmax[s] != pmin[s]) {
				hsd[s]->info.gain = (dmax[s] - dmin[s]) / (pmax[s] - pmin[s]);
				baseline = dmax[s] - pmax[s] * hsd[s]->info.gain;
				if (baseline >= 0.0)
					hsd[s]->info.baseline = baseline + 0.5;
				else
					hsd[s]->info.baseline = baseline - 0.5;
			}
			else			/* gain is undefined */
				hsd[s]->info.gain = hsd[s]->info.baseline = 0;
		}

		for (s = 0; s < nsig; s++)
			wfdb_fread(buf, 1, 80, ifile);	/* filtering information (ignored) */

		for (s = framelen = 0; s < nsig; s++) {
			int n;

			wfdb_fread(buf, 1, 8, ifile);	/* samples per frame (EDF block) */
			buf[8] = ' ';
			for (i = 8; i >= 0 && buf[i] == ' '; i--)
				buf[i] = '\0';
			sscanf(buf, "%d", &n);
			if ((hsd[s]->info.spf = n) > ispfmax) ispfmax = n;
			framelen += n;
		}

		(void)wfdb_fclose(ifile);	/* (don't bother reading nsig*32 bytes of free
					   space) */
		hheader = NULL;	/* make sure getinfo doesn't try to read the EDF file */

		ffreq = 1.0 / spr;	/* frame frequency = 1/(seconds per EDF block) */
		cfreq = ffreq; /* set sampling and counter frequencies to match */
		sfreq = ffreq * ispfmax;
		if (getafreq() == 0.0) setafreq(sfreq);
		gvmode |= WFDB_HIGHRES;
		sprintf(buf, "%02d:%02d:%02d %02d/%02d/%04d",
			hour, minute, second, day, month, year);
		setbasetime(buf);

		SFREE(pmin);
		SFREE(pmax);
		SFREE(dmin);
		SFREE(dmax);
		isedf = 1;
		return (nsig);
	}

	int readheader(const char *record)
	{
		char linebuf[256];
		char *p;
		FrequencyType f;
		SignalType s;
		TimeType ns;
		unsigned int i;
		unsigned int nsig;
		static char sep[] = " \t\n\r";

		/* If another input header file was opened, close it. */
		if (hheader) {
			(void)wfdb_fclose(hheader);
			hheader = nullptr;
		}

		isedf = 0;
		if (strcmp(record, "~") == 0) {
			if (in_msrec && vsd.size()) {
				hsd=new hsdata*[1];
				hsd[0]=new hsdata;
				hsd[0]->info.desc= "~";
				hsd[0]->info.spf = 1;
				hsd[0]->info.fmt = 0;
				hsd[0]->info.nsamp = nsamples = segp->nsamp;
				return (maxhsig = 1);
			}
			return (0);
		}

		/* If the final component of the record name includes a '.', assume it is a
		   file name. */
		char* q = const_cast<char *>(record) + strlen(record) - 1;
		while (q > record && *q != '.' && *q != '/' && *q != ':' && *q != '\\')
			q--;
		if (*q == '.') {
			if ((hheader = wfdb_open(nullptr, record, WFDB_READ)) == nullptr) {
				_context->error("init: can't open %s\n", record);
				return (-1);
			}
			if (strcmp(q + 1, "hea"))	/* assume EDF if suffix is not '.hea' */
				return (edfparse(hheader));
		}

		/* Otherwise, assume the file name is record.hea. */
		else if ((hheader = wfdb_open("hea", record, WFDB_READ)) == nullptr) {
			_context->error("init: can't open header for record %s\n", record);
			return (-1);
		}

		/* Read the first line and check for a magic string. */
		if (wfdb_fgets(linebuf, 256, hheader) == nullptr) {
			_context->error("init: record %s header is empty\n", record);
			return (-2);
		}
		if (strncmp("#wfdb", linebuf, 5) == 0) { /* found the magic string */
			int i, major, minor = 0, release = 0;

			i = sscanf(linebuf + 5, "%d.%d.%d", &major, &minor, &release);
			if ((i > 0 && major > WFDB_MAJOR) ||
				(i > 1 && minor > WFDB_MINOR) ||
				(i > 2 && release > WFDB_RELEASE)) {
				_context->error("init: reading record %s requires WFDB library "
					"version %d.%d.%d or later\n"
					"  (the most recent version is always available from http://physionet.org)\n",
					record, major, minor, release);
				return (-1);
			}
		}

		/* Get the first token (the record name) from the first non-empty,
		   non-comment line. */
		while ((p = strtok(linebuf, sep)) == NULL || *p == '#') {
			if (wfdb_fgets(linebuf, 256, hheader) == NULL) {
				_context->error("init: can't find record name in record %s header\n",
					record);
				return (-2);
			}
		}

		for (q = p + 1; *q && *q != '/'; q++)
			;
		if (*q == '/') {
			if (in_msrec) {
				_context->error(
					"init: record %s cannot be nested in another multi-segment record\n",
					record);
				return (-2);
			}
			segments = strtol(q + 1, NULL, 10);
			*q = '\0';
		}

		/* For local files, be sure that the name (p) within the header file
		   matches the name (record) provided as an argument to this function --
		   if not, the header file may have been renamed in error or its contents
		   may be corrupted.  The requirement for a match is waived for remote
		   files since the user may not be able to make any corrections to them. */
		if (hheader->type == WFDB_LOCAL &&
			hheader->fp != stdin && strncmp(p, record, strlen(p)) != 0) {
			/* If there is a mismatch, check to see if the record argument includes
			   a directory separator (whether valid or not for this OS);  if so,
			   compare only the final portion of the argument against the name in
			   the header file. */
			const char *q, *r, *s;

			for (r = record, q = s = r + strlen(r) - 1; r != s; s--)
				if (*s == '/' || *s == '\\' || *s == ':')
					break;

			if (q > s && (r > s || strcmp(p, s + 1) != 0)) {
				_context->error("init: record name in record %s header is incorrect\n",
					record);
				return (-2);
			}
		}

		/* Identify which type of header file is being read by trying to get
		   another token from the line which contains the record name.  (Old-style
		   headers have only one token on the first line, but new-style headers
		   have two or more.) */
		if ((p = strtok((char *)NULL, sep)) == NULL) {
			/* The file appears to be an old-style header file. */
			_context->error("init: obsolete format in record %s header\n", record);
			return (-2);
		}

		/* The file appears to be a new-style header file.  The second token
		   specifies the number of signals. */
		nsig = (unsigned)strtol(p, NULL, 10);

		/* Determine the frame rate, if present and not set already. */
		if (p = strtok((char *)NULL, sep)) {
			if ((f = (FrequencyType)strtod(p, NULL)) <= (FrequencyType)0.) {
				_context->error(
					"init: sampling frequency in record %s header is incorrect\n",
					record);
				return (-2);
			}
			if (ffreq > (FrequencyType)0. && f != ffreq) {
				_context->error("warning (init):\n");
				_context->error(" record %s sampling frequency differs", record);
				_context->error(" from that of previously opened record\n");
			}
			else
				ffreq = f;
		}
		else if (ffreq == (FrequencyType)0.)
			ffreq = WFDB_DEFFREQ;

		/* Set the sampling rate to the frame rate for now.  This may be
		   changed later by isigopen or by setgvmode, if this is a multi-
		   frequency record and WFDB_HIGHRES mode is in effect. */
		sfreq = ffreq;

		/* Determine the counter frequency and the base counter value. */
		cfreq = bcount = 0.0;
		if (p) {
			for (; *p && *p != '/'; p++)
				;
			if (*p == '/') {
				cfreq = strtod(++p, NULL);
				for (; *p && *p != '('; p++)
					;
				if (*p == '(')
					bcount = strtod(++p, NULL);
			}
		}
		if (cfreq <= 0.0) cfreq = ffreq;

		/* Determine the number of samples per signal, if present and not
		   set already. */
		if (p = strtok((char *)NULL, sep)) {
			if ((ns = (TimeType)strtol(p, NULL, 10)) < 0L) {
				_context->error(
					"init: number of samples in record %s header is incorrect\n",
					record);
				return (-2);
			}
			if (nsamples == (TimeType)0L)
				nsamples = ns;
			else if (ns > (TimeType)0L && ns != nsamples && !in_msrec) {
				_context->error("warning (init):\n");
				_context->error(" record %s duration differs", record);
				_context->error(" from that of previously opened record\n");
				/* nsamples must match the shortest record duration. */
				if (nsamples > ns)
					nsamples = ns;
			}
		}
		else
			ns = (TimeType)0L;

		/* Determine the base time and date, if present and not set already. */
		if ((p = strtok((char *)NULL, "\n\r")) != NULL &&
			btime == 0L && setbasetime(p) < 0)
			return (-2);	/* error message will come from setbasetime */

			/* Special processing for master header of a multi-segment record. */
		if (segments && !in_msrec) {
			msbtime = btime;
			msbdate = bdate;
			msnsamples = nsamples;
			/* Read the names and lengths of the segment records. */
			SALLOC(segarray, segments, sizeof(WFDB_Seginfo));
			segp = segarray;
			for (i = 0, ns = (TimeType)0L; i < segments; i++, segp++) {
				/* Get next segment spec, skip empty lines and comments. */
				do {
					if (wfdb_fgets(linebuf, 256, hheader) == NULL) {
						_context->error(
							"init: unexpected EOF in header file for record %s\n",
							record);
						SFREE(segarray);
						segments = 0;
						return (-2);
					}
				} while ((p = strtok(linebuf, sep)) == NULL || *p == '#');
				if (strlen(p) > WFDB_MAXRNL) {
					_context->error(
						"init: `%s' is too long for a segment name in record %s\n",
						p, record);
					SFREE(segarray);
					segments = 0;
					return (-2);
				}
				(void)strcpy(segp->recname, p);
				if ((p = strtok((char *)NULL, sep)) == NULL ||
					(segp->nsamp = (TimeType)strtol(p, NULL, 10)) < 0L) {
					_context->error(
						"init: length must be specified for segment %s in record %s\n",
						segp->recname, record);
					SFREE(segarray);
					segments = 0;
					return (-2);
				}
				segp->samp0 = ns;
				ns += segp->nsamp;
			}
			segend = --segp;
			segp = segarray;
			if (msnsamples == 0L)
				msnsamples = ns;
			else if (ns != msnsamples) {
				_context->error("warning (init): in record %s, "
					"stated record length (%ld)\n", record, msnsamples);
				_context->error(" does not match sum of segment lengths (%ld)\n", ns);
			}
			return (0);
		}

		/* Allocate workspace. */
		if (maxhsig < nsig) {
			unsigned m = maxhsig;

			SREALLOC(hsd, nsig, sizeof(struct hsdata *));
			while (m < nsig) {
				SUALLOC(hsd[m], 1, sizeof(struct hsdata));
				m++;
			}
			maxhsig = nsig;
		}

		/* Now get information for each signal. */
		for (s = 0; s < nsig; s++) {
			struct hsdata *hp = 0;

			struct hsdata* hs = hsd[s];
			if (s) hp = hsd[s - 1];
			/* Get the first token (the signal file name) from the next
			   non-empty, non-comment line. */
			do {
				if (wfdb_fgets(linebuf, 256, hheader) == NULL) {
					_context->error(
						"init: unexpected EOF in header file for record %s\n",
						record);
					return (-2);
				}
			} while ((p = strtok(linebuf, sep)) == NULL || *p == '#');

			/* Determine the signal group number.  The group number for signal
			   0 is zero.  For subsequent signals, if the file name does not
			   match that of the previous signal, the group number is one
			   greater than that of the previous signal. */
			if (s == 0 || strcmp(p, hp->info.fname)) {
				hs->info.group = (s == 0) ? 0 : hp->info.group + 1;
				SSTRCPY(hs->info.fname, p);
			}
			/* If the file names of the current and previous signals match,
			   they are assigned the same group number and share a copy of the
			   file name.  All signals associated with a given file must be
			   listed together in the header in order to be identified as
			   belonging to the same group;  readheader does not check that
			   this has been done. */
			else {
				hs->info.group = hp->info.group;
				SSTRCPY(hs->info.fname, hp->info.fname);
			}

			/* Determine the signal format. */
			if ((p = strtok((char *)NULL, sep)) == NULL ||
				!isfmt(hs->info.fmt = strtol(p, NULL, 10))) {
				_context->error("init: illegal format for signal %d, record %s\n",
					s, record);
				return (-2);
			}
			hs->info.spf = 1;
			hs->skew = 0;
			hs->start = 0L;
			while (*(++p)) {
				if (*p == 'x' && *(++p))
					if ((hs->info.spf = strtol(p, NULL, 10)) < 1) hs->info.spf = 1;
				if (*p == ':' && *(++p))
					if ((hs->skew = strtol(p, NULL, 10)) < 0) hs->skew = 0;
				if (*p == '+' && *(++p))
					if ((hs->start = strtol(p, NULL, 10)) < 0L) hs->start = 0L;
			}
			/* The resolution for deskewing is one frame.  The skew in samples
			   (given in the header) is converted to skew in frames here. */
			hs->skew = (int)(((double)hs->skew) / hs->info.spf + 0.5);

			/* Determine the gain in ADC units per physical unit.  This number
			   may be zero or missing;  if so, the signal is uncalibrated. */
			if (p = strtok((char *)NULL, sep))
				hs->info.gain = (GainType)strtod(p, NULL);
			else
				hs->info.gain = (GainType)0.;

			/* Determine the baseline if specified, and the physical units
			   (assumed to be millivolts unless otherwise specified). */
			int nobaseline = 1;
			if (p) {
				for (; *p && *p != '(' && *p != '/'; p++)
					;
				if (*p == '(') {
					hs->info.baseline = strtol(++p, NULL, 10);
					nobaseline = 0;
				}
				while (*p)
					if (*p++ == '/' && *p)
						break;
			}
			if (p && *p) {
				SALLOC(hs->info.units, WFDB_MAXUSL + 1, 1);
				(void)strncpy(hs->info.units, p, WFDB_MAXUSL);
			}
			else
				hs->info.units = NULL;

			/* Determine the ADC resolution in bits.  If this number is
			   missing and cannot be inferred from the format, the default
			   value (from wfdb.h) is filled in. */
			if (p = strtok((char *)NULL, sep))
				i = (unsigned)strtol(p, NULL, 10);
			else switch (hs->info.fmt) {
			case 80: i = 8; break;
			case 160: i = 16; break;
			case 212: i = 12; break;
			case 310: i = 10; break;
			case 311: i = 10; break;
			default: i = WFDB_DEFRES; break;
			}
			hs->info.adcres = i;

			/* Determine the ADC zero (assumed to be zero if missing). */
			hs->info.adczero = (p = strtok((char *)NULL, sep)) ? strtol(p, NULL, 10) : 0;

			/* Set the baseline to adczero if no baseline field was found. */
			if (nobaseline) hs->info.baseline = hs->info.adczero;

			/* Determine the initial value (assumed to be equal to the ADC
			   zero if missing). */
			hs->info.initval = (p = strtok((char *)NULL, sep)) ?
				strtol(p, NULL, 10) : hs->info.adczero;

			/* Determine the checksum (assumed to be zero if missing). */
			if (p = strtok((char *)NULL, sep)) {
				hs->info.cksum = strtol(p, NULL, 10);
				hs->info.nsamp = ns;
			}
			else {
				hs->info.cksum = 0;
				hs->info.nsamp = (TimeType)0L;
			}

			/* Determine the block size (assumed to be zero if missing). */
			hs->info.bsize = (p = strtok((char *)NULL, sep)) ? strtol(p, NULL, 10) : 0;

			/* Check that formats and block sizes match for signals belonging
			   to the same group. */
			if (s && (hp == NULL || (hs->info.group == hp->info.group &&
				(hs->info.fmt != hp->info.fmt ||
					hs->info.bsize != hp->info.bsize)))) {
				_context->error("init: error in specification of signal %d or %d\n",
					s - 1, s);
				return (-2);
			}

			/* Get the signal description.  If missing, a description of
			   the form "record xx, signal n" is filled in. */
			SALLOC(hs->info.desc, 1, WFDB_MAXDSL + 1);
			if (p = strtok((char *)NULL, "\n\r"))
				(void)strncpy(hs->info.desc, p, WFDB_MAXDSL);
			else
				(void)sprintf(hs->info.desc,
					"record %s, signal %d", record, s);
		}
		return (s);			/* return number of available signals */
	}

	static void hsdfree(void)
	{
		struct hsdata *hs;

		if (hsd) {
			while (maxhsig)
				if (hs = hsd[--maxhsig]) {
					SFREE(hs->info.fname);
					SFREE(hs->info.units);
					SFREE(hs->info.desc);
					SFREE(hs);
				}
			SFREE(hsd);
		}
		maxhsig = 0;
	}

	static void isigclose(void)
	{
		struct isdata *is;
		struct igdata *ig;

		if (sbuf && !in_msrec) {
			SFREE(sbuf);
			sample_vflag = 0;
		}
		if (isd) {
			while (maxisig)
				if ((is = isd[--maxisig])) {
					SFREE(is->info.fname);
					SFREE(is->info.units);
					SFREE(is->info.desc);
					SFREE(is);
				}
			SFREE(isd);
		}
		maxisig = nisig = 0;

		if (igd) {
			while (maxigroup)
				if (ig = igd[--maxigroup]) {
					if (ig->fp) (void)wfdb_fclose(ig->fp);
					SFREE(ig->buf);
					SFREE(ig);
				}
			SFREE(igd);
		}
		maxigroup = nigroup = 0;

		istime = 0L;
		gvc = ispfmax = 1;
		if (hheader) {
			(void)wfdb_fclose(hheader);
			hheader = NULL;
		}
		if (nosig == 0 && maxhsig != 0)
			hsdfree();
	}

	static void osigclose(void)
	{
		struct osdata *os;
		struct ogdata *og;
		GroupType g;

		for (g = 0; g < nogroup; g++)
			if (ogd && (og = ogd[g]))
				og->force_flush = 1;

		wfdb_osflush();

		if (osd) {
			while (maxosig)
				if (os = osd[--maxosig]) {
					SFREE(os->info.fname);
					SFREE(os->info.units);
					SFREE(os->info.desc);
					SFREE(os);
				}
			SFREE(osd);
		}
		nosig = 0;

		if (ogd) {
			while (maxogroup)
				if (og = ogd[--maxogroup]) {
					if (og->fp) {
						/* If a block size has been defined, null-pad the buffer */
						if (og->bsize)
							while (og->bp != og->be)
								*(og->bp++) = '\0';
						/* Flush the last block unless it's empty. */
						if (og->bp != og->buf)
							(void)wfdb_fwrite(og->buf, 1, og->bp - og->buf, og->fp);
						/* Close file (except stdout, which is closed on exit). */
						if (og->fp->fp != stdout) {
							(void)wfdb_fclose(og->fp);
							og->fp = NULL;
						}
					}
					SFREE(og->buf);
					SFREE(og);
				}
			SFREE(ogd);
		}
		maxogroup = nogroup = 0;

		ostime = 0L;
		if (oheader) {
			(void)wfdb_fclose(oheader);
			if (outinfo == oheader) outinfo = NULL;
			oheader = NULL;
		}
		if (nisig == 0 && maxhsig != 0)
			hsdfree();
	}

	/* Low-level I/O routines.  The input routines each get a single argument (the
	signal group pointer).  The output routines get two arguments (the value to be
	written and the signal group pointer). */

	static int _l;		    /* macro temporary storage for low byte of word */
	static int _lw;		    /* macro temporary storage for low 16 bits of int */
	static int _n;		    /* macro temporary storage for byte count */

#define r8(G)	((G->bp < G->be) ? *(G->bp++) : \
		  ((_n = (G->bsize > 0) ? G->bsize : ibsize), \
		   (G->stat = _n = wfdb_fread(G->buf, 1, _n, G->fp)), \
		   (G->be = (G->bp = G->buf) + _n),\
		  *(G->bp++)))

#define w8(V,G)	(((*(G->bp++) = (char)V)), \
		  (_l = (G->bp != G->be) ? 0 : \
		   ((_n = (G->bsize > 0) ? G->bsize : obsize), \
		    wfdb_fwrite((G->bp = G->buf), 1, _n, G->fp))))

/* If a short integer is not 16 bits, it may be necessary to redefine r16() and
r61() in order to obtain proper sign extension. */

#ifndef BROKEN_CC
#define r16(G)	    (_l = r8(G), ((int)((short)((r8(G) << 8) | (_l & 0xff)))))
#define w16(V,G)    (w8((V), (G)), w8(((V) >> 8), (G)))
#define r61(G)      (_l = r8(G), ((int)((short)((r8(G) & 0xff) | (_l << 8)))))
#define w61(V,G)    (w8(((V) >> 8), (G)), w8((V), (G)))
#define r24(G)	    (_lw = r16(G), ((int)((r8(G) << 16) | (_lw & 0xffff))))
#define w24(V,G)    (w16((V), (G)), w8(((V) >> 16), (G)))
#define r32(G)	    (_lw = r16(G), ((int)((r16(G) << 16) | (_lw & 0xffff))))
#define w32(V,G)    (w16((V), (G)), w16(((V) >> 16), (G)))
#else

	static int r16(struct igdata *g)
	{
		int l, h;

		l = r8(g);
		h = r8(g);
		return ((int)((short)((h << 8) | (l & 0xff))));
	}

	static void w16(WFDB_Sample v, struct ogdata *g)
	{
		w8(v, g);
		w8((v >> 8), g);
	}

	static int r61(struct igdata *g)
	{
		int l, h;

		h = r8(g);
		l = r8(g);
		return ((int)((short)((h << 8) | (l & 0xff))));
	}

	static void w61(WFDB_Sample v, struct ogdata *g)
	{
		w8((v >> 8), g);
		w8(v, g);
	}

	/* r24: read and return the next sample from a format 24 signal file */
	static int r24(struct igdata *g)
	{
		int l, h;

		l = r16(g);
		h = r8(g);
		return ((int)((h << 16) | (l & 0xffff)));
	}

	/* w24: write the next sample to a format 24 signal file */
	static void w24(WFDB_Sample v, struct ogdata *g)
	{
		w16(v, g);
		w8((v >> 16), g);
	}

	/* r32: read and return the next sample from a format 32 signal file */
	static int r32(struct igdata *g)
	{
		int l, h;

		l = r16(g);
		h = r16(g);
		return ((int)((h << 16) | (l & 0xffff)));
	}

	/* w32: write the next sample to a format 32 signal file */
	static void w32(WFDB_Sample v, struct ogdata *g)
	{
		w16(v, g);
		w16((v >> 16), g);
	}
#endif

#define r80(G)		((r8(G) & 0xff) - (1 << 7))
#define w80(V, G)	(w8(((V) & 0xff) + (1 << 7), G))

#define r160(G)		((r16(G) & 0xffff) - (1 << 15))
#define w160(V, G)	(w16(((V) & 0xffff) + (1 << 15), G))

	/* r212: read and return the next sample from a format 212 signal file
	   (2 12-bit samples bit-packed in 3 bytes) */
	static int r212(struct igdata *g)
	{
		int v;

		/* Obtain the next 12-bit value right-justified in v. */
		switch (g->count++) {
		case 0:	v = g->data = r16(g); break;
		case 1:
		default:	g->count = 0;
			v = ((g->data >> 4) & 0xf00) | (r8(g) & 0xff); break;
		}
		/* Sign-extend from the twelfth bit. */
		if (v & 0x800) v |= ~(0xfff);
		else v &= 0xfff;
		return (v);
	}

	/* w212: write the next sample to a format 212 signal file */
	static void w212(SampleType v, struct ogdata *g)
	{
		/* Samples are buffered here and written in pairs, as three bytes. */
		switch (g->count++) {
		case 0:	g->data = v & 0xfff; break;
		case 1:	g->count = 0;
			g->data |= (v << 4) & 0xf000;
			w16(g->data, g);
			w8(v, g);
			break;
		}
	}

	/* f212: flush output to a format 212 signal file */
	static void f212(struct ogdata *g)
	{
		/* If we have one leftover sample, write it as two bytes. */
		if (g->count == 1) {
			w16(g->data, g);
			g->nrewind = 2;
		}
	}

	/* r310: read and return the next sample from a format 310 signal file
	   (3 10-bit samples bit-packed in 4 bytes) */
	static int r310(struct igdata *g)
	{
		int v;

		/* Obtain the next 10-bit value right-justified in v. */
		switch (g->count++) {
		case 0:	v = (g->data = r16(g)) >> 1; break;
		case 1:	v = (g->datb = r16(g)) >> 1; break;
		case 2:
		default:	g->count = 0;
			v = ((g->data & 0xf800) >> 11) | ((g->datb & 0xf800) >> 6);
			break;
		}
		/* Sign-extend from the tenth bit. */
		if (v & 0x200) v |= ~(0x3ff);
		else v &= 0x3ff;
		return (v);
	}

	/* w310: write the next sample to a format 310 signal file */
	static void w310(SampleType v, struct ogdata *g)
	{
		/* Samples are buffered here and written in groups of three, as two
		   left-justified 15-bit words. */
		switch (g->count++) {
		case 0:	g->data = (v << 1) & 0x7fe; break;
		case 1:	g->datb = (v << 1) & 0x7fe; break;
		case 2:
		default:	g->count = 0;
			g->data |= (v << 11); w16(g->data, g);
			g->datb |= ((v << 6) & ~0x7fe); w16(g->datb, g);
			break;
		}
	}

	/* f310: flush output to a format 310 signal file */
	static void f310(struct ogdata *g)
	{
		switch (g->count) {
		case 0:  break;
			/* If we have one leftover sample, write it as two bytes. */
		case 1:  w16(g->data, g);
			g->nrewind = 2;
			break;
			/* If we have two leftover samples, write them as four bytes.
		   In this case, the file will appear to have an extra (zero)
		   sample at the end. */
		case 2:
		default: w16(g->data, g);
			w16(g->datb, g);
			g->nrewind = 4;
			break;
		}
	}

	/* r311: read and return the next sample from a format 311 signal file
	   (3 10-bit samples bit-packed in 4 bytes; note that formats 310 and 311
	   differ in the layout of the bit-packed data) */
	static int r311(struct igdata *g)
	{
		int v;

		/* Obtain the next 10-bit value right-justified in v. */
		switch (g->count++) {
		case 0:	v = (g->data = r16(g)); break;
		case 1:	g->datb = (r8(g) & 0xff);
			v = ((g->data & 0xfc00) >> 10) | ((g->datb & 0xf) << 6);
			break;
		case 2:
		default:	g->count = 0;
			g->datb |= r8(g) << 8;
			v = g->datb >> 4; break;
		}
		/* Sign-extend from the tenth bit. */
		if (v & 0x200) v |= ~(0x3ff);
		else v &= 0x3ff;
		return (v);
	}

	/* w311: write the next sample to a format 311 signal file */
	static void w311(SampleType v, struct ogdata *g)
	{
		/* Samples are buffered here and written in groups of three, bit-packed
		   into the 30 low bits of a 32-bit word. */
		switch (g->count++) {
		case 0:	g->data = v & 0x3ff; break;
		case 1:	g->data |= (v << 10); w16(g->data, g);
			g->datb = (v >> 6) & 0xf; break;
		case 2:
		default:	g->count = 0;
			g->datb |= (v << 4); g->datb &= 0x3fff; w16(g->datb, g);
			break;
		}
	}

	/* f311: flush output to a format 311 signal file */
	static void f311(struct ogdata *g)
	{
		switch (g->count) {
		case 0:	break;
			/* If we have one leftover sample, write it as two bytes. */
		case 1:	w16(g->data, g);
			g->nrewind = 2;
			break;
			/* If we have two leftover samples, write them as four bytes
		   (note that the first two bytes will already have been written
		   by w311(), above.)  The file will appear to have an extra
		   (zero) sample at the end.  It would be possible to write only
		   three bytes here, but older versions of WFDB would not be
		   able to read the resulting file. */
		case 2:
		default:	w16(g->datb, g);
			g->nrewind = 2;
			break;
		}
	}

	static int isgsetframe(GroupType g, TimeType t)
	{
		int i, trem = 0;
		long nb, tt;
		struct igdata *ig;
		SignalType s;
		unsigned int b, d = 1, n, nn;

		/* Do nothing if there is no more than one input signal group and
		   the input pointer is correct already. */
		if (nigroup < 2 && istime == t && gvc == ispfmax &&
			igd[g]->start == 0)
			return (0);

		/* Find the first signal that belongs to group g. */
		for (s = 0; s < nisig && g != isd[s]->info.group; s++)
			;
		if (s == nisig) {
			_context->error("isgsettime: incorrect signal group number %d\n", g);
			return (-2);
		}

		/* Mark the contents of the deskewing buffer (if any) as invalid. */
		dsbi = -1;

		/* If the current record contains multiple segments, locate the segment
		   containing the desired sample. */
		if (in_msrec) {
			WFDB_Seginfo *tseg = segp;
			GroupType h;

			if (t >= msnsamples) {
				_context->error("isigsettime: improper seek on signal group %d\n", g);
				return (-1);
			}
			while (t < tseg->samp0)
				tseg--;
			while (t >= tseg->samp0 + tseg->nsamp && tseg < segend)
				tseg++;
			if (segp != tseg) {
				segp = tseg;
				if (isigopen(segp->recname, NULL, (int)nvsig) < 0) {
					_context->error("isigsettime: can't open segment %s\n",
						segp->recname);
					return (-1);
				}
				/* Following isigopen(), nigroup may have changed and
				   group numbers may not make any sense anymore.  However,
				   isigsettime() will still call isgsettime() once for
				   each non-zero group (if the previous segment had
				   multiple groups) and then once for group zero.

				   Calling isgsetframe() multiple times for a non-zero
				   group is mostly harmless, but seeking on group 0 should
				   only be done once.  Thus, when we jump to a new
				   segment, implicitly seek on all non-zero groups
				   (regardless of g), but only seek on group 0 if g is 0.

				   (Note that isgsettime() is not and has never been fully
				   functional for multi-segment records, because it cannot
				   read signals from two different segments at once.) */
				for (h = 1; h < nigroup; h++)
					if (i = isgsetframe(h, t))
						return (i);
				if (g == 0)
					return (isgsetframe(0, t));
				else
					return (0);
			}
			t -= segp->samp0;
		}

		ig = igd[g];
		/* Determine the number of samples per frame for signals in the group. */
		for (n = nn = 0; s + n < nisig && isd[s + n]->info.group == g; n++)
			nn += isd[s + n]->info.spf;
		/* Determine the number of bytes per sample interval in the file. */
		switch (isd[s]->info.fmt) {
		case 0:
			if (t < nsamples) {
				gvc = ispfmax;
				if (s == 0) istime = (in_msrec) ? t + segp->samp0 : t;
				isd[s]->info.nsamp = nsamples - t;
				ig->stat = 1;
				return (0);
			}
			else {
				if (s == 0) istime = (in_msrec) ? msnsamples : nsamples;
				isd[s]->info.nsamp = 0L;
				return (-1);
			}
		case 8:
		case 80:
		default: b = nn; break;
		case 16:
		case 61:
		case 160: b = 2 * nn; break;
		case 212:
			/* Reset the input counter. */
			ig->count = 0;
			/* If the desired sample does not lie on a byte boundary, seek to
			   the previous sample and then read ahead. */
			if ((nn & 1) && (t & 1)) {
				if (in_msrec)
					t += segp->samp0;	/* restore absolute time */
				if (i = isgsetframe(g, t - 1))
					return (i);
				for (i = 0; i < nn; i++)
					(void)r212(ig);
				istime++;
				for (n = 0; s + n < nisig && isd[s + n]->info.group == g; n++)
					isd[s + n]->info.nsamp = (TimeType)0L;
				return (0);
			}
			b = 3 * nn; d = 2; break;
		case 310:
			/* Reset the input counter. */
			ig->count = 0;
			/* If the desired sample does not lie on a byte boundary, seek to
			   the closest previous sample that does, then read ahead. */
			if ((nn % 3) && (trem = (t % 3))) {
				if (in_msrec)
					t += segp->samp0;	/* restore absolute time */
				if (i = isgsetframe(g, t - trem))
					return (i);
				for (i = nn * trem; i > 0; i--)
					(void)r310(ig);
				istime += trem;
				for (n = 0; s + n < nisig && isd[s + n]->info.group == g; n++)
					isd[s + n]->info.nsamp = (TimeType)0L;
				return (0);
			}
			b = 4 * nn; d = 3; break;
		case 311:
			/* Reset the input counter. */
			ig->count = 0;
			/* If the desired sample does not lie on a byte boundary, seek to
			   the closest previous sample that does, then read ahead. */
			if ((nn % 3) && (trem = (t % 3))) {
				if (in_msrec)
					t += segp->samp0;	/* restore absolute time */
				if (i = isgsetframe(g, t - trem))
					return (i);
				for (i = nn * trem; i > 0; i--)
					(void)r311(ig);
				istime += trem;
				for (n = 0; s + n < nisig && isd[s + n]->info.group == g; n++)
					isd[s + n]->info.nsamp = (TimeType)0L;
				return (0);
			}
			b = 4 * nn; d = 3; break;
		case 24: b = 3 * nn; break;
		case 32: b = 4 * nn; break;
		}

		/* Seek to the beginning of the block which contains the desired sample.
		   For normal files, use fseek() to do so. */
		if (ig->seek) {
			tt = t * b;
			nb = tt / d + ig->start;
			if ((i = ig->bsize) == 0) i = ibsize;
			/* Seek to a position such that the next block read will contain the
			   desired sample. */
			tt = nb / i;
			if (wfdb_fseek(ig->fp, tt*i, 0)) {
				_context->error("isigsettime: improper seek on signal group %d\n", g);
				return (-1);
			}
			nb %= i;
		}
		/* For special files, rewind if necessary and then read ahead. */
		else {
			long t0, t1;

			/* Get the time of the earliest buffered sample ... */
			t0 = istime - (ig->bp - ig->buf) / b;
			/* ... and that of the earliest unread sample. */
			t1 = t0 + (ig->be - ig->buf) / b;
			/* There are three possibilities:  either the desired sample has been
			   read and has passed out of the buffer, requiring a rewind ... */
			if (t < t0) {
				if (wfdb_fseek(ig->fp, 0L, 0)) {
					_context->error("isigsettime: improper seek on signal group %d\n",
						g);
					return (-1);
				}
				tt = t * b;
				nb = tt / d + ig->start;
			}
			/* ... or it is in the buffer already ... */
			else if (t < t1) {
				tt = (t - t0)*b;
				ig->bp = ig->buf + tt / d;
				return (0);
			}
			/* ... or it has not yet been read. */
			else {
				tt = (t - t1) * b;
				nb = tt / d;
			}
			while (nb > ig->bsize && !wfdb_feof(ig->fp))
				nb -= wfdb_fread(ig->buf, 1, ig->bsize, ig->fp);
		}

		/* Reset the block pointer to indicate nothing has been read in the
		   current block. */
		ig->bp = ig->be;
		ig->stat = 1;
		/* Read any bytes in the current block that precede the desired sample. */
		while (nb-- > 0 && ig->stat > 0)
			i = r8(ig);
		if (ig->stat <= 0) return (-1);

		/* Reset the getvec sample-within-frame counter. */
		gvc = ispfmax;

		/* Reset the time (if signal 0 belongs to the group) and disable checksum
		   testing (by setting the number of samples remaining to 0). */
		if (s == 0) istime = in_msrec ? t + segp->samp0 : t;
		while (n-- != 0)
			isd[s + n]->info.nsamp = (TimeType)0L;
		return (0);
	}

	/* VFILL provides the value returned by getskewedframe() for a missing or
	   invalid sample */
#define VFILL	((gvmode & WFDB_GVPAD) ? is->samp : WFDB_INVALID_SAMPLE)

	static int getskewedframe(SampleType *vector)
	{
		int c, stat;
		struct isdata *is;
		struct igdata *ig;
		GroupType g;
		SampleType v;
		SignalType s;

		if ((stat = (int)nisig) == 0) return (0);
		if (istime == 0L) {
			for (s = 0; s < nisig; s++)
				isd[s]->samp = isd[s]->info.initval;
			for (g = nigroup; g; ) {
				/* Go through groups in reverse order since seeking on group 0
				   should always be done last. */
				if (--g == 0 || igd[g]->start > 0L)
					(void)isgsetframe(g, 0L);
			}
		}

		for (s = 0; s < nisig; s++) {
			is = isd[s];
			ig = igd[is->info.group];
			for (c = 0; c < is->info.spf; c++, vector++) {
				switch (is->info.fmt) {
				case 0:	/* null signal: return sample tagged as invalid */
					*vector = v = VFILL;
					if (is->info.nsamp == 0) ig->stat = -1;
					break;
				case 8:	/* 8-bit first differences */
				default:
					*vector = v = is->samp += r8(ig); break;
				case 16:	/* 16-bit amplitudes */
					*vector = v = r16(ig);
					if (v == -1 << 15)
						*vector = VFILL;
					else
						is->samp = *vector;
					break;
				case 61:	/* 16-bit amplitudes, bytes swapped */
					*vector = v = r61(ig);
					if (v == -1 << 15)
						*vector = VFILL;
					else
						is->samp = *vector;
					break;
				case 80:	/* 8-bit offset binary amplitudes */
					*vector = v = r80(ig);
					if (v == -1 << 7)
						*vector = VFILL;
					else
						is->samp = *vector;
					break;
				case 160:	/* 16-bit offset binary amplitudes */
					*vector = v = r160(ig);
					if (v == -1 << 15)
						*vector = VFILL;
					else
						is->samp = *vector;
					break;
				case 212:	/* 2 12-bit amplitudes bit-packed in 3 bytes */
					*vector = v = r212(ig);
					if (v == -1 << 11)
						*vector = VFILL;
					else
						is->samp = *vector;
					break;
				case 310:	/* 3 10-bit amplitudes bit-packed in 4 bytes */
					*vector = v = r310(ig);
					if (v == -1 << 9)
						*vector = VFILL;
					else
						is->samp = *vector;
					break;
				case 311:	/* 3 10-bit amplitudes bit-packed in 4 bytes */
					*vector = v = r311(ig);
					if (v == -1 << 9)
						*vector = VFILL;
					else
						is->samp = *vector;
					break;
				case 24:	/* 24-bit amplitudes */
					*vector = v = r24(ig);
					if (v == -1 << 23)
						*vector = VFILL;
					else
						is->samp = *vector;
					break;
				case 32:	/* 32-bit amplitudes */
					*vector = v = r32(ig);
					if (v == -1 << 31)
						*vector = VFILL;
					else
						is->samp = *vector;
					break;
				}
				if (ig->stat <= 0) {
					/* End of file -- reset input counter. */
					ig->count = 0;
					if (is->info.nsamp > (TimeType)0L) {
						_context->error("getvec: unexpected EOF in signal %d\n", s);
						stat = -3;
					}
					else if (in_msrec && segp && segp < segend) {
						segp++;
						if (isigopen(segp->recname, NULL, (int)nvsig) < 0) {
							_context->error("getvec: error opening segment %s\n",
								segp->recname);
							stat = -3;
							return (stat);  /* avoid looping if segment is bad */
						}
						else {
							istime = segp->samp0;
							return (getskewedframe(vector));
						}
					}
					else
						stat = -1;
					if (is->info.nsamp > (TimeType)0L) {
						_context->error("getvec: unexpected EOF in signal %d\n", s);
						stat = -3;
					}
					else
						stat = -1;
				}
				is->info.cksum -= v;
			}
			if (--is->info.nsamp == (TimeType)0L &&
				(is->info.cksum & 0xffff) &&
				!in_msrec && !isedf &&
				is->info.fmt != 0) {
				_context->error("getvec: checksum error in signal %d\n", s);
				stat = -4;
			}
		}
		return (stat);
	}

	static int rgetvec(SampleType *vector)
	{
		SampleType *tp;
		SignalType s;
		static int stat;

		if (ispfmax < 2)	/* all signals at the same frequency */
			return (getframe(vector));

		if ((gvmode & WFDB_HIGHRES) != WFDB_HIGHRES) {
			/* return one sample per frame, decimating by averaging if necessary */
			unsigned c;
			long v;

			stat = getframe(tvector);
			for (s = 0, tp = tvector; s < nvsig; s++) {
				int sf = vsd[s]->info.spf;

				for (c = v = 0; c < sf && *tp != WFDB_INVALID_SAMPLE; c++)
					v += *tp++;
				if (c == sf)
					*vector++ = v / sf;
				else {
					*vector++ = WFDB_INVALID_SAMPLE;
					tp += sf - c;
				}
			}
		}
		else {			/* return ispfmax samples per frame, using
					   zero-order interpolation if necessary */
			if (gvc >= ispfmax) {
				stat = getframe(tvector);
				gvc = 0;
			}
			for (s = 0, tp = tvector; s < nvsig; s++) {
				int sf = vsd[s]->info.spf;

				*vector++ = tp[(sf*gvc) / ispfmax];
				tp += sf;
			}
			gvc++;
		}
		return (stat);
	}

	/* WFDB library functions. */

	FINT isigopen(char *record, WFDB_Siginfo *siarray, int nsig)
	{
		int navail;
		int ngroups;
		struct isdata *is;
		SignalType s, si, sj;
		GroupType g;

		/* Close previously opened input signals unless otherwise requested. */
		if (*record == '+') record++;
		else isigclose();

		/* Remove trailing .hea, if any, from record name. */
		wfdb_striphea(record);

		/* Save the current record name. */
		if (!in_msrec) wfdb_setirec(record);

		/* Read the header and determine how many signals are available. */
		if ((navail = readheader(record)) <= 0) {
			if (navail == 0 && segments) {	/* this is a multi-segment record */
				in_msrec = 1;
				/* Open the first segment to get signal information. */
				if (segp && (navail = readheader(segp->recname)) >= 0) {
					if (msbtime == 0L) msbtime = btime;
					if (msbdate == (DateType)0) msbdate = bdate;
				}
			}
			if (navail == 0 && nsig)
				_context->error("isigopen: record %s has no signals\n", record);
			if (navail <= 0)
				return (navail);
		}

		/* If nsig <= 0, isigopen fills in up to (-nsig) members of siarray based
		   on the contents of the header, but no signals are actually opened.  The
		   value returned is the number of signals named in the header. */
		if (nsig <= 0) {
			nsig = -nsig;
			if (navail < nsig) nsig = navail;
			if (siarray != NULL)
				for (s = 0; s < nsig; s++)
					siarray[s] = hsd[s]->info;
			in_msrec = 0;	/* necessary to avoid errors when reopening */
			return (navail);
		}

		/* Determine how many new signals we should attempt to open.  The caller's
		   upper limit on this number is nsig, and the upper limit defined by the
		   header is navail. */
		if (nsig > navail) nsig = navail;

		/* Allocate input signals and signal group workspace. */
		int nn = nisig + nsig;
		if (allocisig(nn) != nn)
			return (-1);	/* failed, nisig is unchanged, allocisig emits error */
		nsig = nn;
		nn = nigroup + hsd[nsig - nisig - 1]->info.group + 1;
		if (allocigroup(nn) != nn)
			return (-1);	/* failed, allocigroup emits error */
		ngroups = nn;

		/* Set default buffer size (if not set already by setibsize). */
		if (ibsize <= 0) ibsize = BUFSIZ;

		/* Open the signal files.  One signal group is handled per iteration.  In
		   this loop, si counts through the entries that have been read from hsd,
		   and s counts the entries that have been added to isd. */
		for (g = si = s = 0; si < navail && s < nsig; si = sj) {
			struct hsdata* hs = hsd[si];
			is = isd[nisig + s];
			struct igdata* ig = igd[nigroup + g];

			/* Find out how many signals are in this group. */
			for (sj = si + 1; sj < navail; sj++)
				if (hsd[sj]->info.group != hs->info.group) break;

			/* Skip this group if there are too few slots in the caller's array. */
			if (sj - si > nsig - s) continue;

			/* Set the buffer size and the seek capability flag. */
			if (hs->info.bsize < 0) {
				ig->bsize = hs->info.bsize = -hs->info.bsize;
				ig->seek = 0;
			}
			else {
				if ((ig->bsize = hs->info.bsize) == 0) ig->bsize = ibsize;
				ig->seek = 1;
			}
			SALLOC(ig->buf, 1, ig->bsize);

			/* Check that the signal file is readable. */
			if (hs->info.fmt == 0)
				ig->fp = NULL;	/* Don't open a file for a null signal. */
			else {
				ig->fp = wfdb_open(hs->info.fname, (char *)NULL, WFDB_READ);
				/* Skip this group if the signal file can't be opened. */
				if (ig->fp == NULL) {
					SFREE(ig->buf);
					continue;
				}
			}

			/* All tests passed -- fill in remaining data for this group. */
			ig->be = ig->bp = ig->buf + ig->bsize;
			ig->start = hs->start;
			ig->stat = 1;
			while (si < sj && s < nsig) {
				copysi(&is->info, &hs->info);
				is->info.group = nigroup + g;
				is->skew = hs->skew;
				++s;
				if (++si < sj) {
					hs = hsd[si];
					is = isd[nisig + s];
				}
			}
			g++;
		}

		/* Produce a warning message if none of the requested signals could be
		   opened. */
		if (s == 0 && nsig)
			_context->error("isigopen: none of the signals for record %s is readable\n",
				record);

		/* Copy the WFDB_Siginfo structures to the caller's array.  Use these
		   data to construct the initial sample vector, and to determine the
		   maximum number of samples per signal per frame and the maximum skew. */
		for (si = 0; si < s; si++) {
			is = isd[nisig + si];
			if (siarray)
				copysi(&siarray[si], &is->info);
			is->samp = is->info.initval;
			if (ispfmax < is->info.spf) ispfmax = is->info.spf;
			if (skewmax < is->skew) skewmax = is->skew;
		}
		nisig += s;		/* Update the count of open input signals. */
		nigroup += g;	/* Update the count of open input signal groups. */
		if (sigmap_init() < 0)
			return (-1);
		setgvmode(gvmode);	/* Reset sfreq if appropriate. */
		gvc = ispfmax;	/* Initialize getvec's sample-within-frame counter. */

		/* Determine the total number of samples per frame. */
		for (si = framelen = 0; si < nisig; si++)
			framelen += isd[si]->info.spf;

		/* Allocate workspace for getvec, isgsettime, and tnextvec. */
		if (framelen > tuvlen) {
			SREALLOC(tvector, framelen, sizeof(WFDB_Sample));
			SREALLOC(uvector, framelen, sizeof(WFDB_Sample));
			if (nvsig > nisig) {
				int vframelen;
				for (si = vframelen = 0; si < nvsig; si++)
					vframelen += vsd[si]->info.spf;
				SREALLOC(vvector, vframelen, sizeof(WFDB_Sample));
			}
			else
				SREALLOC(vvector, framelen, sizeof(WFDB_Sample));
			tuvlen = framelen;
		}

		/* If deskewing is required, allocate the deskewing buffer (unless this is
		   a multi-segment record and dsbuf has been allocated already). */
		if (skewmax != 0 && (!in_msrec || dsbuf == NULL)) {
			dsbi = -1;	/* mark buffer contents as invalid */
			dsblen = framelen * (skewmax + 1);
			SALLOC(dsbuf, dsblen, sizeof(WFDB_Sample));
		}
		return (s);
	}

	FINT osigopen(char *record, WFDB_Siginfo *siarray, unsigned int nsig)
	{
		int n;
		struct osdata *os, *op;
		struct ogdata *og;
		SignalType s;
		unsigned int ga;

		/* Close previously opened output signals unless otherwise requested. */
		if (*record == '+') record++;
		else osigclose();

		/* Remove trailing .hea, if any, from record name. */
		wfdb_striphea(record);

		if ((n = readheader(record)) < 0)
			return (n);
		if (n < nsig) {
			_context->error("osigopen: record %s has fewer signals than needed\n",
				record);
			return (-3);
		}

		/* Allocate workspace for output signals. */
		if (allocosig(nosig + nsig) < 0) return (-3);
		/* Allocate workspace for output signal groups. */
		if (allocogroup(nogroup + hsd[nsig - 1]->info.group + 1) < 0) return (-3);

		/* Initialize local variables. */
		if (obsize <= 0) obsize = BUFSIZ;

		/* Set the group number adjustment.  This quantity is added to the group
		   numbers of signals which are opened below;  it accounts for any output
		   signals which were left open from previous calls. */
		ga = nogroup;

		/* Open the signal files.  One signal is handled per iteration. */
		for (s = 0, os = osd[nosig]; s < nsig; s++, nosig++, siarray++) {
			op = os;
			os = osd[nosig];

			/* Copy signal information from readheader's workspace. */
			copysi(&os->info, &hsd[s]->info);
			copysi(siarray, &hsd[s]->info);
			if (os->info.spf < 1) os->info.spf = siarray->spf = 1;
			os->info.cksum = siarray->cksum = 0;
			os->info.nsamp = siarray->nsamp = (TimeType)0L;
			os->info.group += ga; siarray->group += ga;

			if (s == 0 || os->info.group != op->info.group) {
				/* This is the first signal in a new group; allocate buffer. */
				size_t obuflen;

				og = ogd[os->info.group];
				og->bsize = os->info.bsize;
				obuflen = og->bsize ? og->bsize : obsize;
				SALLOC(og->buf, 1, obuflen);
				og->bp = og->buf;
				og->be = og->buf + obuflen;
				if (os->info.fmt == 0) {
					/* If the signal file name was NULL or "~", don't create a
					   signal file. */
					if (os->info.fname == NULL || strcmp("~", os->info.fname) == 0)
						og->fp = NULL;
					/* Otherwise, assume that the user wants to write a signal
					   file in the default format (16). */
					else
						os->info.fmt = 16;
				}
				if (os->info.fmt != 0) {
					/* An error in opening an output file is fatal. */
					og->fp = wfdb_open(os->info.fname, (char *)NULL, WFDB_WRITE);
					if (og->fp == NULL) {
						_context->error("osigopen: can't open %s\n", os->info.fname);
						SFREE(og->buf);
						osigclose();
						return (-3);
					}
				}
				nogroup++;
			}
			else {
				/* This signal belongs to the same group as the previous signal. */
				if (os->info.fmt != op->info.fmt ||
					os->info.bsize != op->info.bsize) {
					_context->error(
						"osigopen: error in specification of signal %d or %d\n",
						s - 1, s);
					return (-2);
				}
			}
		}
		return (s);
	}

	FINT osigfopen(WFDB_Siginfo *siarray, unsigned int nsig)
	{
		struct osdata *os, *op;
		struct ogdata *og;
		int s;
		WFDB_Siginfo *si;

		/* Close any open output signals. */
		osigclose();

		/* Do nothing further if there are no signals to open. */
		if (siarray == NULL || nsig == 0) return (0);

		if (obsize <= 0) obsize = BUFSIZ;

		/* Prescan siarray to check the signal specifications and to determine
		   the number of signal groups. */
		for (s = 0, si = siarray; s < nsig; s++, si++) {
			/* The combined lengths of the fname and desc strings should be 200
			   characters or less, the bsize field must not be negative, the
			   format should be legal, group numbers should be the same if and
			   only if file names are the same, and group numbers should begin
			   at zero and increase in steps of 1. */
			if (strlen(si->fname) + strlen(si->desc) > 200 ||
				si->bsize < 0 || !isfmt(si->fmt)) {
				_context->error("osigfopen: error in specification of signal %d\n",
					s);
				return (-2);
			}
			if (!((s == 0 && si->group == 0) ||
				(s && si->group == (si - 1)->group &&
					strcmp(si->fname, (si - 1)->fname) == 0) ||
					(s && si->group == (si - 1)->group + 1 &&
						strcmp(si->fname, (si - 1)->fname) != 0))) {
				_context->error(
					"osigfopen: incorrect file name or group for signal %d\n",
					s);
				return (-2);
			}
		}

		/* Allocate workspace for output signals. */
		if (allocosig(nsig) < 0) return (-3);
		/* Allocate workspace for output signal groups. */
		if (allocogroup((si - 1)->group + 1) < 0) return (-3);

		/* Open the signal files.  One signal is handled per iteration. */
		for (os = osd[0]; nosig < nsig; nosig++, siarray++) {

			op = os;
			os = osd[nosig];
			/* Check signal specifications.  The combined lengths of the fname
			   and desc strings should be 200 characters or less, the bsize field
			   must not be negative, the format should be legal, group numbers
			   should be the same if and only if file names are the same, and
			   group numbers should begin at zero and increase in steps of 1. */
			if (strlen(siarray->fname) + strlen(siarray->desc) > 200 ||
				siarray->bsize < 0 || !isfmt(siarray->fmt)) {
				_context->error("osigfopen: error in specification of signal %d\n",
					nosig);
				return (-2);
			}
			if (!((nosig == 0 && siarray->group == 0) ||
				(nosig && siarray->group == (siarray - 1)->group &&
					strcmp(siarray->fname, (siarray - 1)->fname) == 0) ||
					(nosig && siarray->group == (siarray - 1)->group + 1 &&
						strcmp(siarray->fname, (siarray - 1)->fname) != 0))) {
				_context->error(
					"osigfopen: incorrect file name or group for signal %d\n",
					nosig);
				return (-2);
			}

			/* Copy signal information from the caller's array. */
			copysi(&os->info, siarray);
			if (os->info.spf < 1) os->info.spf = 1;
			os->info.cksum = 0;
			os->info.nsamp = (TimeType)0L;

			/* Check if this signal is in the same group as the previous one. */
			if (nosig == 0 || os->info.group != op->info.group) {
				size_t obuflen;

				og = ogd[os->info.group];
				og->bsize = os->info.bsize;
				obuflen = og->bsize ? og->bsize : obsize;
				/* This is the first signal in a new group; allocate buffer. */
				SALLOC(og->buf, 1, obuflen);
				og->bp = og->buf;
				og->be = og->buf + obuflen;
				if (os->info.fmt == 0) {
					/* If the signal file name was NULL or "~", don't create a
					   signal file. */
					if (os->info.fname == NULL || strcmp("~", os->info.fname) == 0)
						og->fp = NULL;
					/* Otherwise, assume that the user wants to write a signal
					   file in the default format (16). */
					else
						os->info.fmt = 16;
				}
				if (os->info.fmt != 0) {
					/* An error in opening an output file is fatal. */
					og->fp = wfdb_open(os->info.fname, (char *)NULL, WFDB_WRITE);
					if (og->fp == NULL) {
						_context->error("osigfopen: can't open %s\n", os->info.fname);
						SFREE(og->buf);
						osigclose();
						return (-3);
					}
				}
				nogroup++;
			}
			else {
				/* This signal belongs to the same group as the previous signal. */
				if (os->info.fmt != op->info.fmt ||
					os->info.bsize != op->info.bsize) {
					_context->error(
						"osigfopen: error in specification of signal %d or %d\n",
						nosig - 1, nosig);
					return (-2);
				}
			}
		}

		return (nosig);
	}

	/* Function findsig finds an open input signal with the name specified by its
	(string) argument, and returns the associated signal number.  If the argument
	is a decimal numeral and is less than the number of open input signals, it is
	assumed to represent a signal number, which is returned.  Otherwise, findsig
	looks for a signal with a description matching the string, and returns the
	first match if any, or -1 if not. */

	int findsig(char *p)
	{
		char *q = p;
		int s;

		while ('0' <= *q && *q <= '9')
			q++;
		if (*q == 0) {	/* all digits, probably a signal number */
			s = strtol(p, NULL, 10);
			if (s < nisig || s < nvsig) return (s);
		}
		/* Otherwise, p is either an integer too large to be a signal number or a
		   string containing a non-digit character.  Assume it's a signal name. */
		if (need_sigmap) {
			for (s = 0; s < nvsig; s++)
				if ((q = vsd[s]->info.desc) && strcmp(p, q) == 0) return (s);
		}
		else {
			for (s = 0; s < nisig; s++)
				if ((q = isd[s]->info.desc) && strcmp(p, q) == 0) return (s);
		}
		/* No match found. */
		return (-1);
	}

	/* Function getvec can operate in two different modes when reading
	multifrequency records.  In WFDB_LOWRES mode, getvec returns one sample of each
	signal per frame (decimating any oversampled signal by returning the average of
	all of its samples within the frame).  In WFDB_HIGHRES mode, each sample of any
	oversampled signal is returned by successive invocations of getvec; samples of
	signals sampled at lower frequencies are returned on two or more successive
	invocations of getvec as appropriate.  Function setgvmode can be used to change
	getvec's operating mode, which is determined by environment variable
	WFDBGVMODE or constant DEFWFDBGVMODE by default.  When reading
	ordinary records (with all signals sampled at the same frequency), getvec's
	behavior is independent of its operating mode.

	Since sfreq and ffreq are always positive, the effect of adding 0.5 to their
	quotient before truncating it to an int (see below) is to round the quotient
	to the nearest integer.  Although sfreq should always be an exact multiple
	of ffreq, loss of precision in representing non-integer frequencies can cause
	problems if this rounding is omitted.  Thanks to Guido Muesch for pointing out
	this problem.
	 */

	FINT getspf(void)
	{
		return ((sfreq != ffreq) ? (int)(sfreq / ffreq + 0.5) : 1);
	}

	FVOID setgvmode(int mode)
	{
		if (mode < 0) {	/* (re)set to default mode */
			char *p;

			if (p = getenv("WFDBGVMODE"))
				mode = strtol(p, NULL, 10);
			else
				mode = DEFWFDBGVMODE;
		}

		if ((mode & WFDB_HIGHRES) == WFDB_HIGHRES) {
			gvmode |= WFDB_HIGHRES;
			if (ispfmax == 0) ispfmax = 1;
			sfreq = ffreq * ispfmax;
		}
		else {
			gvmode &= ~(WFDB_HIGHRES);
			sfreq = ffreq;
		}
	}

	FINT getgvmode(void)
	{
		return (gvmode);
	}

	/* An application can specify the input sampling frequency it prefers by
	   calling setifreq after opening the input record. */

	static long mticks, nticks, mnticks;
	static int rgvstat;
	static TimeType rgvtime, gvtime;
	static SampleType *gv0, *gv1;

	FINT setifreq(FrequencyType f)
	{
		FrequencyType error, g = sfreq;

		if (g <= 0.0) {
			ifreq = 0.0;
			_context->error("setifreq: no open input record\n");
			return (-1);
		}
		if (f > 0.0) {
			if (nvsig > 0) {
				SREALLOC(gv0, nvsig, sizeof(WFDB_Sample));
				SREALLOC(gv1, nvsig, sizeof(WFDB_Sample));
			}
			setafreq(ifreq = f);
			/* The 0.005 below is the maximum tolerable error in the resampling
			   frequency (in Hz).  The code in the while loop implements Euclid's
			   algorithm for finding the greatest common divisor of two integers,
			   but in this case the integers are (implicit) multiples of 0.005. */
			while ((error = f - g) > 0.005 || error < -0.005)
				if (f > g) f -= g;
				else g -= f;
			/* f is now the GCD of sfreq and ifreq in the sense described above.
			   We divide each raw sampling interval into mticks subintervals. */
			mticks = (long)(sfreq / f + 0.5);
			/* We divide each resampled interval into nticks subintervals. */
			nticks = (long)(ifreq / f + 0.5);
			/* Raw and resampled intervals begin simultaneously once every mnticks
			   subintervals; we say an epoch begins at these times. */
			mnticks = mticks * nticks;
			/* gvtime is the number of subintervals from the beginning of the
			   current epoch to the next sample to be returned by getvec(). */
			gvtime = 0;
			rgvstat = rgetvec(gv0);
			rgvstat = rgetvec(gv1);
			/* rgvtime is the number of subintervals from the beginning of the
			   current epoch to the most recent sample returned by rgetvec(). */
			rgvtime = nticks;
			return (0);
		}
		else {
			ifreq = 0.0;
			_context->error("setifreq: improper frequency %g (must be > 0)\n", f);
			return (-1);
		}
	}

	FFREQUENCY getifreq(void)
	{
		return (ifreq > (FrequencyType)0 ? ifreq : sfreq);
	}

	FINT getvec(SampleType *vector)
	{
		int i, nsig;

		if (ifreq == 0.0 || ifreq == sfreq)	/* no resampling necessary */
			return (rgetvec(vector));

		/* Resample the input. */
		if (rgvtime > mnticks) {
			rgvtime -= mnticks;
			gvtime -= mnticks;
		}
		nsig = (nvsig > nisig) ? nvsig : nisig;
		while (gvtime > rgvtime) {
			for (i = 0; i < nsig; i++)
				gv0[i] = gv1[i];
			rgvstat = rgetvec(gv1);
			rgvtime += nticks;
		}
		for (i = 0; i < nsig; i++) {
			vector[i] = gv0[i] + (gvtime%nticks)*(gv1[i] - gv0[i]) / nticks;
			gv0[i] = gv1[i];
		}
		gvtime += mticks;
		return (rgvstat);
	}

	FINT getframe(SampleType *vector)
	{
		int stat;

		if (dsbuf) {	/* signals must be deskewed */
			int c, i, j, nsig, s;

			/* First, obtain the samples needed. */
			if (dsbi < 0) {	/* dsbuf contents are invalid -- refill dsbuf */
				for (dsbi = i = 0; i < dsblen; dsbi = i += framelen)
					stat = getskewedframe(dsbuf + dsbi);
				dsbi = 0;
			}
			else {		/* replace oldest frame in dsbuf only */
				stat = getskewedframe(dsbuf + dsbi);
				if ((dsbi += framelen) >= dsblen) dsbi = 0;
			}
			/* Assemble the deskewed frame from the data in dsbuf. */
			nsig = (nvsig > nisig) ? nvsig : nisig;
			for (j = s = 0; s < nsig; s++) {
				if ((i = j + dsbi + isd[s]->skew*framelen) >= dsblen) i -= dsblen;
				for (c = 0; c < isd[s]->info.spf; c++)
					vector[j++] = dsbuf[i++];
			}
		}
		else		/* no deskewing necessary */
			stat = getskewedframe(vector);
		if (need_sigmap && stat > 0)
			stat = sigmap(vector);
		istime++;
		return (stat);
	}

	FINT putvec(SampleType *vector)
	{
		int c, dif, stat = (int)nosig;
		struct osdata *os;
		struct ogdata *og;
		SignalType s;
		GroupType g;

		for (s = 0; s < nosig; s++) {
			os = osd[s];
			g = os->info.group;
			og = ogd[os->info.group];
			if (os->info.nsamp++ == (TimeType)0L)
				os->info.initval = os->samp = *vector;
			for (c = 0; c < os->info.spf; c++, vector++) {
				/* Replace invalid samples with lowest possible value for format */
				if (*vector == WFDB_INVALID_SAMPLE)
					switch (os->info.fmt) {
					case 0:
					case 8:
					case 16:
					case 61:
					case 160:
					default:
						*vector = -1 << 15; break;
					case 80:
						*vector = -1 << 7; break;
					case 212:
						*vector = -1 << 11; break;
					case 310:
					case 311:
						*vector = -1 << 9; break;
					case 24:
						*vector = -1 << 23; break;
					case 32:
						*vector = -1 << 31; break;
					}
				switch (os->info.fmt) {
				case 0:	/* null signal (do not write) */
					os->samp = *vector; break;
				case 8:	/* 8-bit first differences */
				default:
					/* Handle large slew rates sensibly. */
					if ((dif = *vector - os->samp) < -128) { dif = -128; stat = 0; }
					else if (dif > 127) { dif = 127; stat = 0; }
					os->samp += dif;
					w8(dif, og);
					break;
				case 16:	/* 16-bit amplitudes */
					w16(*vector, og); os->samp = *vector; break;
				case 61:	/* 16-bit amplitudes, bytes swapped */
					w61(*vector, og); os->samp = *vector; break;
				case 80:	/* 8-bit offset binary amplitudes */
					w80(*vector, og); os->samp = *vector; break;
				case 160:	/* 16-bit offset binary amplitudes */
					w160(*vector, og); os->samp = *vector; break;
				case 212:	/* 2 12-bit amplitudes bit-packed in 3 bytes */
					w212(*vector, og); os->samp = *vector; break;
				case 310:	/* 3 10-bit amplitudes bit-packed in 4 bytes */
					w310(*vector, og); os->samp = *vector; break;
				case 311:	/* 3 10-bit amplitudes bit-packed in 4 bytes */
					w311(*vector, og); os->samp = *vector; break;
				case 24: /* 24-bit amplitudes */
					w24(*vector, og); os->samp = *vector; break;
				case 32: /* 32-bit amplitudes */
					w32(*vector, og); os->samp = *vector; break;
				}
				if (wfdb_ferror(og->fp)) {
					_context->error("putvec: write error in signal %d\n", s);
					stat = -1;
				}
				else
					os->info.cksum += os->samp;
			}
		}
		ostime++;
		return (stat);
	}

	FINT isigsettime(TimeType t)
	{
		GroupType g;
		TimeType curtime;
		int stat = 0;

		/* Return immediately if no seek is needed. */
		if (nisig == 0) return (0);
		if (ifreq <= (FrequencyType)0) {
			if (sfreq == ffreq)
				curtime = istime;
			else
				curtime = (istime - 1) * ispfmax + gvc;
			if (t == curtime) return (0);
		}

		for (g = 1; g < nigroup; g++)
			if ((stat = isgsettime(g, t)) < 0) break;
		/* Seek on signal group 0 last (since doing so updates istime and would
		   confuse isgsettime if done first). */
		if (stat == 0) stat = isgsettime(0, t);
		return (stat);
	}

	FINT isgsettime(GroupType g, TimeType t)
	{
		int spf, stat, trem = 0;

		/* Handle negative arguments as equivalent positive arguments. */
		if (t < 0L) t = -t;

		/* Convert t to raw sample intervals if we are resampling. */
		if (ifreq > (FrequencyType)0)
			t = (TimeType)(t * sfreq / ifreq);

		/* If we're in WFDB_HIGHRES mode, convert t from samples to frames, and
		   save the remainder (if any) in trem. */
		if (sfreq != ffreq) {
			spf = (int)(sfreq / ffreq);
			trem = t % spf;
			t /= spf;
		}

		if ((stat = isgsetframe(g, t)) == 0 && g == 0) {
			while (trem-- > 0) {
				if (rgetvec(uvector) < 0) {
					_context->error("isigsettime: improper seek on signal group %d\n",
						g);
					return (-1);
				}
			}
			if (ifreq > (FrequencyType)0 && ifreq != sfreq) {
				gvtime = 0;
				rgvstat = rgetvec(gv0);
				rgvstat = rgetvec(gv1);
				rgvtime = nticks;
			}
		}

		return (stat);
	}

	FSITIME tnextvec(SignalType s, TimeType t)
	{
		int stat = 0;
		TimeType tf;

		if (in_msrec && need_sigmap) { /* variable-layout multi-segment record */
			if (s >= nvsig) {
				_context->error("nextvect: illegal signal number %d\n", s);
				return ((TimeType)-1);
			}
			/* Go to the start (t) if not already there. */
			if (t != istime && isigsettime(t) < 0) return ((TimeType)-1);
			while (stat >= 0) {
				char *p = vsd[s]->info.desc, *q;
				int ss;

				tf = segp->samp0 + segp->nsamp;  /* end of current segment */
				/* Check if signal s is available in the current segment. */
				for (ss = 0; ss < nisig; ss++)
					if ((q = isd[ss]->info.desc) && strcmp(p, q) == 0)
						break;
				if (ss < nisig) {
					/* The current segment contains the desired signal.
					   Read samples until we find a valid one or reach
					   the end of the segment. */
					for (; t <= tf && (stat = getvec(vvector)) > 0; t++)
						if (vvector[s] != WFDB_INVALID_SAMPLE) {
							isigsettime(t);
							return (t);
						}
					if (stat < 0) return ((TimeType)-1);
				}
				/* Go on to the next segment. */
				if (t != tf) stat = isigsettime(t = tf);
			}
		}
		else {	/* single-segment or fixed-layout multi-segment record */
		/* Go to the start (t) if not already there. */
			if (t != istime && isigsettime(t) < 0) return ((TimeType)-1);
			if (s >= nisig) {
				_context->error("nextvect: illegal signal number %d\n", s);
				return ((TimeType)-1);
			}
			for (; (stat = getvec(vvector)) > 0; t++)
				/* Read samples until we find a valid one or reach the end of the
				   record. */
				if (vvector[s] != WFDB_INVALID_SAMPLE) {
					isigsettime(t);
					return (t);
				}
		}
		/* Error or end of record without finding another sample of signal s. */
		return ((TimeType)stat);
	}

	int inputBufferSize(int n)
	{
		if (nisig) {
			_context->error("setibsize: can't change buffer size after isigopen\n");
			return (-1);
		}
		if (n < 0) {
			_context->error("setibsize: illegal buffer size %d\n", n);
			return (-2);
		}
		if (n == 0) n = BUFSIZ;
		return (ibsize = n);
	}
	int inputBufferSize() const
	{
		return ibsize;
	}

	int outputBufferSize(int n)
	{
		if (nosig) {
			_context->error("setobsize: can't change buffer size after osig[f]open\n");
			return (-1);
		}
		if (n < 0) {
			_context->error("setobsize: illegal buffer size %d\n", n);
			return (-2);
		}
		if (n == 0) n = BUFSIZ;
		return (obsize = n);
	}
	int outputBufferSize()const
	{
		return obsize;
	}

	int newheader(char *record)
	{
		int stat;
		SignalType s;

		/* Remove trailing .hea, if any, from record name. */
		wfdb_striphea(record);
		WFDB_Siginfo* osi = new WFDB_Siginfo[nosig];
		for (s = 0; s < nosig; s++)
			copysi(&osi[s], &osd[s]->info);
		stat = setheader(record, osi, nosig);
		delete[] osi;
		return (stat);
	}

	int setheader(char *record, WFDB_Siginfo *siarray, unsigned int nsig)
	{
		char *p;
		SignalType s;

		/* If another output header file was opened, close it. */
		if (oheader) {
			(void)wfdb_fclose(oheader);
			if (outinfo == oheader) outinfo = NULL;
			oheader = NULL;
		}

		/* Remove trailing .hea, if any, from record name. */
		wfdb_striphea(record);

		/* Quit (with message from wfdb_checkname) if name is illegal. */
		if (wfdb_checkname(record, "record"))
			return (-1);

		/* Try to create the header file. */
		if ((oheader = wfdb_open("hea", record, WFDB_WRITE)) == NULL) {
			_context->error("newheader: can't create header for record %s\n", record);
			return (-1);
		}

		/* Write the general information line. */
		(void)wfdb_fprintf(oheader, "%s %d %.12g", record, nsig, ffreq);
		if ((cfreq > 0.0 && cfreq != ffreq) || bcount != 0.0) {
			(void)wfdb_fprintf(oheader, "/%.12g", cfreq);
			if (bcount != 0.0)
				(void)wfdb_fprintf(oheader, "(%.12g)", bcount);
		}
		(void)wfdb_fprintf(oheader, " %ld", nsig > 0 ? siarray[0].nsamp : 0L);
		if (btime != 0L || bdate != (DateType)0) {
			if (btime == 0L)
				(void)wfdb_fprintf(oheader, " 0:00");
			else if (btime % 1000 == 0)
				(void)wfdb_fprintf(oheader, " %s",
					ftimstr(btime, 1000.0));
			else
				(void)wfdb_fprintf(oheader, " %s",
					fmstimstr(btime, 1000.0));
		}
		if (bdate)
			(void)wfdb_fprintf(oheader, "%s", datstr(bdate));
		(void)wfdb_fprintf(oheader, "\r\n");

		/* Write a signal specification line for each signal. */
		for (s = 0; s < nsig; s++) {
			(void)wfdb_fprintf(oheader, "%s %d", siarray[s].fname, siarray[s].fmt);
			if (siarray[s].spf > 1)
				(void)wfdb_fprintf(oheader, "x%d", siarray[s].spf);
			if (osd && osd[s]->skew)
				(void)wfdb_fprintf(oheader, ":%d", osd[s]->skew*siarray[s].spf);
			if (ogd && ogd[osd[s]->info.group]->start)
				(void)wfdb_fprintf(oheader, "+%ld",
					ogd[osd[s]->info.group]->start);
			else if (prolog_bytes)
				(void)wfdb_fprintf(oheader, "+%ld", prolog_bytes);
			(void)wfdb_fprintf(oheader, " %.12g", siarray[s].gain);
			if (siarray[s].baseline != siarray[s].adczero)
				(void)wfdb_fprintf(oheader, "(%d)", siarray[s].baseline);
			if (siarray[s].units && (p = strtok(siarray[s].units, " \t\n\r")))
				(void)wfdb_fprintf(oheader, "/%s", p);
			(void)wfdb_fprintf(oheader, " %d %d %d %d %d",
				siarray[s].adcres, siarray[s].adczero, siarray[s].initval,
				(short int)(siarray[s].cksum & 0xffff), siarray[s].bsize);
			if (siarray[s].desc && (p = strtok(siarray[s].desc, "\n\r")))
				(void)wfdb_fprintf(oheader, " %s", p);
			(void)wfdb_fprintf(oheader, "\r\n");
		}
		prolog_bytes = 0L;
		(void)wfdb_fflush(oheader);
		return (0);
	}

	FINT getseginfo(WFDB_Seginfo **sarray)
	{
		*sarray = segarray;
		return (segments);
	}

	FINT setmsheader(char *record, char **segment_name, unsigned int nsegments)
	{
		FrequencyType msfreq, mscfreq;
		double msbcount;
		int n, nsig, old_in_msrec = in_msrec;
		long *ns;
		unsigned i;

		isigclose();	/* close any open input signals */

		/* If another output header file was opened, close it. */
		if (oheader) {
			(void)wfdb_fclose(oheader);
			if (outinfo == oheader) outinfo = NULL;
			oheader = NULL;
		}

		/* Remove trailing .hea, if any, from record name. */
		wfdb_striphea(record);

		/* Quit (with message from wfdb_checkname) if name is illegal. */
		if (wfdb_checkname(record, "record"))
			return (-1);

		if (nsegments < 1) {
			_context->error("setmsheader: record must contain at least one segment\n");
			return (-1);
		}

		SUALLOC(ns, nsegments, (sizeof(long)*nsegments));
		for (i = 0; i < nsegments; i++) {
			if (strlen(segment_name[i]) > WFDB_MAXRNL) {
				_context->error(
					"setmsheader: `%s' is too long for a segment name in record %s\n",
					segment_name[i], record);
				SFREE(ns);
				return (-2);
			}
			in_msrec = 1;
			n = readheader(segment_name[i]);
			in_msrec = old_in_msrec;
			if (n < 0) {
				_context->error("setmsheader: can't read segment %s header\n",
					segment_name[i]);
				SFREE(ns);
				return (-3);
			}
			if ((ns[i] = hsd[0]->info.nsamp) <= 0L) {
				_context->error("setmsheader: length of segment %s must be specified\n",
					segment_name[i]);
				SFREE(ns);
				return (-4);
			}
			if (i == 0) {
				nsig = n;
				msfreq = ffreq;
				mscfreq = cfreq;
				msbcount = bcount;
				msbtime = btime;
				msbdate = bdate;
				msnsamples = ns[i];
			}
			else {
				if (nsig != n) {
					_context->error(
						"setmsheader: incorrect number of signals in segment %s\n",
						segment_name[i]);
					SFREE(ns);
					return (-4);
				}
				if (msfreq != ffreq) {
					_context->error(
						"setmsheader: incorrect sampling frequency in segment %s\n",
						segment_name[i]);
					SFREE(ns);
					return (-4);
				}
				msnsamples += ns[i];
			}
		}

		/* Try to create the header file. */
		if ((oheader = wfdb_open("hea", record, WFDB_WRITE)) == NULL) {
			_context->error("setmsheader: can't create header file for record %s\n",
				record);
			SFREE(ns);
			return (-1);
		}

		/* Write the first line of the master header. */
		(void)wfdb_fprintf(oheader, "%s/%u %d %.12g", record, nsegments, nsig, msfreq);
		if ((mscfreq > 0.0 && mscfreq != msfreq) || msbcount != 0.0) {
			(void)wfdb_fprintf(oheader, "/%.12g", mscfreq);
			if (msbcount != 0.0)
				(void)wfdb_fprintf(oheader, "(%.12g)", msbcount);
		}
		(void)wfdb_fprintf(oheader, " %ld", msnsamples);
		if (msbtime != 0L || msbdate != (DateType)0) {
			if (msbtime % 1000 == 0)
				(void)wfdb_fprintf(oheader, " %s",
					ftimstr(msbtime, 1000.0));
			else
				(void)wfdb_fprintf(oheader, " %s",
					fmstimstr(msbtime, 1000.0));
		}
		if (msbdate)
			(void)wfdb_fprintf(oheader, "%s", datstr(msbdate));
		(void)wfdb_fprintf(oheader, "\r\n");

		/* Write a line for each segment. */
		for (i = 0; i < nsegments; i++)
			(void)wfdb_fprintf(oheader, "%s %ld\r\n", segment_name[i], ns[i]);

		SFREE(ns);
		return (0);
	}

	FINT wfdbgetskew(SignalType s)
	{
		if (s < nisig)
			return (vsd[s]->skew);
		else
			return (0);
	}

	/* Careful!!  This function is dangerous, and should be used only to restore
	   skews when they have been reset as a side effect of using, e.g., sampfreq */
	FVOID wfdbsetiskew(SignalType s, int skew)
	{
		if (s < nisig)
			vsd[s]->skew = skew;
	}

	/* Note: wfdbsetskew affects *only* the skew to be written by setheader.
	   It does not affect how getframe deskews input signals, nor does it
	   affect the value returned by wfdbgetskew. */
	FVOID wfdbsetskew(SignalType s, int skew)
	{
		if (s < nosig)
			osd[s]->skew = skew;
	}

	FLONGINT wfdbgetstart(SignalType s)
	{
		if (s < nisig)
			return (igd[vsd[s]->info.group]->start);
		else if (s == 0 && hsd != NULL)
			return (hsd[0]->start);
		else
			return (0L);
	}

	/* Note: wfdbsetstart affects *only* the byte offset to be written by
	   setheader.  It does not affect how isgsettime calculates byte offsets, nor
	   does it affect the value returned by wfdbgetstart. */
	FVOID wfdbsetstart(SignalType s, long int bytes)
	{
		if (s < nosig)
			ogd[osd[s]->info.group]->start = bytes;
		prolog_bytes = bytes;
	}

	FINT wfdbputprolog(char *buf, long int size, SignalType s)
	{
		long int n;
		GroupType g = osd[s]->info.group;

		n = wfdb_fwrite(buf, 1, size, ogd[g]->fp);
		wfdbsetstart(s, n);
		if (n != size)
			_context->error("wfdbputprolog: only %ld of %ld bytes written\n", n, size);
		return (n == size ? 0 : -1);
	}

	/* Create a .info file (or open it for appending) */
	FINT setinfo(char *record)
	{
		/* Close any previously opened output info file. */
		wfdb_oinfoclose();

		/* Quit unless a record name has been specified. */
		if (record == NULL) return (0);

		/* Remove trailing .hea, if any, from record name. */
		wfdb_striphea(record);

		/* Quit (with message from wfdb_checkname) if name is illegal. */
		if (wfdb_checkname(record, "record"))
			return (-1);

		/* Try to create the .info file. */
		if ((outinfo = wfdb_open("info", record, WFDB_APPEND)) == NULL) {
			_context->error("setinfo: can't create info file for record %s\n", record);
			return (-1);
		}

		/* Success! */
		return (0);
	}

	/* Write an info string to the open output .hea or .info file */
	FINT putinfo(char *s)
	{
		if (outinfo == NULL) {
			if (oheader) outinfo = oheader;
			else {
				_context->error("putinfo: caller has not specified a record name\n");
				return (-1);
			}
		}
		(void)wfdb_fprintf(outinfo, "#%s\r\n", s);
		(void)wfdb_fflush(outinfo);
		return (0);
	}

	/* getinfo: On the first call, read all info strings from the .hea file and (if
	available) the .info file for the specified record, and return a pointer to the
	first one.  On subsequent calls, return a pointer to the next info string.
	Return NULL if there are no more info strings. */

	FSTRING getinfo(char *record)
	{
		static char buf[256], *p;
		static int i;
		WFDB_FILE *ifile;

		if (record)
			wfdb_freeinfo();

		if (pinfo == NULL) {	/* info for record has not yet been read */
			if (record == NULL && (record = wfdb_getirec()) == NULL) {
				_context->error("getinfo: caller did not specify record name\n");
				return (NULL);
			}

			if (ninfo) {
				wfdb_freeinfo();  /* free memory allocated previously to info */
				ninfo = 0;
			}

			i = 0;
			nimax = 16;	       /* initial allotment of info string pointers */
			SALLOC(pinfo, nimax, sizeof(char *));

			/* Read info from the .hea file, if available (skip for EDF files) */
			if (!isedf) {
				/* Remove trailing .hea, if any, from record name. */
				wfdb_striphea(record);
				if ((ifile = wfdb_open("hea", record, WFDB_READ))) {
					while (wfdb_fgets(buf, 256, ifile))
						if (*buf != '#') break; /* skip initial comments, if any */
					while (wfdb_fgets(buf, 256, ifile))
						if (*buf == '#') break; /* skip header content */
					while (*buf) {	/* read and save info */
						if (*buf == '#') {	    /* skip anything that isn't info */
							p = buf + strlen(buf) - 1;
							if (*p == '\n') *p-- = '\0';
							if (*p == '\r') *p = '\0';
							if (ninfo >= nimax) {
								int j = nimax;
								nimax += 16;
								SREALLOC(pinfo, nimax, sizeof(char *));
								memset(pinfo + j, 0, (size_t)(16 * sizeof(char *)));
							}
							SSTRCPY(pinfo[ninfo], buf + 1);
							ninfo++;
						}
						if (wfdb_fgets(buf, 256, ifile) == NULL) break;
					}
					wfdb_fclose(ifile);
				}
			}
			/* Read more info from the .info file, if available */
			if ((ifile = wfdb_open("info", record, WFDB_READ))) {
				while (wfdb_fgets(buf, 256, ifile)) {
					if (*buf == '#') {
						p = buf + strlen(buf) - 1;
						if (*p == '\n') *p-- = '\0';
						if (*p == '\r') *p = '\0';
						if (ninfo >= nimax) {
							int j = nimax;
							nimax += 16;
							SREALLOC(pinfo, nimax, sizeof(char *));
							memset(pinfo + j, 0, (size_t)(16 * sizeof(char *)));
						}
						SSTRCPY(pinfo[ninfo], buf + 1);
						ninfo++;
					}
				}
				wfdb_fclose(ifile);
			}
		}
		if (i < ninfo)
			return pinfo[i++];
		else
			return (NULL);
	}

	FFREQUENCY sampfreq(char *record)
	{
		int n;

		/* Remove trailing .hea, if any, from record name. */
		wfdb_striphea(record);

		if (record != NULL) {
			/* Save the current record name. */
			wfdb_setirec(record);
			/* Don't require the sampling frequency of this record to match that
			   of the previously opened record, if any.  (readheader will
			   complain if the previously defined sampling frequency was > 0.) */
			setsampfreq(0.);
			/* readheader sets sfreq if successful. */
			if ((n = readheader(record)) < 0)
				/* error message will come from readheader */
				return ((FrequencyType)n);
		}
		return (sfreq);
	}

	FINT setsampfreq(FrequencyType freq)
	{
		if (freq >= 0.) {
			sfreq = ffreq = freq;
			if ((gvmode & WFDB_HIGHRES) == WFDB_HIGHRES) sfreq *= ispfmax;
			return (0);
		}
		_context->error("setsampfreq: sampling frequency must not be negative\n");
		return (-1);
	}

	static char date_string[12] = "";
	static char time_string[30];

#ifndef __STDC__
#ifndef _WINDOWS
	typedef long time_t;
#endif
#endif

	FINT setbasetime(char *string)
	{
		char *p;

		if (string == NULL || *string == '\0') {
#ifndef NOTIME
			struct tm *now;
			time_t t;

			t = time((time_t *)NULL);    /* get current time from system clock */
			now = localtime(&t);
			(void)sprintf(date_string, "%02d/%02d/%d",
				now->tm_mday, now->tm_mon + 1, now->tm_year + 1900);
			bdate = strdat(date_string);
			(void)sprintf(time_string, "%d:%d:%d",
				now->tm_hour, now->tm_min, now->tm_sec);
			btime = fstrtim(time_string, 1000.0);
#endif
			return (0);
		}
		while (*string == ' ') string++;
		if (p = strchr(string, ' '))
			*p++ = '\0';	/* split time and date components */
		btime = fstrtim(string, 1000.0);
		bdate = p ? strdat(p) : (DateType)0;
		if (btime == 0L && bdate == (DateType)0 && *string != '[') {
			if (p) *(--p) = ' ';
			_context->error("setbasetime: incorrect time format, '%s'\n", string);
			return (-1);
		}
		return (0);
	}

	/* Convert sample number to string, using the given sampling
	   frequency */
	static char *ftimstr(TimeType t, FrequencyType f)
	{
		char *p;

		p = strtok(fmstimstr(t, f), ".");		 /* discard msec field */
		if (t <= 0L && (btime != 0L || bdate != (DateType)0)) { /* time of day */
			(void)strcat(p, date_string);		  /* append dd/mm/yyyy */
			(void)strcat(p, "]");
		}
		return (p);
	}

	FSTRING timstr(TimeType t)
	{
		double f;

		if (ifreq > 0.) f = ifreq;
		else if (sfreq > 0.) f = sfreq;
		else f = 1.0;

		return ftimstr(t, f);
	}

	static DateType pdays = -1;

	/* Convert sample number to string, using the given sampling
	   frequency */
	static char *fmstimstr(TimeType t, FrequencyType f)
	{
		int hours, minutes, seconds, msec;
		DateType days;
		double tms;
		long s;

		if (t > 0L || (btime == 0L && bdate == (DateType)0)) { /* time interval */
			if (t < 0L) t = -t;
			/* Convert from sample intervals to seconds. */
			s = (long)(t / f);
			msec = (int)((t - s * f) * 1000 / f + 0.5);
			if (msec == 1000) { msec = 0; s++; }
			t = s;
			seconds = t % 60;
			t /= 60;
			minutes = t % 60;
			hours = t / 60;
			if (hours > 0)
				(void)sprintf(time_string, "%2d:%02d:%02d.%03d",
					hours, minutes, seconds, msec);
			else
				(void)sprintf(time_string, "   %2d:%02d.%03d",
					minutes, seconds, msec);
		}
		else {			/* time of day */
		/* Convert to milliseconds since midnight. */
			tms = btime - (t * 1000.0 / f);
			/* Convert to seconds. */
			s = (long)(tms / 1000.0);
			msec = (int)((tms - s * 1000.0) + 0.5);
			if (msec == 1000) { msec = 0; s++; }
			t = s;
			seconds = t % 60;
			t /= 60;
			minutes = t % 60;
			t /= 60;
			hours = t % 24;
			days = t / 24;
			if (days != pdays) {
				if (bdate > 0)
					(void)datstr((pdays = days) + bdate);
				else if (days == 0)
					date_string[0] = '\0';
				else
					(void)sprintf(date_string, " %ld", days);
			}
			(void)sprintf(time_string, "[%02d:%02d:%02d.%03d%s]",
				hours, minutes, seconds, msec, date_string);
		}
		return (time_string);
	}

	FSTRING mstimstr(TimeType t)
	{
		double f;

		if (ifreq > 0.) f = ifreq;
		else if (sfreq > 0.) f = sfreq;
		else f = 1.0;

		return fmstimstr(t, f);
	}

	FFREQUENCY getcfreq(void)
	{
		return (cfreq > 0. ? cfreq : ffreq);
	}

	FVOID setcfreq(FrequencyType freq)
	{
		cfreq = freq;
	}

	FDOUBLE getbasecount(void)
	{
		return (bcount);
	}

	FVOID setbasecount(double counter)
	{
		bcount = counter;
	}

	/* Convert string to sample number, using the given sampling
	   frequency */
	static TimeType fstrtim(char *string, FrequencyType f)
	{
		char *p, *q, *r;
		double x, y, z;
		DateType days;
		TimeType t;

		while (*string == ' ' || *string == '\t' || *string == '\n' || *string == '\r')
			string++;
		switch (*string) {
		case 'c': return (cfreq > 0. ?
			(TimeType)((strtod(string + 1, NULL) - bcount)*f / cfreq) :
			(TimeType)(strtol(string + 1, NULL, 10)));
		case 'e':	return ((in_msrec ? msnsamples : nsamples) *
			(((gvmode&WFDB_HIGHRES) == WFDB_HIGHRES) ? ispfmax : 1));
		case 'f': return (TimeType)(strtol(string + 1, NULL, 10)*f / ffreq);
		case 'i':	return (TimeType)(istime *
			(ifreq > 0.0 ? (ifreq / sfreq) : 1.0) *
			(((gvmode&WFDB_HIGHRES) == WFDB_HIGHRES) ? ispfmax : 1));
		case 'o':	return (ostime);
		case 's':	return ((TimeType)strtol(string + 1, NULL, 10));
		case '[':	  /* time of day, possibly with date or days since start */
			if ((q = strchr(++string, ']')) == NULL)
				return ((TimeType)0);	/* '[...': malformed time string */
			if ((p = strchr(string, ' ')) == NULL || p > q)
				days = (DateType)0;/* '[hh:mm:ss.sss]': time since midnight only */
			else if ((r = strchr(p + 1, '/')) == NULL || r > q)
				days = (DateType)strtol(p + 1, NULL, 10); /* '[hh:mm:ss.sss d]' */
			else
				days = strdat(p + 1) - bdate; /* '[hh:mm:ss.sss dd/mm/yyyy]' */
			x = fstrtim(string, 1000.0) - btime;
			if (days > 0L) x += (days*(24 * 60 * 60 * 1000.0));
			t = (TimeType)(x * f / 1000.0 + 0.5);
			return (-t);
		default:
			x = strtod(string, NULL);
			if ((p = strchr(string, ':')) == NULL) return ((long)(x*f + 0.5));
			y = strtod(++p, NULL);
			if ((p = strchr(p, ':')) == NULL) return ((long)((60.*x + y)*f + 0.5));
			z = strtod(++p, NULL);
			return ((TimeType)((3600.*x + 60.*y + z)*f + 0.5));
		}
	}

	FSITIME strtim(char *string)
	{
		double f;

		if (ifreq > 0.) f = ifreq;
		else if (sfreq > 0.) f = sfreq;
		else f = 1.0;

		return fstrtim(string, f);
	}

	/* The functions datstr and strdat convert between Julian dates (used
	   internally) and dd/mm/yyyy format dates.  (Yes, this is overkill for this
	   application.  For the astronomically-minded, Julian dates are supposed
	   to begin at noon GMT, but these begin at midnight local time.)  They are
	   based on similar functions in chapter 1 of "Numerical Recipes", by Press,
	   Flannery, Teukolsky, and Vetterling (Cambridge U. Press, 1986). */

	FSTRING datstr(DateType date)
	{
		int d, m, y, gcorr, jm, jy;
		DateType jd;

		if (date >= 2299161L) {	/* Gregorian calendar correction */
			gcorr = (int)(((date - 1867216L) - 0.25) / 36524.25);
			date += 1 + gcorr - (long)(0.25*gcorr);
		}
		date += 1524;
		jy = (int)(6680 + ((date - 2439870L) - 122.1) / 365.25);
		jd = (DateType)(365L * jy + (0.25*jy));
		jm = (int)((date - jd) / 30.6001);
		d = date - jd - (int)(30.6001*jm);
		if ((m = jm - 1) > 12) m -= 12;
		y = jy - 4715;
		if (m > 2) y--;
		if (y <= 0) y--;
		(void)sprintf(date_string, " %02d/%02d/%d", d, m, y);
		return (date_string);
	}

	FDATE strdat(char *string)
	{
		char *mp, *yp;
		int d, m, y, gcorr, jm, jy;
		DateType date;

		if ((mp = strchr(string, '/')) == NULL || (yp = strchr(mp + 1, '/')) == NULL ||
			(d = strtol(string, NULL, 10)) < 1 || d > 31 ||
			(m = strtol(mp + 1, NULL, 10)) < 1 || m > 12 ||
			(y = strtol(yp + 1, NULL, 10)) == 0)
			return (0L);
		if (m > 2) { jy = y; jm = m + 1; }
		else { jy = y - 1; 	jm = m + 13; }
		if (jy > 0) date = (DateType)(365.25*jy);
		else date = -(long)(-365.25 * (jy + 0.25));
		date += (int)(30.6001*jm) + d + 1720995L;
		if (d + 31L * (m + 12L * y) >= (15 + 31L * (10 + 12L * 1582))) { /* 15/10/1582 */
			gcorr = (int)(0.01*jy);		/* Gregorian calendar correction */
			date += 2 - gcorr + (int)(0.25*gcorr);
		}
		return (date);
	}

	FINT adumuv(SignalType s, SampleType a)
	{
		double x;
		GainType g = (s < nvsig) ? vsd[s]->info.gain : WFDB_DEFGAIN;

		if (g == 0.) g = WFDB_DEFGAIN;
		x = a * 1000. / g;
		if (x >= 0.0)
			return ((int)(x + 0.5));
		else
			return ((int)(x - 0.5));
	}

	FSAMPLE muvadu(SignalType s, int v)
	{
		double x;
		GainType g = (s < nvsig) ? vsd[s]->info.gain : WFDB_DEFGAIN;

		if (g == 0.) g = WFDB_DEFGAIN;
		x = g * v*0.001;
		if (x >= 0.0)
			return ((int)(x + 0.5));
		else
			return ((int)(x - 0.5));
	}

	FDOUBLE aduphys(SignalType s, SampleType a)
	{
		double b;
		GainType g;

		if (s < nvsig) {
			b = vsd[s]->info.baseline;
			g = vsd[s]->info.gain;
			if (g == 0.) g = WFDB_DEFGAIN;
		}
		else {
			b = 0;
			g = WFDB_DEFGAIN;
		}
		return ((a - b) / g);
	}

	FSAMPLE physadu(SignalType s, double v)
	{
		int b;
		GainType g;

		if (s < nvsig) {
			b = vsd[s]->info.baseline;
			g = vsd[s]->info.gain;
			if (g == 0.) g = WFDB_DEFGAIN;
		}
		else {
			b = 0;
			g = WFDB_DEFGAIN;
		}
		v *= g;
		if (v >= 0)
			return ((int)(v + 0.5) + b);
		else
			return ((int)(v - 0.5) + b);
	}

	/* sample(s, t) provides buffered random access to the input signals.  The
	arguments are the signal number (s) and the sample number (t); the returned
	value is the sample from signal s at time t.  On return, the global variable
	sample_vflag is true (non-zero) if the requested sample is not beyond the end
	of the record, false (zero) otherwise.  The caller must open the input signals
	and must set the global variable nisig to the number of input signals before
	invoking sample().  Once this has been done, the caller may request samples in
	any order. */

#define BUFLN   4096	/* must be a power of 2, see sample() */

	FSAMPLE sample(SignalType s, TimeType t)
	{
		static SampleType v;
		static TimeType tt;
		int nsig = (nvsig > nisig) ? nvsig : nisig;

		/* Allocate the sample buffer on the first call. */
		if (sbuf == NULL) {
			SALLOC(sbuf, nsig, BUFLN * sizeof(WFDB_Sample));
			tt = (TimeType)-1L;
		}

		/* If the caller requested a sample from an unavailable signal, return
		   an invalid value.  Note that sample_vflag is not cleared in this
		   case.  */
		if (s < 0 || s >= nsig) {
			sample_vflag = -1;
			return (WFDB_INVALID_SAMPLE);
		}

		/* If the caller specified a negative sample number, prepare to return
		   sample 0.  This behavior differs from the convention that only the
		   absolute value of the sample number matters. */
		if (t < 0L) t = 0L;

		/* If the caller has requested a sample that is no longer in the buffer,
		   or if the caller has requested a sample that is further ahead than the
		   length of the buffer, we need to reset the signal file pointer(s).
		   If we do this, we must be sure that the buffer is refilled so that
		   any subsequent requests for samples between t - BUFLN+1 and t will
		   receive correct responses. */
		if (t <= tt - BUFLN || t > tt + BUFLN) {
			tt = t - BUFLN;
			if (tt < 0L) tt = -1L;
			else if (isigsettime(tt - 1) < 0) exit(2);
		}
		/* If the requested sample is not yet in the buffer, read and buffer
		   more samples.  If we reach the end of the record, clear sample_vflag
		   and return the last valid value. */
		while (t > tt)
			if (getvec(sbuf + nsig * ((++tt)&(BUFLN - 1))) < 0) {
				--tt;
				sample_vflag = 0;
				return (*(sbuf + nsig * (tt&(BUFLN - 1)) + s));
			}

		/* The requested sample is in the buffer.  Set sample_vflag and
		   return the requested sample. */
		if ((v = *(sbuf + nsig * (t&(BUFLN - 1)) + s)) == WFDB_INVALID_SAMPLE)
			sample_vflag = -1;
		else
			sample_vflag = 1;
		return (v);
	}

	FINT sample_valid(void)
	{
		return (sample_vflag);
	}

	/* Private functions (for use by other WFDB library functions only). */

	void wfdb_sampquit(void)
	{
		if (sbuf) {
			SFREE(sbuf);
			sample_vflag = 0;
		}
	}

	void wfdb_sigclose(void)
	{
		isigclose();
		osigclose();
		btime = bdate = nsamples = msbtime = msbdate = msnsamples = (TimeType)0;
		sfreq = ifreq = ffreq = (FrequencyType)0;
		pdays = (DateType)-1;
		segments = in_msrec = skewmax = 0;
		if (dsbuf) {
			SFREE(dsbuf);
			dsbi = -1;
		}
		if (segarray) {
			int i;

			SFREE(segarray);
			segp = segend = (WFDB_Seginfo *)NULL;
			for (i = 0; i < maxisig; i++) {
				SFREE(isd[i]->info.fname);  /* missing before 10.4.6 */
				SFREE(isd[i]->info.desc);
				SFREE(isd[i]->info.units);
			}
		}
		SFREE(gv0);
		SFREE(gv1);
		SFREE(tvector);
		SFREE(uvector);
		tuvlen = 0;

		sigmap_cleanup();
	}

	void wfdb_osflush(void)
	{
		GroupType g;
		SignalType s;
		struct ogdata *og;
		struct osdata *os;

		if (!osd || !ogd)
			return;

		for (s = 0; s < nosig; s++) {
			if ((os = osd[s]) && (og = ogd[os->info.group]) && og->nrewind == 0) {
				if (!og->force_flush && og->seek == 0) {
					if (og->bsize == 0 && !wfdb_fseek(og->fp, 0L, SEEK_CUR))
						og->seek = 1;
					else
						og->seek = -1;
				}
				if (og->force_flush || og->seek > 0) {
					/* Either we are closing the file (osigclose() was called),
					   or the file is seekable: write out any
					   partially-completed sets of bit-packed samples. */
					switch (os->info.fmt) {
					case 212: f212(og); break;
					case 310: f310(og); break;
					case 311: f311(og); break;
					default: break;
					}
				}
			}
		}

		for (g = 0; g < nogroup; g++) {
			og = ogd[g];
			if (og->bsize == 0 && og->bp != og->buf) {
				(void)wfdb_fwrite(og->buf, 1, og->bp - og->buf, og->fp);
				og->bp = og->buf;
			}
			(void)wfdb_fflush(og->fp);

			if (!og->force_flush && og->nrewind != 0) {
				/* Rewind the file so that subsequent samples will be
				   written in the right place. */
				wfdb_fseek(og->fp, -((long)og->nrewind), SEEK_CUR);
				og->nrewind = 0;
			}
		}
	}

	/* Release resources allocated for info string handling */
	void wfdb_freeinfo(void)
	{
		int i;

		for (i = 0; i < nimax; i++)
			SFREE(pinfo[i]);
		SFREE(pinfo);
		nimax = ninfo = 0;
	}

	/* Close any previously opened output info file. */
	void wfdb_oinfoclose(void)
	{
		if (outinfo && outinfo != oheader)
			wfdb_fclose(outinfo);
		outinfo = NULL;
	}

};

}
