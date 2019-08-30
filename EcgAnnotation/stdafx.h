// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#pragma warning(disable : 4996)


#include <iostream>
#include <tchar.h>

// TODO: reference additional headers your program requires here
#include <windows.h>
#include <stdio.h>
#include <math.h>

#include <vector>
using namespace std;


static char anncodes [51][10] =  { 
	"notQRS", "N",  "LBBB", "RBBB", "ABERR", "PVC",
	"FUSION", "NPC", "APC", "SVPB", "VESC", "NESC",
	"PACE", "UNKNOWN", "NOISE", "q", "ARFCT", "Q",
	"STCH", "TCH", "SYSTOLE", "DIASTOLE", "NOTE", "MEASURE",
	"P", "BBB", "PACESP", "T", "RTM", "U",
	"LEARN", "FLWAV", "VFON", "VFOFF", "AESC", "SVESC",
	"LINK", "NAPC", "PFUSE", "(", ")", "RONT",
	//user defined beats//
	"(p", "p)", "(t", "t)", "ECT",
	"r", "R", "s", "S"
};