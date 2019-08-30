#pragma once
#include "ecgtypes.h"

class ContinuousWaveletTransform
{
public:
	ContinuousWaveletTransform();
	~ContinuousWaveletTransform();

	// Data
	enum WAVELET { MHAT, INV, MORL, MORLPOW, MORLFULL, GAUS, GAUS1, GAUS2, GAUS3, GAUS4, GAUS5, GAUS6, GAUS7 };
	enum SCALE_TYPE { LINEAR_SCALE, LOG_SCALE };

	// Operators
			//const CWT& operator=(const CWT& cwt);

	// Operations
	//float* CwtCreateFileHeader(wchar_t *name, PCWT_HEADER hdr, enum WAVELET wavelet, double w);
	//float* CwtReadFile(const wchar_t *name);
	static double HzToScale(double f, double sr, enum WAVELET wavelet, double w);
    static void ConvertName(char *name, enum WAVELET wavelet, double w);

	void init(int size, enum WAVELET wavelet, double w, double sr);
	void close();
	double* Transform(const double *data, double freq, bool periodicBoundary = true, double lv = 0, double rv = 0);

	// Access
	double GetMinFreq() const;
	double GetMaxFreq() const;
	double GetFreqInterval() const;
	int GetScaleType() const;
	int GetFreqRange() const;

	// Inquiry

private:
	ContinuousWaveletTransform(const ContinuousWaveletTransform& cwt) = delete;
	const ContinuousWaveletTransform& operator=(const ContinuousWaveletTransform& cwt) = delete;

	double _transform(int x, double scale) const;

	PCWT_HEADER _pHDR;

	double _minFrequency;
	double _maxFrequency;
	double _frequencyInterval;
	double _w0;

	enum SCALE_TYPE _scaleType;
	enum WAVELET _wavelet;                   //Wavelet

	int _signalSize;
	const double *_pData;               //pointer to original signal
	double *_pSpectrum;              //buffer with spectra
	double *_pReal;
	double *_pImage;

	bool _isPrecision;
	int _precisionSize;

	bool _isPeriodicBoundary;                //periodic boundary extension
	double _leftValue;
	double _rightValue;
	double _sampleRate;

};

/*//////////////////////////////////////////////
		CWT *cwt;
		cwt->InitCWT(size, CWT::MHAT, w0, SR);
		pCwtSpectrum = cwt->Transform(signal, freq);
		cwt->CloseCWT();
//////////////////////////////////////////////*/

// Inlines
