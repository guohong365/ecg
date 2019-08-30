#include "stdafx.h"
#include "SignalReader.h"
#include "helper.h"
#include <fstream>
#include <string>

enum SIGNAL_FILE_TYPE
{
	NONE,
	TEXT_SIGNAL,
	MITDB_SIGNAL,
	CUSTOM_SIGNAL
};
static SIGNAL_FILE_TYPE fileType(const char* file) {
	FILE* fp = nullptr;
	fopen_s(&fp, file, "rb");
	if (!fp) return NONE;
	char mark[4];
	if (fread(&mark, 1, 4, fp) == 4)
	{
		if (memcmp(mark, "DATA", 4) == 0) {
			fclose(fp);
			return CUSTOM_SIGNAL;
		}
	}
	fclose(fp);
	fopen_s(&fp, file, "rt");
	if (!fp) return NONE;
	double temp;
	const int ret = fscanf_s(fp, "%lf", &temp);
	if (ret == EOF || ret == 0) {
		fclose(fp);
		return MITDB_SIGNAL;
	}
	fclose(fp);
	return TEXT_SIGNAL;
}

class SignalReaderBase : public SignalReader
{
public:
	virtual ~SignalReaderBase()
	{
		if(_fp) fclose(_fp);
	}
protected:
	SignalReaderBase(FILE* fp)
		:_fp(fp)
	{
		
	}
	FILE* _fp;
};


class TextSignalReader : public SignalReader
{
public:
	TextSignalReader(){}
protected:
	bool _read(FILE* fp, Signal* pSignal) override
	{
		double temp;
		std::vector<double> data;
		for (;;)
		{
			const int res = fscanf_s(fp, "%lf", &temp);
			if (res == EOF || res == 0) break;
			data.push_back(temp);
		}
		if (data.size() < 2) return false;
		DATA_HEADER hdr={0};
		double *buffer = new double[data.size()];
		hdr.size = data.size();
		hdr.bits = 32;
		hdr.bline = 0;
		hdr.umv = 1;
		for(size_t i=0; i< data.size(); i++)
		{
			buffer[i] = data[i];
		}
		pSignal->addSeries(hdr, buffer);
		return true;
	}
};
class MitdbSignalReader : public SignalReader
{
public:
	MitdbSignalReader(){}
	bool _read(FILE* fp, Signal* pSignal) override
	{
		char header[_MAX_PATH] = { 0 };
		strcpy_s(header, _MAX_PATH, _filename.c_str());
		ChangeExtension(header, ".hea");		
		std::vector<DATA_HEADER> hdrs;

		if(parseHeader(hdrs, header))
		{
			const int num = hdrs.size();
			std::vector<double*> data;
			for (int i = 0; i < num; i++) {
				data.push_back(new double[hdrs[i].size]);
			}
			short tmp;
			rewind(fp);
			fseek(fp, 0, SEEK_END);
			const int length = ftell(fp);
			rewind(fp);
			char * buffer = new char[length];
			const int r = fread_s(buffer, length, length, 1, fp);
			if(r!=1)
			{
				clear(data, buffer);
			}
			char* lpc = static_cast<char*>(buffer);
			short* lps = reinterpret_cast<short*>(buffer);
			const int size = hdrs[0].size;
			for (int s = 0; s < size; s++) {
				for (int n = 0; n < num; n++) {
					double *p = data[n];
					DATA_HEADER * pHeader =&hdrs[n];
					switch (pHeader->bits) {
					case 12:                                             //212 format   12bit
						if (fread_s(lpc, 2, 2, 1, fp) != 1) {
							clear(data, buffer);
							
							return false;
						}
						if ((s*num + n) % 2 == 0) {
							tmp = MAKEWORD(lpc[0], lpc[1] & 0x0f);
							if (tmp > 0x7ff)
								tmp |= 0xf000;
							
						}
						else {
							tmp = MAKEWORD(lpc[2], (lpc[1] & 0xf0) >> 4);
							if (tmp > 0x7ff)
								tmp |= 0xf000;
							lpc += 3;
						}
						p[s] = double(tmp - pHeader->bline) / pHeader->umv;
						break;
					case 16:                                      //16format
						p[s] = double(*lps++ - pHeader->bline) / pHeader->umv;
						break;
					default:
						clear(data, buffer);
						return false;
					}
				}
			}
			for(int i=0; i< num; i++)
			{
				pSignal->addSeries(hdrs[i], data[i]);
			}
			delete[] buffer;
			return true;
		}
		return false;
	}

