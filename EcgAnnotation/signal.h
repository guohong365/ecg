

#ifndef Signal_h
#define Signal_h

#include <stdio.h>
#include <vector>
#include "ecgtypes.h"
using namespace std;
#define _USE_MATH_DEFINES

class Signal
{
public:
	Signal();
	Signal(const DATA_HEADER & hdr,const double* buffer): _ecgFileName{}
	{
		double* p = new double[hdr.size];
		memcpy_s(p, hdr.size * sizeof(double), buffer, hdr.size * sizeof(double));
		addSeries(hdr, p);
	}

	virtual ~Signal();

	// Operators
			//const Signal& operator=(const Signal& signal);

	// Operations
	void addSeries(const DATA_HEADER& hdr, double* data)
	{
		_ecgHeaders.push_back(hdr);
		_ecgSignals.push_back(data);
	}

	void setFileName(const char* filename)
	{
		strcpy_s(_ecgFileName, _MAX_PATH, filename);
	}

	bool ToTxt(const wchar_t* name, const double* buffer, int size);
	static void mSecToTime(int millisecond, int& h, int& m, int& s, int& ms);

	// Access        
	double* GetData(int index = 0);
	DATA_HEADER * getHeader(int index=0)
	{
		if (!_ecgSignals.size())
			return nullptr;
		if (index > int(_ecgSignals.size()) - 1)
			index = int(_ecgSignals.size()) - 1;
		else if (index < 0)
			index = 0;
		return &_ecgHeaders[index];
	}
	inline int GetLength(int index = 0) const;
	inline int GetBits(int index = 0) const;
	inline int GetUmV(int index = 0) const;
	inline int GetLead(int index = 0) const;
	inline int GetLeadsNum(int index = 0) const;
	inline double GetSR(int index = 0) const;
	inline int GetH(int index = 0) const;
	inline int GetM(int index = 0) const;
	inline int GetS(int index = 0) const;

	// Inquiry

protected:
	//double * _pData;
	//double _sr;
	//int _lead;
	//int _umV;
	//int _bits;
	//int _length;
	//int _hh;
	//int _mm;
	//int _ss;

private:
	Signal(const Signal& Signal) = delete;
	const Signal& operator=(const Signal& Signal) = delete;
	//binary or text file
	char _ecgFileName[_MAX_PATH];         //file name

	vector<DATA_HEADER> _ecgHeaders;             //arrays of headers
	vector<double *> _ecgSignals;       //arrays of signals        
};

/*////////info////////////////////////////////
to OPEN file:
		Signal Ecg;
		double* pEcgData;

		pEcgData = Ecg.ReadFile("filename");              //error pEcgData = NULL
		Length = Ecg.GetLength();
		SR = Ecg.GetSR();
		UmV = Ecg.GetUmV();
		Bits = Ecg.GetBits();

to SAVE file:
		Ecg.SaveFile("EcgFileName", pEcgData, SR, Length, Bits, Umv)
////////////////////////////////////////////*/


// Inlines
inline int Signal::GetLength(int index) const
{
	return _ecgHeaders[index].size;
}

inline int Signal::GetBits(int index) const
{
	return _ecgHeaders[index].bits;
}

inline int Signal::GetUmV(int index) const
{
	return _ecgHeaders[index].umv;
}

inline int Signal::GetLead(int index) const
{
	return _ecgHeaders[index].lead;
}

inline int Signal::GetLeadsNum(int index) const
{
	return int(_ecgSignals.size());
}

inline double Signal::GetSR(int index) const
{
	return _ecgHeaders[index].sr;
}

inline int Signal::GetH(int index) const
{
	return _ecgHeaders[index].hh;
}

inline int Signal::GetM(int index) const
{
	return _ecgHeaders[index].mm;
}

inline int Signal::GetS(int index) const
{
	return _ecgHeaders[index].ss;
}


#endif Signal_h

