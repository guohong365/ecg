

#include "stdafx.h"
#include "signal.h"
#include "fwt.h"
#include "ecgdenoise.h"
#include "ecgannotation.h"


EcgDenoise::EcgDenoise() : _filtersDir{}, _pEcgData(nullptr), _pTmpData(nullptr)
{
}

EcgDenoise::~EcgDenoise()
{
        if (_pTmpData != 0) 
                free(_pTmpData);
}

void EcgDenoise::InitDenoise(const wchar_t* fltdir, double* data, int size, double sr, bool mirror)
{
        wcscpy(_filtersDir, fltdir);

        _pEcgData = data;
        _sr = sr;
        _length = size;

        if (_pTmpData) free(_pTmpData);
        _pTmpData = static_cast<double *>(malloc(sizeof(double) * size_t(_length + 2 * _sr)));    // [SR add] [sig] [SR add]

        for (int i = 0; i < _length; i++)           //signal
                _pTmpData[i+int(_sr)] = _pEcgData[i];

        if (_length < _sr) mirror = false;

        if (mirror) { //mirror extension of signal
                for (int i = 0; i < int(_sr); i++)          //left side
                        _pTmpData[i] = _pEcgData[int(_sr)-i];
                for (int i = _length + _sr; i < _length + 2*_sr; i++)         //right side
                        _pTmpData[i] = _pEcgData[(_length-2) - (i-(_length+int(_sr)))];
        } else {
                for (int i = 0; i < int(_sr); i++)          //left side
                        _pTmpData[i] = _pEcgData[0];
                for (int i = _length + _sr; i < _length + 2*_sr; i++)         //right side
                        _pTmpData[i] = _pEcgData[_length-1];
        }
}

void EcgDenoise::CloseDenoise()
{
        if (_pTmpData) {
                free(_pTmpData);
                _pTmpData = 0;
        }
}

bool  EcgDenoise::LFDenoise()
{
        wchar_t filter[_MAX_PATH];


        //get base line J///////
	const int J = ceil(log2(_sr / 0.8)) - 1;

        wcscpy(filter, _filtersDir);
        wcscat(filter, L"\\daub2.flt");
        if (InitFWT(filter, _pTmpData, _length + 2*_sr) == false)
                return false;


        //transform////////////////////////
        FwtTrans(J);
        ///////////////////////////////////


        int *Jnumbs = GetJnumbs(J, _length + 2 * _sr);
        double *lo = GetFwtSpectrum();
        for (int i = 0; i < Jnumbs[0]; i++)
                lo[i] = 0.0;


        //synth/////////////////////////////
        FwtSynth(J);
        ////////////////////////////////////

        for (int i = 0; i < _length; i++)
                _pEcgData[i] = lo[i+int(_sr)];


        CloseFWT();
        return true;
}

bool EcgDenoise::HFDenoise()
{
        wchar_t filter[_MAX_PATH];


        //get HF scale J///////
	const int J = ceil(log2(_sr / 23.0)) - 2;     //[30Hz - ...] hf denoising

        wcscpy(filter, _filtersDir);
        wcscat(filter, L"\\bior97.flt");
        if (InitFWT(filter, _pTmpData, _length + 2*_sr) == false)
                return false;


        //transform////////////////////////
        FwtTrans(J);
        ///////////////////////////////////


        int *Jnumbs = GetJnumbs(J, _length + 2 * _sr);
        int hinum, lonum;
        HiLoNumbs(J, _length + 2*_sr, hinum, lonum);
        double *lo = GetFwtSpectrum();
        double *hi = GetFwtSpectrum() + (int(_length + 2 * _sr) - hinum);

	for (int j = J; j > 0; j--) {
		const int window = 3.0 * _sr / pow(2.0, double(j));

                denoise(hi, Jnumbs[J-j], window);
                hi += Jnumbs[J-j];
        }



        //synth/////////////////////////////
        FwtSynth(J);
        ////////////////////////////////////

        for (int i = 0; i < _length; i++)
                _pEcgData[i] = lo[i+int(_sr)];


        CloseFWT();
        return true;
}

bool EcgDenoise::LFHFDenoise()
{
        wchar_t filter[_MAX_PATH];
        int*Jnumbs;
        double *lo, *hi;


        //get base line J///////
        int J = ceil(log2(_sr / 0.8)) - 1;

        wcscpy(filter, _filtersDir);
        wcscat(filter, L"\\daub2.flt");
        if (InitFWT(filter, _pTmpData, _length + 2*_sr) == false)
                return false;


        //transform////////////////////////
        FwtTrans(J);
        ///////////////////////////////////

        Jnumbs = GetJnumbs(J, _length + 2 * _sr);
        lo = GetFwtSpectrum();
        for (int i = 0; i < Jnumbs[0]; i++)
                lo[i] = 0.0;

        //synth/////////////////////////////
        FwtSynth(J);
        ////////////////////////////////////

        for (int i = 0; i < _length + 2*_sr; i++)
                _pTmpData[i] = lo[i];

        ////////get min max///////////////////
        double min, max;
        MinMax(&lo[int(_sr)], _length, min, max);

        CloseFWT();



        //get HF scale J///////
        J = ceil(log2(_sr / 23.0)) - 2;     //[30Hz - ...] hf denoising

        wcscpy(filter, _filtersDir);
        wcscat(filter, L"\\bior97.flt");
        if (InitFWT(filter, _pTmpData, _length + 2*_sr) == false)
                return false;


        //transform////////////////////////
        FwtTrans(J);
        ///////////////////////////////////

        Jnumbs = GetJnumbs(J, _length + 2 * _sr);
        int hinum, lonum;
        HiLoNumbs(J, _length + 2*_sr, hinum, lonum);
        lo = GetFwtSpectrum();
        hi = GetFwtSpectrum() + (int(_length + 2 * _sr) - hinum);

        int window;   //3sec window
        for (int j = J; j > 0; j--) {
                window = 3.0 * _sr / pow(2.0, double(j));

                denoise(hi, Jnumbs[J-j], window);
                hi += Jnumbs[J-j];
        }

        //synth/////////////////////////////
        FwtSynth(J);
        ////////////////////////////////////

        for (int i = 0; i < _length; i++)
                _pEcgData[i] = lo[i+int(_sr)];

        //renormalize
		NormalizeByMinMax(_pEcgData, _length, min, max);

        CloseFWT();
        return true;
}
