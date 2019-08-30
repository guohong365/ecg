#include <math.h>
#include "Denoise.h"
#include "helper.h"

Denoise::Denoise()
	: _pData(nullptr)
	, _pBuffer(nullptr)
	, _bufferSize(0)
	, _sampleRate(0)
	, _length(0)
{
}

Denoise::~Denoise()
{
	delete[] _pBuffer;
}

void Denoise::init(double* data, int size, double sampleRate, bool mirror)
{
	_pData = data;
	_sampleRate = sampleRate;
	_length = size;
	if (_pBuffer) delete[] _pBuffer;
	_bufferSize = int(_length + 2 * _sampleRate);
	_pBuffer = new double[_bufferSize];  // [SR add] [sig] [SR add]

	for (int i = 0; i < _length; i++)           //signal
		_pBuffer[i + int(_sampleRate)] = _pData[i];

	if (_length < _sampleRate) mirror = false;

	if (mirror) { //mirror extension of signal
		for (int i = 0; i < int(_sampleRate); i++)          //left side
			_pBuffer[i] = _pData[int(_sampleRate) - i];
		for (int i = int(_length + _sampleRate); i < _bufferSize; i++)         //right side
			_pBuffer[i] = _pData[(_length - 2) - (i - (_length + int(_sampleRate)))];
	}
	else {
		for (int i = 0; i < int(_sampleRate); i++)          //left side
			_pBuffer[i] = _pData[0];
		for (int i = int(_length + _sampleRate); i < _bufferSize; i++)         //right side
			_pBuffer[i] = _pData[_length - 1];
	}
}

void Denoise::close()
{
	if (_pBuffer) {
		delete[] _pBuffer;
		_pBuffer = nullptr;
	}
}

bool  Denoise::LFDenoise()
{
	//get base line J///////
	const int J =int(ceil(log2(_sampleRate / 0.8)) - 1);

	if (FastWaveletTransform::init(_pBuffer, _bufferSize, "daub2.flt") == false)
		return false;


	//transform////////////////////////
	transform(J);
	///////////////////////////////////


	int *jNumbers = GetJNumbers(J, _bufferSize);
	double *lo = GetFwtSpectrum();
	for (int i = 0; i < jNumbers[0]; i++)
		lo[i] = 0.0;


	//synth/////////////////////////////
	synthesis(J);
	////////////////////////////////////

	for (int i = 0; i < _length; i++)
		_pData[i] = lo[i + int(_sampleRate)];


	close();
	return true;
}

bool Denoise::HFDenoise()
{
	//get HF scale J///////
	const int J =int(ceil(log2(_sampleRate / 23.0)) - 2);     //[30Hz - ...] hf denoising

	if (FastWaveletTransform::init(_pBuffer, _bufferSize, "bior97.flt") == false)
		return false;


	//transform////////////////////////
	transform(J);
	///////////////////////////////////


	int *jNumbers = GetJNumbers(J, _bufferSize);
	int hiNum;
	int loNum;
	hiLoNumbers(J, _bufferSize, hiNum, loNum);
	double *lo = GetFwtSpectrum();
	double *hi = GetFwtSpectrum() + (_bufferSize - hiNum);

	for (int j = J; j > 0; j--) {
		const int window =int(3.0 * _sampleRate / pow(2.0, double(j)));

		denoise(hi, jNumbers[J - j], window);
		hi += jNumbers[J - j];
	}



	//synth/////////////////////////////
	synthesis(J);
	////////////////////////////////////

	for (int i = 0; i < _length; i++)
		_pData[i] = lo[i + int(_sampleRate)];


	close();
	return true;
}

bool Denoise::LFHFDenoise()
{
	//get base line J///////
	int J = int(ceil(log2(_sampleRate / 0.8)) - 1);

	if (FastWaveletTransform::init(_pBuffer, _bufferSize, "daub2.flt") == false)
		return false;


	//transform////////////////////////
	transform(J);
	///////////////////////////////////

	int* jNumbers = GetJNumbers(J, _bufferSize);
	double* lo = GetFwtSpectrum();
	for (int i = 0; i < jNumbers[0]; i++)
		lo[i] = 0.0;

	//synth/////////////////////////////
	synthesis(J);
	////////////////////////////////////

	for (int i = 0; i < _bufferSize; i++)
		_pBuffer[i] = lo[i];

	////////get min max///////////////////
	double min, max;
	MinMax(&lo[int(_sampleRate)], _length, min, max);

	close();



	//get HF scale J///////
	J =int(ceil(log2(_sampleRate / 23.0)) - 2);     //[30Hz - ...] hf denoising

	if (FastWaveletTransform::init(_pBuffer, _bufferSize, "bior97.flt") == false)
		return false;


	//transform////////////////////////
	transform(J);
	///////////////////////////////////

	jNumbers = GetJNumbers(J, _bufferSize);
	int hiNum;
	int loNum;
	hiLoNumbers(J, _bufferSize, hiNum, loNum);
	lo = GetFwtSpectrum();
	double* hi = GetFwtSpectrum() + (_bufferSize - hiNum);

	for (int j = J; j > 0; j--) {
		const int window =int(3.0 * _sampleRate / pow(2.0, double(j)));

		denoise(hi, jNumbers[J - j], window);
		hi += jNumbers[J - j];
	}

	//synth/////////////////////////////
	synthesis(J);
	////////////////////////////////////

	for (int i = 0; i < _length; i++)
		_pData[i] = lo[i + int(_sampleRate)];

	//renormalize
	NormalizeByMinMax(_pData, _length, min, max);

	close();
	return true;
}
