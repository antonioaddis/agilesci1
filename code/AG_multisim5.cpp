////////////////////////////////////////////////////////////////////////////////
// DESCRIPTION
//       AGILE Science Tools
//       Alike multi sim
//       Authors: Andrew Chen, Tommaso Contessi (IASF-Milano),
//       Andrea Bulgarelli, Andrea Zoli (IASF-Bologna)
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


#include <TRandom3.h>
#include <RoiMulti5.h>
#include <PilParams.h>
#include <Eval.h>
#include <FitsUtils.h>
#include <sstream>

#define DEBUG 0
#ifdef DEBUG
#include <fstream>
#endif

using std::cout;
using std::cerr;
using std::endl;

const char* startString = {
"#################################################################\n"
"###      AG_multisim B25 v1.3.0 - A.B. A.C. T.C. A.Z.         ###\n"
"#################################################################\n"
};

const char* endString = {
"##################################################################\n"
"###    AG_multisim5 B25 exiting .............................. ###\n"
"##################################################################\n"
};

const PilDescription paramsDescr[] = {
	{ PilInt,    "opmode", "Operation Mode" },
	{ PilInt,    "block", "Block" },
	{ PilInt,    "nruns", "Number of runs" },
	{ PilInt,    "seed", "Seed" },
	{ PilString, "sarfile", "SAR file name" },
	{ PilString, "edpfile", "EDP file name" },
	{ PilString, "psdfile", "PSD file name" },
	{ PilString, "maplistsim", "Map list for simulation" },
	{ PilString, "srclistsim", "Source list for simulation" },
	{ PilString, "outfile", "Output file name" },
	{ PilString, "maplistanalysis", "Map list for analysis" },
	{ PilString, "srclistanalysis", "Source list for analysis" },
	{ PilReal,   "ranal", "Radius of analysis" },
	{ PilInt,    "galmode", "Diffuse emission mode" },
	{ PilInt,    "isomode", "Isotropic emission mode" },
	{ PilReal,   "ulcl",    "Upper limit confidence level" },
	{ PilReal,   "loccl",   "Location contour confidence level" },
	{ PilString, "resmatrices", "Response matrices" },
	{ PilString, "respath", "Response matrices path" },
	/*
	{ PilBool, "expratioevaluation","If 'yes' (or 'y') the exp-ratio evaluation will be enabled."},
	{ PilBool, "isExpMapNormalized","If 'yes' (or 'y') you assert that the exp-map is already normalized. Insert 'no' (or 'n') instead and the map will be normalized before carrying out the exp-ratio evaluation."},
	{ PilReal, "minThreshold", "The lower bound for the threshold level in exp-ratio evaluation"},
	{ PilReal, "maxThreshold", "The upper bound for the threshold level in exp-ratio evaluation"},
	{ PilReal, "squareSize", "The edge degree dimension of the exp-ratio evaluation area"},
	 */
	{ PilNone,   "", "" }
};

enum { Concise=1, SkipAnalysis=2, DoubleAnalysis=4, SaveMaps=8 };

AgileMap SumMaps(const AgileMap* mapArr, int offset, int block) {
	AgileMap m(mapArr[offset]);
	for (int i=offset+1; i<offset+block; ++i)
		m += mapArr[i];
	return m;
}

AgileMap SumExposure(const MapMaps& maps, int offset, int block) {
	AgileMap m(maps.ExpMap(offset));
	for (int i=offset+1; i<offset+block; ++i)
		m += maps.ExpMap(i);
	return m;
}

