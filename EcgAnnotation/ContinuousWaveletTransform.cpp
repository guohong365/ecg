#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "ContinuousWaveletTransform.h"

ContinuousWaveletTransform::ContinuousWaveletTransform() : _pHDR(nullptr), _minFrequency(0), _maxFrequency(0), _frequencyInterval(0),
                                                           _w0(0), _scaleType(LINEAR_SCALE), _wavelet(),
                                                           _signalSize(0), _pData(nullptr),
                                                           _pSpectrum(nullptr),
                                                           _pReal(nullptr), _pImage(nullptr), _isPrecision(false), _precisionSize(0),
                                                           _isPeriodicBoundary(false), _leftValue(0),
                                                           _rightValue(0), _sampleRate(0)
{
}

ContinuousWaveletTransform::~ContinuousWaveletTransform()
{
	if (_pReal) free(_pReal);
	if (_pImage) free(_pImage);
	if (_pSpectrum) free(_pSpectrum);
}

double ContinuousWaveletTransform::_transform(int x, double scale) const
{
	double res;
	double real = 0;
	double image = 0;

	for (int t = 0; t < _signalSize; t++) {                   //main
		if (_isPrecision == true) {
			if (t < x - _precisionSize)
				t = x - (_precisionSize - 1);  //continue;
			if (t >= _precisionSize + x)
				break;
		}

		real += _pReal[((_signalSize - 1) - x) + t] * _pData[t];
		if (_wavelet == MORLPOW || _wavelet == MORLFULL)
			image += _pImage[((_signalSize - 1) - x) + t] * _pData[t];
	}

	////////////////////boundaries///////////////////////////////////////////////
	
	for (int i = (_signalSize - _precisionSize); i < (_signalSize - 1) - x; i++) {        // Left edge calculations
		if (_isPeriodicBoundary) {
			real += _pReal[i] * _pData[(_signalSize - 1) - i - x];  //IsPeriodicBoundary
		}
		else {
			if (_leftValue != 0.0)
				real += _pReal[i] * _leftValue;
			else
				real += _pReal[i] * _pData[0];
		}

		if (_wavelet == MORLPOW || _wavelet == MORLFULL) { //Im part for complex wavelet
			if (_isPeriodicBoundary) {
				image += _pImage[i] * _pData[(_signalSize - 1) - i - x];
			}
			else {
				if (_leftValue != 0.0)
					image += _pImage[i] * _leftValue;
				else
					image += _pImage[i] * _pData[0];
			}
		}
	}
	int q = 0;
	for (int i = 2 * _signalSize - (x + 1); i < _signalSize + _precisionSize - 1; i++) {     // Right edge calculations
		if (_isPeriodicBoundary)
			real += _pReal[i] * _pData[(_signalSize - 2) - q]; //IsPeriodicBoundary
		else {
			if (_rightValue != 0.0)
				real += _pReal[i] * _rightValue;
			else
				real += _pReal[i] * _pData[_signalSize - 1];
		}

		if (_wavelet == MORLPOW || _wavelet == MORLFULL) {
			if (_isPeriodicBoundary) {
				image += _pImage[i] * _pData[(_signalSize - 2) - q];
			}
			else {
				if (_rightValue != 0.0)
					image += _pImage[i] * _rightValue;
				else
					image += _pImage[i] * _pData[_signalSize - 1];
			}
		}
		q++;
	}
	////////////////////boundaries///////////////////////////////////////////////


	switch (_wavelet) {
	case MORL:
		res = (1 / sqrt(6.28)) * real;
		break;
	case MORLPOW:
		res = sqrt(real * real + image * image);
		res *= (1 / sqrt(6.28));
		break;
	case MORLFULL:
		res = sqrt(real * real + image * image);
		res *= (1 / pow(3.14, 0.25));
		break;

	default:
		res = real;
	}

	res = (1 / sqrt(scale)) * res;

	return res;
}

int ContinuousWaveletTransform::GetFreqRange() const
{
	if (_scaleType == LINEAR_SCALE)
		return int((_maxFrequency + _frequencyInterval - _minFrequency) / _frequencyInterval);
	if (_scaleType == LINEAR_SCALE)
		return int((log(_maxFrequency) + _frequencyInterval - log(_minFrequency)) / _frequencyInterval);
	return 0;
}

double ContinuousWaveletTransform::GetMinFreq() const
{
	return _minFrequency;
}

double ContinuousWaveletTransform::GetMaxFreq() const
{
	return _maxFrequency;
}

