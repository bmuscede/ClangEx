//
// Created by bmuscede on 16/11/16.
//

#include <tuple>
#include <iostream>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem.hpp>
#include "ClangArgParse.h"

using namespace std;
using namespace boost::filesystem;

const string ClangArgParse::OUT_LONG = "output";
const string ClangArgParse::OUT_SHORT = "o";
const string ClangArgParse::OUT_HELP = "Specifies the output file for the generated Tuple-Attribute (TA)\n"
        "file. If not specified, a default file will be used.";
const string ClangArgParse::HELP_MESSAGE = "\n---------------------------------------\n"
        "ClangEx is a modified Clang compiler used to extract essential\n"
        "program details from C/C++ source code. The program will go through\n"
        "all supplied source files and pull out essential details into a\n"
        "tuple-attribute (TA) file.";
const string ClangArgParse::FIND_LONG = "find";
const string ClangArgParse::FIND_SHORT = "f";
const string ClangArgParse::FIND_HELP = "This option causes ClangEx to automatically scan for C/C++ source\n"
        "files within a given directory. Only files listed here will be used.";
const string ClangArgParse::EXTRA_LONG = "extra-arg";
const string ClangArgParse::EXTRA_SHORT = "e";
const string ClangArgParse::EXTRA_HELP = "Allows users to pass extra compiler-based commands to the Clang\n"
        "compiler. Commands with spaces should be encapsulated with quotes.";
const string ClangArgParse::EXCLUDE_LONG = "exclude-sys";
const string ClangArgParse::EXCLUDE_SHORT = "ex";
const string ClangArgParse::EXCLUDE_HELP = "This option tells ClangEx not use Clang's system headers.\n"
        "Most projects won't build with this option.";
const string ClangArgParse::DB_LONG = "compile-db";
const string ClangArgParse::DB_SHORT = "db";
const string ClangArgParse::DB_HELP = "This tells the Clang compiler to use a prebuilt JSON database to\n"
        "build a project. This works best for large projects with lots of compiler flags.";
const string ClangArgParse::HELP_LONG = "help";
const string ClangArgParse::HELP_SHORT = "h";
const string ClangArgParse::HELP_HELP = "Display's program help.";

const string ClangArgParse::CLANG_SYS_LIB = "include";
const string ClangArgParse::INCLUDE_ERROR_MSG = "Error: Clang system libraries not found! Check if the " + CLANG_SYS_LIB +
                                             " directory exists and retry!";
const string ClangArgParse::INCLUDE_WARNING_MSG = "Warning: Clang system libraries are not being used. This may lead to "
                                             "unexpected compilation errors.";

ClangArgParse::ClangArgParse(){
    //Configures help message.
    setHelpMessage(ClangArgParse::HELP_MESSAGE);

    //Adds arguments.
    addOption(ClangArgParse::FIND_LONG, ClangArgParse::FIND_SHORT, ClangArgParse::FIND_HELP);
    addOption(ClangArgParse::EXTRA_LONG, ClangArgParse::EXTRA_SHORT, ClangArgParse::EXTRA_HELP);
    addOption(ClangArgParse::OUT_LONG, ClangArgParse::OUT_SHORT, ClangArgParse::OUT_HELP);

    //Adds flags.
    addFlag(ClangArgParse::EXCLUDE_LONG, ClangArgParse::EXCLUDE_SHORT, ClangArgParse::EXCLUDE_HELP);
    addFlag(ClangArgParse::DB_LONG, ClangArgParse::DB_SHORT, ClangArgParse::DB_HELP);
    addFlag(ClangArgParse::HELP_LONG, ClangArgParse::HELP_SHORT, ClangArgParse::HELP_HELP);

}

ClangArgParse::~ClangArgParse(){
    //Deletes the argument vectors.
    for (auto item : optionList)
        delete item;
    for (auto item : flagList)
        delete item;
}

bool ClangArgParse::addOption(string longVal, string shortVal, string description){
    //Check if the option already exists.
    for (auto item : optionList){
        if (longVal.compare(item->first.at(0)) == 0 || shortVal.compare(item->first.at(1)) == 0){
            return false;
        }
    }

    //Simply add the option to the option list.
    pair<vector<string>, vector<string>>* entry = new pair<vector<string>, vector<string>>();
    entry->first.push_back(longVal);
    entry->first.push_back(shortVal);
    entry->first.push_back(description);
    entry->second = vector<string>();

    //Adds it to the list.
    optionList.push_back(entry);

    return true;
}

bool ClangArgParse::addFlag(string longVal, string shortVal, string description){
    //Check if the option already exists.
    for (auto item : flagList){
        if (longVal.compare(item->first.at(0)) == 0 || shortVal.compare(item->first.at(1)) == 0){
            return false;
        }
    }

    //Simply add the flag to the flag list.
    pair<vector<string>, bool>* entry = new pair<vector<string>, bool>();
    entry->first.push_back(longVal);
    entry->first.push_back(shortVal);
    entry->first.push_back(description);
    entry->second = DEFAULT_FLAG;

    //Adds it to the list.
    flagList.push_back(entry);

    return true;
}

