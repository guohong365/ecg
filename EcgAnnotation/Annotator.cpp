#include <math.h>
#include "Annotator.h"
#include "helper.h"
#include "ContinuousWaveletTransform.h"
#include "FastWaveletTransform.h"
#include "Denoise.h"
#include "signal.h"

std::string Annotator::_filterPath = "filter";

int Annotator::getQRSNumber() const
{
	return _qrsNum;
}

int Annotator::getAnnotationSize() const
{
	return _annNum;
}

int** Annotator::getAnnotation() const
{
	return _ann;
}

int** Annotator::getQRSAnnotation() const
{
	return _qrsAnn;
}

inline char** Annotator::getAuxData() const
{
	return _aux;
}

PANN_HEADER Annotator::getAnnotationHeader()
{
	return &_hdr;
}

bool Annotator::_isNoise(const double *data, const int window)
{
	for (int i = 0; i < window; i++)
		if (abs(data[i]) > FLOAT_EQ_ERR) return true;

	return false;
}

void Annotator::_find_RS(const double *data, const int size, int &R, int &S, const double err) //find RS or QR
{
	double min, max;
	MinMax(data, size, min, max);

	R = -1;
	S = -1;
	if (!(max < 0.0 ||
		abs(max - data[0]) < FLOAT_EQ_ERR ||
		abs(max - data[size - 1]) < FLOAT_EQ_ERR ||
		max < err)) { //(fabs(max-data[0])<err && fabs(max-data[size-1])<err) ))
		for (int i = 1; i < size - 1; i++)
			if (abs(data[i] - max) < FLOAT_EQ_ERR) {
				R = i;
				break;
			}
	}
	if (!(min > 0.0 ||
		abs(min - data[0]) < FLOAT_EQ_ERR ||
		abs(min - data[size - 1]) < FLOAT_EQ_ERR ||
		-min < err)) { //(fabs(min-data[0])<err && fabs(min-data[size-1])<err) ))
		for (int i = 1; i < size - 1; i++)
			if (abs(data[i] - min) < FLOAT_EQ_ERR) {
				S = i;
				break;
			}
	}
}

int Annotator::_findTMax(const double *data, const int size) const  //find T max/min peak position
{
	double min, max;
	MinMax(data, size, min, max);

	int tMin = -1;
	int tMax = -1;
	for (int i = 0; i < size; i++) {
		if (abs(data[i] - max) < FLOAT_EQ_ERR) {
			tMax = i;
			break;
		}
	}
	for (int i = 0; i < size; i++) {
		if (abs(data[i] - min) < FLOAT_EQ_ERR) {
			tMin = i;
			break;
		}
	}

	//max closest to the center
	if (tMin == -1 || tMax == -1) //??no max min found
		return -1;
	if (abs(tMax - (size / 2)) < abs(tMin - (size / 2)))
		return tMax;
	return tMin;
}

inline int Annotator::_find_r(const double *data, int size, double err) const //find small r in PQ-S
{
	double min, max;
	MinMax(data, size, min, max);

	if (max < 0.0 ||
		abs(max - data[0]) < FLOAT_EQ_ERR ||
		abs(max - data[size - 1]) < FLOAT_EQ_ERR ||
		fabs(max - data[0]) < err) return -1;
	const double tmp = max;

	for (int i = 1; i < size - 1; i++) {
		if (abs(data[i] - tmp) < FLOAT_EQ_ERR)
			return i;
	}
	return -1;
}
inline int Annotator::_find_q(const double *data, int size, double err) const //find small q in PQ-R
{
	double min, max;
	MinMax(data, size, min, max);

	if (min > 0.0 ||
		abs(min - data[0]) < FLOAT_EQ_ERR ||
		abs(min - data[size - 1]) < FLOAT_EQ_ERR ||
		fabs(min - data[0]) < err) return -1;
	const double tmp = min;

	for (int i = 1; i < size - 1; i++) {
		if (abs(data[i] - tmp) < FLOAT_EQ_ERR)
			return i;
	}
	return -1;
}
inline int Annotator::_find_s(const double *data, int size, double err) const  //find small s in R-Jpnt
{
	double tmp, min, max;
	MinMax(data, size, min, max);

	if (min > 0.0 ||
		abs(min - data[0]) < FLOAT_EQ_ERR ||
		abs(min - data[size - 1]) < FLOAT_EQ_ERR ||
		fabs(min - data[size - 1]) < err) return -1;
	else tmp = min;

	for (int i = 1; i < size - 1; i++) {
		if (abs(data[i] - tmp) < FLOAT_EQ_ERR)
			return i;
	}
	return -1;
}