int main(int argc,char **argv) {
	cout << startString << endl;

	PilParams params(paramsDescr);
	if (!params.Load(argc, argv))
		return EXIT_FAILURE;

	cout << endl << "INPUT PARAMETERS:" << endl;
	params.Print();

	int opmode = params["opmode"];
	int block = params["block"];
	int nruns = params["nruns"];
	int seed = params["seed"];
	const char* sarfilename = params["sarfile"];
	const char* edpfilename = params["edpfile"];
	const char* psdfilename = params["psdfile"];
	const char* maplistsimname = params["maplistsim"];
	const char* srclistsim = params["srclistsim"];
	const char* outfilename = params["outfile"];
	const char* maplistanalysisname = params["maplistanalysis"];
	const char* srclistanalysis = params["srclistanalysis"];
	const char* resmatrices = params["resmatrices"];
	const char* respath = params["respath"];
	double ranal = params["ranal"];
	int galmode = params["galmode"];
	int isomode = params["isomode"];
	double ulcl = params["ulcl"];
	double loccl = params["loccl"];

	/*
	bool expratioevaluation = params["expratioevaluation"];
	bool isExpMapNormalized = params["isExpMapNormalized"];
	double minThreshold = params["minThreshold"];
	double maxThreshold = params["maxThreshold"];
	int squareSize = params["squareSize"];
*/

	if (seed)
		SetSeed(seed);

	MapList maplistsim;
	if (!maplistsim.Read(maplistsimname)) {
		cout << "AG_multisim5..................... exiting AG_multisim5 ERROR:" << endl;
		cout << endString << endl;
		return -1;
	}

	MapList maplistanalysis;
	if ((opmode&SkipAnalysis)==0) {
		if (!maplistanalysis.Read(maplistanalysisname)) {
			cout << "AG_multisim5..................... exiting AG_multisim5 ERROR:" << endl;
			cout << endString << endl;
			return -1;
		}
	}

	if (block>0 && (opmode&SkipAnalysis)==0) {
		if (maplistsim.Count()!=maplistanalysis.Count()) {
			cout << "AG_multisim5..................... exiting AG_multisim5 ERROR:" << endl;
			cout << "ERROR: The two map lists must have the same number of rows" << endl;
			cout << endString << endl;
			return -1;
		}
	}
	if (block>maplistsim.Count())
		block = maplistsim.Count();

	MapData mapData;
	if (!mapData.Load(maplistsim, true)) {
		cout << "AG_multisim5..................... exiting AG_multisim5 ERROR:" << endl;
		cout << "Error reading the simulation map list" << endl;
		cout << endString << endl;
		return -1;
	}
	MapData mapDataAna;
	if ((opmode&SkipAnalysis)==0) {
		if (!mapDataAna.Load(maplistanalysis, true)) {
			cout << "AG_multisim5..................... exiting AG_multisim5 ERROR:" << endl;
			cout << "Error reading the analysis map list" << endl;
			cout << endString << endl;
			return -1;
		}
	}

	SourceDataArray srcSimArr = ReadSourceFile(srclistsim);
	SourceDataArray srcAnaArr;
	if ((opmode&SkipAnalysis)==0)
		srcAnaArr = ReadSourceFile(srclistanalysis);

	/// #define _SINGLE_INSTANCE_
#ifdef _SINGLE_INSTANCE_
	cout << "Single RoiMulti instance" << endl;
		RoiMulti roiMulti;
		if (!roiMulti.SetPsf(psdfilename, sarfilename, edpfilename)) {
			cout << "AG_multisim5..................... exiting AG_multisim5 ERROR:" << endl;
			cout << "ERROR setting PSF data" << endl;
			cout << endString << endl;
			return -1;
		}
#else
	cout << "Multiple RoiMulti instance" << endl;
#endif

	double sumTS = 0;
	int analysisCount = 0;

	for (int i=0; i<nruns; ++i) {
#ifndef _SINGLE_INSTANCE_
		RoiMulti roiMulti;
		if (!roiMulti.SetPsf(psdfilename, sarfilename, edpfilename)) {
			cout << "AG_multisim5..................... exiting AG_multisim5 ERROR:" << endl;
			cout << "ERROR setting PSF data" << endl;
			cout << endString << endl;
			return -1;
		}
#endif
		cout << endl << "AG_Multisim loop #" << i+1 << endl << endl;

		mapData.MapCoeff::Load(maplistsim);
		roiMulti.SetMaps(mapData);
		cout << "New count maps simulation array size=" << mapData.Count() << endl;
		AgileMap* simArr = roiMulti.NewSimulationArray(srcSimArr); // simArr size == maplistsim size

#ifdef DEBUG
                ofstream ds("debug.cts");
#endif

		if (block) {
			int last = mapData.Length()-block;
			cout << "Using block size=" << block << endl;

			if(opmode & SaveMaps) {
				for(int iii=0; iii<mapData.Length(); iii++) {
					char mapName[256];
					sprintf(mapName, "%010d_BLOCK%03d_%s.cts.gz", i+1, iii, outfilename);
					if (simArr[iii].Write(mapName))
						cerr << "Error writing simulated block counts map " << mapName << endl;
					else
						cerr << mapName << " written" << endl;

					sprintf(mapName, "%010d_BLOCK%03d_%s.exp.gz", i+1, iii, outfilename);
					if (mapData.ExpMap(i).Write(mapName))
						cerr << "Error writing simulated exp map " << mapName << endl;
					else
						cerr << mapName << " written" << endl;
				}
			}

			for (int j=0; j<=last; ++j) {
				cout << endl << "Summing maps from " << j+1 << " to " << j+block << " [loop " << i+1 << "]" << endl << endl;

#ifdef DEBUG
                                for (int b=0; b<block; ++b) {
                                    const AgileMap& map = simArr[j+b];
                                    long counts = 0;
                                    for (int y=0; y<map.Dim(0); ++y)
                                        for (int x=0; x<map.Dim(1); ++x)
                                            counts += map(y, x);
                                    ds << counts << " ";
                                }
                                ds << std::endl;
#endif

				AgileMap ctsMap = SumMaps(simArr, j, block);
				AgileMap expMap = SumExposure(mapData, j, block);
				/// Writing cts and exp maps
				if (opmode & SaveMaps) {
					char mapName[256];
					sprintf(mapName, "%010d_SUM%03d_%s.cts.gz", i+1, j+1, outfilename);
					if (ctsMap.Write(mapName))
						cerr << "Error writing simulated counts map " << mapName << endl;
					else
						cerr << mapName << " written" << endl;
					sprintf(mapName, "%010d_SUM%03d_%s.exp.gz", i+1, j+1, outfilename);
					if (expMap.Write(mapName))
						cerr << "Error writing simulated counts map " << mapName << endl;
					else
						cerr << mapName << " written" << endl;
				}

				if ((opmode&SkipAnalysis)==0) {
					AgileMap gasMap;
					std::stringstream ss;
					ss << respath << "/" << expMap.GetEmin() << "_" << expMap.GetEmax() << "." << resmatrices << ".disp.conv.sky.gz";
                    std::string diffuseFile = ss.str();
					int status = eval::EvalGasMap(gasMap, expMap, diffuseFile.c_str(), diffuseFile.c_str());
					if(status) {
						cout << "Error during gas map evaluation" << endl;
						cout << "AG_multisim5..................... exiting AG_multisim5 ERROR:" << endl;
						fits_report_error(stdout, status);
						return status;
					}
					if(opmode & SaveMaps) {
						char mapName[256];
						sprintf(mapName, "%010d_SUM%03d_%s.gas.gz", i+1, j+1, outfilename);
						if (gasMap.Write(mapName))
							cerr << "Error writing simulated gas map " << mapName << endl;
						else
							cerr << mapName << " written" << endl;
					}
					cout << endl << "AG_Multisim: Analysis step" << endl << endl;
					MapData analysisMaps(ctsMap, expMap, gasMap, 0, 1, 1);
					analysisMaps.MapCoeff::Load(maplistanalysis);
					roiMulti.SetMaps(analysisMaps, galmode, isomode);
					roiMulti.DoFit(srcAnaArr, ranal, ulcl, loccl);
					if (opmode & DoubleAnalysis) {
						cout << endl << "AG_Multisim: Second analysis step" << endl << endl;
						SourceDataArray newSources = roiMulti.GetFitData();
						roiMulti.DoFit(newSources, ranal, ulcl, loccl);
					}

					++analysisCount;
					SourceDataArray results = roiMulti.GetFitData();
					int dataCount = results.Count();
					for (int s=0; s<dataCount; ++s)
						if (results[s].fixflag!=AllFixed)
							sumTS += results[s].TS;

					if (opmode & Concise)
						roiMulti.LogSources(outfilename, i+1, simArr, maplistsim.Count());
					else {
						char fileName[256];
						sprintf(fileName, "%010d_%03d_%s", i+1, j+1, outfilename);
						roiMulti.Write(fileName);
						roiMulti.WriteSources(fileName, false, false, 0, 15, 10, true, true);
					}
				}
			}
		}
		else {
			if (opmode & SaveMaps) {
				char ctsName[256];
				for (int m=0; m<mapData.Length(); ++m) {
					sprintf(ctsName, "%010d_%03d_%s.cts.gz", i+1, m+1, outfilename);
					if (simArr[0].Write(ctsName))
						cerr << "Error writing simulated counts map " << ctsName << endl;
					else
						cerr << ctsName << " written" << endl;
				}
			}
			if ((opmode&SkipAnalysis)==0) {
				cout << endl << "AG_Multisim: Analysis step" << endl << endl;
				mapDataAna.ReplaceCtsMaps(simArr);

				roiMulti.SetMaps(mapDataAna, galmode, isomode);
				roiMulti.DoFit(srcAnaArr, ranal, ulcl, loccl);

				if (opmode & DoubleAnalysis) {
					cout << endl << "AG_Multisim: Second analysis step" << endl << endl;
					SourceDataArray newSources = roiMulti.GetFitData();
					roiMulti.DoFit(newSources, ranal, ulcl, loccl);
				}

				++analysisCount;
				SourceDataArray results = roiMulti.GetFitData();
				int dataCount = results.Count();
				for (int s=0; s<dataCount; ++s)
					if (results[s].fixflag!=AllFixed)
						sumTS += results[s].TS;

				if (opmode & Concise)
					roiMulti.LogSources(outfilename, i+1, simArr, maplistsim.Count());
				else {
					char fileName[256];
					sprintf(fileName, "%010d_%s", i+1, outfilename);
					roiMulti.Write(fileName);
					roiMulti.WriteSources(fileName, false, false, 0, 15, 10, true, true);
				}
			}
		}
		delete[] simArr;
	}
	if (analysisCount)
		cout << endl << "AG_Multisim performed the analysis " << analysisCount << " times" << endl << "Total TS: " << sumTS << ", average: " << sumTS/analysisCount << endl << endl;

	cout << endString << endl;

	return 0;
}
