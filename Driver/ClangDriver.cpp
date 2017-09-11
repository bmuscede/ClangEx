/////////////////////////////////////////////////////////////////////////////////////////////////////////
// ClangDriver.cpp
//
// Created By: Bryan J Muscedere
// Date: 05/09/17.
//
// Driver that allows user commands to be converted into slome sort of
// ClangEx action. Can add files, manage TA files, output files, and
// run ClangEx on a desired source file. Ensures commands are handled gracefully

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

#include <regex>
#include "../Printer/MinimalPrinter.h"
#include "../Printer/VerbosePrinter.h"
#include "../TupleAttribute/TAProcessor.h"
#include "../Walker/ASTWalker.h"
#include "../Walker/BlobWalker.h"
#include "../Walker/PartialWalker.h"
#include <ostream>
#include <fstream>
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include <llvm/Support/CommandLine.h>
#include "ClangDriver.h"

using namespace std;
using namespace clang::tooling;

/**
 * Constructor. Simply configures C/C++ extensions.
 */
ClangDriver::ClangDriver() {
    //Sets the C, C++ extensions.
    ext.push_back(C_FILE_EXT);
    ext.push_back(CPLUS_FILE_EXT);
    ext.push_back(CPLUSPLUS_FILE_EXT);
}

/**
 * Destructor. Stub.
 */
ClangDriver::~ClangDriver() { }

/**
 * Prints the current status of ClangEx.
 * @param files Whether files are outputted.
 * @param ta Whether TA numbers are outputted.
 * @param toggle Whether language features are outputted.
 * @return String containing the results.
 */
string ClangDriver::printStatus(bool files, bool ta, bool toggle){
    string output = "";

    //First, processes the file list.
    if (files) {
        output += "There are " + to_string(this->files.size()) + " file(s) in the queue to be processed.\n";
        if (this->files.size() > 0) {
            output += "Files:\n";
            for (path curFile : this->files) {
                output += curFile.string() + "\n";
            }
        }
        output += "\n";
    }

    if (ta){
        output += "There are " + to_string(graphs.size()) + " graph(s) to be outputted.\n\n";
    }

    if (toggle){
        vector<string> enabledStrings = getEnabled();
        vector<string> disabledStrings = getDisabled();

        output += "Language Features:\n";
        if (enabledStrings.size() > 0) {
            output += "Enabled -\n";
            for (string enString : enabledStrings) {
                output += "\t" + enString + "\n";
            }
        }

        if (disabledStrings.size() > 0) {
            output += "Disabled -\n";
            for (string disString : disabledStrings) {
                output += "\t" + disString + "\n";
            }
        }
    }

    return output;
}

/**
 * Returns the language string.
 * @return Indicates the supported languages.
 */
string ClangDriver::getLanguageString(){
    return langString;
}

/**
 * Gets the number of graphs.
 * @return The number of graphs.
 */
int ClangDriver::getNumGraphs(){
    return (int) graphs.size();
}

/**
 * Gets the number of files.
 * @return The number of files.
 */
int ClangDriver::getNumFiles(){
    return (int) files.size();
}

/**
 * Enables a feature based on a string.
 * @param feature The feature to enable.
 * @return Whether a feature was enabled.
 */
bool ClangDriver::enableFeature(string feature){
    if (feature.compare("cSubSystem") == 0){
        toggle.cSubSystem = false;
    } else if (feature.compare("cVariable") == 0){
        toggle.cVariable = false;
    } else if (feature.compare("cUnion") == 0){
        toggle.cUnion = false;
    } else if (feature.compare("cStruct") == 0){
        toggle.cStruct = false;
    } else if (feature.compare("cFunction") == 0){
        toggle.cFunction = false;
    } else if (feature.compare("cFile") == 0){
        toggle.cFile = false;
    } else if (feature.compare("cEnum") == 0){
        toggle.cEnum = false;
    } else if (feature.compare("cClass") == 0){
        toggle.cClass = false;
    } else {
        return false;
    }

    return true;
}

/**
 * Disables a feature based on a string.
 * @param feature The feature to disable.
 * @return Whether a feature was disabled.
 */