void ClangArgParse::setHelpMessage(string msg){
    //Simply sets the help message.
    helpMessage = msg;
}

string ClangArgParse::isOption(string key, bool* option){
    bool argLong = false;

    //Check if it starts with -- or -.
    string trueKey = "";
    if (boost::starts_with(key, ARG_LONG)){
        argLong = true;
        trueKey = key.substr(ARG_LONG.size(), key.size() - ARG_LONG.size());
    } else if (boost::starts_with(key, ARG_SHORT)){
        trueKey = key.substr(ARG_SHORT.size(), key.size() - ARG_SHORT.size());
    } else {
        *option = false;
        return NOT_ARG;
    }

    //Now iterate through the option system.
    *option = false;
    for (auto entry : optionList){
        //Check if our option works.
        if (trueKey.compare(entry->first.at((argLong) ? 0 : 1)) == 0){
            *option = true;
            return trueKey;
        }
    }

    return EMPTY_ARG;
}

string ClangArgParse::isFlag(string key, bool* arg){
    bool argLong = false;

    //Check if it starts with -- or -.
    string trueKey = "";
    if (boost::starts_with(key, ARG_LONG)){
        argLong = true;
        trueKey = key.substr(ARG_LONG.size(), key.size() - ARG_LONG.size());
    } else if (boost::starts_with(key, ARG_SHORT)){
        trueKey = key.substr(ARG_SHORT.size(), key.size() - ARG_SHORT.size());
    } else {
        *arg = false;
        return NOT_ARG;
    }

    //Now iterate through the option system.
    *arg = false;
    for (auto entry : flagList){
        //Check if our option works.
        if (trueKey.compare(entry->first.at((argLong) ? 0 : 1)) == 0){
            *arg = true;
            return trueKey;
        }
    }

    return EMPTY_ARG;
}

void ClangArgParse::addOptionValue(string key, string value){
    //Find key entry.
    for (auto entry : optionList){
        if (key.compare(entry->first.at(0)) == 0 || key.compare(entry->first.at(1)) == 0){
            //Add value to entry.
            entry->second.push_back(value);
        }
    }
}
void ClangArgParse::addFlagValue(string key){
    //Find key entry.
    for (auto entry : flagList){
        if (key.compare(entry->first.at(0)) == 0 || key.compare(entry->first.at(1)) == 0){
            //Add true to value.
            entry->second = true;
        }
    }
}

bool ClangArgParse::parseArguments(int argc, const char** argv){
    //Adds the filename.
    fileName = string(argv[0]);

    //Manages the arguments.
    bool optionAlert = false;
    string currentOption;
    for (int i = 1; i < argc; i++){
        string current = string(argv[i]);

        //Check if we have an option.
        if (optionAlert){
            optionAlert = false;

            //Checks if its an option/flag.
            if ((currentOption.compare(ClangArgParse::EXTRA_LONG) != 0 &&
                    currentOption.compare(ClangArgParse::EXTRA_SHORT) != 0) &&
                    (boost::starts_with(current, ARG_LONG) || boost::starts_with(current, ARG_SHORT))){
                cerr << "ERROR: Option " << currentOption << " is missing it's associated argument." << endl;
                printHelpMessage();
                return false;
            }

            //Adds the option.
            addOptionValue(currentOption, current);
            continue;
        }

        //Next, checks if we're dealing with a opt or flag.
        bool option;
        string optionCheck = isOption(current, &option);
        if (optionCheck.compare(EMPTY_ARG) != 0 && option == true){
            currentOption = optionCheck;
            optionAlert = true;
            continue;
        }

        //Check if we have a flag.
        bool flag;
        string flagCheck = isFlag(current, &flag);
        if (flagCheck.compare(EMPTY_ARG) != 0 && flag == true){
            //Sets the flag.
            addFlagValue(flagCheck);
            continue;
        }

        //Check if we have invalid input.
        if (flagCheck.compare(EMPTY_ARG) == 0 && optionCheck.compare(EMPTY_ARG) == 0 && option == false && flag == false){
            cerr << "ERROR: Invalid option/flag specified. " << current << " does not exist!" << endl;
            printHelpMessage();
            return false;
        }

        //Otherwise, adds the current string to our list of arguments.
        argumentList.push_back(current);
    }

    //Next, check if help is set.
    if (getFlag(ClangArgParse::HELP_LONG)){
        printHelpMessage();
        return false;
    }

    return true;
}

string ClangArgParse::getFilename(){
    return fileName;
}

vector<string> ClangArgParse::getOption(string key){
    for (auto entry : optionList){
        if (key.compare(entry->first.at(0)) == 0 || key.compare(entry->first.at(1)) == 0){
            return entry->second;
        }
    }

    return vector<string>();
}

