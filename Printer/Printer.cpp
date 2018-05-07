/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Printer.cpp
//
// Created By: Bryan J Muscedere
// Date: 24/02/17.
//
// Abstract method that implements a printer system for ClangEx.
// Defines methods that both the minimal and verbose printer must
// have for them to function.
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
#include "Printer.h"

using namespace std;

/**
 * Default Destructor.
 */
Printer::~Printer() {}

/**
 * Prints information about the merge.
 * @param fileName The filename being merged.
 */
void Printer::printMerge(string fileName){
    cout << "Reading TA file " << fileName << "..." << endl;
}

/**
 * Notifies the file being currently processed.
 * @param fileName The filename being processed.
 */
void Printer::printFileName(string fileName){
    cout << "\tCurrently processing: " << fileName << endl;
}

/**
 * Prints a new line for the next filename.
 */
void Printer::printFileNameDone() {
    cout << endl;
}

/**
 * Prints whether the TA generation was successfully or unsuccessfully completed.
 * @param fileName The filename for the TA file.
 * @param success Whether the TA file was generated successfully.
 */
void Printer::printGenTADone(std::string fileName, bool success) {
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
void Printer::printProcessStatus(Printer::PrintStatus status){
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
void Printer::printResolveRefDone(int resolved, int unresolved) {
    cout << "Overall, " << resolved << " references were resolved and " << unresolved
         << " references could not be resolved." << endl << endl;
}

/**
 * Method that notifies the user on a processing error.
 * @return The user's repsonse if they want to continue or not.
 */
bool Printer::printProcessFailure() {
    cout << "Compilation finished but errors were detected." << endl;

    bool loop = true;
    while (loop){
        cout << "Do you want to output the TA model? (Y/N): ";

        //Gets the user's input.
        string prompt;
        cin >> prompt;

        //Then, check the response.
        if (prompt.compare("N") == 0 || prompt.compare("n") == 0){
            return false;
        } else if (prompt.compare("Y") == 0 || prompt.compare("y") == 0){
            loop = false;
        } else {
            cout << endl << "Invalid input. Please type either 'Y' or 'N'" << endl;
        }
    }

    return true;
}

/**
 * Error message for if an error is encountered when processing a TA file.
 * @param lineNum The line number.
 * @param message The error message.
 */
void Printer::printErrorTAProcess(int lineNum, std::string message) {
    cout << "Invalid input on line " << lineNum << "." << endl;
    cout << message << endl;
}

/**
 * Error message for if an error is encountered when processing a TA file.
 * @param type The type of error.
 * @param name The name of the entity.
 */
void Printer::printErrorTAProcess(Printer::ProcessStatusError type, string name){
    cout << "TA file does not have a ";

    switch (type){
        case RELATION_FIND:
        case RELATION_ATTRIBUTE:
            cout << "relation ";
            break;

        case ENTITY_ATTRIBUTE:
            cout << "entity ";
    }
    cout << "called " << name << "!" << endl;

    switch (type){
        case RELATION_FIND:
            cout << "Exiting program..." << endl;
            break;

        case RELATION_ATTRIBUTE:
        case ENTITY_ATTRIBUTE:
            cout << "This item needs to be specified before giving it attributes." << endl;
    }
}

/**
 * Error message for if the TA structure created is malformed.
 */
void Printer::printErrorTAProcessMalformed() {
    cout << "The TA structure in memory is malformed." << endl;
    cout << "Please check the relation attributes in the TA file!" << endl;
}

/**
 * Error that is printed if there is an error reading a TA file.
 * @param fileName The filename for the TA file.
 */
void Printer::printErrorTAProcessRead(std::string fileName){
    cout << "The TA file " << fileName << " does not exist!" << endl;
    cout << "Exiting program..." << endl;
}

/**
 * Error that is printed if there is an error writing a TA file.
 * @param fileName The filename for the TA file.
 */
void Printer::printErrorTAProcessWrite(std::string fileName){
    cout << "The TA file " << fileName << " could not be written!" << endl;
    cout << "Exiting program..." << endl;
}

/**
 * Error that is printed if a TA graph is invalid.
 */
void Printer::printErrorTAProcessGraph() {
    cout << "Invalid TA graph object supplied." << endl;
    cout << "Please supply an initialized TA graph object!" << endl;
}

/**
 * Default Constructor.
 */
Printer::Printer() { }