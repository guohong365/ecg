

#include "stdafx.h"
#include "signal.h"
#include "fwt.h"


FWT::FWT(): _filter(nullptr), _phdr(nullptr), _filterName{}, _tH(nullptr), _tG(nullptr), _h(nullptr), _g(nullptr),
            _thL(0), _tgL(0), _hL(0), _gL(0), _thZ(0), _tgZ(0), _hZ(0), _gZ(0),
            _j(0), _jnumbs(nullptr), _signalSize(0), _loBandSize(0),
            _pFwtSpectrum(nullptr), _pTmpSpectrum(nullptr), _pHiData(nullptr), _pLoData(nullptr), _hiNum(0), _loNum(0)
{
}

FWT::~FWT()
{
        if (_tH) delete[] _tH;
        if (_tG) delete[] _tG;
        if (_h) delete[] _h;
        if (_g) delete[] _g;

        if (_pFwtSpectrum) free(_pFwtSpectrum);
        if (_pTmpSpectrum) free(_pTmpSpectrum);

        if (_jnumbs) delete[] _jnumbs;
}

bool FWT::InitFWT(const wchar_t* fltname, const double* data, int size)
{
        _filter = _wfopen(fltname, L"rt");
        if (_filter) {
                _tH = LoadFilter(_thL, _thZ);
                _tG = LoadFilter(_tgL, _tgZ);
                _h = LoadFilter(_hL, _hZ);
                _g = LoadFilter(_gL, _gZ);
                fclose(_filter);

                _loBandSize = size;
                _signalSize = size;
                _pFwtSpectrum = static_cast<double *>(malloc(sizeof(double) * size));
                _pTmpSpectrum = static_cast<double *>(malloc(sizeof(double) * size));
                _pLoData = _pTmpSpectrum;
                _pHiData = _pTmpSpectrum + size;

                for (int i = 0; i < size; i++)
                        _pFwtSpectrum[i] = data[i];
                memset(_pTmpSpectrum, 0, sizeof(double)*size);

                _j = 0;

                return true;
        } else
                return false;
}

double* FWT::LoadFilter(int& L, int& Z) const
{
        fscanf(_filter, "%d", &L);
        fscanf(_filter, "%d", &Z);

        double *flt = new double[L];

        for (int i = 0; i < L; i++)
                fscanf(_filter, "%lf", &flt[i]);

        return flt;
}

void FWT::CloseFWT()
{
        if (_tH) {
                delete[] _tH;
                _tH = 0;
        }
        if (_tG) {
                delete[] _tG;
                _tG = 0;
        }
        if (_h) {
                delete[] _h;
                _h = 0;
        }
        if (_g) {
                delete[] _g;
                _g = 0;
        }

        if (_pFwtSpectrum) {
                free(_pFwtSpectrum);
                _pFwtSpectrum = 0;
        }
        if (_pTmpSpectrum) {
                free(_pTmpSpectrum);
                _pTmpSpectrum = 0;
        }

        if (_jnumbs) {
                delete[] _jnumbs;
                _jnumbs = 0;
        }
}


//////////////////////transforms///////////////////////////////////////////////////////////////////
void FWT::HiLoTrans()
{
        int n;
        double s, d;

        for (int k = 0; k < _loBandSize / 2; k++) {
                s = 0;
                d = 0;

                for (int m = -_thZ; m < _thL - _thZ; m++) {
                        n = 2 * k + m;
                        if (n < 0) n = 0 - n;
                        if (n >= _loBandSize) n -= (2 + n - _loBandSize);
                        s += _tH[m+_thZ] * _pFwtSpectrum[n];
                }

                for (int m = -_tgZ; m < _tgL - _tgZ; m++) {
                        n = 2 * k + m;
                        if (n < 0) n = 0 - n;
                        if (n >= _loBandSize) n -= (2 + n - _loBandSize);
                        d += _tG[m+_tgZ] * _pFwtSpectrum[n];
                }

                _pLoData[k] = s;
                _pHiData[k] = d;
        }

        for (int i = 0; i < _signalSize; i++)
                _pFwtSpectrum[i] = _pTmpSpectrum[i];
}

void FWT::FwtTrans(int scales)
{
        for (int j = 0; j < scales; j++) {
                _pHiData -= _loBandSize / 2;
                HiLoTrans();

                _loBandSize /= 2;
                _j++;
        }
}

