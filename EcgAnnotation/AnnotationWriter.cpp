#include "AnnotationWriter.h"
#include <vector>
#include "ecgtypes.h"
#include "signal.h"
#include "SignalWriter.h"
AnnotationWriter::AnnotationWriter()
{
}


AnnotationWriter::~AnnotationWriter()
{
}


bool AnnotationWriter::SaveQTseq(const char *name, int **ann, int annsize, double sr, int length)
{
	std::vector<double> QT;
	int q = 0;


	for (int i = 0; i < annsize; i++) {
		switch (ann[i][1]) {
		case 14:            //noise
		case 15:            //q
		case 16:            //artifact
		case 17:            //Q
		case 18:            //ST change
		case 19:            //T change
		case 20:            //systole
		case 21:            //diastole
		case 22:
		case 23:
		case 24:            //P
		case 25:
		case 26:
		case 27:            //T
		case 28:
		case 29:
		case 30:
		case 31:
		case 32:
		case 33:
		case 36:
		case 37:
		case 39:
		case 40:
		case 42:  //(p
		case 43:  //p)
		case 44:  //(t
		case 47:  //r
		case 48:  //R
		case 49:  //s
		case 50:  //S		
			continue;
		default:
			break;
		}

		if (ann[i][1] == 45)
		{
			//45 - t)
			const int t = ann[i][0];
			if (q < t)
				QT.push_back(double(t - q) / sr);
		}
		else {
			/*if(i+1<annsize && (ann[i+1][1]==47 || ann[i+1][1]==48))  //r only
			 q = ann[i+1][0];
			else if(i+2<annsize && (ann[i+2][1]==47 || ann[i+2][1]==48))  //q,r
			 q = ann[i+2][0];
			else*/
			q = ann[i][0];
		}
	}


	if (QT.size()) {
		DATA_HEADER hdr;
		memset(&hdr, 0, sizeof(DATA_HEADER));

		memcpy(hdr.hdr, "DATA", 4);
		hdr.size = QT.size();
		hdr.sr = float(double(QT.size()) / (double(length) / sr));
		hdr.bits = 32;
		hdr.umv = 1;
		Signal signal(hdr, &QT[0]);
		SignalWriter::writeText(name, &signal);

		return true;
	}
	return false;
}

bool AnnotationWriter::SavePQseq(const char *name, int **ann, int annsize, double sr, int length)
{
	std::vector <double> PQ;
	int p = length;


	for (int i = 0; i < annsize; i++) {
		switch (ann[i][1]) {
		case 14:            //noise
		case 15:            //q
		case 16:            //artifact
		case 17:            //Q
		case 18:            //ST change
		case 19:            //T change
		case 20:            //systole
		case 21:            //diastole
		case 22:
		case 23:
		case 24:            //P
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31:
		case 32:
		case 33:
		case 36:
		case 37:
		case 39:
		case 40:
		case 43:  //p)
		case 44:  //(t
		case 45:  //t)
		case 47:  //r
		case 48:  //R
		case 49:  //s
		case 50:  //S
			continue;
		default:
			break;
		}

		if (ann[i][1] == 42)   //42 - (p
			p = ann[i][0];
		else {
			const int q = ann[i][0];
			if (p < q) {
				PQ.push_back(double(q - p) / sr);
				p = length;
			}
		}
	}

	if (PQ.size()) {
		DATA_HEADER hdr;
		memset(&hdr, 0, sizeof(DATA_HEADER));

		memcpy(hdr.hdr, "DATA", 4);
		hdr.size = PQ.size();
		hdr.sr = float(double(PQ.size()) / (double(length) / sr));
		hdr.bits = 32;
		hdr.umv = 1;

		Signal signal(hdr, &PQ[0]);
		SignalWriter::writeText(name, &signal);

		return true;
	}
	return false;
}

bool AnnotationWriter::SavePPseq(const char *name, int **ann, int annsize, double sr, int length)
{
	std::vector <double> PP;
	int p1 = 0;

	for (int i = 0; i < annsize; i++) {
		if (ann[i][1] == 42)      //42 - (p
			p1 = ann[i][0];
		else if (ann[i][1] == 43)
		{
			//43 - p)
			const int p2 = ann[i][0];
			PP.push_back(double(p2 - p1) / sr);
		}
	}

	if (PP.size()) {
		DATA_HEADER hdr;
		memset(&hdr, 0, sizeof(DATA_HEADER));

		memcpy(hdr.hdr, "DATA", 4);
		hdr.size = PP.size();
		hdr.sr = float(double(PP.size()) / (double(length) / sr));
		hdr.bits = 32;
		hdr.umv = 1;

		Signal signal(hdr, &PP[0]);
		SignalWriter::writeText(name, &signal);

		return true;
	}
	return false;
}