bool ClangDriver::disableFeature(string feature){
    if (feature.compare("cSubSystem") == 0){
        toggle.cSubSystem = true;
    } else if (feature.compare("cVariable") == 0){
        toggle.cVariable = true;
    } else if (feature.compare("cUnion") == 0){
        toggle.cUnion = true;
    } else if (feature.compare("cStruct") == 0){
        toggle.cStruct = true;
    } else if (feature.compare("cFunction") == 0){
        toggle.cFunction = true;
    } else if (feature.compare("cFile") == 0){
        toggle.cFile = true;
    } else if (feature.compare("cEnum") == 0){
        toggle.cEnum = true;
    } else if (feature.compare("cClass") == 0){
        toggle.cClass = true;
    } else {
        return false;
    }

    return true;
}

/**
 * Main interface with ClangEx. Runs ClangEx on a set of
 * input files that are specified by the user.
 * @param blobMode Whether blob mode is enabled.
 * @param mergeFile Whether the user wants to merge files.
 * @param verboseMode Whether the user wants verbose output.
 * @return The success of ClangEx.
 */
bool ClangDriver::processAllFiles(bool blobMode, string mergeFile, bool verboseMode){
    bool success = true;
    llvm::cl::OptionCategory ClangExCategory("ClangEx Options");

    //Sets up the printer.
    Printer* clangPrint;
    if (verboseMode) {
        clangPrint = new VerbosePrinter();
    } else {
        clangPrint = new MinimalPrinter();
    }

    //Get the number of files.
    if (!verboseMode) static_cast<MinimalPrinter*>(clangPrint)->setNumSteps(getNumFiles() + 3);

    //Creates the command line arguments.
    int argc = 0;
    char** argv = prepareArgs(&argc);

    //Get the exclusions.
    ClangExclude exclude = toggle;

    //Sets up the processor.
    CommonOptionsParser OptionsParser(argc, (const char**) argv, ClangExCategory);
    ClangTool Tool(OptionsParser.getCompilations(),
                   OptionsParser.getSourcePathList());

    //Gets whether whether we're dealing with a merge.
    TAGraph* mergeGraph = nullptr;
    bool merge = false;
    if (mergeFile.compare("") != 0){
        //We're dealing with a merge.
        merge = true;
        clangPrint->printMerge(mergeFile);

        //Loads the file.
        TAProcessor processor = TAProcessor(INSTANCE_FLAG, clangPrint);
        bool succ = processor.readTAFile(mergeFile);

        if (!succ) {
            delete clangPrint;
            return false;
        }

        //Gets the graph.
        TAGraph* graph = processor.writeTAGraph();
        if (graph == nullptr) {
            delete clangPrint;
            return 1;
        }
    }

    ASTWalker* walker;
    if (blobMode){
        if (merge)
            walker = new BlobWalker(clangPrint, exclude, mergeGraph);
        else
            walker = new BlobWalker(clangPrint, exclude);
    } else {
        if (merge)
            walker = new PartialWalker(clangPrint, exclude, mergeGraph);
        else
            walker = new PartialWalker(clangPrint, exclude);
    }

    //Generates a matcher system.
    MatchFinder finder;

    //Next, processes the matching conditions.
    walker->generateASTMatches(&finder);

    //Runs the Clang tool.
    clangPrint->printProcessStatus(Printer::COMPILING);
    int code = Tool.run(newFrontendActionFactory(&finder).get());
    clangPrint->printFileNameDone();

    //Gets the code and checks for warnings.
    if (code != 0) {
        cerr << "Warning: Compilation errors were detected." << endl;
        success = false;
    }

    //Shifts the graphs.
    if (success) {
        walker->resolveExternalReferences();
        walker->resolveFiles();

        TAGraph *graph = walker->getGraph();
        graphs.push_back(graph);
    }

    //Clears the graph.
    files.clear();

    //Returns the success code.
    delete walker;
    return success;
}

/**
 * Outputs an individual TA model to TA format.
 * @param modelNum The number of the model to output.
 * @param fileName The filename to output as.
 * @return Boolean indicating success.
 */
