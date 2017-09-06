//
// Created by bmuscede on 24/02/17.
//

#include <iostream>
#include "VerbosePrinter.h"

using namespace std;

VerbosePrinter::VerbosePrinter() { }

void VerbosePrinter::printMerge(string fileName){
    cout << "Reading TA file " << fileName << "..." << endl;
}

void VerbosePrinter::printFileName(string fileName){
    cout << "\tCurrently processing: " << fileName << endl;
}

void VerbosePrinter::printFileNameDone() {
    cout << endl;
}

void VerbosePrinter::printGenTADone(std::string fileName, bool success) {
    if (success) {
        cout << "TA file successfully written to " << fileName << "!" << endl;
    } else {
        cout << "Failure! The TA file could not be written to " << fileName << "!" << endl;
    }
}

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

void VerbosePrinter::printResolveRefDone(int resolved, int unresolved) {
    cout << "Overall, " << resolved << " references were resolved and " << unresolved
         << " references could not be resolved." << endl << endl;
}