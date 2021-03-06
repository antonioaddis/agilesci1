////////////////////////////////////////////////////////////////////////////////
// DESCRIPTION
//       AGILE Science Tools
//       AG_ctsmapgen
//       First release: 26/Gen/2005
//       Authors: Andrew Chen, Tomaso Contessi (NEON SAS), Alberto Pellizzoni,
//		 Andrea Bulgarelli, Alessio Trois, Andrea Zoli (IASF-Milano and INAF/OAS Bologna)
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


#include <iostream>
#include <string.h>

//#define DEBUG 1

#include <FitsUtils.h>
#include <Selection.h>
#include <Eval.h>
#include <PilParams.h>

using std::cout;
using std::endl;
using std::vector;

const char* startString = {
"################################################################\n"
"### Task AG_ctsmapgen B25 v1.3.0 - A.C. T.C. A.T. A.B. A.Z.  ###\n"
"################################################################\n"
};

const char* endString = {
"#################################################################\n"
"###      AG_ctsmapgen5 exiting .............................. ###\n"
"#################################################################\n"
};

const PilDescription paramsDescr[] = {
    { PilString, "outfile", "Output file name" },
    { PilString, "evtfile", "Event file index file name" },
    { PilString, "timelist", "Time intervals list" },
    { PilReal, "mdim", "Size of Map (degrees)" },
    { PilReal, "mres", "Bin size (degrees)" },
    { PilReal, "la", "Longitude of map center (galactic)" },
    { PilReal, "ba", "Latitude of map center (galactic)" },
    { PilReal, "lonpole", "Rotation of map (degrees)" },
    { PilReal, "albrad", "Radius of earth albedo (degrees)" },
    { PilInt, "phasecode", "Orbital phase code" },
    { PilInt, "filtercode", "Event filter code" },
    { PilString, "projection", "Projection (ARC or AIT)" },
    { PilReal, "tmin", "Initial time(sec)" },
    { PilReal, "tmax", "Final time(sec)" },
    { PilReal, "emin", "Min energy" },
    { PilReal, "emax", "Max energy" },
    { PilReal, "fovradmin", "Min off-axis angle (degrees)" },
    { PilReal, "fovradmax", "Max off-axis angle (degrees)" },
    { PilNone, "", "" }
};

int main(int argc, char *argv[])
{
    cout << startString << endl;

    PilParams params(paramsDescr);
    if (!params.Load(argc, argv))
        return EXIT_FAILURE;

    Intervals intervals;
    double tmin = params["tmin"];
    double tmax = params["tmax"];
    if (!eval::LoadTimeList(params["timelist"], intervals, tmin, tmax)) {
        cerr << "Error loading timelist file '" << params["timelist"].GetStr() << "'" << endl;
        return EXIT_FAILURE;
    }

    cout << endl << "INPUT PARAMETERS:" << endl;
    params.Print();

    cout << "INTERVALS N=" << intervals.Count() << ":" << endl;
    for (int i=0; i<intervals.Count(); ++i)
        cout << "   " << intervals[i].String() << endl;

    cout << "Selecting the events.." << endl;
    char selectionFilename[FLEN_FILENAME];
    char templateFilename[FLEN_FILENAME];
    tmpnam(selectionFilename);
    tmpnam(templateFilename);
    char *evtfile = (char*) params["evtfile"].GetStr();
    if (evtfile && evtfile[0]=='@')
        ++evtfile;
    string evtExpr = selection::EvtExprString(intervals, params["emin"], params["emax"],
                                    params["albrad"], params["fovradmax"], params["fovradmin"],
                                    params["phasecode"], params["filtercode"]);
    int status = selection::MakeSelection(evtfile, intervals, evtExpr, selectionFilename, templateFilename);
    if (status != 0 && status != -118) {
        cout << endl << "AG_ctsmapgen5......................selection failed" << endl;
        cout << endString << endl;
        FitsFile sfile(selectionFilename);
        sfile.Delete();
        FitsFile tfile(templateFilename);
        tfile.Delete();
        return 0;
    }

    vector< vector<int> > counts;
    status = eval::EvalCounts(params["outfile"], params["projection"], params["tmin"],
                       params["tmax"], params["mdim"], params["mres"],
                       params["la"], params["ba"], params["lonpole"],
                       params["emin"], params["emax"], params["fovradmax"],
                       params["fovradmin"], params["albrad"], params["phasecode"],
                       params["filtercode"], selectionFilename, templateFilename,
                       intervals, counts, true);
    FitsFile sfile(selectionFilename);
    sfile.Delete();
    FitsFile tfile(templateFilename);
    tfile.Delete();

    if (status) {
        cout << "AG_ctsmapgen5..................... exiting AG_ctsmapgen ERROR:" << endl;
        cout << endString << endl;
        fits_report_error(stdout, status);
        return status;
    }
    cout << endString << endl;

    return status;
}