Annotator::Annotator(PANN_HEADER p) : _ann(nullptr), _annNum(0), _qrsAnn(nullptr),
_qrsNum(0), _auxNum(0), _aux(nullptr)
{
	if (p) {
		memcpy(&_hdr, p, sizeof(ANN_HEADER));
	}
	else { //defaults
		_hdr.minbpm = 40;       //min bpm
		_hdr.maxbpm = 200;      //max bpm
		_hdr.minQRS = 0.04;     //min QRS duration
		_hdr.maxQRS = 0.2;      //max QRS duration
		_hdr.qrsFreq = 13.0;    //QRS filtration frequency
		_hdr.ampQRS = INTER1;   //inter1 filter
		_hdr.minUmV = 0.2;      //min UmV of R,S peaks
		_hdr.minPQ = 0.07;      //min PQ duration
		_hdr.maxPQ = 0.20;      //max PQ duration
		_hdr.minQT = 0.21;      //min QT duration
		_hdr.maxQT = 0.48;      //max QT duration
		_hdr.pFreq = 9.0;       //cwt Hz for P wave
		_hdr.tFreq = 3.0;       //cwt Hz for T wave
		_hdr.biTwave = NORMAL;  //normal wave                
	}
}

Annotator::~Annotator()
{
	if (_ann) {
		for (int i = 0; i < _annNum; i++)
			delete[] _ann[i];
		delete[] _ann;
	}
	if (_qrsAnn) {
		for (int i = 0; i < _qrsNum; i++)
			delete[] _qrsAnn[i];
		delete[] _qrsAnn;
	}
	if (_aux) {
		for (int i = 0; i < _auxNum; i++)
			delete[] _aux[i];
		delete[] _aux;
	}
}

//-----------------------------------------------------------------------------
// 10Hz cwt trans of signal
// spectrum in cwt class
// create qrsANN array   with qrsNum records  num of heart beats = qrsNum/2
//
int** Annotator::getQRS(const double *data, int size, double sampleRate)
{

	double *pdata = new double[size_t(size) * sizeof(double)];
	for (int i = 0; i < size; i++)
		pdata[i] = data[i];

	if (_filter30Hz(pdata, size, sampleRate) == false) { //pdata filed with filterd signal
		delete[] pdata;
		return nullptr;
	}


	double eCycle = (60.0 / double(_hdr.maxbpm)) - _hdr.maxQRS;  //secs
	if (int(eCycle*sampleRate) <= 0) {
		eCycle = 0.1;
		_hdr.maxbpm = int(60.0 / (_hdr.maxQRS + eCycle));
	}
	//////////////////////////////////////////////////////////////////////////////

	int lqNum = 0;
	std::vector <int> qrs;    //clean QRS detected
	int add = 0;

	while (abs(pdata[add]) > FLOAT_EQ_ERR) add += int(0.1 * sampleRate);                  //skip QRS in begining
	while (abs(pdata[add]) < FLOAT_EQ_ERR) add++;                            //get  1st QRS

	qrs.push_back(add - 1);
	/////////////////////////////////////////////////////////////////////////////
	for (int m = add; m < size; m++) {                  //MAX 200bpm  [0,3 sec  min cario size]
		m += int(_hdr.maxQRS * sampleRate);                     //smpl + 0,20sec    [0,20 max QRS length]


		if (m >= size) m = size - 1;
		add = 0;

		//noise checking///////////////////////////////////////
		if (m + int(eCycle*sampleRate) >= size) { //near end of signal
			qrs.pop_back();
			break;
		}
		if (_isNoise(&pdata[m], int(eCycle*sampleRate))) {  //smp(0.10sec)+0,20sec in noise
			if (lqNum != int(qrs.size()) - 1)
				_ma.push_back(qrs[qrs.size() - 1]);     //push MA noise location

			qrs.pop_back();
			lqNum = int(qrs.size());

			//Find for next possible QRS start
			while (_isNoise(&pdata[m], int(eCycle*sampleRate))) {
				m += int(eCycle * sampleRate);
				if (m >= size - int(eCycle*sampleRate)) break;   //end of signal
			}

			if (m >= size - int(eCycle*sampleRate)) break;
			while (pdata[m] > FLOAT_EQ_ERR) {
				m++;
				if (m >= size) break;
			}
			if (m >= size) break;
			qrs.push_back(m - 1);
			continue;
		}
		////////////////////////////////////////////////////////


		while (pdata[m - add] == 0.0) add++;               //Find back for QRS end


		if ((m - add + 1) - qrs[qrs.size() - 1] > _hdr.minQRS*sampleRate)  //QRS size > 0.04 sec
			qrs.push_back(m - add + 2); //QRS end
		else
			qrs.pop_back();


		m += int(eCycle * sampleRate);                                //smpl + [0,20+0,10]sec    200bpm MAX
		if (size - m < int(sampleRate / 2)) break;



		while (abs(pdata[m]) < FLOAT_EQ_ERR && (size - m) >= int(sampleRate / 2))   //Find nearest QRS
			m++;

		if (size - m < int(sampleRate / 2)) break;  //end of data

		qrs.push_back(m - 1);  //QRS begin
	}
	/////////////////////////////////////////////////////////////////////////////
	delete[] pdata;



	_qrsNum = qrs.size() / 2;

	if (_qrsNum > 0)                              //         46: ?
	{                                           //          1: N    -1: nodata in **AUX
		_qrsAnn = new int*[size_t(2 * _qrsNum)];                    // [samps] [type] [?aux data]
		for (int i = 0; i < 2 * _qrsNum; i++)
			_qrsAnn[i] = new int[3];

		for (int i = 0; i < 2 * _qrsNum; i++) {
			_qrsAnn[i][0] = qrs[i];                                     //samp

			if (i % 2 == 0)
				_qrsAnn[i][1] = 1;                                        //type N
			else {
				_qrsAnn[i][1] = 40;                                      //type QRS)
				//if( (qrsANN[i][0]-qrsANN[i-1][0]) >= int(sr*0.12) || (qrsANN[i][0]-qrsANN[i-1][0]) <= int(sr*0.03) )
				// qrsANN[i-1][1] = 46;                                  //N or ? beat  (0.03?-0.12secs)
			}

			_qrsAnn[i][2] = -1;                                         //no aux data
		}

		return _qrsAnn;
	}
	return nullptr;
}

