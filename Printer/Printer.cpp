//
// Created by bmuscede on 24/02/17.
//

#include <iostream>
#include "Printer.h"

using namespace std;

Printer::~Printer() {}

bool Printer::printProcessFailure() {
    cout << "Compilation finished but errors were detected." << endl;

    bool loop = true;
    while (loop){
        cout << "Do you want to output the TA model? (Y/N): ";

        //Gets the user's input.
        string prompt;
        cin >> prompt;

        //Then, check the response.
        if (prompt.compare("N") == 0 || prompt.compare("n") == 0){
            return false;
        } else if (prompt.compare("Y") == 0 || prompt.compare("y") == 0){
            loop = false;
        } else {
            cout << endl << "Invalid input. Please type either 'Y' or 'N'" << endl;
        }
    }

    return true;
}

void Printer::printErrorTAProcess(int lineNum, std::string message) {
    cout << "Invalid input on line " << lineNum << "." << endl;
    cout << message << endl;
}

void Printer::printErrorTAProcess(Printer::ProcessStatusError type, string name){
    cout << "TA file does not have a ";

    switch (type){
        case RELATION_FIND:
        case RELATION_ATTRIBUTE:
            cout << "relation ";
            break;

        case ENTITY_ATTRIBUTE:
            cout << "entity ";
    }
    cout << "called " << name << "!" << endl;

    switch (type){
        case RELATION_FIND:
            cout << "Exiting program..." << endl;
            break;

        case RELATION_ATTRIBUTE:
        case ENTITY_ATTRIBUTE:
            cout << "This item needs to be specified before giving it attributes." << endl;
    }
}

void Printer::printErrorTAProcessMalformed() {
    cout << "The TA structure in memory is malformed." << endl;
    cout << "Please check the relation attributes in the TA file!" << endl;
}

void Printer::printErrorTAProcessRead(std::string fileName){
    cout << "The TA file " << fileName << " does not exist!" << endl;
    cout << "Exiting program..." << endl;
}

void Printer::printErrorTAProcessWrite(std::string fileName){
    cout << "The TA file " << fileName << " could not be written!" << endl;
    cout << "Exiting program..." << endl;
}

void Printer::printErrorTAProcessGraph() {
    cout << "Invalid TA graph object supplied." << endl;
    cout << "Please supply an initialized TA graph object!" << endl;
}

void Printer::printErrorNoFiles() {
    cout << "Error: No files passed along to ClangEx!" << endl;
    cout << "Be sure to check that your directory contains source code files." << endl;
}

Printer::Printer() { }