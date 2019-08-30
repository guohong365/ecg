#pragma once
class Transformer
{
public:
	virtual double* transform(const double* buffer, size_t size)=0;
	virtual ~Transformer();
};