bool ClangArgParse::getFlag(string key){
    for (auto entry : flagList){
        if (key.compare(entry->first.at(0)) == 0 || key.compare(entry->first.at(1)) == 0){
            return entry->second;
        }
    }

    return false;
}

vector<string> ClangArgParse::getArguments(){
    return argumentList;
}

const char** ClangArgParse::generateClangArgv(int& argc){
    //Creates a vector.
    vector<string> arguments;
    arguments.push_back(getFilename());

    //First, adds in the default include directory.
    if (!getFlag(EXCLUDE_LONG)){
        path path(system_complete(CLANG_SYS_LIB));
        if (!is_directory(path)){
            cerr << INCLUDE_ERROR_MSG << endl;
            _exit(1);
        } else {
            //Generate the command to add this library.
            arguments.push_back(ClangArgParse::ARG_LONG + ClangArgParse::EXTRA_LONG + "=-I"
                                + system_complete(CLANG_SYS_LIB).string());
        }
    } else {
        cerr << INCLUDE_WARNING_MSG << endl << endl;
    }

    //Next, adds in the extra args.
    vector<string> extraArgs = getOption(ClangArgParse::EXTRA_LONG);
    for (string entry : extraArgs){
        arguments.push_back(ClangArgParse::ARG_LONG + ClangArgParse::EXTRA_LONG + "=" + entry);
    }

    //Next, processes the find function.
    vector<string> searchDirs = getOption(ClangArgParse::FIND_LONG);
    if (searchDirs.size() > 0){
        vector<string> files;
        for (string dir : searchDirs){
            cout << "Searching automatically for C/C++ files with base directory: " << dir
                 << "..." << endl;

            files = findSourceCode(path(dir));
            cout << endl;

            arguments.insert(arguments.end(), files.begin(), files.end());
        }
    }

    //Next, adds in all files.
    arguments.insert(arguments.end(), argumentList.begin(), argumentList.end());

    //Finally, checks if we are using the "--" symbol.
    if (!getFlag(ClangArgParse::DB_LONG)){
        arguments.push_back("--");
    }

    //Convert the vector to a char**.
    char** returnArgv = new char*[(int) arguments.size()];

    //Iterate through our master vector.
    int pos = 0;
    for (string currentElement : arguments){
        //Copy in the element.
        const char* current = currentElement.c_str();
        returnArgv[pos] = new char[strlen(current) + 1];
        strcpy(returnArgv[pos], current);
        pos++;
    }

    //Set argc.
    argc = (int) arguments.size();

    return (const char**) returnArgv;
}

void ClangArgParse::printHelpMessage() {
    const string SEPARATOR = "-----------------------------------------------";

    //Generates usage flag.
    string usage = "USAGE: " + fileName;
    if (optionList.size() > 0) usage += " [OPTIONS ...]";
    if (flagList.size() > 0) usage += " [FLAGS ...]";
    usage += " [C/C++ FILES ...]";
    cout << usage << endl;

    //Prints the help message.
    cout << helpMessage << endl;

    //Prints information about the options.
    if (optionList.size() > 0){
        cout << endl << SEPARATOR << endl;
        cout << "Available Options:";

        for (auto entry : optionList){
            cout << endl << ARG_LONG << entry->first.at(0) << " [" << ARG_SHORT << entry->first.at(1) << "] [ARG] : " << endl;
            cout << entry->first.at(2) << endl;
        }
    }

    //Prints information about the flags.
    if (flagList.size() > 0){
        cout << endl << SEPARATOR << endl;
        cout << "Available Flags:";

        for (auto entry : flagList){
            cout << endl << ARG_LONG << entry->first.at(0) << " [" << ARG_SHORT << entry->first.at(1) << "] : " << endl;
            cout << entry->first.at(2) << endl;
        }
    }
}

vector<string> ClangArgParse::findSourceCode(path curr){
    vector<string> files;
    vector<path> interiorDir;
    directory_iterator endIter;

    //Start by iterating through and inspecting each file.
    for (directory_iterator iter(curr); iter != endIter; iter++){
        //Check what the current file is.
        if (is_regular_file(iter->path())){
            //Check the extension.
            string extFile = extension(iter->path());

            //Iterates through the extension vector.
            for (int i = 0; i < (sizeof(cExtensions)/sizeof(*cExtensions)); i++){
                //Checks the file.
                if (extFile.compare(cExtensions[i]) == 0){
                    files.push_back(iter->path().string());
                    cout << "Found: " << iter->path().string() << "\n";
                }
            }
        } else if (is_directory(iter->path())){
            //Add the directory to the search system.
            interiorDir.push_back(iter->path());
        }
    }

    //Next, goes through all the sub-directories.
    for (int i = 0; i < interiorDir.size(); i++){
        //Gets the path and object files.
        path current = interiorDir.at(i);
        vector<string> newObj = findSourceCode(current);

        //Adds to current vector.
        files.insert(files.end(), newObj.begin(), newObj.end());
    }

    //Return a list of object files.
    return files;
}