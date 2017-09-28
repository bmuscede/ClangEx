/////////////////////////////////////////////////////////////////////////////////////////////////////////
// VerbosePrinter.h
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

#ifndef CLANGEX_VERBOSEPRINTER_H
#define CLANGEX_VERBOSEPRINTER_H

#include "Printer.h"

class VerbosePrinter : public Printer {
public:
    /** Constructor */
    VerbosePrinter();

    /** Print Methods */
    virtual void printMerge(std::string fileName);
    virtual void printFileName(std::string fileName);
    virtual void printFileNameDone();
    virtual void printGenTADone(std::string fileName, bool success);
    virtual void printProcessStatus(Printer::PrintStatus status);
    virtual void printResolveRefDone(int resolved, int unresolved);
};


#endif //CLANGEX_VERBOSEPRINTER_H
