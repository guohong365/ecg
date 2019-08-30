#pragma once
#include "SignalReader.h"

class SignalWriter
{
public:
	virtual ~SignalWriter();
	static void writeBinary(const char* filename, Signal * pSignal);
	static void writeText(const char* filename, Signal * pSignal);
protected:
	SignalWriter();
	virtual bool _write(FILE* fp, Signal * pSignal)=0;
};

