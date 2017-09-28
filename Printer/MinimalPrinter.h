/////////////////////////////////////////////////////////////////////////////////////////////////////////
// MinimalPrinter.h
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

#ifndef CLANGEX_MINIMALPRINTER_H
#define CLANGEX_MINIMALPRINTER_H

#include <iostream>
#include "ProgressBar.h"
#include "Printer.h"

class MinimalPrinter : public Printer {
public:
    /** Constructor/Destructor */
    MinimalPrinter();
    ~MinimalPrinter() override;

    /** Step Methods */
    void setNumSteps(int steps);

    /** Information Printers */
    void printMerge(std::string fileName) override;
    void printFileName(std::string fileName) override;
    void printFileNameDone() override;
    void printGenTADone(std::string fileName, bool success) override;
    void printProcessStatus(Printer::PrintStatus status) override;
    bool printProcessFailure() override;
    void printResolveRefDone(int resolved, int unresolved) override;

private:
    /** Member Variables */
    int currentStep;
    int numSteps;
    std::basic_streambuf<char, std::char_traits<char>>* oldCerrStream;
    progressbar* bar = nullptr;
    bool progBarKilled = false;

    /** Progress Bar Helper Methods */
    void incrementProgressBar();
    void killProgressBar();
};


#endif //CLANGEX_MINIMALPRINTER_H