void FWT::HiLoSynth()
{
        int n;
        double s2k, s2k1;

        for (int i = 0; i < _signalSize; i++)
                _pTmpSpectrum[i] = _pFwtSpectrum[i];

        for (int k = 0; k < _loBandSize; k++) {
                s2k = 0;
                s2k1 = 0;

                for (int m = -_hZ; m < _hL - _hZ; m++) {       //s2k and s2k1 for H[]
                        n = k - m;
                        if (n < 0) n = 0 - n;
                        if (n >= _loBandSize) n -= (2 + n - _loBandSize);

                        if (2*m >= -_hZ && 2*m < _hL - _hZ)
                                s2k += _h[(2*m)+_hZ] * _pLoData[n];
                        if ((2*m + 1) >= -_hZ && (2*m + 1) < _hL - _hZ)
                                s2k1 += _h[(2*m+1)+_hZ] * _pLoData[n];
                }

                for (int m = -_gZ; m < _gL - _gZ; m++) {      //s2k and s2k1 for G[]
                        n = k - m;
                        if (n < 0) n = 0 - n;
                        if (n >= _loBandSize) n -= (2 + n - _loBandSize);

                        if (2*m >= -_gZ && 2*m < _gL - _gZ)
                                s2k += _g[(2*m)+_gZ] * _pHiData[n];
                        if ((2*m + 1) >= -_gZ && (2*m + 1) < _gL - _gZ)
                                s2k1 += _g[(2*m+1)+_gZ] * _pHiData[n];
                }

                _pFwtSpectrum[2*k] = 2.0 * s2k;
                _pFwtSpectrum[2*k+1] = 2.0 * s2k1;
        }
}

void FWT::FwtSynth(int scales)
{
        for (int j = 0; j < scales; j++) {
                HiLoSynth();
                _pHiData += _jnumbs[j];

                _loBandSize *= 2;
                _j--;
        }
}
////////////////////////////////////////////////////////////////////////////////////////////////


int* FWT::GetJnumbs(int j, int size)
{
        if (_jnumbs) delete[] _jnumbs;

        _jnumbs = new int[j];

        for (int i = 0; i < j; i++)
                _jnumbs[i] = size / (int)pow(2, (double)(j - i));

        return _jnumbs;
}

void FWT::HiLoNumbs(int j, int size, int &hinum, int &lonum) const
{
        lonum = 0;
        hinum = 0;

        for (int i = 0; i < j; i++) {
                size /= 2;
                hinum += size;
        }
        lonum = size;
}