double ContinuousWaveletTransform::GetFreqInterval() const
{
	return _frequencyInterval;
}

int ContinuousWaveletTransform::GetScaleType() const
{
	return _scaleType;
}

void ContinuousWaveletTransform::init(int size, enum WAVELET wavelet, double w, double sr)
{
	_signalSize = size;

    if (sr > 0)
		_sampleRate = sr;

	_w0 = w;
	_pReal = static_cast<double *>(malloc(sizeof(double) * (2 * _signalSize - 1)));
	_pImage = static_cast<double *>(malloc(sizeof(double) * (2 * _signalSize - 1)));
	_pSpectrum = static_cast<double *>(malloc(sizeof(double) * (_signalSize)));
	_wavelet = wavelet;

	for (int i = 0; i < 2 * _signalSize - 1; i++) {
		_pReal[i] = 0;
		_pImage[i] = 0;
	}
}

void  ContinuousWaveletTransform::close()
{
	if (_pReal) {
		free(_pReal);
		_pReal = nullptr;
	}
	if (_pImage) {
		free(_pImage);
		_pImage = nullptr;
	}
	if (_pSpectrum) {
		free(_pSpectrum);
		_pSpectrum = nullptr;
	}
}
/*
float* ContinuousWaveletTransform::CwtCreateFileHeader(wchar_t *name, PCWT_HEADER hdr, enum WAVELET wavelet, double w)
{
	wchar_t tmp[_MAX_PATH];

	int filesize;

	switch (wavelet) {
	case MHAT:
		wcscat(name, L"(mHat).w");
		break;
	case INV:
		wcscat(name, L"(Inv).w");
		break;
	case MORL:
		wcscat(name, L"(Morl).w");
		break;
	case MORLPOW:
		wcscat(name, L"(MPow).w");
		break;
	case MORLFULL:
		wcscat(name, L"(MComp");
		swprintf(tmp, L"%d", (int)w);
		wcscat(name, tmp);
		wcscat(name, L").w");
		break;

	case GAUS:
		wcscat(name, L"(Gaussian).w");
		break;
	case GAUS1:
		wcscat(name, L"(1Gauss).w");
		break;
	case GAUS2:
		wcscat(name, L"(2Gauss).w");
		break;
	case GAUS3:
		wcscat(name, L"(3Gauss).w");
		break;
	case GAUS4:
		wcscat(name, L"(4Gauss).w");
		break;
	case GAUS5:
		wcscat(name, L"(5Gauss).w");
		break;
	case GAUS6:
		wcscat(name, L"(6Gauss).w");
		break;
	case GAUS7:
		wcscat(name, L"(7Gauss).w");
		break;
	}

	if (hdr->type)
		filesize = hdr->size * (int)ceil((log(hdr->fmax) + hdr->fstep - log(hdr->fmin)) / hdr->fstep);
	else
		filesize = hdr->size * (int)ceil((hdr->fmax + hdr->fstep - hdr->fmin) / hdr->fstep);


	filesize = sizeof(float) * filesize + sizeof(CWT_HEADER);

	_fp = CreateFileW(name, GENERIC_WRITE | GENERIC_READ, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (_fp == INVALID_HANDLE_VALUE)
		return 0;
	_fpmap = CreateFileMapping(_fp, 0, PAGE_READWRITE, 0, filesize, 0);
	_lpMap = MapViewOfFile(_fpmap, FILE_MAP_WRITE, 0, 0, filesize);

	_lpf = (float *)_lpMap;

	memset(_lpMap, 0, filesize);
	memcpy(_lpMap, hdr, sizeof(CWT_HEADER));

	return (_lpf + sizeof(CWT_HEADER) / sizeof(float));
}

float* ContinuousWaveletTransform::CwtReadFile(const wchar_t *name)
{
	_fp = CreateFileW(name, GENERIC_WRITE | GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (_fp == INVALID_HANDLE_VALUE)
		return 0;
	_fpmap = CreateFileMapping(_fp, 0, PAGE_READWRITE, 0, 0, 0);
	_lpMap = MapViewOfFile(_fpmap, FILE_MAP_WRITE, 0, 0, 0);

	_pHDR = (CWT_HEADER)_lpMap;
	_lpf = (float *)_lpMap;


	if (memcmp(_pHDR->hdr, "WLET", 4))
		return 0;

	_minFreq = _pHDR->fmin;
	_maxFreq = _pHDR->fmax;
	_freqInterval = _pHDR->fstep;
	_length = _pHDR->size;
	_sr = _pHDR->sr;
	if (_pHDR->type == LINEAR_SCALE)
		_scaleType = LINEAR_SCALE;
	else if (_pHDR->type == LOG_SCALE)
		_scaleType = LOG_SCALE;

	return (_lpf + sizeof(CWT_HEADER) / sizeof(float));
}
*/
double ContinuousWaveletTransform::HzToScale(double f, double sr, enum WAVELET wavelet, double w)
{
	double k;

	switch (wavelet) {
	case MHAT:
		k = 0.22222 * sr;
		break;
	case INV:
		k = 0.15833 * sr;
		break;
	case MORL:
	case MORLPOW:
		k = sr;
		break;
	case MORLFULL:
		k = sr * w * 0.1589;
		break;
	case GAUS:
		k = 0.2 * sr;
		break;
	case GAUS1:
		k = 0.16 * sr;
		break;
	case GAUS2:
		k = 0.224 * sr;
		break;
	case GAUS3:
		k = 0.272 * sr;
		break;
	case GAUS4:
		k = 0.316 * sr;
		break;
	case GAUS5:
		k = 0.354 * sr;
		break;
	case GAUS6:
		k = 0.388 * sr;
		break;
	case GAUS7:
		k = 0.42 * sr;
		break;
	default:
		k = 0;
	}

	return (k / f);
}

