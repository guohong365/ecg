/*

ecg.cpp - ECG annotation console app based lib.cpp ECG library.
Copyright (C) 2007 YURIY V. CHESNOKOV

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


You may contact the author by e-mail (chesnokov.yuriy@gmail.com or chesnokov_yuriy@mail.ru)
or postal mail (Unilever Centre for Molecular Sciences Informatics,
University Chemical Laboratory, Cambridge University, Lensfield Road, Cambridge, CB2 1EW, UK)

*/


#include "stdafx.h"
#include "signal.h"
#include "Annotator.h"
#include "helper.h"
#include "SignalReader.h"
char params[_MAX_PATH] = "params";

void tic();
void toc();
void help();
int parse_params(class Annotator &ann);
void change_extension(char* path, const char* ext);

int main(int argc, char* argv[])
{
	char annName[_MAX_PATH];
	char hrvName[_MAX_PATH];

	if (argc < 2)
	{
		help();
		return 0;
	}
	int leadNumber = 0;
	if (argc >= 2 + 1) {
		leadNumber = atoi(argv[2]) - 1;
		if (leadNumber < 0) leadNumber = 0;
	}

	class Signal* signal = SignalReader::read(argv[1]);
	if (signal) {
		const int size = signal->GetLength();
		const double sampleRate = signal->GetSR();
		int h, m, s, ms;
		int millisecond = int((double(size) / sampleRate) * 1000.0);
		signal->mSecToTime(millisecond, h, m, s, ms);

		printf("  leads: %d\n", signal->GetLeadsNum());
		printf("     sr: %.2lf Hz\n", sampleRate);
		printf("   bits: %d\n", signal->GetBits());
		printf("    UmV: %d\n", signal->GetUmV());
		printf(" length: %02d:%02d:%02d.%03d\n\n", h, m, s, ms);

		double* data = signal->GetData(leadNumber);


		class Annotator ann;  //default annotation params
		if (argc >= 3 + 1) {
			strcpy_s(params, _MAX_PATH, argv[3]);
			parse_params(ann);
		}


		printf(" getting QRS complexes... ");
		tic();
		int** qrsAnn = ann.getQRS(data, size, sampleRate);         //get QRS complexes                        
		if (qrsAnn) {
			printf(" %d beats.\n", ann.getQRSNumber());
			ann.getEctopia(qrsAnn, ann.getQRSNumber(), sampleRate);        //label Ectopic beats

			printf(" getting P, T waves... ");
			int annNum;
			int** ANN = ann.getPTU(data, size, sampleRate, qrsAnn, ann.getQRSNumber());     //find P,T waves
			if (ANN) {
				annNum = ann.getAnnotationSize();
				printf(" done.\n");
				toc();
				printf("\n");
				//save ECG annotation
				strcpy(annName, argv[1]);
				change_extension(annName, ".atr");
				ann.SaveAnnotation(annName, ANN, annNum);
			}
			else {
				ANN = qrsAnn;
				annNum = 2 * ann.getQRSNumber();
				printf(" failed.\n");
				toc();
				printf("\n");
			}

			//printing out annotation
			for (int i = 0; i < annNum; i++) {
				const int sample = ANN[i][0];
				const int type = ANN[i][1];

				millisecond = int((double(sample) / sampleRate) * 1000.0);
				signal->mSecToTime(millisecond, h, m, s, ms);

				printf("%10d %02d:%02d:%02d.%03d   %s\n", sample, h, m, s, ms, anncodes[type]);
			}

			//saving RR seq
			vector<double> rrs;
			vector<int> rrsPos;
			std::string str;
			
			
			strcpy(hrvName, argv[1]);
			change_extension(hrvName, ".hrv");
			if (ann.getRRSequence(ANN, annNum, sampleRate, &rrs, &rrsPos)) {
				FILE *fp = fopen(hrvName, "wt");
				for (int i = 0; i < int(rrs.size()); i++)
					fprintf(fp, "%lf\n", rrs[i]);
				fclose(fp);

				printf("\n mean heart rate: %.2lf", Mean(&rrs[0], int(rrs.size())));
			}

		}
		else {
			printf(" could not get QRS complexes. make sure you have got \"filters\" directory in the ecg application dir.");
			exit(1);
		}

	}
	else {
		printf(" failed to read %s file", argv[1]);
		exit(1);
	}

	return 0;
}

void help()
{
	printf("usage: ecg.exe physioNetFile.dat [LeadNumber] [params]\n");
	printf("       do not forget about filters dir to be present.");
}

static LARGE_INTEGER m_nFreq;
static LARGE_INTEGER m_nBeginTime;

void tic()
{
	QueryPerformanceFrequency(&m_nFreq);
	QueryPerformanceCounter(&m_nBeginTime);
}
void toc()
{
	LARGE_INTEGER nEndTime;

	QueryPerformanceCounter(&nEndTime);
	const __int64 nCalcTime = (nEndTime.QuadPart - m_nBeginTime.QuadPart) * 1000 / m_nFreq.QuadPart;

	printf(" processing time: %lld ms\n", nCalcTime);
}

void change_extension(char* path, const char* ext)
{
	for (int i = int(strlen(path)) - 1; i > 0; i--) {
		if (path[i] == '.') {
			path[i] = 0;
			strcat(path, ext);
			return;
		}
	}
	strcat(path, ext);
}

int parse_params(class Annotator &ann)
{
	FILE* fp = nullptr;
	fopen_s(&fp, params, "rt");
	if (fp) {
		ANN_HEADER hdr;
		const int res = fscanf(fp, "%*s %d %*s %d"
			"%*s %lf %*s %lf %*s %lf %*s %d %*s %lf"
			"%*s %lf %*s %lf %*s %lf %*s %lf"
			"%*s %lf %*s %lf %*s %d",
			&hdr.minbpm, &hdr.maxbpm,
			&hdr.minQRS, &hdr.maxQRS, &hdr.qrsFreq, &hdr.ampQRS, &hdr.minUmV,
			&hdr.minPQ, &hdr.maxPQ, &hdr.minQT, &hdr.maxQT,
			&hdr.pFreq, &hdr.tFreq, &hdr.biTwave);
		if (res == 14) {
			const PANN_HEADER pHdr = ann.getAnnotationHeader();
			memcpy(pHdr, &hdr, sizeof(ANN_HEADER));
			printf(" using annotation params from file %s\n", params);
			printf("  minBpm  %d\n"
				"  maxBpm  %d\n"
				"  minQRS  %lg\n"
				"  maxQRS  %lg\n"
				" qrsFreq  %lg\n"
				"  ampQRS  %d\n"
				"  minUmV  %lg\n"
				"   minPQ  %lg\n"
				"   maxPQ  %lg\n"
				"   minQT  %1f\n"
				"   maxQT  %lg\n"
				"   pFreq  %lg\n"
				"   tFreq  %lg\n"
				" biTwave  %d\n\n", hdr.minbpm, hdr.maxbpm,
				hdr.minQRS, hdr.maxQRS, hdr.qrsFreq, hdr.ampQRS, hdr.minUmV,
				hdr.minPQ, hdr.maxPQ, hdr.minQT, hdr.maxQT,
				hdr.pFreq, hdr.tFreq, hdr.biTwave);
			fclose(fp);
			return 0;
		}
		fclose(fp);
		printf(" failed to read %s annotation params file, using default ones instead.\n", params);
		return res;
	}
	printf(" failed to open %s annotation params file, using default ones instead.\n", params);
	return -1;
}
