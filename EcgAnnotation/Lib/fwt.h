

#ifndef FWT_h
#define FWT_h

typedef struct _fwthdr {
        char hdr[4];
        unsigned int size;
        float sr;
        unsigned char bits;
        unsigned char lead;
        unsigned short umv;

        char wlet[8];
        unsigned short J;

        char rsrv[14];
} FWTHDR, *PFWTHDR;

class Signal;

class FWT : public Signal
{
public:
        FWT();
        //FWT(const FWT& fwt);
        ~FWT();

// Operators
        //const FWT& operator=(const FWT& fwt);

// Operations
        bool InitFWT(const wchar_t* fltname, const double* data, int size);
        void CloseFWT();

        void FwtTrans(int scales);                      //wavelet transform
        void FwtSynth(int scales);                      //wavelet synthesis

        bool FwtSaveFile(const wchar_t *name, const double *hipass, const double *lopass, PFWTHDR hdr);
        bool FwtReadFile(const wchar_t *name, const char *appdir = 0);

// Access
        inline double* GetFwtSpectrum() const;        
        inline int GetLoBandSize() const;        
        inline int GetJ() const;                
        int* GetJnumbs(int j, int size);
        void HiLoNumbs(int j, int size, int &hinum, int &lonum) const;

// Inquiry

protected:
private:
        FWT(const FWT& fwt);
        const FWT& operator=(const FWT& fwt);

        //filters inits
        FILE* _filter;
        double* LoadFilter(int &L, int &Z) const;
        void HiLoTrans();
        void HiLoSynth();

        PFWTHDR _phdr;
        char _filterName[_MAX_PATH];

        double *_tH, *_tG;     //analysis filters
        double *_h, *_g;       //synth filters
        int _thL, _tgL, _hL, _gL;     //filters lenghts
        int _thZ, _tgZ, _hZ, _gZ;     //filter centers

        int _j;                //scales
        int *_jnumbs;          //hi values per scale
        int _signalSize;       //signal size
        int _loBandSize;       //divided signal size

        //spectra
        double *_pFwtSpectrum;              //buffer with fwt spectra
        double *_pTmpSpectrum;              //temporary
        double *_pHiData, *_pLoData;         //pointers to pTmpSpectrum
        int _hiNum, _loNum;                       //number of lo and hi coeffs


};

// Inlines
inline double* FWT::GetFwtSpectrum() const
{
        return _pFwtSpectrum;
}

inline int FWT::GetLoBandSize() const
{
        return _loBandSize;
}

int FWT::GetJ() const
{
        return _j;
}

#endif FWT_h

