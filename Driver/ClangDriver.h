//
// Created by bmuscede on 05/09/17.
//

#ifndef CLANGEX_CLANGDRIVER_H
#define CLANGEX_CLANGDRIVER_H

#include <vector>
#include <boost/filesystem.hpp>
#include "../Graph/TAGraph.h"

using namespace boost::filesystem;

class ClangDriver {
public:
    ClangDriver();
    ~ClangDriver();

    std::string printStatus(bool files, bool ta, bool toggle);
    std::string getLanguageString();

    int getNumGraphs();
    int getNumFiles();

    bool enableFeature(std::string feature);
    bool disableFeature(std::string feature);

    bool processAllFiles(bool blobMode, std::string mergeFile, bool verboseMode);

    /** Output Helpers */
    bool outputIndividualModel(int modelNum, std::string fileName = std::string());
    bool outputAllModels(std::string baseFileName);

    int addByPath(path curPath);
    int removeByPath(path curPath);
    int removeByRegex(std::string regex);

    /** Toggle System */
    typedef struct {
        bool cSubSystem = false;
        bool cFile = false;
        bool cClass = false;
        bool cFunction = false;
        bool cVariable = false;
        bool cEnum = false;
        bool cStruct = false;
        bool cUnion = false;
    } ClangExclude;
private:
    /** Default Arguments */
    const std::string INSTANCE_FLAG = "$INSTANCE";
    const std::string DEFAULT_EXT = ".ta";
    const std::string DEFAULT_FILENAME = "out";
    const std::string DEFAULT_START = "./ClangEx";
    const std::string INCLUDE_DIR = "./include";
    const std::string INCLUDE_DIR_LOC = "--extra-arg=-I" + INCLUDE_DIR;
    const int BASE_LEN = 2;

    /** Private Variables */
    std::vector<TAGraph*> graphs;
    std::vector<path> files;
    std::vector<std::string> ext;

    /** Toggle System */
    std::string langString = "\tcSubSystem\n\tcFile\n\tcClass\n\tcFunction\n\tcVariable\n\tcEnum\n\tcStruct\n\tcUnion\n";
    ClangExclude toggle;

    /** C/C++ Extensions */
    const std::string C_FILE_EXT = ".c";
    const std::string CPLUS_FILE_EXT = ".cc";
    const std::string CPLUSPLUS_FILE_EXT = ".cpp";

    /** Add/Remove Helper Methods */
    int addFile(path file);
    int addDirectory(path directory);
    int removeFile(path file);
    int removeDirectory(path directory);

    /** Enabled Strings */
    std::vector<std::string> getEnabled();
    std::vector<std::string> getDisabled();

    /** Output Helper Method */
    bool outputTAString(int modelNum, std::string fileName);

    /** Argument Helpers */
    char** prepareArgs(int *argc);
};


#endif //CLANGEX_CLANGDRIVER_H
