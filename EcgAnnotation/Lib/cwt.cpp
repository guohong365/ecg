

#include "stdafx.h"
#include "signal.h"
#include "cwt.h"


CWT::CWT(): _phdr(nullptr), _minFreq(0), _maxFreq(0), _freqInterval(0), _w0(0), _scaleType(LINEAR_SCALE), _wavelet(),
            _signalSize(0), _pData(nullptr),
            _pCwtSpectrum(0),
            _pReal(0), _pImag(0), _isPrecision(false), _precisionSize(0), _isPeriodicBoundary(false), _leftVal(0),
            _rightVal(0)
{
}

CWT::~CWT()
{
        if (_pReal) free(_pReal);
        if (_pImag) free(_pImag);
        if (_pCwtSpectrum) free(_pCwtSpectrum);
}

void CWT::initCwt(int size, enum WAVELET wavelet, double w, double sr)
{
        _signalSize = size;

        if (sr)
                _sr = sr;

        _w0 = w;
        _pReal = (double *)malloc(sizeof(double) * (2 * _signalSize - 1));
        _pImag = (double *)malloc(sizeof(double) * (2 * _signalSize - 1));
        _pCwtSpectrum = (double *)malloc(sizeof(double) * (_signalSize));
        _wavelet = wavelet;        

        for (int i = 0; i < 2*_signalSize - 1; i++) {
                _pReal[i] = 0;
                _pImag[i] = 0;
        }
}

void  CWT::closeCwt()
{
        if (_pReal) {
                free(_pReal);
                _pReal = 0;
        }
        if (_pImag) {
                free(_pImag);
                _pImag = 0;
        }
        if (_pCwtSpectrum) {
                free(_pCwtSpectrum);
                _pCwtSpectrum = 0;
        }
}

float* CWT::CwtCreateFileHeader(wchar_t *name, PCWTHDR hdr, enum WAVELET wavelet, double w)
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


        filesize = sizeof(float) * filesize + sizeof(CWTHDR);

        _fp = CreateFileW(name, GENERIC_WRITE | GENERIC_READ, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (_fp == INVALID_HANDLE_VALUE)
                return 0;
        _fpmap = CreateFileMapping(_fp, 0, PAGE_READWRITE, 0, filesize, 0);
        _lpMap = MapViewOfFile(_fpmap, FILE_MAP_WRITE, 0, 0, filesize);

        _lpf = (float *)_lpMap;

        memset(_lpMap, 0, filesize);
        memcpy(_lpMap, hdr, sizeof(CWTHDR));

        return (_lpf + sizeof(CWTHDR) / sizeof(float));
}