bool Annotator::_filter30Hz(double *data, int size, double sampleRate) const
{
	ContinuousWaveletTransform cwt;
	///////////CWT 10Hz transform//////////////////////////////////////////
	cwt.init(size, ContinuousWaveletTransform::GAUS1, 0, sampleRate);        //gauss1 wavelet 6-index
	double *pSpec = cwt.Transform(data, _hdr.qrsFreq);      //10-13 Hz transform?
	for (int i = 0; i < size; i++)
		data[i] = pSpec[i];
	cwt.close();

	//debug
	//ToTxt(L"10hz.txt",data,size);

	const char* flt;
	switch (_hdr.ampQRS) {
	default:
	case INTER1:
		flt = "inter1.flt";
		break;

	case BIOR13:
		for (int i = 0; i < size; i++)  //ridges
			data[i] *= (fabs(data[i]) / 2.0);
		flt = "bior13.flt";
		break;
	}
	FastWaveletTransform fwt;
	////////////FWT 0-30Hz removal//////////////////////////////////////////
	if (fwt.init(data, size, flt) == false)
		return false;

	const int J = int(ceil(log2(sampleRate / 23.0)) - 2);
	//trans///////////////////////////////////////////////////
	fwt.transform(J);

	int *jNumbers = fwt.GetJNumbers(J, size);
	int hiNum, loNum;
	fwt.hiLoNumbers(J, size, hiNum, loNum);
	double *lo = fwt.GetFwtSpectrum();
	double *hi = fwt.GetFwtSpectrum() + (size - hiNum);


	for (int j = J; j > 0; j--) {
		const int window = int((2.0 * sampleRate) / pow(2.0, double(j)));    //2.0sec interval

		denoise(hi, jNumbers[J - j], window, 0, false);  //hard,MINIMAX denoise [30-...Hz]
		hi += jNumbers[J - j];
	}
	for (int i = 0; i < loNum; i++)               //remove [0-30Hz]
		lo[i] = 0.0;

	//synth/////////////////////////////
	fwt.synthesis(J);

	for (int i = 0; i < fwt.getLoBandSize(); i++)
		data[i] = lo[i];
	for (int i = size - (size - fwt.getLoBandSize()); i < size; i++)
		data[i] = 0.0;

	fwt.close();

	//debug
	//ToTxt(L"10hz(intr1).txt",data,size);
	return true;
}


