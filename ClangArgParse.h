//
// Created by bmuscede on 16/11/16.
//

#ifndef CLANGEX_CLANGARGPARSE_H
#define CLANGEX_CLANGARGPARSE_H

#include <cstdlib>
#include <string>
#include <vector>

class ClangArgParse {
public:
    ClangArgParse();
    ~ClangArgParse();

    bool addOption(std::string longVal, std::string shortVal, std::string description);
    bool addFlag(std::string longVal, std::string shortVal, std::string description);
    void setHelpMessage(std::string msg);

    std::string isOption(std::string key, bool* option);
    std::string isFlag(std::string key, bool* option);

    void addOptionValue(std::string key, std::string value);
    void addFlagValue(std::string key);

    bool parseArguments(int argc, const char** argv);
    const char** generateClangArgv(int& argc);

    void printHelpMessage();

    std::string getFilename();
    std::vector<std::string> getOption(std::string key);
    bool getFlag(std::string key);
    std::vector<std::string> getArguments();

    /** PUBLIC ARGUMENTS AND FLAGS */
    const static std::string OUT_LONG;
    const static std::string OUT_SHORT;
    const static std::string OUT_HELP;
private:
    const bool DEFAULT_FLAG = false;
    const std::string ARG_LONG = "--";
    const std::string ARG_SHORT = "-";
    const std::string EMPTY_ARG = "";
    const std::string NOT_ARG = "not_arg";

    /** PRIVATE ARGUMENTS AND FLAGS */
    const static std::string HELP_MESSAGE;
    const static std::string FIND_LONG;
    const static std::string FIND_SHORT;
    const static std::string FIND_HELP;
    const static std::string EXTRA_LONG;
    const static std::string EXTRA_SHORT;
    const static std::string EXTRA_HELP;
    const static std::string EXCLUDE_LONG;
    const static std::string EXCLUDE_SHORT;
    const static std::string EXCLUDE_HELP;
    const static std::string DB_LONG;
    const static std::string DB_SHORT;
    const static std::string DB_HELP;
    const static std::string HELP_LONG;
    const static std::string HELP_SHORT;
    const static std::string HELP_HELP;

    const std::string cExtensions[4] = {".c", ".cc", ".cpp", ".cxx"};

    const static std::string CLANG_SYS_LIB;
    const static std::string INCLUDE_ERROR_MSG;
    const static std::string INCLUDE_WARNING_MSG;

    std::vector<std::pair<std::vector<std::string>, std::vector<std::string>>*> optionList;
    std::vector<std::pair<std::vector<std::string>, bool>*> flagList;
    std::vector<std::string> argumentList;
    std::string helpMessage;
    std::string fileName;

    std::vector<std::string> findSourceCode(boost::filesystem::path path);
};


#endif //CLANGEX_CLANGARGPARSE_H