bool ClangDriver::outputIndividualModel(int modelNum, string fileName){
    if (fileName.compare(string()) == 0) fileName = DEFAULT_FILENAME;

    //First, check if the number if valid.
    if (modelNum < 0 || modelNum > getNumGraphs() - 1) return false;

    int succ = outputTAString(modelNum, fileName + DEFAULT_EXT);
    if (succ == 0) {
        cerr << "Error writing to " << fileName << "!" << endl
             << "Check the file and retry!" << endl;
        return false;
    }

    return true;
}

/**
 * Outputs all models generated based on a file name.
 * @param baseFileName The base file name to output on.
 * @return A boolean indicating success.
 */
bool ClangDriver::outputAllModels(string baseFileName){
    bool succ = true;

    //Simply goes through and outputs.
    for (int i = 0; i < getNumGraphs(); i++){
        bool temp = outputIndividualModel(i, baseFileName + to_string(i));
        if (!temp) succ = false;
    }

    return succ;
}

/**
 * Adds a file/directory to the queue.
 * @param curPath The path to add.
 * @return The number of files added.
 */
int ClangDriver::addByPath(path curPath){
    int num = 0;

    //Determines what the path is.
    if (is_directory(curPath)){
        num = addDirectory(curPath);
    } else {
        num = addFile(curPath);
    }

    return num;
}

/**
 * Removes a file/directory from the queue.
 * @param curPath The path to remove.
 * @return The number of files removed.
 */
int ClangDriver::removeByPath(path curPath){
    int num = 0;

    //Determines what the path is.
    if (is_directory(curPath)){
        num = removeDirectory(curPath);
    } else {
        num = removeFile(curPath);
    }

    return num;
}

/**
 * Removes files and directories from the queue by regular expression.
 * @param regex The regular expression string to apply.
 * @return The number of files removed.
 */
int ClangDriver::removeByRegex(string regex) {
    int num = 0;
    std::regex rregex(regex);

    //Iterates through the file.
    vector<path>::iterator it;
    for (it = files.begin(); it != files.end();){
        string path = it->string();

        //Checks the regex.
        if (regex_match(path, rregex)){
            it = files.erase(it);
            num++;
        } else {
            it++;
        }
    }

    return num;
}

/**
 * Adds a file to the queue.
 * @param file The file to add.
 * @return Returns 1.
 */
int ClangDriver::addFile(path file){
    //Gets the string that is added.
    files.push_back(file);
    return 1;
}

/**
 * Recursively adds a directory to the queue.
 * @param directory The directory to add.
 * @return The number of files added.
 */
int ClangDriver::addDirectory(path directory){
    int numAdded = 0;
    vector<path> interiorDir = vector<path>();
    directory_iterator endIter;

    //Start by iterating through and inspecting each file.
    for (directory_iterator iter(directory); iter != endIter; iter++){
        //Check what the current file is.
        if (is_regular_file(iter->path())){
            //Check the extension.
            string extFile = extension(iter->path());

            //Iterates through the extension vector.
            for (int i = 0; i < ext.size(); i++){
                //Checks the file.
                if (extFile.compare(ext.at(i)) == 0){
                    numAdded += addFile(iter->path());
                }
            }
        } else if (is_directory(iter->path())){
            //Add the directory to the search system.
            interiorDir.push_back(iter->path());
        }
    }

    //Next, goes to all the internal directories.
    for (path cur : interiorDir){
        numAdded += addDirectory(cur);
    }

    return numAdded;
}

/**
 * Removes a file from the queue.
 * @param file The file to remove.
 * @return 1 if the file is removed, 0 if it wasn't found.
 */
int ClangDriver::removeFile(path file){
    file = canonical(file);

    //Check if the path exists.
    int i = 0;
    for (path curFile : files){
        curFile = canonical(curFile);
        if (curFile.compare(file.string()) == 0){
            //Remove from vector.
            files.erase(files.begin() + i);
            return 1;
        }
        i++;
    }

    return 0;
}

/**
 * Removes a directory from the queue.
 * @param directory The directory to remove.
 * @return The number removed.
 */
