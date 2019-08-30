

#include <stdafx.h>
#include "signal.h"


Signal::Signal()
	: _ecgFileName{}
{
}

Signal::~Signal()
{

	for (int i = 0; i < int(_ecgSignals.size()); i++) {
		delete[] _ecgSignals[i];
	}
}

double* Signal::GetData(int index)
{
	if (!_ecgSignals.size())
		return nullptr;
	if (index > int(_ecgSignals.size()) - 1)
		index = int(_ecgSignals.size()) - 1;
	else if (index < 0)
		index = 0;
	return _ecgSignals[index];
}


bool Signal::ToTxt(const wchar_t* name, const double* buffer, int size)
{
	FILE* in = _wfopen(name, L"wt");

	if (in) {
		for (int i = 0; i < size; i++)
			fwprintf(in, L"%lf\n", buffer[i]);

		fclose(in);
		return true;
	}
	return false;
}

void Signal::mSecToTime(int millisecond, int& h, int& m, int& s, int& ms)
{
	ms = millisecond % 1000;
	millisecond /= 1000;

	if (millisecond < 60) {
		h = 0;
		m = 0;
		s = millisecond;                 //sec to final
	}
	else {
		double tmp = double(millisecond % 60) / 60;
		tmp *= 60;
		s = int(tmp);
		millisecond /= 60;

		if (millisecond < 60) {
			h = 0;
			m = millisecond;
		}
		else {
			h = millisecond / 60;
			tmp = double(millisecond % 60) / 60;
			tmp *= 60;
			m = int(tmp);
		}
	}
}

