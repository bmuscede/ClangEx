/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Printer.h
//
// Created By: Bryan J Muscedere
// Date: 24/02/17.
//
// Method that implements a printer system for ClangEx.
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

#ifndef CLANGEX_PRINTER_H
#define CLANGEX_PRINTER_H

#include <string>

class Printer {
public:
    /** Print Status Enums */
    enum PrintStatus {COMPILING, RESOLVE_REF, RESOLVE_FILE};
    enum ProcessStatusError {RELATION_FIND, ENTITY_ATTRIBUTE, RELATION_ATTRIBUTE};

    /** Constructor */
    Printer();

    /** Destructor */
    ~Printer();

    /** Print Methods */
    void printMerge(std::string fileName);
    void printFileName(std::string fileName);
    void printFileNameDone();
    void printGenTADone(std::string fileName, bool success);
    void printProcessStatus(Printer::PrintStatus status);
    bool printProcessFailure();
    void printResolveRefDone(int resolved, int unresolved);
    
    /** Print Error Methods */
    void printErrorTAProcess(int lineNum, std::string message);
    void printErrorTAProcess(Printer::ProcessStatusError type, std::string name);
    void printErrorTAProcessMalformed();
    void printErrorTAProcessRead(std::string fileName);
    void printErrorTAProcessWrite(std::string fileName);
    void printErrorTAProcessGraph();
};

#endif //CLANGEX_PRINTER_H
