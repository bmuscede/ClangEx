/////////////////////////////////////////////////////////////////////////////////////////////////////////
// MinimalPrinter.cpp
//
// Created By: Bryan J Muscedere
// Date: 24/02/17.
//
// Printer method that quietly prints information about the state of ClangEx.
// Avoids printing verbose information except where necessary.
//
// Copyright (C) 2017, Bryan J. Muscedere
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <sstream>
#include "MinimalPrinter.h"

using namespace std;

/**
 * Constructor. Creates the progress bar and prepares it.
 */
MinimalPrinter::MinimalPrinter() {
    //Suppresses standard error.
    oldCerrStream = cerr.rdbuf();
    stringstream ss;
    cerr.rdbuf (ss.rdbuf());

    numSteps = 0;
    currentStep = 0;
    progBarKilled = false;
}

/**
 * Destructor. Destroys the progress bar to ensure proper memory management.
 */
MinimalPrinter::~MinimalPrinter() {
    //Restores cerr.
    cerr.rdbuf(oldCerrStream);

    //Kills the progress bar.
    killProgressBar();
}

/**
 * Sets the number of steps for the progress bar.
 * @param steps The number of progress bar sets.
 */
void MinimalPrinter::setNumSteps(int steps) {
    if (steps <= 0) return;
    numSteps = steps;
}

/**
 * Prints the TA file that is being merged.
 * @param fileName The file name of the merged TA file.
 */
void MinimalPrinter::printMerge(std::string fileName){
    cout << "Reading TA file: " << fileName << "..." << endl;
}

/**
 * Prints the file name. Does this by incrementing the progress bar.
 * @param fileName The file name being processed.
 */
void MinimalPrinter::printFileName(std::string fileName){
    incrementProgressBar();
}

/**
 * Prints that the current file is done. Does nothing.
 */
void MinimalPrinter::printFileNameDone() {}

/**
 * Prints that the TA file has been successfully written.
 * @param fileName The file name the TA file is written to.
 * @param success Whether the TA generation was successful.
 */
void MinimalPrinter::printGenTADone(std::string fileName, bool success){
    killProgressBar();

    if (success) {
        cout << "TA file successfully written to " << fileName << "!" << endl;
    } else {
        cout << "Failure! The TA file could not be written to " << fileName << "!" << endl;
    }
}

/**
 * Performs a print of progress bar information.
 * @param status The status of the progress bar.
 */
void MinimalPrinter::printProcessStatus(Printer::PrintStatus status){
    switch (status) {
        case Printer::COMPILING:
            //Create the progress bar struct.
            bar = progressbar_new("Processing", (unsigned int) numSteps);
            break;

        case Printer::RESOLVE_REF:
        case Printer::RESOLVE_FILE:
            incrementProgressBar();
            break;
    }
}

/**
 * Informs on failure processing.
 * @return Boolean indicating user's response.
 */
bool MinimalPrinter::printProcessFailure() {
    //Stop the progress bar.
    killProgressBar();

    //Next, get the user's response.
    return Printer::printProcessFailure();
}

/**
 * Blank method that is used to print when the reference resolution has completed.
 * @param resolved Number resolved.
 * @param unresolved Number unresolved.
 */
void MinimalPrinter::printResolveRefDone(int resolved, int unresolved) {}

/**
 * Increments the progress bar by one.
 */
void MinimalPrinter::incrementProgressBar() {
    if (currentStep <= numSteps && !progBarKilled){
        //Increment it.
        progressbar_inc(bar);
        currentStep++;
    }
}

/**
 * Destroys the progress bar.
 */
void MinimalPrinter::killProgressBar(){
    //Kills the progress bar.
    if (!progBarKilled && bar) {
        progressbar_finish(bar);
        progBarKilled = true;
    }
}