////////////////////////////////////////////////////////////////////////////////
// Find ectopic beats in HRV data
void Annotator::getEctopia(int **annotations, const int qrsNum, const double sampleRate) const
{
	if (qrsNum < 3)
		return;

	std::vector<double> RRs;
	for (int n = 0; n < qrsNum - 1; n++)
		RRs.push_back(double(annotations[n * 2 + 2][0] - annotations[n * 2][0]) / sampleRate); //qrsNum-1 rr's
	RRs.push_back(RRs[RRs.size() - 1]);

	//  [RR1  RR2  RR3]   RR2 beat classification
	double rr1, rr2, rr3;
	for (int n = -2; n < int(RRs.size()) - 2; n++) {
		if (n == -2) {
			rr1 = RRs[1];  //2
			rr2 = RRs[0];  //1
			rr3 = RRs[0];  //0
		}
		else if (n == -1) {
			rr1 = RRs[1];
			rr2 = RRs[0];
			rr3 = RRs[1];
		}
		else {
			rr1 = RRs[n];
			rr2 = RRs[n + 1];
			rr3 = RRs[n + 2];
		}

		if (60.0 / rr1 < _hdr.minbpm || 60.0 / rr1 > _hdr.maxbpm) //if RR's within 40-200bpm
			continue;
		if (60.0 / rr2 < _hdr.minbpm || 60.0 / rr2 > _hdr.maxbpm) //if RR's within 40-200bpm
			continue;
		if (60.0 / rr3 < _hdr.minbpm || 60.0 / rr3 > _hdr.maxbpm) //if RR's within 40-200bpm
			continue;

		if (1.15*rr2 < rr1 && 1.15*rr2 < rr3) {
			annotations[n * 2 + 4][1] = 46;
			continue;
		}
		if (fabs(rr1 - rr2) < 0.3 && rr1 < 0.8 && rr2 < 0.8 && rr3 > 2.4*(rr1 + rr2)) {
			annotations[n * 2 + 4][1] = 46;
			continue;
		}
		if (fabs(rr1 - rr2) < 0.3 && rr1 < 0.8 && rr2 < 0.8 && rr3 > 2.4*(rr2 + rr3)) {
			annotations[n * 2 + 4][1] = 46;
			continue;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
//out united annotation with QRS PT////////////////////////////////////////////
// **ann [PQ,JP] pairs
int** Annotator::getPTU(const double *data, const int length, const double sampleRate, int **annotations, const int qrsNum)
{
	int size, annPos;
	int T1 = -1;
	int T = -1;
	int T2 = -1;
	int tWaves = 0;
	int P1 = -1;
	int P = -1;
	int P2 = -1;
	int pWaves = 0;
	ContinuousWaveletTransform cwt;
	std::vector <int> pWave;
	std::vector <int> tWave;                             //Twave [ ( , T , ) ]
	double min, max;                           //min,max for gaussian1 wave, center is zero crossing

	bool sign;

	const int add = 0;//int(sr*0.04);  //prevent imprecise QRS end detection

	int maNum = 0;
	for (int n = 0; n < qrsNum - 1; n++) {
		annPos = annotations[n * 2 + 1][0];                //i
		size = annotations[n * 2 + 2][0] - annotations[n * 2 + 1][0];  //i   size of  (QRS) <----> (QRS)

		bool maNs = false;
		for (int i = maNum; i < int(_ma.size()); i++) {
			if (_ma[i] > annotations[n * 2 + 1][0] && _ma[i] < annotations[n * 2 + 2][0]) {
				pWave.push_back(0);
				pWave.push_back(0);
				pWave.push_back(0);
				tWave.push_back(0);
				tWave.push_back(0);
				tWave.push_back(0);
				maNum++;
				maNs = true;
				break;
			}
		}
		if (maNs) continue;


		const double rr = double(annotations[n * 2 + 2][0] - annotations[n * 2][0]) / sampleRate;
		if (60.0 / rr < _hdr.minbpm || 60.0 / rr > _hdr.maxbpm - 20) { //check if normal RR interval (40bpm - 190bpm)
			pWave.push_back(0);
			pWave.push_back(0);
			pWave.push_back(0);
			tWave.push_back(0);
			tWave.push_back(0);
			tWave.push_back(0);
			continue;
		}


		///////////////search for TWAVE///////////////////////////////////////////////////////////

		if (sampleRate*_hdr.maxQT - (annotations[n * 2 + 1][0] - annotations[n * 2 + 0][0]) > size - add)
			size = size - add;
		else
			size = int(sampleRate * _hdr.maxQT - (annotations[n * 2 + 1][0] - annotations[n * 2 + 0][0]) - add);


		//double avg = Mean(data+annPos+add,size);         //avrg extension on boundaries
		//double lvl,rvl;
		//lvl = data[annPos+add];
		//rvl = data[annPos+add+size-1];
		if (_hdr.biTwave == BIPHASE)
			cwt.init(size, ContinuousWaveletTransform::GAUS, 0, sampleRate);                 //5-Gauss wlet
		else
			cwt.init(size, ContinuousWaveletTransform::GAUS1, 0, sampleRate);                //6-Gauss1 wlet

		double* pSpec = cwt.Transform(data + annPos + add, _hdr.tFreq);//,false,lvl,rvl);   //3Hz transform  pspec = size-2*add

		//cwt.ToTxt(L"debugS.txt",data+annPos+add,size);    //T wave
		//cwt.ToTxt(L"debugC.txt",pspec,size);               //T wave spectrum

		MinMax(pSpec, size, min, max);
		for (int i = 0; i < size; i++) {
			if (abs(pSpec[i] - min) < FLOAT_EQ_ERR) T1 = i + annPos + add;
			if (abs(pSpec[i] - max) < FLOAT_EQ_ERR) T2 = i + annPos + add;
		}
		if (T1 > T2)std::swap(T1, T2);

		//additional constraints on [T1 T T2] duration, symmetry, QT interval
		bool t_wave = false;
		if ((pSpec[T1 - annPos - add] < 0 && pSpec[T2 - annPos - add] > 0) || (pSpec[T1 - annPos - add] > 0 && pSpec[T2 - annPos - add] < 0))
			t_wave = true;
		if (t_wave) {
			if (double(T2 - T1) >= 0.09*sampleRate) { // && (double)(T2-T1)<=0.24*sr)   //check for T wave duration
				if (double(T2 - annotations[n * 2 + 0][0]) >= _hdr.minQT*sampleRate && double(T2 - annotations[n * 2 + 0][0]) <= _hdr.maxQT*sampleRate)
					t_wave = true;
				else
					t_wave = false;
			}
			else
				t_wave = false;
		}

		if (t_wave) {
			if (pSpec[T1 - annPos - add] > 0) sign = true;
			else sign = false;

			for (int i = T1 - annPos - add; i < T2 - annPos - add; i++) {
				if (sign) {
					if (pSpec[i] > 0) continue;
				}
				else {
					if (pSpec[i] < 0) continue;
				}

				T = i + annPos + add;
				break;
			}

			//check for T wave symetry//////////////////////////
			double ratio;
			if (T2 - T < T - T1) ratio = double(T2 - T) / double(T - T1);
			else ratio = double(T - T1) / double(T2 - T);
			////////////////////////////////////////////////////

			if (ratio < 0.4) { //not a T wave
				tWave.push_back(0);
				tWave.push_back(0);
				tWave.push_back(0);
				t_wave = false;
			}
			else {
				//adjust center of T wave
				//smooth it with gaussian, Find max ?
				//cwt.ToTxt(L"debugS.txt",data+annPos+add,size);
				int T_cntr = _findTMax(data + T1, T2 - T1);
				if (T_cntr != -1) {
					T_cntr += T1;
					if (abs((T_cntr - T1) - ((T2 - T1) / 2)) < abs((T - T1) - ((T2 - T1) / 2)))  //which is close to center 0-cross or T max
						T = T_cntr;
				}

				tWaves++;
				tWave.push_back(T1);
				tWave.push_back(T);
				tWave.push_back(T2);
			}
		}
		else { //no T wave???    empty (QRS) <-------> (QRS)
			tWave.push_back(0);
			tWave.push_back(0);
			tWave.push_back(0);
		}
		T = -1;
		///////////////search for TWAVE///////////////////////////////////////////////////////////
		cwt.close();





		///////////////search for PWAVE///////////////////////////////////////////////////////////

		size = annotations[n * 2 + 2][0] - annotations[n * 2 + 1][0];  //n   size of  (QRS) <----> (QRS)

		if (sampleRate*_hdr.maxPQ < size)
			size = int(sampleRate * _hdr.maxPQ);

		if (t_wave) {
			if (T2 > annotations[n * 2 + 2][0] - size - int(0.04*sampleRate))   // pwave wnd far from Twave at least on 0.02sec
				size -= T2 - (annotations[n * 2 + 2][0] - size - int(0.04 * sampleRate));
		}
		const int size23 = (annotations[n * 2 + 2][0] - annotations[n * 2 + 1][0]) - size;

		//size -= 0.02*sr;   //impresize QRS begin detection
		if (size <= 0.03*sampleRate) {
			pWave.push_back(0);
			pWave.push_back(0);
			pWave.push_back(0);
			continue;
		}


		//avg = Mean(data+annPos+size23,size);                     //avrg extension on boundaries
		//lvl = data[annPos+size23];
		//rvl = data[annPos+size23+size-1];
		cwt.init(size, ContinuousWaveletTransform::GAUS1, 0, sampleRate);                                        //6-Gauss1 wlet
		pSpec = cwt.Transform(data + annPos + size23, _hdr.pFreq);//,false,lvl,rvl);    //9Hz transform  pspec = size-2/3size

		//cwt.ToTxt(L"debugS.txt",data+annPos+size23,size);
		//cwt.ToTxt(L"debugC.txt",pspec,size);

		MinMax(pSpec, size, min, max);
		for (int i = 0; i < size; i++) {
			if (abs(pSpec[i] - min) < FLOAT_EQ_ERR) P1 = i + annPos + size23;
			if (abs(pSpec[i] - max) < FLOAT_EQ_ERR) P2 = i + annPos + size23;
		}
		if (P1 > P2) std::swap(P1, P2);

		//additional constraints on [P1 P P2] duration, symmetry, PQ interval
		bool p_wave = false;
		if ((pSpec[P1 - annPos - size23] < 0 && pSpec[P2 - annPos - size23] > 0) || (pSpec[P1 - annPos - size23] > 0 && pSpec[P2 - annPos - size23] < 0))
			p_wave = true;
		if (p_wave) {
			if (double(P2 - P1) >= 0.03*sampleRate && double(P2 - P1) <= 0.15*sampleRate) { //check for P wave duration  9Hz0.03 5Hz0.05
				if (double(annotations[n * 2 + 2][0] - P1) >= _hdr.minPQ*sampleRate && double(annotations[n * 2 + 2][0] - P1) <= _hdr.maxPQ*sampleRate)
					p_wave = true;
				else
					p_wave = false;
			}
			else
				p_wave = false;
		}

		if (p_wave) {
			if (pSpec[P1 - annPos - size23] > 0) sign = true;
			else sign = false;

			for (int i = P1 - annPos - size23; i < P2 - annPos - size23; i++) {
				if (sign) {
					if (pSpec[i] > 0) continue;
				}
				else {
					if (pSpec[i] < 0) continue;
				}

				P = i + annPos + size23;
				break;
			}

			//check for T wave symetry//////////////////////////
			double ratio;
			if (P2 - P < P - P1) ratio = double(P2 - P) / double(P - P1);
			else ratio = double(P - P1) / double(P2 - P);
			////////////////////////////////////////////////////

			if (ratio < 0.4) { //not a P wave
				pWave.push_back(0);
				pWave.push_back(0);
				pWave.push_back(0);
			}
			else {
				pWaves++;
				pWave.push_back(P1);
				pWave.push_back(P);
				pWave.push_back(P2);
			}
		}
		else {
			pWave.push_back(0);
			pWave.push_back(0);
			pWave.push_back(0);
		}
		P1 = -1;
		P = -1;
		P2 = -1;
		///////////////search for PWAVE///////////////////////////////////////////////////////////
		cwt.close();

	}

	/////////////////get q,r,s peaks//////////////////////////////////////////////////////////
	// on a denoised signal

	int peaksNum = 0;
	int R;
	int S;
	std::vector <int> qrsPeaks;          //q,r,s peaks [ q , r , s ]
									//            [ 0,  R , 0 ]  zero if not defined
	std::vector <char> qrsTypes;         //[q,r,s] or [_,R,s], etc...


	for (int n = 0; n < qrsNum; n++) { //fill with zeros
		for (int i = 0; i < 3; i++) {
			qrsPeaks.push_back(0);
			qrsTypes.push_back(' ');
		}
	}


	double *buff = static_cast<double *>(malloc(length * sizeof(double)));
	for (int i = 0; i < length; i++)
		buff[i] = data[i];

	Denoise denoise;
	denoise.init(buff, length, sampleRate);
	if (denoise.LFDenoise()) {
		for (int n = 0; n < qrsNum; n++) {
			annPos = annotations[n * 2][0];   //PQ
			size = annotations[n * 2 + 1][0] - annotations[n * 2][0] + 1; //PQ-Jpnt, including Jpnt

			double* pBuff = &buff[annPos];

			int Q = -1;
			_find_RS(pBuff, size, R, S, _hdr.minUmV);
			if (R != -1) R += annPos;
			if (S != -1) S += annPos;

			if (R != -1 && S != -1) {  // Rpeak > 0mV Speak < 0mV
				if (S < R) { //check for S
					if (buff[R] > -buff[S]) {
						Q = S;
						S = -1;

						size = annotations[n * 2 + 1][0] - R + 1;  //including Jpnt
						pBuff = &buff[R];
						S = _find_s(pBuff, size, 0.05);
						if (S != -1) S += R;
					}
				}
				else {   //check for Q
					size = R - annPos + 1;  //including R peak
					Q = _find_q(pBuff, size, 0.05);
					if (Q != -1) Q += annPos;
				}
			}
			//else if only S
			else if (S != -1) { //Find small r if only S detected  in rS large T lead
				size = S - annPos + 1; //including S peak
				pBuff = &buff[annPos];
				R = _find_r(pBuff, size, 0.05);
				if (R != -1) R += annPos;
			}
			//else if only R
			else if (R != -1) { //only R Find small q,s
				size = R - annPos + 1; //including R peak
				Q = _find_q(pBuff, size, 0.05);
				if (Q != -1) Q += annPos;
				size = annotations[n * 2 + 1][0] - R + 1;  //including Jpnt
				pBuff = &buff[R];
				S = _find_s(pBuff, size, 0.05);
				if (S != -1) S += R;
			}


			//put peaks to qrsPeaks vector
			if (R == -1 && S == -1) { //no peaks
				annotations[n * 2][1] = 16;   //ARTEFACT
				//remove P,T
				if (n != 0) {
					if (pWave[3 * (n - 1)]) {
						pWaves--;
						pWave[3 * (n - 1)] = 0;
						pWave[3 * (n - 1) + 1] = 0;
						pWave[3 * (n - 1) + 2] = 0;
					}
				}
				if (n != qrsNum - 1) {
					if (tWave[3 * n]) {
						tWaves--;
						tWave[3 * n] = 0;
						tWave[3 * n + 1] = 0;
						tWave[3 * n + 2] = 0;
					}
				}
			}
			if (Q != -1) {
				peaksNum++;
				qrsPeaks[n * 3] = Q;
				if (fabs(buff[Q]) > 0.5)
					qrsTypes[n * 3] = 17; //'Q';
				else
					qrsTypes[n * 3] = 15; //'q';
			}
			if (R != -1) {
				peaksNum++;
				qrsPeaks[n * 3 + 1] = R;
				if (fabs(buff[R]) > 0.5)
					qrsTypes[n * 3 + 1] = 48; //'R';
				else
					qrsTypes[n * 3 + 1] = 47; //'r';
			}
			if (S != -1) {
				peaksNum++;
				qrsPeaks[n * 3 + 2] = S;
				if (fabs(buff[S]) > 0.5)
					qrsTypes[n * 3 + 2] = 50; //'S';
				else
					qrsTypes[n * 3 + 2] = 49; //'s';
			}
		}
	}

	free(buff);
	/////////////////get q,r,s peaks//////////////////////////////////////////////////////////






	///////////////////////// complete annotation array///////////////////////////////////////
	maNum = 0;

	//Pwave vec size = Twave vec size
	_annNum = pWaves * 3 + qrsNum * 2 + peaksNum + tWaves * 3 + int(_ma.size());   //P1 P P2 [QRS] T1 T T2  noise annotation
	if (_annNum > qrsNum)                        //42-(p 43-p) 24-Pwave
	{                                           //44-(t 45-t) 27-Twave
		_ann = new int*[_annNum];                    // [samps] [type] [?aux data]
		for (int i = 0; i < _annNum; i++)
			_ann[i] = new int[3];

		int index = 0; //index to ANN
		int qIndex = 0;  //index to qrsANN

		for (int i = 0; i < int(tWave.size()); i += 3) {   //Twave=Pwaves=qrsPeaks size
				//QRS complex
			_ann[index][0] = annotations[qIndex][0];           //(QRS
			_ann[index][1] = annotations[qIndex][1];           //
			_ann[index++][2] = annotations[qIndex++][2];       //aux
			if (qrsPeaks[i]) { //q
				_ann[index][0] = qrsPeaks[i];
				_ann[index][1] = qrsTypes[i];
				_ann[index++][2] = -1;              //no aux
			}
			if (qrsPeaks[i + 1]) { //r
				_ann[index][0] = qrsPeaks[i + 1];
				_ann[index][1] = qrsTypes[i + 1];
				_ann[index++][2] = -1;              //no aux
			}
			if (qrsPeaks[i + 2]) { //s
				_ann[index][0] = qrsPeaks[i + 2];
				_ann[index][1] = qrsTypes[i + 2];
				_ann[index++][2] = -1;              //no aux
			}
			_ann[index][0] = annotations[qIndex][0];           //QRS)
			_ann[index][1] = annotations[qIndex][1];           //
			_ann[index++][2] = annotations[qIndex++][2];       //aux

			//T wave
			if (tWave[i]) {
				_ann[index][0] = tWave[i];                //(t
				_ann[index][1] = 44;
				_ann[index++][2] = -1;                    //no aux
				_ann[index][0] = tWave[i + 1];              //T
				_ann[index][1] = 27;
				_ann[index++][2] = -1;                    //no aux
				_ann[index][0] = tWave[i + 2];              //t)
				_ann[index][1] = 45;
				_ann[index++][2] = -1;                    //no aux
			}
			//P wave
			if (pWave[i]) {
				_ann[index][0] = pWave[i];                //(t
				_ann[index][1] = 42;
				_ann[index++][2] = -1;                    //no aux
				_ann[index][0] = pWave[i + 1];              //T
				_ann[index][1] = 24;
				_ann[index++][2] = -1;                    //no aux
				_ann[index][0] = pWave[i + 2];              //t)
				_ann[index][1] = 43;
				_ann[index++][2] = -1;                    //no aux
			}

			if (!tWave[i] && !pWave[i]) {          //check for MA noise
				for (int m = maNum; m < int(_ma.size()); m++) {
					if (_ma[m] > annotations[qIndex - 1][0] && _ma[m] < annotations[qIndex][0]) {
						_ann[index][0] = _ma[m];      //Noise
						_ann[index][1] = 14;
						_ann[index++][2] = -1;       //no aux
						maNum++;
						break;
					}
				}
			}
		}

		//last QRS complex
		const int ii = 3 * (qrsNum - 1);
		_ann[index][0] = annotations[qIndex][0];           //(QRS
		_ann[index][1] = annotations[qIndex][1];           //
		_ann[index++][2] = annotations[qIndex++][2];       //aux
		if (qrsPeaks[ii]) { //q
			_ann[index][0] = qrsPeaks[ii];
			_ann[index][1] = qrsTypes[ii];
			_ann[index++][2] = -1;              //no aux
		}
		if (qrsPeaks[ii + 1]) { //r
			_ann[index][0] = qrsPeaks[ii + 1];
			_ann[index][1] = qrsTypes[ii + 1];
			_ann[index++][2] = -1;              //no aux
		}
		if (qrsPeaks[ii + 2]) { //s
			_ann[index][0] = qrsPeaks[ii + 2];
			_ann[index][1] = qrsTypes[ii + 2];
			_ann[index++][2] = -1;              //no aux
		}
		_ann[index][0] = annotations[qIndex][0];           //QRS)
		_ann[index][1] = annotations[qIndex][1];           //
		_ann[index++][2] = annotations[qIndex++][2];       //aux

		//check if noise after last qrs
		if (maNum < int(_ma.size())) {
			if (_ma[maNum] > annotations[qIndex - 1][0]) {
				_ann[index][0] = _ma[maNum];      //Noise
				_ann[index][1] = 14;
				_ann[index][2] = -1;
			}
		}

		return _ann;
	}
	return nullptr;
}
//-----------------------------------------------------------------------------

void Annotator::addAnnotationOffset(int add) const
{
	for (int i = 0; i < _qrsNum; i++)
		_qrsAnn[i][0] += add;

	for (int i = 0; i < _annNum; i++)
		_ann[i][0] += add;
}

// SaveAnnotation (**aux)   aux data
bool Annotator::SaveAnnotation(const char *name, int **ann, int num)
{
	int samples;
	unsigned short annCode = 0;
	char buff[1024];

	FILE* fp;
	fopen_s(&fp, name, "wt");
	if (!fp) return false;

	samples = ann[0][0];
	buff[0] = 0;
	buff[1] = char(0xEC);

	fwrite(buff, 2, 1, fp);
	fwrite(&samples, sizeof(samples), 1, fp);
	unsigned short type = ann[0][1];
	annCode |= (type << 10);
	fwrite(&annCode, sizeof(short), 1, fp);
	for (int i = 1; i < num; i++) {
		samples = ann[i][0] - ann[i - 1][0];
		annCode = 0;
		type = ann[i][1];
		if (samples > 1023) {
			fwrite(buff, 2, 1, fp);
			fwrite(&samples, sizeof(samples), 1, fp);
		}
		else
		{
			annCode = samples;
		}
		annCode |= (type << 10);
		fwrite(&annCode, sizeof(short), 1, fp);
	}
	buff[1] = 0;
	fwrite(buff, 2, 1, fp);
	fclose(fp);
	return true;
}

bool Annotator::getRRSequence(int **annotations, const int num, const double sampleRate, std::vector<double> *RR, std::vector<int> *RR_position) const
{
	int add = -1;
	double r1 = 0, r2 = 0;

	RR->clear();
	RR_position->clear();

	//R peak or S peak annotation
	bool rrs = true;
	int rNum = 0, sNum = 0;
	for (int i = 0; i < num; i++) {
		if (annotations[i][1] == 47 || annotations[i][1] == 48) rNum++;
		else if (annotations[i][1] == 49 || annotations[i][1] == 50) sNum++;
	}
	if (int(1.2f*float(rNum)) < sNum)
		rrs = false;  //R peaks less than S ones


	for (int i = 0; i < num; i++) {
		switch (annotations[i][1]) {
		case 0:    //non beats
		case 15:   //q
		case 17:   //Q
		case 18:   //ST change
		case 19:   //T change
		case 20:   //systole
		case 21:   //diastole
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31:
		case 32:
		case 33:
		case 36:
		case 37:
		case 40:   //')' J point
		case 42:   //(p
		case 43:   //p)
		case 44:   //(t
		case 45:   //t)
		case 47:   //r
		case 48:   //R
		case 49:   //s
		case 50:   //S
			continue;
		case 14:   //noise
		case 16:   //artifact
			add = -1;
			continue;
		default:
			break;
		}

		if (add != -1) 
		{
			//annotation on RRs peaks
			if (rrs)
			{
				if (i + 1 < num && (annotations[i + 1][1] == 47 || annotations[i + 1][1] == 48))  //r only
					r2 = annotations[i + 1][0];
				else if (i + 2 < num && (annotations[i + 2][1] == 47 || annotations[i + 2][1] == 48))  //q,r
					r2 = annotations[i + 2][0];
				else //(ann[i][1]==N,ECT,...)  //no detected R only S
					r2 = annotations[i][0];

				if (add + 1 < num && (annotations[add + 1][1] == 47 || annotations[add + 1][1] == 48))
					r1 = annotations[add + 1][0];
				else if (add + 2 < num && (annotations[add + 2][1] == 47 || annotations[add + 2][1] == 48))
					r1 = annotations[add + 2][0];
				else //(ann[add][1]==N,ECT,...) //no detected R only S
					r1 = annotations[add][0];
			}
			//annotation on S peaks
			else
			{
				if (i + 1 < num && (annotations[i + 1][1] == 40))  //N)
					r2 = annotations[i][0];
				else if (i + 1 < num && (annotations[i + 1][1] == 49 || annotations[i + 1][1] == 50))  //Sr
					r2 = annotations[i + 1][0];
				else if (i + 2 < num && (annotations[i + 2][1] == 49 || annotations[i + 2][1] == 50))  //rS
					r2 = annotations[i + 2][0];
				else if (i + 3 < num && (annotations[i + 3][1] == 49 || annotations[i + 3][1] == 50))  //errQ rS
					r2 = annotations[i + 3][0];
				else if (i + 1 < num && (annotations[i + 1][1] == 47 || annotations[i + 1][1] == 48))  //no S
					r2 = annotations[i + 1][0];
				else if (i + 2 < num && (annotations[i + 2][1] == 47 || annotations[i + 2][1] == 48))  //no S
					r2 = annotations[i + 2][0];

				if (add + 1 < num && (annotations[add + 1][1] == 40))  //N)
					r1 = annotations[add][0];
				else if (add + 1 < num && (annotations[add + 1][1] == 49 || annotations[add + 1][1] == 50))
					r1 = annotations[add + 1][0];
				else if (add + 2 < num && (annotations[add + 2][1] == 49 || annotations[add + 2][1] == 50))
					r1 = annotations[add + 2][0];
				else if (add + 3 < num && (annotations[add + 3][1] == 49 || annotations[add + 3][1] == 50))
					r1 = annotations[add + 3][0];
				else if (add + 1 < num && (annotations[add + 1][1] == 47 || annotations[add + 1][1] == 48))  //no S
					r1 = annotations[add + 1][0];
				else if (add + 2 < num && (annotations[add + 2][1] == 47 || annotations[add + 2][1] == 48))  //no S
					r1 = annotations[add + 2][0];
			}

			double rr = 60.0 / ((r2 - r1) / sampleRate);
			if (rr >= _hdr.minbpm && rr <= _hdr.maxbpm) {
				RR->push_back(rr);         //in bpm
				RR_position->push_back(int(r1));
			}
		}
		add = i;
	}

	return RR->size() != 0;
}
