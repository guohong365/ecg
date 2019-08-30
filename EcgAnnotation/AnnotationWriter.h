#pragma once
#include "ecgtypes.h"

class AnnotationWriter
{
public:
	AnnotationWriter();
	~AnnotationWriter();
	bool SaveQTseq(const char *name, int **ann, int annsize, double sr, int length);
	bool SavePQseq(const char *name, int **ann, int annsize, double sr, int length);
	bool SavePPseq(const char *name, int **ann, int annsize, double sr, int length);
	bool SaveRRseq(char* name, ANN_HEADER _hdr, int** ann, int nums, double sr, int length) const;
	bool SaveRRnseq(char* name, ANN_HEADER& _hdr, int** ann, int nums, double sr, int length) const;
};

