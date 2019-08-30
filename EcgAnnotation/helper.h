#pragma once
const char * getAnnotationCodes(int type);

void MinMax(const double* buffer, int size, double& min, double& max);
double Mean(const double* buffer, int size);
//±ê×¼²î
double StandardDeviation(const double* buffer, int size);
void NormalizeByMean(double* buffer, int size);
void NormalizeByMinMax(double* buffer, int size, double min, double max);
void nZscore(double* buffer, int size);
void nSoftmax(double* buffer, int size);
void nEnergy(double* buffer, int size, int L = 2);

double MINIMAX(const double* buffer, int size);
double FIXTHRES(const double* buffer, int size);
double SURE(const double* buffer, int size);
void denoise(double* buffer, int size, int window, int type = 0, bool soft = true);
void HardTH(double* buffer, int size, double TH, double l = 0.0);
void SoftTH(double* buffer, int size, double TH, double l = 0.0);
void AutoCov(double* buffer, int size);
void AutoCov1(double* buffer, int size);
void AutoCor(double* buffer, int size);
//double log2(double x);


void ChangeExtension(char* path, const char* ext);
