#include "ecgtypes.h"
#include "SignalWriter.h"

#ifndef LOBYTE 
#define LOBYTE(a) (a)
#define HIBYTE(a) (a)
#endif

class CustomSignalWriter : public SignalWriter
{
public:
	CustomSignalWriter(){}
protected:
	bool _write(FILE* fp, Signal* pSignal) override
	{
		DATA_HEADER*hdr = pSignal->getHeader();
		double * buffer = pSignal->GetData();
		int up = 0;
		int down = 0;
		switch (hdr->bits)
		{
		case 12:
			up = 2047;
			down = -2048;
			break;
		case 16:
			up = 32767;
			down = -32768;
			break;
		default:
			break;
		}
		for(int i=0; i< int(hdr->size); i++)
		{
			int temp = int(buffer[i] * hdr->umv);
			if (temp > up) temp = up;
			if (temp < down) temp = down;
			fprintf(fp, "%d\n", temp);
		}
		return true;
	}	
};

class TextSignalWriter : public SignalWriter
{
public:
	TextSignalWriter(){}
protected:
	bool _write(FILE* fp, Signal* pSignal) override
	{
		DATA_HEADER * hdr = pSignal->getHeader();		
		int fileSize = 0;
		int tmp;
		switch (hdr->bits) {
		case 12:
			if (hdr->size % 2 != 0)
				fileSize = int(double(hdr->size + 1) * 1.5);
			else
				fileSize = int(double(hdr->size) * 1.5);
			break;
		case 16:
			fileSize = hdr->size * 2;
			break;
		case 0:
		case 32:
			fileSize = hdr->size * 4;
			break;
		default:
			break;		
		}
		char * lpMap = new char[fileSize + sizeof(DATA_HEADER)];

		float* lpf = reinterpret_cast<float *>(lpMap);
		short* lps = reinterpret_cast<short *>(lpMap);
		char* lpc = lpMap;

		memset(lpMap, 0, fileSize);
		
		lpf += sizeof(DATA_HEADER) / sizeof(float);
		lps += sizeof(DATA_HEADER) / sizeof(short);
		lpc += sizeof(DATA_HEADER);

		double * buffer = pSignal->GetData();

		for (unsigned int i = 0; i < hdr->size; i++) {
			switch (hdr->bits) {
			case 12:                                               //212 format   12bit
				tmp = int(buffer[i] * double(hdr->umv));
				if (tmp > 2047) tmp = 2047;
				if (tmp < -2048) tmp = -2048;

				if (i % 2 == 0) {
					lpc[0] = LOBYTE(short(tmp));
					lpc[1] = 0;
					lpc[1] = HIBYTE(short(tmp)) & 0x0f;
				}
				else {
					lpc[2] = LOBYTE(short(tmp));
					lpc[1] |= HIBYTE(short(tmp)) << 4;
					lpc += 3;
				}
				break;

			case 16:                                               //16format
				tmp = int(buffer[i] * double(hdr->umv));
				if (tmp > 32767) tmp = 32767;
				if (tmp < -32768) tmp = -32768;
				*lps++ = short(tmp);
				break;

			case 0:
			case 32:                                               //32bit float
				*lpf++ = float(buffer[i]);
				break;
			default:
				break;
			}
		}
		const bool ret = fwrite(hdr, sizeof(DATA_HEADER), 1, fp) == 1 &&
			fwrite(lpMap, fileSize, 1, fp) == 1;
		delete[] lpMap;
		return ret;
	}
};



SignalWriter::SignalWriter()
{
}


SignalWriter::~SignalWriter()
{
}

void SignalWriter::writeBinary(const char* filename, Signal* pSignal)
{
	wchar_t name[_MAX_PATH] = { 0 };
	mbtowc(name, filename, strlen(filename));
	static CustomSignalWriter customSignalWriter;
	SignalWriter * pWriter = &customSignalWriter;
	FILE* fp = nullptr;
	fopen_s(&fp, filename, "wb");
	pWriter->_write(fp, pSignal);
	fclose(fp);
}

void SignalWriter::writeText(const char* filename, Signal* pSignal)
{
	static TextSignalWriter textSignalWriter;
	SignalWriter *pWriter = &textSignalWriter;
	FILE* fp = nullptr;
	fopen_s(&fp, filename, "wt");
	pWriter->_write(fp, pSignal);
	fclose(fp);
}
