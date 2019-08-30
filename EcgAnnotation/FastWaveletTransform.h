#pragma once
#include <stdio.h>
#include "ecgtypes.h"
#include <string>

class FastWaveletTransform
{
public:
	FastWaveletTransform();
	~FastWaveletTransform();

	// Operators
			//const FWT& operator=(const FWT& fwt);

	// Operations
    bool init(const double* data, int size, const char* filter);
	void close();

	void transform(int scales);                      //wavelet transform
	void synthesis(int scales);                      //wavelet synthesis

	//bool FwtSaveFile(const wchar_t *name, const double *hipass, const double *lopass, PFWT_HEADER hdr);
	//bool FwtReadFile(const wchar_t *name, const char *appdir = 0);

	// Access
	inline double* GetFwtSpectrum() const;
	inline int getLoBandSize() const;
	inline int getJ() const;
	int* GetJNumbers(int j, int size);

	static void hiLoNumbers(int j, int size, int &hiNum, int &loNum);

	static void setFilterDir(const char* filterDir)
	{
        _filterDir = filterDir ? filterDir : "";
        char c=*_filterDir.rbegin();
        if(c != '\\' && c!='/'){
            _filterDir.append("/");
        }
    }
    static const char* getFilterDir(){
        return _filterDir.c_str();
    }
protected:
	static std::string _filterDir;

private:
	FastWaveletTransform(const FastWaveletTransform& fwt) = delete;
	const FastWaveletTransform& operator=(const FastWaveletTransform& fwt) = delete;

	static double* _loadFilter(FILE* fp, int &L, int &Z);
	void _hiLoTransform() const;
	void _hiLoSynthesis() const;

	PFWT_HEADER _pHDR;
	
	double *_tH, *_tG;     //analysis filters
	double *_h, *_g;       //synth filters
	int _thL, _tgL, _hL, _gL;     //filters lenghts
	int _thZ, _tgZ, _hZ, _gZ;     //filter centers

	int _j;                //scales
	int *_jNumbers;          //hi values per scale
	int _signalSize;       //signal size
	int _loBandSize;       //divided signal size

	//spectra
	double *_pSpectrum;              //buffer with fwt spectra
	double *_pTmpSpectrum;              //temporary
	double *_pHiData;
	double *_pLoData;
	int _hiNum;
	int _loNum;
};

// Inlines
inline double* FastWaveletTransform::GetFwtSpectrum() const
{
	return _pSpectrum;
}

inline int FastWaveletTransform::getLoBandSize() const
{
	return _loBandSize;
}

int FastWaveletTransform::getJ() const
{
	return _j;
}
