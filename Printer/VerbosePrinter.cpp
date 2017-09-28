/////////////////////////////////////////////////////////////////////////////////////////////////////////
// VerbosePrinter.cpp
//
// Created By: Bryan J Muscedere
// Date: 24/02/17.
//
// Printer method that verbosely prints information about the state of ClangEx.
// Instead of a progress bar. Simply just prints information about what the system
// is currently processing.
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
#include "VerbosePrinter.h"

using namespace std;

/**
 * Default Constructor.
 */
VerbosePrinter::VerbosePrinter() { }

/**
 * Prints information about the merge.
 * @param fileName The filename being merged.
 */
void VerbosePrinter::printMerge(string fileName){
    cout << "Reading TA file " << fileName << "..." << endl;
}

/**
 * Notifies the file being currently processed.
 * @param fileName The filename being processed.
 */
void VerbosePrinter::printFileName(string fileName){
    cout << "\tCurrently processing: " << fileName << endl;
}

/**
 * Prints a new line for the next filename.
 */
void VerbosePrinter::printFileNameDone() {
    cout << endl;
}

/**
 * Prints whether the TA generation was successfully or unsuccessfully completed.
 * @param fileName The filename for the TA file.
 * @param success Whether the TA file was generated successfully.
 */
void VerbosePrinter::printGenTADone(std::string fileName, bool success) {
    if (success) {
        cout << "TA file successfully written to " << fileName << "!" << endl;
    } else {
        cout << "Failure! The TA file could not be written to " << fileName << "!" << endl;
    }
}

/**
 * Prints the state of the processor depending on the print status enum.
 * @param status The status of ClangEx.
 */
void VerbosePrinter::printProcessStatus(Printer::PrintStatus status){
    switch (status){
        case Printer::COMPILING:
            cout << "Compiling the source code..." << endl;
            break;

        case Printer::RESOLVE_REF:
            cout << "Resolving external references..." << endl;
            break;

        case Printer::RESOLVE_FILE:
            cout << "Resolving source code file paths..." << endl;
    }
}

/**
 * Notifies when the references have been resolved.
 * @param resolved The number resolved.
 * @param unresolved The number unresolved.
 */
void VerbosePrinter::printResolveRefDone(int resolved, int unresolved) {
    cout << "Overall, " << resolved << " references were resolved and " << unresolved
         << " references could not be resolved." << endl << endl;
}