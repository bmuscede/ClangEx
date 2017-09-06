//
// Created by bmuscede on 24/02/17.
//

#ifndef CLANGEX_MINIMALPRINTER_H
#define CLANGEX_MINIMALPRINTER_H

#include <iostream>
#include "ProgressBar.h"
#include "Printer.h"

class MinimalPrinter : public Printer {
public:
    MinimalPrinter();
    virtual ~MinimalPrinter();

    void setNumSteps(int steps);

    virtual void printMerge(std::string fileName);

    virtual void printFileName(std::string fileName);
    virtual void printFileNameDone();

    virtual void printGenTADone(std::string fileName, bool success);

    virtual void printProcessStatus(Printer::PrintStatus status);
    virtual bool printProcessFailure();

    virtual void printResolveRefDone(int resolved, int unresolved);

private:
    int currentStep;
    int numSteps;
    std::basic_streambuf<char, std::char_traits<char>>* oldCerrStream;
    progressbar* bar = nullptr;
    bool progBarKilled = false;

    void incrementProgressBar();
    void killProgressBar();
};


#endif //CLANGEX_MINIMALPRINTER_H