int ClangDriver::removeDirectory(path directory){
    int numRemoved = 0;
    vector<path> interiorDir = vector<path>();
    directory_iterator endIter;

    //Start by iterating through and inspecting each file.
    for (directory_iterator iter(directory); iter != endIter; iter++){
        //Check what the current file is.
        if (is_regular_file(iter->path())){
            //Check the extension.
            string extFile = extension(iter->path());

            //Iterates through the extension vector.
            for (int i = 0; i < ext.size(); i++){
                //Checks the file.
                if (extFile.compare(ext.at(i)) == 0){
                    numRemoved += removeFile(iter->path());
                }
            }
        } else if (is_directory(iter->path())){
            //Add the directory to the search system.
            interiorDir.push_back(iter->path());
        }
    }

    //Next, goes to all the internal directories.
    for (path cur : interiorDir){
        numRemoved += removeDirectory(cur);
    }

    return numRemoved;
}

/**
 * Gets all the enabled features.
 * @return The enabled language features.
 */
vector<string> ClangDriver::getEnabled(){
    vector<string> enabled;
    if (!toggle.cClass){
        enabled.push_back("cClass");
    }
    if (!toggle.cEnum){
        enabled.push_back("cEnum");
    }
    if (!toggle.cFile){
        enabled.push_back("cFile");
    }
    if (!toggle.cFunction){
        enabled.push_back("cFunction");
    }
    if (!toggle.cStruct){
        enabled.push_back("cStruct");
    }
    if (!toggle.cSubSystem){
        enabled.push_back("cSubSystem");
    }
    if (!toggle.cUnion){
        enabled.push_back("cUnion");
    }
    if (!toggle.cVariable){
        enabled.push_back("cVariable");
    }

    return enabled;
}

/**
 * Gets all the disabled language features.
 * @return The disabled langauge features.
 */
vector<string> ClangDriver::getDisabled(){
    vector<string> disabled;
    if (toggle.cClass){
        disabled.push_back("cClass");
    }
    if (toggle.cEnum){
        disabled.push_back("cEnum");
    }
    if (toggle.cFile){
        disabled.push_back("cFile");
    }
    if (toggle.cFunction){
        disabled.push_back("cFunction");
    }
    if (toggle.cStruct){
        disabled.push_back("cStruct");
    }
    if (toggle.cSubSystem){
        disabled.push_back("cSubSystem");
    }
    if (toggle.cUnion){
        disabled.push_back("cUnion");
    }
    if (toggle.cVariable){
        disabled.push_back("cVariable");
    }

    return disabled;
}

/**
 * Outputs a TA file to a file.
 * @param modelNum The number of the model.
 * @param fileName The file name to output.
 * @return Success or failure of the output.
 */
bool ClangDriver::outputTAString(int modelNum, string fileName){
    //First, runs the graph builder process.
    string tupleAttribute = graphs.at(modelNum)->generateTAFormat();

    //Next, writes to disk.
    std::ofstream taFile;
    taFile.open(fileName.c_str());

    //Check if the file is opened.
    if (!taFile.is_open()){
        return false;
    }

    taFile << tupleAttribute;
    taFile.close();

    return true;
}

/**
 * Generates an argv array based on the files in the queue and Clang's input format.
 * @param argc The number of tokens.
 * @return The new argv command.
 */
char** ClangDriver::prepareArgs(int *argc){
    int size = BASE_LEN + (int) files.size();

    //Sets argc.
    *argc = size;

    //Next, argv.
    char** argv = new char*[size];

    //Copies the base start.
    argv[0] = new char[DEFAULT_START.size() + 1];
    argv[1] = new char[INCLUDE_DIR_LOC.size() + 1];

    //Next, moves them over.
    strcpy(argv[0], DEFAULT_START.c_str());
    strcpy(argv[1], INCLUDE_DIR_LOC.c_str());

    //Next, loops through the files and copies.
    for (int i = 0; i < files.size(); i++){
        argv[i + BASE_LEN] = new char[files.at(i).string().size() + 1];
        strcpy(argv[i + BASE_LEN], files.at(i).c_str());
    }

    return argv;
}