#pragma once
#include "signal.h"

class SignalReader
{
public:
	virtual ~SignalReader();
	static Signal * read(const char* filename);
protected:
	SignalReader();
	virtual bool _read(FILE* fp, Signal * pSignal) = 0;
};

