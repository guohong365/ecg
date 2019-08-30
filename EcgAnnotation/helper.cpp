#include <math.h>
#include <string.h>
#include "helper.h"

static char anncodes [51][10] =  {
    "notQRS", "N", "LBBB", "RBBB", "ABERR",
    "PVC", "FUSION", "NPC", "APC", "SVPB",
    "VESC", "NESC", "PACE", "UNKNOWN", "NOISE",
    "q", "ARFCT", "Q", "STCH", "TCH", "SYSTOLE",
    "DIASTOLE", "NOTE", "MEASURE", "P", "BBB",
    "PACESP", "T", "RTM", "U", "LEARN", "FLWAV",
    "VFON", "VFOFF", "AESC", "SVESC", "LINK",
    "NAPC", "PFUSE", "(", ")", "RONT",
    //user defined beats//
    "(p", "p)", "(t", "t)", "ECT", "r", "R", "s", "S"
};

const char * getAnnotationCodes(int type){
    if(type > 50) return "";
    return anncodes[type];
}

void MinMax(const double* buffer, int size, double& min, double& max) 
{
	max = buffer[0];
	min = buffer[0];
	for (int i = 1; i < size; i++) {
		if (buffer[i] > max)max = buffer[i];
		if (buffer[i] < min)min = buffer[i];
	}
}

void  NormalizeByMinMax(double* buffer, int size, double a, double b)
{
	double min, max;
	MinMax(buffer, size, min, max);

	for (int i = 0; i < size; i++) {
		if (max - min)
			buffer[i] = (buffer[i] - min) * ((b - a) / (max - min)) + a;
		else
			buffer[i] = a;
	}
}

double Mean(const double* buffer, int size) 
{
	double mean = 0;

	for (int i = 0; i < size; i++)
		mean += buffer[i];

	return mean / double(size);
}
void NormalizeByMean(double* buffer, int size)
{
	const double mean = Mean(buffer, size);

	for (int i = 0; i < size; i++)
		buffer[i] = buffer[i] - mean;
}
void nZscore(double* buffer, int size)
{
	const double mean = Mean(buffer, size);
	double disp = StandardDeviation(buffer, size);

	if (disp == 0.0) disp = 1.0;
	for (int i = 0; i < size; i++)
		buffer[i] = (buffer[i] - mean) / disp;
}

void nSoftmax(double* buffer, const int size)
{
	const double mean = Mean(buffer, size);
	double disp = StandardDeviation(buffer, size);

	if (disp == 0.0) disp = 1.0;
	for (int i = 0; i < size; i++)
		buffer[i] = 1.0 / (1 + exp(-((buffer[i] - mean) / disp)));
}
void nEnergy(double* buffer, const int size, const int L) 
{
	double energy = 0.0;
	for (int i = 0; i < size; i++)
		energy += pow(fabs(buffer[i]), double(L));

	energy = pow(energy, 1.0 / double(L));
	if (energy == 0.0) energy = 1.0;

	for (int i = 0; i < size; i++)
		buffer[i] /= energy;
}


double StandardDeviation(const double* buffer, int size)
{
	double disp = 0;

	const double mean = Mean(buffer, size);

	for (int i = 0; i < size; i++)
		disp += (buffer[i] - mean) * (buffer[i] - mean);

	return (sqrt(disp / static_cast<double>(size - 1)));
}

double MINIMAX(const double* buffer, const int size)
{
	return StandardDeviation(buffer, size)*(0.3936 + 0.1829*log(double(size)));
}
double FIXTHRES(const double* buffer, const int size)
{
	return StandardDeviation(buffer, size)*sqrt(2.0*log(double(size)));
}
double SURE(const double* buffer, int size) 
{
	return StandardDeviation(buffer, size)*sqrt(2.0*log((double)size*log((double)size)));
}
void  HardTH(double* buffer, int size, double TH, double l) 
{
	for (int i = 0; i < size; i++)
		if (fabs(buffer[i]) <= TH)
			buffer[i] *= l;
}
void  SoftTH(double* buffer, int size, double TH, double l)
{
	for (int i = 0; i < size; i++) {
		if (fabs(buffer[i]) <= TH) {
			buffer[i] *= l;
		}
		else {
			if (buffer[i] > 0) {
				buffer[i] -= TH * (1 - l);
			}
			else {
				buffer[i] += TH * (1 - l);
			}
		}
	}
}