float* CWT::CwtReadFile(const wchar_t *name)
{
        _fp = CreateFileW(name, GENERIC_WRITE | GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (_fp == INVALID_HANDLE_VALUE)
                return 0;
        _fpmap = CreateFileMapping(_fp, 0, PAGE_READWRITE, 0, 0, 0);
        _lpMap = MapViewOfFile(_fpmap, FILE_MAP_WRITE, 0, 0, 0);

        _phdr = (PCWTHDR)_lpMap;
        _lpf = (float *)_lpMap;


        if (memcmp(_phdr->hdr, "WLET", 4))
                return 0;

        _minFreq = _phdr->fmin;
        _maxFreq = _phdr->fmax;
        _freqInterval = _phdr->fstep;
        _length = _phdr->size;
        _sr = _phdr->sr;
        if (_phdr->type == LINEAR_SCALE)
                _scaleType = LINEAR_SCALE;
        else if(_phdr->type == LOG_SCALE)
                _scaleType = LOG_SCALE;

        return (_lpf + sizeof(CWTHDR) / sizeof(float));
}

double CWT::HzToScale(double f, double sr, enum WAVELET wavelet, double w) const
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

void CWT::ConvertName(wchar_t *name, enum WAVELET wavelet, double w) const
{
        wchar_t tmp[_MAX_PATH];

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
}


double* CWT::CwtTrans(const double *data, double freq, bool periodicboundary, double lval, double rval)
{
        _isPeriodicBoundary = periodicboundary;
        _leftVal = lval;
        _rightVal = rval;

        _isPrecision = false;
        _precisionSize = 0;                                      //0,0000001 float prsision
        double t, sn, cs, scale;

        scale = HzToScale(freq, _sr, _wavelet, _w0);

///////////wavelet calculation//////////////////////////////////////////////////////////////////
///////// center = SignalSize-1 in wavelet mass////////////////////////////////////////////////////

        for (int i = 0; i < _signalSize; i++) {                     //positive side
                t = ((double)i) / scale;

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
                        _pReal[(_signalSize-1)+i] = exp(-t * t / 2) * (-t * t + 1);
                        break;
                case INV:
                        _pReal[(_signalSize-1)+i] = t * exp(-t * t / 2);
                        break;
                case MORL:
                        _pReal[(_signalSize-1)+i] = exp(-t * t / 2) * (cs - sn);
                        break;
                case MORLPOW:
                        _pReal[(_signalSize-1)+i] = exp(-t * t / 2) * cs;
                        _pImag[(_signalSize-1)+i] = exp(-t * t / 2) * sn;
                        break;
                case MORLFULL:
                        _pReal[(_signalSize-1)+i] = exp(-t * t / 2) * (cs - exp(-_w0 * _w0 / 2));
                        _pImag[(_signalSize-1)+i] = exp(-t * t / 2) * (sn - exp(-_w0 * _w0 / 2));
                        break;

                case GAUS:
                        _pReal[(_signalSize-1)+i] = exp(-t * t / 2);
                        break;
                case GAUS1:
                        _pReal[(_signalSize-1)+i] = -t * exp(-t * t / 2);
                        break;
                case GAUS2:
                        _pReal[(_signalSize-1)+i] = (t * t - 1) * exp(-t * t / 2);
                        break;
                case GAUS3:
                        _pReal[(_signalSize-1)+i] = (2 * t + t - t * t * t) * exp(-t * t / 2);
                        break;
                case GAUS4:
                        _pReal[(_signalSize-1)+i] = (3 - 6 * t * t + t * t * t * t) * exp(-t * t / 2);
                        break;
                case GAUS5:
                        _pReal[(_signalSize-1)+i] = (-15 * t + 10 * t * t * t - t * t * t * t * t) * exp(-t * t / 2);
                        break;
                case GAUS6:
                        _pReal[(_signalSize-1)+i] = (-15 + 45 * t * t - 15 * t * t * t * t + t * t * t * t * t * t) * exp(-t * t / 2);
                        break;
                case GAUS7:
                        _pReal[(_signalSize-1)+i] = (105 * t - 105 * t * t * t + 21 * t * t * t * t * t - t * t * t * t * t * t * t) * exp(-t * t / 2);
                        break;
                }

                if (fabs(_pReal[(_signalSize-1)+i]) < 0.0000001)
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
                t = ((double)i) / scale;

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
                        _pReal[(_signalSize-1)+i] = exp(-t * t / 2) * (-t * t + 1);
                        break;
                case INV:
                        _pReal[(_signalSize-1)+i] = t * exp(-t * t / 2);
                        break;
                case MORL:
                        _pReal[(_signalSize-1)+i] = exp(-t * t / 2) * (cs - sn);
                        break;
                case MORLPOW:
                        _pReal[(_signalSize-1)+i] = exp(-t * t / 2) * cs;
                        _pImag[(_signalSize-1)+i] = exp(-t * t / 2) * sn;
                        break;
                case MORLFULL:
                        _pReal[(_signalSize-1)+i] = exp(-t * t / 2) * (cs - exp(-_w0 * _w0 / 2));
                        _pImag[(_signalSize-1)+i] = exp(-t * t / 2) * (sn - exp(-_w0 * _w0 / 2));
                        break;

                case GAUS:
                        _pReal[(_signalSize-1)+i] = exp(-t * t / 2);    //gauss
                        break;
                case GAUS1:
                        _pReal[(_signalSize-1)+i] = -t * exp(-t * t / 2);   //gauss1
                        break;
                case GAUS2:
                        _pReal[(_signalSize-1)+i] = (t * t - 1) * exp(-t * t / 2);     //gauss2
                        break;
                case GAUS3:
                        _pReal[(_signalSize-1)+i] = (2 * t + t - t * t * t) * exp(-t * t / 2);    //gauss3
                        break;
                case GAUS4:
                        _pReal[(_signalSize-1)+i] = (3 - 6 * t * t + t * t * t * t) * exp(-t * t / 2);    //gauss4
                        break;
                case GAUS5:
                        _pReal[(_signalSize-1)+i] = (-15 * t + 10 * t * t * t - t * t * t * t * t) * exp(-t * t / 2);    //gauss5
                        break;
                case GAUS6:
                        _pReal[(_signalSize-1)+i] = (-15 + 45 * t * t - 15 * t * t * t * t + t * t * t * t * t * t) * exp(-t * t / 2);   //gauss6
                        break;
                case GAUS7:
                        _pReal[(_signalSize-1)+i] = (105 * t - 105 * t * t * t + 21 * t * t * t * t * t - t * t * t * t * t * t * t) * exp(-t * t / 2);    //gauss7
                        break;
                }
        }
///////end wavelet calculations////////////////////////////////////////////

        _pData = data;
        for (int x = 0; x < _signalSize; x++)
                _pCwtSpectrum[x] = CwtTrans(x, scale);

        return _pCwtSpectrum;
}
