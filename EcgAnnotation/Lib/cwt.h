

#ifndef CWT_h
#define CWT_h

typedef struct _cwthdr {
        char hdr[4];
        float fmin;
        float fmax;
        float fstep;
        unsigned int size;
        float sr;
        unsigned char type;   //1-log; 0-norm scale

        char rsrv[15];
} CWTHDR, *PCWTHDR;

class Signal;

class CWT : public Signal
{
public:
        CWT();
        //CWT(const CWT& cwt);
        ~CWT();

// Data
        enum WAVELET {MHAT, INV, MORL, MORLPOW, MORLFULL, GAUS, GAUS1, GAUS2, GAUS3, GAUS4, GAUS5, GAUS6, GAUS7};
        enum SCALE_TYPE {LINEAR_SCALE, LOG_SCALE};

// Operators
        //const CWT& operator=(const CWT& cwt);

// Operations
        float* CwtCreateFileHeader(wchar_t *name, PCWTHDR hdr, enum WAVELET wavelet, double w);
        float* CwtReadFile(const wchar_t *name);
        double HzToScale(double f, double sr, enum WAVELET wavelet, double w) const;
        void ConvertName(wchar_t *name, enum WAVELET wavelet, double w) const;

        void initCwt(int size, enum WAVELET wavelet, double w, double sr);
        void closeCwt();
        double* CwtTrans(const double *data, double freq, bool periodicboundary = true, double lv = 0, double rv = 0);

// Access
        inline double GetMinFreq() const;
        inline double GetMaxFreq() const;
        inline double GetFreqInterva() const;
        inline int GetScaleType() const;         
        inline int GetFreqRange() const;

// Inquiry

protected:
private:
        CWT(const CWT& cwt);
        const CWT& operator=(const CWT& cwt);

        inline double CwtTrans(int x, double scale);

        PCWTHDR _phdr;

        double _minFreq;
        double _maxFreq;
        double _freqInterval;
        double _w0;        
        
        enum SCALE_TYPE _scaleType;        
        enum WAVELET _wavelet;                   //Wavelet

        int _signalSize; 
        const double *_pData;               //pointer to original signal
        double *_pCwtSpectrum;              //buffer with spectra
        double *_pReal, *_pImag;             //Re,Im parts of wavelet
        
        bool _isPrecision; 
        int _precisionSize; 

        bool _isPeriodicBoundary;                //periodic boundary extension
        double _leftVal, _rightVal;          //left/right value for signal ext at boundaries


};

/*//////////////////////////////////////////////
        CWT *cwt;
        cwt->InitCWT(size, CWT::MHAT, w0, SR);
        pCwtSpectrum = cwt->CwtTrans(signal, freq);
        cwt->CloseCWT();
//////////////////////////////////////////////*/

// Inlines
inline double CWT::CwtTrans(int x, double scale)
{
        double res = 0, Re = 0, Im = 0;

        for (int t = 0; t < _signalSize; t++) {                   //main
                if (_isPrecision == true) {
                        if (t < x - _precisionSize) 
                                t = x - (_precisionSize - 1);  //continue;
                        if (t >= _precisionSize + x) 
                                break;
                }

                Re += _pReal[((_signalSize-1)-x) + t] * _pData[t];
                if (_wavelet == MORLPOW || _wavelet == MORLFULL)
                        Im += _pImag[((_signalSize-1)-x) + t] * _pData[t];
        }
        
        ////////////////////boundaries///////////////////////////////////////////////
        int p = 0;
        for (int i = (_signalSize - _precisionSize); i < (_signalSize - 1) - x; i++) {        // Left edge calculations
                if (_isPeriodicBoundary) {
                        Re += _pReal[i] * _pData[(_signalSize-1)-i-x];  //IsPeriodicBoundary
                } else {
                        if (_leftVal != 0.0)
                                Re += _pReal[i] * _leftVal;
                        else
                                Re += _pReal[i] * _pData[0];
                }

                if (_wavelet == MORLPOW || _wavelet == MORLFULL) { //Im part for complex wavelet
                        if (_isPeriodicBoundary) {
                                Im += _pImag[i] * _pData[(_signalSize-1)-i-x];
                        } else {
                                if (_leftVal != 0.0)
                                        Im += _pImag[i] * _leftVal;
                                else
                                        Im += _pImag[i] * _pData[0];
                        }
                }
        }
        int q = 0;
        for (int i = 2 * _signalSize - (x + 1); i < _signalSize + _precisionSize - 1; i++) {     // Right edge calculations
                if (_isPeriodicBoundary)
                        Re += _pReal[i] * _pData[(_signalSize-2)-q]; //IsPeriodicBoundary
                else {
                        if (_rightVal != 0.0)
                                Re += _pReal[i] * _rightVal;
                        else
                                Re += _pReal[i] * _pData[_signalSize-1];
                }

                if (_wavelet == MORLPOW || _wavelet == MORLFULL) {
                        if (_isPeriodicBoundary) {
                                Im += _pImag[i] * _pData[(_signalSize-2)-q];
                        } else {
                                if (_rightVal != 0.0)
                                        Im += _pImag[i] * _rightVal;
                                else
                                        Im += _pImag[i] * _pData[_signalSize-1];
                        }
                }
                q++;
        }
        ////////////////////boundaries///////////////////////////////////////////////


        switch (_wavelet) {
        case MORL:
                res = (1 / sqrt(6.28)) * Re;
                break;
        case MORLPOW:
                res = sqrt(Re * Re + Im * Im);
                res *= (1 / sqrt(6.28));
                break;
        case MORLFULL:
                res = sqrt(Re * Re + Im * Im);
                res *= (1 / pow(3.14, 0.25));
                break;

        default:
                res = Re;
        }

        res = (1 / sqrt(scale)) * res;

        return res;
}

inline int CWT::GetFreqRange() const
{
        if (_scaleType == LINEAR_SCALE)
                return (int)((_maxFreq + _freqInterval - _minFreq) / _freqInterval);
        else if (_scaleType == LINEAR_SCALE)
                return (int)((log(_maxFreq) + _freqInterval - log(_minFreq)) / _freqInterval);
        else 
                return 0;
}

inline double CWT::GetMinFreq() const
{
        return _minFreq;
}

inline double CWT::GetMaxFreq() const
{
        return _maxFreq;
}

inline double CWT::GetFreqInterva() const 
{
        return _freqInterval;
}

inline int CWT::GetScaleType() const
{
        return _scaleType;
}

#endif CWT_h

