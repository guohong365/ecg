#ifndef ECGTYPES_H
#define ECGTYPES_H

#ifndef _MAX_PATH
#define _MAX_PATH 260
#endif

#define FLOAT_EQ_ERR 0.000000001

typedef struct _cwt_hdr {
    char hdr[4];
    float fmin;
    float fmax;
    float fstep;
    unsigned int size;
    float sr;
    unsigned char type;   //1-log; 0-norm scale

    char rsrv[15];
} CWT_HEADER, *PCWT_HEADER;

typedef struct _fwt_hdr {
    char hdr[4];
    unsigned int size;
    float sr;
    unsigned char bits;
    unsigned char lead;
    unsigned short umv;

    char wlet[8];
    unsigned short J;

    char rsrv[14];
} FWT_HEADER, *PFWT_HEADER;


typedef struct _data_hdr {
        char hdr[4];
        unsigned int size;
        float sr;
        unsigned char bits;
        unsigned char lead;
        unsigned short umv;
        unsigned short bline;
        unsigned char hh;
        unsigned char mm;
        unsigned char ss;
        char rsrv[19];
} DATA_HEADER, *PDATA_HEADER;


typedef struct _ann_record {
    unsigned int pos;    //offset
    unsigned int type;   //beat type
    unsigned int aux;    //index to aux array
} ANN_RECORD, *PANN_RECORD;

typedef struct _ann_hdr {
    int minbpm;
    int maxbpm;
    double minQRS;          //min QRS duration
    double maxQRS;          //max QRS duration
    double qrsFreq;         //qrs filtration freq 13Hz default
    int ampQRS;             //amplify QRS complex
    double minUmV;          //min R,S amplitude
    double minPQ;
    double maxPQ;
    double minQT;
    double maxQT;
    double pFreq;           //p wave freq for CWT
    double tFreq;           //t wave freq for CWT
    int biTwave;            //biphasic T wave - 1, normal - 0
} ANN_HEADER, *PANN_HEADER;

typedef struct _annotation{
    _ann_record record;
    char name[100];
} ANNOTATION, *PANNOTATION;

#endif // ECGTYPES_H
