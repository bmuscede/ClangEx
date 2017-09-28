/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Printer.h
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

#ifndef CLANGEX_PRINTER_H
#define CLANGEX_PRINTER_H

#include <string>

class Printer {
public:
    /** Print Status Enums */
    enum PrintStatus {COMPILING, RESOLVE_REF, RESOLVE_FILE};
    enum ProcessStatusError {RELATION_FIND, ENTITY_ATTRIBUTE, RELATION_ATTRIBUTE};

    /** Destructor */
    virtual ~Printer();

    /** Print Methods */
    virtual void printMerge(std::string fileName) = 0;
    virtual void printFileName(std::string fileName) = 0;
    virtual void printFileNameDone() = 0;
    virtual void printGenTADone(std::string fileName, bool success) = 0;
    virtual void printProcessStatus(Printer::PrintStatus status) = 0;
    virtual bool printProcessFailure();
    virtual void printResolveRefDone(int resolved, int unresolved) = 0;

    /** Print Error Methods */
    void printErrorTAProcess(int lineNum, std::string message);
    void printErrorTAProcess(Printer::ProcessStatusError type, std::string name);
    void printErrorTAProcessMalformed();
    void printErrorTAProcessRead(std::string fileName);
    void printErrorTAProcessWrite(std::string fileName);
    void printErrorTAProcessGraph();

protected:
    /** Constructor */
    Printer();

};

#endif //CLANGEX_PRINTER_H
