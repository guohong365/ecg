#pragma once
#include "FastWaveletTransform.h"

class Denoise : public FastWaveletTransform
{
public:
	Denoise();
	~Denoise();

	// Operators
			//const EcgDenoise& operator=(const EcgDenoise& ecgdenoise);

	// Operations
    void init(double* data, int size, double sampleRate, bool mirror = true);
	void close();

	bool LFDenoise();         //baseline wander removal
	bool HFDenoise();         //hf denoising
	bool LFHFDenoise();       //baseline and hf denoising

// Access
// Inquiry

private:
	Denoise(const Denoise& denoise) = delete;
	const Denoise& operator=(const Denoise& denoise) = delete;

	double* _pData;            //pointer to [original sig]
	double* _pBuffer;           //[SRadd][original sig][SRadd]
	int _bufferSize;
	double _sampleRate;
	int _length;

};

