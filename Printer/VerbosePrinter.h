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

    virtual void printFileSearchStart(std::string startDir);
    virtual void printFileSearch(std::string fileName);
    virtual void printFileSearchDone();

    virtual void printGenTA(std::string fileName);
    virtual void printGenTADone(std::string fileName, bool success);

    virtual void printProcessStatus(Printer::PrintStatus status);

    virtual void printResolveRefDone(int resolved, int unresolved);
    virtual void printResolvePathDone();

};


#endif //CLANGEX_VERBOSEPRINTER_H
