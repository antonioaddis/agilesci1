////////////////////////////////////////////////////////////////////////////////
// DESCRIPTION
//       AGILE Science Tools
//       AG_iterativeGenSrcList
//       Author: Andrea Bulgarelli, Tomaso Contessi
//
// FILE HISTORY
//       31/Jul/2009
//       First release: V1.0
//
// NOTICE
//       Any information contained in this software
//       is property of the AGILE TEAM and is strictly
//       private and confidential.
//       Copyright (C) 2005-2019 AGILE Team. All rights reserved.
/*
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
////////////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <iostream>

#include "fitsio.h"
#include "pil.h"

#include <MathUtils.h>
#include <AgileMap.h>
#include <RoiMulti5.h>

using namespace std;



static void GetPilParam(
	const char* paramName,
	char*       strParam,
	int&        status)
{
if (status==PIL_OK) {
	status = PILGetString(paramName, strParam);
	if (status!=PIL_OK)
		cerr << "Error getting PIL parameter " << paramName << endl;
	}
}

static void GetPilParam(
	const char* paramName,
	double&     dblParam,
	int&        status)
{
if (status==PIL_OK) {
	status = PILGetReal(paramName, &dblParam);
	if (status!=PIL_OK)
		cerr << "Error getting PIL parameter " << paramName << endl;
	}
}

static void GetPilParam(
	const char* paramName,
	int&        intParam,
	int&        status)
{
if (status==PIL_OK) {
	status = PILGetInt(paramName, &intParam);
	if (status!=PIL_OK)
		cerr << "Error getting PIL parameter " << paramName << endl;
	}
}

static int DoPIL(int argc, char *argv[],
	char*   ctsfilename,
	double& lcenter,
	double& bcenter,
	double& rextract,
	int&    binstep,
	double& index,
	int&    fixflag,
	double& minsqrts,
	char*   outfilename,
	double& maxdistthr)
{
int status = PILInit(argc, argv);
int numpar = 0;
status = status || PILGetNumParameters(&numpar);
GetPilParam("ctsfile", ctsfilename, status);
GetPilParam("lcenter", lcenter, status);
GetPilParam("bcenter", bcenter, status);
GetPilParam("rextract", rextract, status);
GetPilParam("binstep", binstep, status);
GetPilParam("index", index, status);
GetPilParam("fixflag", fixflag, status);
GetPilParam("minsqrts", minsqrts, status);
GetPilParam("outfile", outfilename, status);
GetPilParam("maxdistthr", maxdistthr, status);
PILClose(status);
return status;
}


static void PrintInput(
	const char* ctsfilename,
	double      lcenter,
	double      bcenter,
	double      rextract,
	int         binstep,
	double      index,
	int         fixflag,
	double      minsqrts,
	const char* outfilename,
	double      maxdistthr)
{
cout << " "<< endl;
cout << " "<< endl;	
cout << "#################################################################"<< endl;
cout << "###    AG_iterativeGenSrcList B25 v1.2.0 - A.B. T.C.          ###"<< endl;
cout << "#################################################################"<< endl;
cout << " "<< endl;
cout << "INPUT PARAMETERS:" << endl << endl;
cout << "Counts file name: " << ctsfilename << endl;
cout << "L centre: " << lcenter << endl;
cout << "B centre: " << bcenter << endl;
cout << "Extracion radius (degrees): " << rextract << endl;
cout << "Bin step size: " << binstep << endl;
cout << "Spectral index: " << index << endl;
cout << "Fix flag (0, 1, 2): " << fixflag << endl;
cout << "Min sqrt(TS) treshold: " << minsqrts << endl;
cout << "Output file name: " << outfilename << endl;
cout << "Max distance threshold: " << maxdistthr << endl;
cout << endl << endl;
}


/**
static string CycleNumber(int n)
{
char buffer[16]; 
sprintf(buffer, "_%02d", n);
return string(buffer);
}
*/


int main(int argc, char *argv[])
{
char   ctsfilename[FLEN_FILENAME];
double lcenter;
double bcenter;
double rextract;
int    binstep;
double index;
int    fixflag;
double minsqrts;
double maxdistthr;
char   outfilename[FLEN_FILENAME];
	
int status = DoPIL(argc, argv, ctsfilename, lcenter, bcenter, rextract, binstep, index, fixflag, minsqrts, outfilename, maxdistthr);
if (status==PIL_OK) {
	PrintInput(ctsfilename, lcenter, bcenter, rextract, binstep, index, fixflag, minsqrts, outfilename, maxdistthr);
	AgileMap ctsmap(ctsfilename);
	ofstream outfile(outfilename, ios::app);
	if (outfile.is_open()) {
		int binProg = 0;
		int rowCount = ctsmap.Rows();
		int colCount = ctsmap.Cols();
		for (int row=0; row<rowCount; row+=binstep)
			for (int col=0; col<colCount; col+=binstep) {
				double l = ctsmap.l(row, col);
				double b = ctsmap.b(row, col);
				if (SphDistDeg(l, b, lcenter, bcenter)<rextract) {
					char srcName[16];
					sprintf(srcName, "%05d", ++binProg+10000);
					outfile << "0.00000e-08 " << l << " " << b << " " << index << " " << fixflag << " " << minsqrts << " " << srcName << " " << maxdistthr << endl;
					}
				}
		outfile.close();
		}
	else
		cerr << "Could not open " << outfilename << " for output" << endl;

	printf("\n\n\n#################################################################\n");
	printf("########## AG_iterativeGenSrcList B25........ exiting ###########\n");
	printf("#################################################################\n\n\n");
	}
else
	cerr << "PIL Intialization Error" << endl;
return status;
}
