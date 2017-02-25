//
// Created by bmuscede on 24/02/17.
//

#ifndef CLANGEX_PRINTER_H
#define CLANGEX_PRINTER_H

#include <string>

class Printer {
public:
    enum PrintStatus {COMPILING, RESOLVE_REF, RESOLVE_FILE};
    enum ProcessStatusError {RELATION_FIND, ENTITY_ATTRIBUTE, RELATION_ATTRIBUTE};

    virtual ~Printer();

    virtual void printMerge(std::string fileName) = 0;

    virtual void printFileName(std::string fileName) = 0;
    virtual void printFileNameDone() = 0;

    virtual void printFileSearchStart(std::string startDir) = 0;
    virtual void printFileSearch(std::string fileName) = 0;
    virtual void printFileSearchDone() = 0;

    virtual void printGenTA(std::string fileName) = 0;
    virtual void printGenTADone(std::string fileName, bool success) = 0;

    virtual void printProcessStatus(Printer::PrintStatus status) = 0;
    virtual bool printProcessFailure();

    virtual void printResolveRefDone(int resolved, int unresolved) = 0;
    virtual void printResolvePathDone() = 0;

    void printErrorTAProcess(int lineNum, std::string message);
    void printErrorTAProcess(Printer::ProcessStatusError type, std::string name);
    void printErrorTAProcessMalformed();
    void printErrorTAProcessRead(std::string fileName);
    void printErrorTAProcessWrite(std::string fileName);
    void printErrorTAProcessGraph();

protected:
    Printer();

};


#endif //CLANGEX_PRINTER_H