void ContinuousWaveletTransform::ConvertName(char *name, enum WAVELET wavelet, double w)
{
    char tmp[_MAX_PATH];

	switch (wavelet) {
	case MHAT:
        strcat(name, "(mHat).w");
		break;
	case INV:
        strcat(name, "(Inv).w");
		break;
	case MORL:
        strcat(name, "(Morl).w");
		break;
	case MORLPOW:
        strcat(name, "(MPow).w");
		break;
	case MORLFULL:
        strcat(name, "(MComp");

        sprintf(tmp, "%d", int(w));
        strcat(name, tmp);
        strcat(name, ").w");
		break;

	case GAUS:
        strcat(name, "(Gaussian).w");
		break;
	case GAUS1:
        strcat(name, "(1Gauss).w");
		break;
	case GAUS2:
        strcat(name, "(2Gauss).w");
		break;
	case GAUS3:
        strcat(name, "(3Gauss).w");
		break;
	case GAUS4:
        strcat(name, "(4Gauss).w");
		break;
	case GAUS5:
        strcat(name, "(5Gauss).w");
		break;
	case GAUS6:
        strcat(name, "(6Gauss).w");
		break;
	case GAUS7:
        strcat(name, "(7Gauss).w");
		break;
	}
}


double* ContinuousWaveletTransform::Transform(const double* data, const double freq, const bool periodicBoundary, const double lValue,
                                              const double rValue)
{
	_isPeriodicBoundary = periodicBoundary;
	_leftValue = lValue;
	_rightValue = rValue;

	_isPrecision = false;
	_precisionSize = 0;                                      //0,0000001 float prsision
	double t, sn = 0, cs = 0;

	const double scale = HzToScale(freq, _sampleRate, _wavelet, _w0);

	///////////wavelet calculation//////////////////////////////////////////////////////////////////
	///////// center = SignalSize-1 in wavelet mass////////////////////////////////////////////////////

	for (int i = 0; i < _signalSize; i++) {                     //positive side
		t = double(i) / scale;

		if (_wavelet > INV && _wavelet < MORLFULL) {
			sn = sin(6.28 * t);
			cs = cos(6.28 * t);
		}
		if (_wavelet == MORLFULL) {
			sn = sin(_w0 * t);
			cs = cos(_w0 * t);
		}

		switch (_wavelet) {
		case MHAT:
			_pReal[(_signalSize - 1) + i] = exp(-t * t / 2) * (-t * t + 1);
			break;
		case INV:
			_pReal[(_signalSize - 1) + i] = t * exp(-t * t / 2);
			break;
		case MORL:
			_pReal[(_signalSize - 1) + i] = exp(-t * t / 2) * (cs - sn);
			break;
		case MORLPOW:
			_pReal[(_signalSize - 1) + i] = exp(-t * t / 2) * cs;
			_pImage[(_signalSize - 1) + i] = exp(-t * t / 2) * sn;
			break;
		case MORLFULL:
			_pReal[(_signalSize - 1) + i] = exp(-t * t / 2) * (cs - exp(-_w0 * _w0 / 2));
			_pImage[(_signalSize - 1) + i] = exp(-t * t / 2) * (sn - exp(-_w0 * _w0 / 2));
			break;

		case GAUS:
			_pReal[(_signalSize - 1) + i] = exp(-t * t / 2);
			break;
		case GAUS1:
			_pReal[(_signalSize - 1) + i] = -t * exp(-t * t / 2);
			break;
		case GAUS2:
			_pReal[(_signalSize - 1) + i] = (t * t - 1) * exp(-t * t / 2);
			break;
		case GAUS3:
			_pReal[(_signalSize - 1) + i] = (2 * t + t - t * t * t) * exp(-t * t / 2);
			break;
		case GAUS4:
			_pReal[(_signalSize - 1) + i] = (3 - 6 * t * t + t * t * t * t) * exp(-t * t / 2);
			break;
		case GAUS5:
			_pReal[(_signalSize - 1) + i] = (-15 * t + 10 * t * t * t - t * t * t * t * t) * exp(-t * t / 2);
			break;
		case GAUS6:
			_pReal[(_signalSize - 1) + i] = (-15 + 45 * t * t - 15 * t * t * t * t + t * t * t * t * t * t) * exp(-t * t / 2);
			break;
		case GAUS7:
			_pReal[(_signalSize - 1) + i] = (105 * t - 105 * t * t * t + 21 * t * t * t * t * t - t * t * t * t * t * t * t) * exp(-t * t / 2);
			break;
		}

		if (fabs(_pReal[(_signalSize - 1) + i]) < 0.0000001)
			_precisionSize++;

		if (_precisionSize > 15) {
			_precisionSize = i;
			_isPrecision = true;
			break;
		}
	}
	if (_isPrecision == false)
		_precisionSize = _signalSize;

	for (int i = -(_precisionSize - 1); i < 0; i++) {               //negative side
		t = double(i) / scale;

		if (_wavelet > INV && _wavelet < MORLFULL) {
			sn = sin(6.28 * t);
			cs = cos(6.28 * t);
		}
		if (_wavelet == MORLFULL) {
			sn = sin(_w0 * t);
			cs = cos(_w0 * t);
		}

		switch (_wavelet) {
		case MHAT:
			_pReal[(_signalSize - 1) + i] = exp(-t * t / 2) * (-t * t + 1);
			break;
		case INV:
			_pReal[(_signalSize - 1) + i] = t * exp(-t * t / 2);
			break;
		case MORL:
			_pReal[(_signalSize - 1) + i] = exp(-t * t / 2) * (cs - sn);
			break;
		case MORLPOW:
			_pReal[(_signalSize - 1) + i] = exp(-t * t / 2) * cs;
			_pImage[(_signalSize - 1) + i] = exp(-t * t / 2) * sn;
			break;
		case MORLFULL:
			_pReal[(_signalSize - 1) + i] = exp(-t * t / 2) * (cs - exp(-_w0 * _w0 / 2));
			_pImage[(_signalSize - 1) + i] = exp(-t * t / 2) * (sn - exp(-_w0 * _w0 / 2));
			break;

		case GAUS:
			_pReal[(_signalSize - 1) + i] = exp(-t * t / 2);    //gauss
			break;
		case GAUS1:
			_pReal[(_signalSize - 1) + i] = -t * exp(-t * t / 2);   //gauss1
			break;
		case GAUS2:
			_pReal[(_signalSize - 1) + i] = (t * t - 1) * exp(-t * t / 2);     //gauss2
			break;
		case GAUS3:
			_pReal[(_signalSize - 1) + i] = (2 * t + t - t * t * t) * exp(-t * t / 2);    //gauss3
			break;
		case GAUS4:
			_pReal[(_signalSize - 1) + i] = (3 - 6 * t * t + t * t * t * t) * exp(-t * t / 2);    //gauss4
			break;
		case GAUS5:
			_pReal[(_signalSize - 1) + i] = (-15 * t + 10 * t * t * t - t * t * t * t * t) * exp(-t * t / 2);    //gauss5
			break;
		case GAUS6:
			_pReal[(_signalSize - 1) + i] = (-15 + 45 * t * t - 15 * t * t * t * t + t * t * t * t * t * t) * exp(-t * t / 2);   //gauss6
			break;
		case GAUS7:
			_pReal[(_signalSize - 1) + i] = (105 * t - 105 * t * t * t + 21 * t * t * t * t * t - t * t * t * t * t * t * t) * exp(-t * t / 2);    //gauss7
			break;
		}
	}
	///////end wavelet calculations////////////////////////////////////////////

	_pData = data;
	for (int x = 0; x < _signalSize; x++)
		_pSpectrum[x] = _transform(x, scale);

	return _pSpectrum;
}
