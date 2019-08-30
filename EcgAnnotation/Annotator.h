#pragma once
#include <vector>
#include "ecgtypes.h"

class Annotator
{

public:
    Annotator(PANN_HEADER p = nullptr);
	//EcgAnnotation(const EcgAnnotation& annotation);
    ~Annotator();

	// Data
	enum AMPLIFY_QRS { INTER1, BIOR13 };
	enum WAVE_TYPE { NORMAL, BIPHASE };
	// Operators
			//const EcgAnnotation& operator=(const EcgAnnotation& annotation);

	// Operations
    int** getQRS(const double* data, int size, double sampleRate); //get RR's classification
	void getEctopia(int **annotations, int qrsNum, double sampleRate) const;                                                   //classify ectopic beats
    int** getPTU(const double *data, int length, double sampleRate, int **annotations, int qrsNum);

	void addAnnotationOffset(int add) const;    //add if annotated within fromX-toX
    static bool SaveAnnotation(const char *name, int **annotations, int num);
	//bool  ReadANN(wchar_t *);

    bool getRRSequence(int **annotations, int num, double sampleRate, std::vector<double> *RR, std::vector<int> *RR_position) const;

	// Access
	 int getQRSNumber() const;
	 int getAnnotationSize() const;
	 int** getAnnotation() const;
	 int** getQRSAnnotation() const;
	 char** getAuxData() const;
	 ANN_HEADER* getAnnotationHeader();

	// Inquiry

private:
    Annotator(const Annotator& annotation) = delete;
    const Annotator& operator=(const Annotator& annotation) = delete;

	static bool _isNoise(const double *data, int window);        //check for noise in window len
    bool _filter30Hz(double *data, int size, double sampleRate) const;    //0-30Hz removal

	static void _find_RS(const double *data, int size, int &R, int &S, double err = 0.0);  //find RS or QR
	int _find_r(const double *data, int size, double err = 0.0) const;  //find small r in PQ-S
	int _find_q(const double *data, int size, double err = 0.0) const;  //find small q in PQ-R
	int _find_s(const double *data, int size, double err = 0.0) const;  //find small s in R-Jpnt
	int _findTMax(const double *data, int size) const;


	ANN_HEADER _hdr;                    //annotation ECG params

	int **_ann;
	int _annNum;
	int **_qrsAnn;
	int _qrsNum;
	std::vector <int> _ma;                //MA noise
	int _auxNum;
	char **_aux;                     //auxiliary ECG annotation data
	static std::string _filterPath;
};

/*                      0           1             2
		int **ANN [x][samples][annotation type][aux data index]

		wchar_t *dir = "c:\\dir_for_fwt_filters\\filters";
		double *sig;   //signal massive
		double SR;     //sampling rate of the signal
		int size;   //size of the signal

		EcgAnnotation ann;

		int **qrsAnn;  //qrs annotation massive
		qrsAnn = ann.GetQRS(sig,size,SR,dir);       //get QRS complexes
		//qrsAnn = ann->GetQRS(psig,size,SR,umess,qNOISE);    //get QRS complexes if signal is quite noisy

		int **ANN; //QRS + PT annotation
		if(qrsAnn) {
				ann.GetEctopics(qrsAnn,ann->GetQrsNumber(),SR);     //label Ectopic beats
				ANN = ann.GetPTU(sig,size,SR,dir,qrsAnn,ann.GetQrsNumber());   //find P,T waves
		}
*/



//annotation codes types
/*
   skip=59
   num=60
   sub=61
   chn=62
   aux=63

char anncodes [51][10] =  {"notQRS", "N",       "LBBB",    "RBBB",     "ABERR", "PVC",
						   "FUSION", "NPC",     "APC",     "SVPB",     "VESC",  "NESC",
						   "PACE",   "UNKNOWN", "NOISE",   "q",        "ARFCT", "Q",
						   "STCH",   "TCH",     "SYSTOLE", "DIASTOLE", "NOTE",  "MEASURE",
						   "P",      "BBB",     "PACESP",  "T",        "RTM",   "U",
						   "LEARN",  "FLWAV",   "VFON",    "VFOFF",    "AESC",  "SVESC",
						   "LINK",   "NAPC",    "PFUSE",   "(",        ")",     "RONT",

	//user defined beats//
						   "(p",     "p)",      "(t",      "t)",       "ECT",
						   "r",      "R",       "s",       "S"};


				  [16] - ARFCT
				  [15] - q
				  [17] - Q
				  [24] - P
				  [27] - T
				  [39, 40] - '(' QRS ')'  PQ, J point
				  42 - (p Pwave onset
				  43 - p) Pwave offset
				  44 - (t Twave onset
				  45 - t) Twave offset
				  46 - ect Ectopic of any origin beat
				  47 - r
				  48 - R
				  49 - s
				  50 - S
											   */

