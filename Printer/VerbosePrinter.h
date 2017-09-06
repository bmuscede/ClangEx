//
// Created by bmuscede on 24/02/17.
//

#ifndef CLANGEX_VERBOSEPRINTER_H
#define CLANGEX_VERBOSEPRINTER_H

#include "Printer.h"

class VerbosePrinter : public Printer {
public:
    VerbosePrinter();

    virtual void printMerge(std::string fileName);

    virtual void printFileName(std::string fileName);
    virtual void printFileNameDone();

    virtual void printGenTADone(std::string fileName, bool success);

    virtual void printProcessStatus(Printer::PrintStatus status);

    virtual void printResolveRefDone(int resolved, int unresolved);

};


#endif //CLANGEX_VERBOSEPRINTER_H
