//
// Created by bmuscede on 24/02/17.
//

#include <iostream>
#include <sstream>
#include "MinimalPrinter.h"

using namespace std;

MinimalPrinter::MinimalPrinter() {
    //Suppresses standard error.
    oldCerrStream = cerr.rdbuf();
    stringstream ss;
    cerr.rdbuf (ss.rdbuf());

    numSteps = 0;
    currentStep = 0;
    progBarKilled = false;
}

MinimalPrinter::~MinimalPrinter() {
    //Restores cerr.
    cerr.rdbuf(oldCerrStream);

    //Kills the progress bar.
    killProgressBar();
}

void MinimalPrinter::setNumSteps(int steps) {
    if (steps <= 0) return;
    numSteps = steps;
}

void MinimalPrinter::printMerge(std::string fileName){
    cout << "Reading TA file: " << fileName << "..." << endl;
}

void MinimalPrinter::printFileName(std::string fileName){
    incrementProgressBar();
}

void MinimalPrinter::printFileNameDone() {}

void MinimalPrinter::printGenTADone(std::string fileName, bool success){
    killProgressBar();

    if (success) {
        cout << "TA file successfully written to " << fileName << "!" << endl;
    } else {
        cout << "Failure! The TA file could not be written to " << fileName << "!" << endl;
    }
}

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

bool MinimalPrinter::printProcessFailure() {
    //Stop the progress bar.
    killProgressBar();

    //Next, get the user's response.
    return Printer::printProcessFailure();
}

void MinimalPrinter::printResolveRefDone(int resolved, int unresolved) {}

void MinimalPrinter::incrementProgressBar() {
    if (currentStep <= numSteps && !progBarKilled){
        //Increment it.
        progressbar_inc(bar);
        currentStep++;
    }
}

void MinimalPrinter::killProgressBar(){
    //Kills the progress bar.
    if (!progBarKilled && bar) {
        progressbar_finish(bar);
        progBarKilled = true;
    }
}