bool AnnotationWriter::SaveRRseq(char *name, ANN_HEADER _hdr, int **ann, int nums, double sr, int length) const
{
	vector <double> RR;
	int add = -1;
	double r1 = 0, r2 = 0;

	//R peak or S peak annotation////////////////////////////
	bool rrs = true;
	int rNum = 0, sNum = 0;
	for (int i = 0; i < nums; i++) {
		if (ann[i][1] == 47 || ann[i][1] == 48) rNum++;
		else if (ann[i][1] == 49 || ann[i][1] == 50) sNum++;
	}
	if (int(1.1f*float(rNum)) < sNum) {
		rrs = false;  //R peaks less than S ones
		strcat(name, "_SS.dat");
	}
	else
		strcat(name, "_RR.dat");

	////////////////////////////////////////////////////////


	for (int i = 0; i < nums; i++) {
		switch (ann[i][1]) {
		case 0:    //non beats
		case 15:   //q
		case 17:   //Q
		case 18:   //ST change
		case 19:   //T change
		case 20:   //systole
		case 21:   //diastole
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31:
		case 32:
		case 33:
		case 36:
		case 37:
		case 40:   //')' J point
		case 42:   //(p
		case 43:   //p)
		case 44:   //(t
		case 45:   //t)
		case 47:   //r
		case 48:   //R
		case 49:   //s
		case 50:   //S
			continue;

		case 14:   //noise
		case 16:   //artifact
			add = -1;
			continue;
		default:
			break;
		}

		if (add != -1) {
			//annotation on RRs peaks
			if (rrs) {
				if (i + 1 < nums && (ann[i + 1][1] == 47 || ann[i + 1][1] == 48))  //r only
					r2 = ann[i + 1][0];
				else if (i + 2 < nums && (ann[i + 2][1] == 47 || ann[i + 2][1] == 48))  //q,r
					r2 = ann[i + 2][0];
				else //(ann[i][1]==N,ECT,...)  //no detected R only S
					r2 = ann[i][0];

				if (add + 1 < nums && (ann[add + 1][1] == 47 || ann[add + 1][1] == 48))
					r1 = ann[add + 1][0];
				else if (add + 2 < nums && (ann[add + 2][1] == 47 || ann[add + 2][1] == 48))
					r1 = ann[add + 2][0];
				else //(ann[add][1]==N,ECT,...) //no detected R only S
					r1 = ann[add][0];
			}
			//annotation on S peaks
			else {
				if (i + 1 < nums && (ann[i + 1][1] == 40))  //N)
					r2 = ann[i][0];
				else if (i + 1 < nums && (ann[i + 1][1] == 49 || ann[i + 1][1] == 50))  //Sr
					r2 = ann[i + 1][0];
				else if (i + 2 < nums && (ann[i + 2][1] == 49 || ann[i + 2][1] == 50))  //rS
					r2 = ann[i + 2][0];
				else if (i + 3 < nums && (ann[i + 3][1] == 49 || ann[i + 3][1] == 50))  //errQ rS
					r2 = ann[i + 3][0];
				else if (i + 1 < nums && (ann[i + 1][1] == 47 || ann[i + 1][1] == 48))  //no S
					r2 = ann[i + 1][0];
				else if (i + 2 < nums && (ann[i + 2][1] == 47 || ann[i + 2][1] == 48))  //no S
					r2 = ann[i + 2][0];

				if (add + 1 < nums && (ann[add + 1][1] == 40))  //N)
					r1 = ann[add][0];
				else if (add + 1 < nums && (ann[add + 1][1] == 49 || ann[add + 1][1] == 50))
					r1 = ann[add + 1][0];
				else if (add + 2 < nums && (ann[add + 2][1] == 49 || ann[add + 2][1] == 50))
					r1 = ann[add + 2][0];
				else if (add + 3 < nums && (ann[add + 3][1] == 49 || ann[add + 3][1] == 50))
					r1 = ann[add + 3][0];
				else if (add + 1 < nums && (ann[add + 1][1] == 47 || ann[add + 1][1] == 48))  //no S
					r1 = ann[add + 1][0];
				else if (add + 2 < nums && (ann[add + 2][1] == 47 || ann[add + 2][1] == 48))  //no S
					r1 = ann[add + 2][0];
			}

			double rr = 60.0 / ((r2 - r1) / sr);
			if (rr >= _hdr.minbpm && rr <= _hdr.maxbpm)
				RR.push_back(rr);         //in bpm
		}
		add = i;
	}

	if (RR.size()) {
		DATA_HEADER hdr;
		memset(&hdr, 0, sizeof(DATA_HEADER));

		memcpy(hdr.hdr, "DATA", 4);
		hdr.size = RR.size();
		hdr.sr = float(double(RR.size()) / (double(length) / sr));
		hdr.bits = 32;
		hdr.umv = 1;
		Signal signal(hdr, &RR[0]);
		SignalWriter::writeText(name, &signal);
		return true;
	}
	return false;
}