	static void clear(std::vector<double*> a, char * p)
	{
		for(std::vector<double*>::iterator i = a.begin(); i!=a.end(); ++i)
		{
			delete[] * i;
		}
		delete[] p;
	}
	void setFileName(const char* filename) { _filename = filename; }
	bool parseHeader(std::vector<DATA_HEADER> & hdrs,const char* filename) const
	{		
		static char leadStr[18][6] = { "I", "II", "III", "aVR", "aVL", "aVF", "v1", "v2",
							  "v3", "v4", "v5", "v6", "MLI", "MLII", "MLIII", "vX", "vY", "vZ" };
		try {
			std::ifstream stream(filename, ios_base::in);
			std::string line;
			std::getline(stream , line);
			if (!stream.good()) return false;
			char str[10][256];
			int hh = 0, mm = 0, ss = 0;
			int res=sscanf_s(line.c_str(), "%s %s %s %s %s", str[1], 256, str[2], 256, str[3], 256, str[4], 256, str[5], 256);
			if(res < 4) return false;
			const int leads = atoi(str[2]);
			const float frequency = float(atof(str[3]));
			const int size = atoi(str[4]);
			if(res==5 && strlen(str[5])==8)
			{
				hh = atoi(str[5]);
				mm = atoi(str[5] + 3);
				ss = atoi(str[5] + 6);
			}
			for (int i = 0; i < leads; i++) {
				std::getline(stream, line);
				if(!stream.good()) return false;
				memset(str[9], 0, 256);

				res = sscanf_s(line.c_str(), "%s %s %s %s %s %s %s %s %s", str[1], 256, str[2], 256, str[3], 256, str[4], 256, str[5], 256, str[6], 256, str[7], 256, str[8], 256, str[9], 256);
				if (res < 5) return false;

				const int bits = atoi(str[2]);
				const int umv = atoi(str[3]);
				const int baseline = atoi(str[5]);

				DATA_HEADER hdr ={0};
				memset(&hdr, 0, sizeof(DATA_HEADER));
				hdr.sr = frequency;
				hdr.bits = (bits == 212) ? 12 : bits;
				hdr.umv = (umv == 0) ? 200 : umv;
				hdr.bline = baseline;
				hdr.size = size;
				hdr.hh = hh;
				hdr.mm = mm;
				hdr.ss = ss;
				for (int l = 0; l < 18; l++) {
					if (!_stricmp(leadStr[l], str[9])) {
						hdr.lead = l + 1;
						break;
					}
				}
				hdrs.push_back(hdr);
			}
			return true;
		}
		catch (...)
		{
			return false;
		}

	}
private:
	std::string _filename;
};

class CustomSignalReader : public SignalReader
{
protected:
	bool _read(FILE* fp, Signal * pSignal) override
	{
		rewind(fp);
		DATA_HEADER hdr={0};
		if (fread_s(&hdr, sizeof(DATA_HEADER), sizeof(DATA_HEADER), 1, fp) != 1) return false;
		double *data = new double[hdr.size];
		if(fread_s(data, hdr.size, hdr.size, 1, fp)!=1)
		{
			delete[] data;
			return false;
		}
		pSignal->addSeries(hdr, data);
		return true;
	}
};

SignalReader::SignalReader()
{
}


SignalReader::~SignalReader()
{
}

Signal* SignalReader::read(const char* filename)
{
	static TextSignalReader textSignalReader;
	static MitdbSignalReader mitdbSignalReader;
	static CustomSignalReader customSignalReader;
	const SIGNAL_FILE_TYPE type = fileType(filename);
	FILE* fp=nullptr;
	Signal * pSignal=nullptr;
	SignalReader * pReader = nullptr;
	switch (type)
	{
	case TEXT_SIGNAL:
		fopen_s(&fp, filename, "rt");
		if (fp) 
		{
			pSignal = new Signal();
			pReader = &textSignalReader;
		}
		break;
	case MITDB_SIGNAL:
		fopen_s(&fp, filename, "rb");
		if(fp)
		{
			pSignal = new Signal();
			mitdbSignalReader.setFileName(filename);
			pReader =&mitdbSignalReader;
		}
		break;
	case CUSTOM_SIGNAL:
		fopen_s(&fp, filename, "rb");
		if(fp)
		{
			pSignal = new Signal();
			pReader = &customSignalReader;
		}
		break;
	default:
		return nullptr;
	}
	if(!fp || !pSignal || !pReader)
	{
		if (fp) fclose(fp);
		delete pSignal;
		return nullptr;
	}
	pSignal->setFileName(filename);
	const bool ret = pReader->_read(fp, pSignal);
	fclose(fp);
	return ret ? pSignal : nullptr;
}