bool FWT::FwtSaveFile(const wchar_t *name, const double *hipass, const double *lopass, PFWTHDR hdr)
{
        int filesize;
        short tmp;

        HiLoNumbs(hdr->J, hdr->size, _hiNum, _loNum);

        switch (hdr->bits) {
        case 12:
                if ((_hiNum + _loNum) % 2 != 0)
                        filesize = int((double)((_hiNum + _loNum) + 1) * 1.5);
                else
                        filesize = int((double)(_hiNum + _loNum) * 1.5);
                break;
        case 16:
                filesize = (_hiNum + _loNum) * 2;
                break;
        case 0:
        case 32:
                filesize = (_hiNum + _loNum) * 4;
                break;
        }

        _fp = CreateFileW(name, GENERIC_WRITE | GENERIC_READ, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (_fp == INVALID_HANDLE_VALUE) {
                _fp = 0;
                return false;
        }
        _fpmap = CreateFileMapping(_fp, 0, PAGE_READWRITE, 0, filesize + sizeof(FWTHDR), 0);
        _lpMap = MapViewOfFile(_fpmap, FILE_MAP_WRITE, 0, 0, filesize + sizeof(FWTHDR));

        _lpf = static_cast<float *>(_lpMap);
        _lps = static_cast<short *>(_lpMap);
        _lpc = static_cast<char *>(_lpMap);

        memset(_lpMap, 0, filesize + sizeof(FWTHDR));
        memcpy(_lpMap, hdr, sizeof(FWTHDR));

        _lpf += sizeof(FWTHDR) / sizeof(float);
        _lps += sizeof(FWTHDR) / sizeof(short);
        _lpc += sizeof(FWTHDR);

        for (int i = 0; i < _hiNum + _loNum; i++) {
                if (i < _hiNum)
                        tmp = short(hipass[i] * (double)hdr->umv);
                else
                        tmp = short(lopass[i-_hiNum] * (double)hdr->umv);

                switch (hdr->bits) {
                case 12:
                        if (i % 2 == 0) {
                                _lpc[0] = LOBYTE(tmp);
                                _lpc[1] = 0;
                                _lpc[1] = HIBYTE(tmp) & 0x0f;
                        } else {
                                _lpc[2] = LOBYTE(tmp);
                                _lpc[1] |= HIBYTE(tmp) << 4;
                                _lpc += 3;
                        }
                        break;

                case 16:                                               //16format
                        *_lps++ = tmp;
                        break;

                case 0:
                case 32:
                        if (i < _hiNum)                                        //32bit float
                                *_lpf++ = (float)hipass[i];
                        else
                                *_lpf++ = (float)lopass[i-_hiNum];
                        break;
                }
        }

        CloseFile();
        return true;
}

bool FWT::FwtReadFile(const wchar_t *name, const char *appdir)
{
        short tmp;

        _fp = CreateFileW(name, GENERIC_WRITE | GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (_fp == INVALID_HANDLE_VALUE) {
                _fp = 0;
                return false;
        }
        _fpmap = CreateFileMapping(_fp, 0, PAGE_READWRITE, 0, 0, 0);
        _lpMap = MapViewOfFile(_fpmap, FILE_MAP_WRITE, 0, 0, 0);

        _phdr = (PFWTHDR)_lpMap;
        _lpf = (float *)_lpMap;
        _lps = (short *)_lpMap;
        _lpc = (char *)_lpMap;

        _length = _phdr->size;
        _sr = _phdr->sr;
        _bits = _phdr->bits;
        _lead = _phdr->lead;
        _umV = _phdr->umv;
        strcpy(_filterName, _phdr->wlet);
        _j = _phdr->J;


        _lpf += sizeof(FWTHDR) / sizeof(float);
        _lps += sizeof(FWTHDR) / sizeof(short);
        _lpc += sizeof(FWTHDR);

        HiLoNumbs(_j, _length, _hiNum, _loNum);
        _signalSize = _loNum + _hiNum;
        _loBandSize = _loNum;

        _pFwtSpectrum = new double[_signalSize];
        _pTmpSpectrum = new double[_signalSize];
        memset(_pTmpSpectrum, 0, sizeof(double)*_signalSize);
        _pLoData = _pFwtSpectrum;
        _pHiData = _pFwtSpectrum + _loNum;

        for (int i = 0; i < _signalSize; i++) {
                switch (_bits) {
                case 12:                                             //212 format   12bit
                        if (i % 2 == 0) {
                                tmp = MAKEWORD(_lpc[0], _lpc[1] & 0x0f);
                                if (tmp > 0x7ff)
                                        tmp |= 0xf000;
                        } else {
                                tmp = MAKEWORD(_lpc[2], (_lpc[1] & 0xf0) >> 4);
                                if (tmp > 0x7ff)
                                        tmp |= 0xf000;
                                _lpc += 3;
                        }

                        if (i < _hiNum) 
                                _pHiData[i] = (double)tmp / (double)_umV;
                        else 
                                _pLoData[i-_hiNum] = (double)tmp / (double)_umV;                        
                        break;

                case 16:                                             //16format
                        if (i < _hiNum) 
                                _pHiData[i] = (double)_lps[i] / (double)_umV;
                        else 
                                _pLoData[i-_hiNum] = (double)_lps[i] / (double)_umV;                        
                        break;

                case 0:
                case 32:                                             //32bit float
                        if (i < _hiNum) 
                                _pHiData[i] = (double)_lpf[i];
                        else 
                                _pLoData[i-_hiNum] = (double)_lpf[i];                        
                        break;
                }

        }

        _pLoData = _pTmpSpectrum;
        _pHiData = _pTmpSpectrum + _loNum;


        if (appdir) { //load filter for synthesis
                char flt[256];
                strcpy(flt, appdir);
                strcat(flt, "\\filters\\");
                strcat(flt, _filterName);
                strcat(flt, ".flt");

                _filter = fopen(flt, "rt");
                if (_filter) {
                        _tH = LoadFilter(_thL, _thZ);
                        _tG = LoadFilter(_tgL, _tgZ);
                        _h = LoadFilter(_hL, _hZ);
                        _g = LoadFilter(_gL, _gZ);
                        fclose(_filter);
                } else {
                        CloseFile();
                        return false;
                }
        }


        CloseFile();
        return true;
}