void denoise(double* buffer, int size, int window, int type, bool soft)
{
	double th = 0;

	for (int i = 0; i < size / window; i++) {
		switch (type) {
		case 0:
			th = MINIMAX(buffer, window);
			break;
		case 1:
			th = FIXTHRES(buffer, window);
			break;
		case 2:
			th = SURE(buffer, window);
			break;
		default: ;
		}
		if (soft)
			SoftTH(buffer, window, th);
		else
			HardTH(buffer, window, th);

		buffer += window;
	}

	if (size % window > 5) { //skip len=1
		switch (type) {
		case 0:
			th = MINIMAX(buffer, size % window);
			break;
		case 1:
			th = FIXTHRES(buffer, size % window);
			break;
		case 2:
			th = SURE(buffer, size % window);
			break;
		}
		if (soft)
			SoftTH(buffer, size % window, th);
		else
			HardTH(buffer, size % window, th);
	}
}

void AutoCov(double* buffer, int size)
{
	double* rk = new double[size];

	double mu = Mean(buffer, size);

	for (int k = 0; k < size; k++) {
		rk[k] = 0;

		int t = 0;
		while (t + k != size) {
			rk[k] += (buffer[t] - mu) * (buffer[t + k] - mu);
			t++;
		}

		rk[k] /= double(t);                       // rk[k] /= t ?  autocovariance
	}

	for (int i = 0; i < size; i++)
		buffer[i] = rk[i];

	delete[] rk;
}

void AutoCov1(double* buffer, int size)
{
	double* rk = new double[size];

	const double mu = Mean(buffer, size);

	for (int k = 0; k < size; k++) {
		rk[k] = 0;

		for (int t = 0; t < size; t++) {
			if (t + k >= size)
				rk[k] += (buffer[t] - mu) * (buffer[2 * size - (t + k + 2)] - mu);
			else
				rk[k] += (buffer[t] - mu) * (buffer[t + k] - mu);
		}

		rk[k] /= double(size);
	}

	for (int i = 0; i < size; i++)
		buffer[i] = rk[i];

	delete[] rk;
}

void AutoCor(double* buffer, int size)
{
	double* rk = new double[size];

	const double mu = Mean(buffer, size);
	const double std = StandardDeviation(buffer, size);

	for (int k = 0; k < size; k++) {
		rk[k] = 0;

		int t = 0;
		while (t + k != size) {
			rk[k] += (buffer[t] - mu) * (buffer[t + k] - mu);
			t++;
		}

		rk[k] /= double(t) * std * std;
	}

	for (int i = 0; i < size; i++)
		buffer[i] = rk[i];

	delete[] rk;
}

void AutoCor1(double* buffer, int size)
{
	double* rk = new double[size];

	const double mu = Mean(buffer, size);
	const double std = StandardDeviation(buffer, size);

	for (int k = 0; k < size; k++) {
		rk[k] = 0;

		for (int t = 0; t < size; t++) {
			if (t + k >= size)
				rk[k] += (buffer[t] - mu) * (buffer[2 * size - (t + k + 2)] - mu);
			else
				rk[k] += (buffer[t] - mu) * (buffer[t + k] - mu);
		}

		rk[k] /= double(size) * std * std;
	}

	for (int i = 0; i < size; i++)
		buffer[i] = rk[i];

	delete[] rk;
}
/*
double log2(double x) 
{
	return log(x) / log(2.0);
}
*/
void ChangeExtension(char* path, const char* ext)
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