bool AnnotationWriter::SaveRRnseq(char *name, ANN_HEADER& _hdr, int **ann, int nums, double sr, int length) const
{
	vector <double> RR;
	int add = -1;
	double r1 = 0, r2 = 0;

	//R peak or S peak annotation////////////////////////////
	bool rrs = true;
	int rNum = 0, sNum = 0;
	for (int i = 0; i < nums; i++) {
		if (ann[i][1] == 47 || ann[i][1] == 48) rNum++;
		else if (ann[i][1] == 49 || ann[i][1] == 50) sNum++;
	}
	if (int(1.1f*float(rNum)) < sNum) {
		rrs = false;  //R peaks less than S ones
		strcat(name, "_SSn.dat");
	}
	else
		strcat(name, "_RRn.dat");
	////////////////////////////////////////////////////////


	for (int i = 0; i < nums; i++) {
		switch (ann[i][1]) {
		case 1:             //N
			if (add == -1) {
				add = i;
				continue;
			}
			break;

		case 0:     //ectopic beats
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:    //noise
		case 16:    //artefact
		case 34:
		case 35:
		case 38:
		case 46:
			add = -1;   //reset counter
			continue;

		default:     //other types
			continue;
		}

		//annotation on RRs peaks
		if (rrs) {
			if (i + 1 < nums && (ann[i + 1][1] == 47 || ann[i + 1][1] == 48))  //r only
				r2 = ann[i + 1][0];
			else if (i + 2 < nums && (ann[i + 2][1] == 47 || ann[i + 2][1] == 48))  //q,r
				r2 = ann[i + 2][0];
			else //(ann[i][1]==N,ECT,...)  //no detected R only S
				r2 = ann[i][0];

			if (add + 1 < nums && (ann[add + 1][1] == 47 || ann[add + 1][1] == 48))
				r1 = ann[add + 1][0];
			else if (add + 2 < nums && (ann[add + 2][1] == 47 || ann[add + 2][1] == 48))
				r1 = ann[add + 2][0];
			else //(ann[add][1]==N,ECT,...) //no detected R only S
				r1 = ann[add][0];
		}
		//annotation on S peaks
		else {
			if (i + 1 < nums && (ann[i + 1][1] == 40))  //N)
				r2 = ann[i][0];
			else if (i + 1 < nums && (ann[i + 1][1] == 49 || ann[i + 1][1] == 50))  //Sr
				r2 = ann[i + 1][0];
			else if (i + 2 < nums && (ann[i + 2][1] == 49 || ann[i + 2][1] == 50))  //rS
				r2 = ann[i + 2][0];
			else if (i + 3 < nums && (ann[i + 3][1] == 49 || ann[i + 3][1] == 50))  //errQ rS
				r2 = ann[i + 3][0];
			else if (i + 1 < nums && (ann[i + 1][1] == 47 || ann[i + 1][1] == 48))  //no S
				r2 = ann[i + 1][0];
			else if (i + 2 < nums && (ann[i + 2][1] == 47 || ann[i + 2][1] == 48))  //no S
				r2 = ann[i + 2][0];

			if (add + 1 < nums && (ann[add + 1][1] == 40))  //N)
				r1 = ann[add][0];
			else if (add + 1 < nums && (ann[add + 1][1] == 49 || ann[add + 1][1] == 50))
				r1 = ann[add + 1][0];
			else if (add + 2 < nums && (ann[add + 2][1] == 49 || ann[add + 2][1] == 50))
				r1 = ann[add + 2][0];
			else if (add + 3 < nums && (ann[add + 3][1] == 49 || ann[add + 3][1] == 50))
				r1 = ann[add + 3][0];
			else if (add + 1 < nums && (ann[add + 1][1] == 47 || ann[add + 1][1] == 48))  //no S
				r1 = ann[add + 1][0];
			else if (add + 2 < nums && (ann[add + 2][1] == 47 || ann[add + 2][1] == 48))  //no S
				r1 = ann[add + 2][0];
		}

		double rr = 60.0 / ((r2 - r1) / sr);
		if (rr >= _hdr.minbpm && rr <= _hdr.maxbpm)
			RR.push_back(rr);         //in bpm

		add = i;
	}


	if (!RR.size()) return false;

	DATA_HEADER hdr;
	memset(&hdr, 0, sizeof(DATA_HEADER));

	memcpy(hdr.hdr, "DATA", 4);
	hdr.size = RR.size();
	hdr.sr = float(double(RR.size()) / (double(length) / sr));
	hdr.bits = 32;
	hdr.umv = 1;
	Signal signal(hdr, &RR[0]);
	SignalWriter::writeText(name, &signal);
	return true;
}
