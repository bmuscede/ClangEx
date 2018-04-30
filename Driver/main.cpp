/////////////////////////////////////////////////////////////////////////////////////////////////////////
// main.cpp
//
// Created By: Bryan J Muscedere
// Date: 10/09/16.
//
// Driver source code for the ClangEx program. Handles
// commands and passes it off to the ClangDriver to
// translate it into a ClangEx action. Also handles
// errors gracefully.
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

#include <fstream>
#include <iostream>
#include <pwd.h>
#include <zconf.h>
#include <vector>
#include <boost/algorithm/string/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/make_shared.hpp>
#include "ClangDriver.h"

using namespace std;
using namespace boost::filesystem;
namespace po = boost::program_options;

/** Forward Declaration */
bool processCommand(string line);

/** Commands */
const static string HELP_ARG = "help";
const static string ABOUT_ARG = "about";
const static string EXIT_ARG = "quit";
const static string GEN_ARG = "generate";
const static string OUT_ARG = "output";
const static string ADD_ARG = "add";
const static string REMOVE_ARG = "remove";
const static string LIST_ARG = "list";
const static string ENABLE_ARG = "enable";
const static string DISABLE_ARG = "disable";
const static string SCRIPT_ARG = "script";
const static string RECOVER_ARG = "recover";
const static string OLOC_ARG = "outLoc";

/** Const Strings */
const string HELP_STRING = "Commands that can be used:\n"
        "help           : Prints help information.\n"
        "about          : Prints about information.\n"
        "quit(!)        : Quits the program.\n"
        "add            : Adds files to be processed.\n"
        "remove         : Removes files from queue.\n"
        "list           : Lists the current tool state.\n"
        "enable         : Enables a collection of language features.\n"
        "disable        : Disables a collection of language features.\n"
        "generate       : Runs ClangEx on loaded files.\n"
        "output         : Outputs generated TA graphs to disk.\n"
        "recover        : Recovers a previous low-memory run.\n"
        "script         : Runs a script that handles program commands.\n"
        "outLoc         : Changes the output location for low memory mode.\n\n"
        "For more help type \"help [argument name]\" for more details.";
const string ABOUT_STRING = "ClangEx - The Fast C/C++ Extractor\n"
        "University of Waterloo, Copyright 2018\n"
        "Licensed under GNU Public License v3\n\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n\n"
        "----------------------------------------------------\n\n"
        "ClangEx is a Clang-based fact extractor designed for extracting program\n"
        "details from C/C++ source code. ClangEx is able to deal with\n"
        "basic C/C++ language features like functions, variables, classes,\n"
        "and other items. You can also run scripts to automate the extraction\n"
        "from source code.\n";

/** Command Handlers */
struct ClangExHandler {
    ClangExHandler(const std::string& name = {}, const po::options_description& desc = {})
            : name(name), desc(boost::make_shared<po::options_description>(desc)) {}

    ClangExHandler& operator=(const ClangExHandler& other) = default;

    boost::shared_ptr<po::options_description> desc;

private:
    std::string name;
};
static map<string, ClangExHandler> helpInfo;
static map<string, string> helpString;

/** Program Files */
bool changed = false;
ClangDriver driver;

/**
 * Prompts the user for a yes or no response.
 * @param promptText The text to prompt the user for.
 * @return Whether they said yes or no.
 */
bool promptForAction(string promptText){
    string result;

    bool loop = true;
    while (loop) {
        cout << promptText;
        getline(cin, result);

        //Checks the value.
        if (result.compare("Y") == 0 || result.compare("y") == 0){
            loop = false;
        } else if (result.compare("N") == 0 || result.compare("n") == 0){
            return false;
        } else {
            cout << "Invalid entry. Please type \'Y\' or \'N\'!" << endl;
        }
    }

    return true;
}

/**
 * Takes a line and splits it into a vector by the ' ' character.
 * @param line The line to tokenize.
 * @return The vector with the tokens.
 */
vector<string> tokenizeBySpace(string line){
    vector<string> result;
    istringstream iss(line);
    for(std::string s; iss >> s;)
        result.push_back(s);

    return result;
}

/**
 * Creates an argv array for use with command processing.
 * @param tokens The tokens to create.
 * @return The created char** array.
 */
char** createArgv(vector<string> tokens){
    //First, create the array.
    char** tokenC = new char*[(int) tokens.size()];
    for (int i = 0; i < tokens.size(); i++){
        tokenC[i] = new char[(int) tokens.at(i).size() + 1];
        strcpy(tokenC[i], tokens.at(i).c_str());
    }

    return tokenC;
}

/**
 * Generates the maps for use with command processing.
 * @param helpMap The map of command line options.
 * @param helpString The map with help text.
 */
void generateCommandSystem(map<string, ClangExHandler>* helpMap, map<string, string>* helpString){
    stringstream ss;

    //First, generate the help information for about.
    (*helpString)[ABOUT_ARG] = string("About Help\nUsage: " + ABOUT_ARG + "\n" "Prints information about the program including\n"
            "license information and program details.\n");

    //Next, generate the help information for exit.
    (*helpString)[EXIT_ARG] = string("Quit Help\nUsage: " + EXIT_ARG + "(!)\n" "Quits the program and returns back"
            " to the terminal.\nTyping " + EXIT_ARG + "will only quit if there are no items left to be processed.\n"
                                             "Typing " + EXIT_ARG + "! will quit the program automatically without any warning.\n");

    //Generate the help for add.
    (*helpMap)[ADD_ARG] = ClangExHandler(ADD_ARG, po::options_description("Options"));
    helpMap->at(ADD_ARG).desc->add_options()
            ("help,h", "Print help message for add.")
            ("source,s", po::value<std::vector<std::string>>(), "A file or directory to add to the current graph.");
    ss.str(string());
    ss << *helpMap->at(ADD_ARG).desc;
    (*helpString)[ADD_ARG] = string("Add Help\nUsage: " + ADD_ARG + " source\nAdds files or directories to process."
            "By adding directories, Rex will recursively\nsearch for source files starting from the root. For"
            " files\nyou can specify any file you want and Rex will add it to the project.\n\n" + ss.str());

    //Generate the help for remove.
    (*helpMap)[REMOVE_ARG] = ClangExHandler(REMOVE_ARG, po::options_description("Options"));
    helpMap->at(REMOVE_ARG).desc->add_options()
            ("help,h", "Print help message for add.")
            ("regex,r", po::value<std::string>(), "Regular expression to process.")
            ("source,s", po::value<std::vector<std::string>>(), "A file or directory to remove from the current graph.");
    ss.str(string());
    ss << *helpMap->at(REMOVE_ARG).desc;
    (*helpString)[REMOVE_ARG] = string("Remove Help\nUsage: " + REMOVE_ARG + " source\nRemoves files or directories "
            "from the processing queue. By removing directories, Rex will recursively\nsearch for files in the queue"
            " that can be removed. Individual files can also be removed\ntoo. Only files that are in the queue to"
            " begin with can be removed.\n\n" + ss.str());

    //Generates the help for script.
    (*helpMap)[SCRIPT_ARG] = ClangExHandler(SCRIPT_ARG, po::options_description("Options"));
    helpMap->at(SCRIPT_ARG).desc->add_options()
            ("help,h", "Print help message for list.")
            ("script,s", po::value<std::string>(), "The script file to run.");
    ss.str(string());
    ss << *helpMap->at(SCRIPT_ARG).desc;
    (*helpString)[SCRIPT_ARG] = string("Script Help\nUsage: " + SCRIPT_ARG + " script\nRuns a script that can "
            "handle any of the commands in this program automatically.\nThe script will terminate when it reaches"
            " the end of the script or hits quit.\n\n" + ss.str());

    //Generates the help for list.
    (*helpMap)[LIST_ARG] = ClangExHandler(LIST_ARG, po::options_description("Options"));
    helpMap->at(LIST_ARG).desc->add_options()
            ("help,h", "Print help message for list.")
            ("num-graphs,g", "Prints the number of graphs already generated.")
            ("files,f", "Lists all the files for the current input.")
            ("toggle,t", "Lists the language features enabled/disabled.");
    ss.str(string());
    ss << *helpMap->at(LIST_ARG).desc;
    (*helpString)[LIST_ARG] = string("List Help\nUsage: " + LIST_ARG + " [options]\nLists information about the "
            "current state of Rex. This includes the number\nof graphs currently generated and all the files\nbeing"
            " processed for the next graph.\nOnly files that are in the queue are listed.\n\n" + ss.str());

    //Next, generate the help information for enable.
    (*helpString)[ENABLE_ARG] = string("Enable Help\nUsage: " + ENABLE_ARG + " [LANGUAGE_FEATURES...]\n"
            "Enables language features that will be outputted when ClangEx is run.\nThese features will be enabled until"
            "disabled or the program is exited.\nCurrent language features available:\n" + driver.getLanguageString());

    //Next, generate the help information for disable.
    (*helpString)[DISABLE_ARG] = string("Disable Help\nUsage: " + DISABLE_ARG + " [LANGUAGE_FEATURES...]\n"
            "Disables language features that will not be outputted when ClangEx is run.\nThese features will be disabled until"
            "enabled.\nCurrent language features available:\n" + driver.getLanguageString());

    //Generate the help for generate.
    (*helpMap)[GEN_ARG] = ClangExHandler(GEN_ARG, po::options_description("Options"));
    helpMap->at(GEN_ARG).desc->add_options()
            ("help,h", "Print help message for generate.")
            ("blob,b", "Runs ClangEx in blob mode.")
            ("low,l", "Enables low-memory mode.")
            ("initial,i", po::value<std::string>(), "An initial TA file to load in to merge.");
    ss.str(string());
    ss << *helpMap->at(GEN_ARG).desc;
    (*helpString)[GEN_ARG] = string("Generate Help\nUsage: " + GEN_ARG + " [options]\nGenerates a graph based on the supplied"
            " C/C++ source files.\nYou must have at least 1 source file in the queue for the graph to be generated.\n"
            "Additionally, in the root directory, there must a \"compile_commands.json\" file.\n\n" + ss.str());

    //Generate the help for recover.
    (*helpMap)[RECOVER_ARG] = ClangExHandler(RECOVER_ARG, po::options_description("Options"));
    helpMap->at(RECOVER_ARG).desc->add_options()
            ("help,h", "Print help message for recover.")
            ("compact,c", "Just finishes the current TA model.")
            ("initial,i", po::value<std::string>(), "The initial location.");
    ss.str(string());
    ss << *helpMap->at(RECOVER_ARG).desc;
    (*helpString)[RECOVER_ARG] = string("Recover Help\nUsage: " + RECOVER_ARG + " [options]\nRecovers a TA file that"
            " was generated from a previous ClangEx run.\nThere must be at least 1 TA file in directory to do this.\n"
            "Additionally, you can either output as is or re-run the generate command.\n\n" + ss.str());

    //Generate the help for output.
    (*helpMap)[OUT_ARG] = ClangExHandler(OUT_ARG, po::options_description("Options"));
    helpMap->at(OUT_ARG).desc->add_options()
            ("help,h", "Print help message for output.")
            ("select,s", po::value<std::string>(), "Only outputs select graphs based on their number.")
            ("outputFile", po::value<std::vector<std::string>>(), "The base file name to save.");
    ss.str(string());
    ss << *helpMap->at(OUT_ARG).desc;
    (*helpString)[OUT_ARG] = string("Output Help\nUsage: " + OUT_ARG + " [options] outputFile\nOutputs the generated"
            " graphs to a tuple-attribute (TA) file based on the\nClangEx schema. These models can then be used"
            " by other programs.\n\n" + ss.str());

    //Generate the help for outLoc.
    (*helpMap)[OLOC_ARG] = ClangExHandler(OLOC_ARG, po::options_description("Options"));
    helpMap->at(OLOC_ARG).desc->add_options()
            ("help,h", "Print help message for output.")
            ("outputDir,o", po::value<std::vector<std::string>>(), "The output directory for low memory graph.");
    ss.str(string());
    ss << *helpMap->at(OLOC_ARG).desc;
    (*helpString)[OLOC_ARG] = string("Output Help\nUsage: " + OLOC_ARG + " [options] outputFile\nOutputs the generated"
            " graphs to a tuple-attribute (TA) file based on the\nClangEx schema. These models can then be used"
            " by other programs.\n\n" + ss.str());
}

/**
 * Prints the header of the program.
 */
void printHeader(){
    cout << " _____ _                   _____\n/  __ \\ |                 |  ___|\n| /  \\/ | __ _ _ __   __ _| |____  "
            "__\n| |   | |/ _` | '_ \\ / _` |  __\\ \\/ /\n| \\__/\\ | (_| | | | | (_| | |___>  <\n \\____/_|\\__,_|_| "
            "|_|\\__, \\____/_/\\_\\\n                      __/ |\n                     |___/" << endl;
    cout << "       The Fast C/C++ Extractor" << endl << endl;
    cout << "Type \'help\' for more information." << endl;
}

/**
 * Processes the help option.
 * @param line The command line in its entirety.
 * @param messages All help messages.
 */
void processHelp(string line, map<string, string> messages){
    auto tokens = tokenizeBySpace(line);

    //Check if the line is empty.
    if (tokens.size() == 1){
        cout << HELP_STRING << endl;
    } else if (tokens.size() == 2){
        //Check if the key exists.
        if (messages.find(tokens.at(1)) == messages.end()){
            cerr << "The token \"" + tokens.at(1) + "\" is not a valid command!" << endl << endl
                 << HELP_STRING << endl;
        } else {
            cout << messages.at(tokens.at(1));
        }
    } else {
        cerr << "Error: The help command must be either typed as \"help\" or as \"help [argument name]\"" << endl;
    }
}

/**
 * Processes the about option.
 */
void processAbout(){
    cout << ABOUT_STRING;
}

/**
 * Processes the quit option.
 * @param line The line typed by the user. (Sees if ! was appended to quit.)
 * @return Whether the user wants to quit or not.
 */
bool processQuit(string line){
    //Checks for the ! at the end.
    if (line.at(line.size() - 1) == '!' || changed == false) return false;

    //Ask the user if they want to act.
    bool result = promptForAction("There are still items to be processed. Are you sure you want to quit (Y/N): ");
    return !result;
}

/**
 * Processes the add option. Adds files and directories.
 * @param line The line entered by the user.
 */
void processAdd(string line){
    //Tokenize by space.
    vector<string> tokens = tokenizeBySpace(line);

    //Next, we check for errors.
    if (tokens.size() == 1) {
        cerr << "Error: You must include at least one file or directory to process." << endl;
        return;
    }

    //Next, we loop through to add these files.
    for (int i = 1; i < tokens.size(); i++){
        path curPath = tokens.at((unsigned int) i);

        //Check if the element exists.
        if (!exists(curPath)){
            bool result = promptForAction("Warning: File does not exist!\nDo you still want to add it (Y/N): ");
            if (!result) continue;
        }

        int numAdded = driver.addByPath(curPath);

        //Checks whether the system is a directory.
        changed = true;
        changed = true;
        if (is_directory(curPath)){
            cout << numAdded << " source files were added from the directory " << curPath.filename() << "!" << endl;
        } else {
            cout << "Added C++ file " << curPath.filename() << "!" << endl;
        }
    }
}

/**
 * Processes the remove option. Removes files and directories based on source or regex.
 * @param line The line entered.
 * @param desc The configured program options for this.
 */
void processRemove(string line, po::options_description desc){
    //Tokenize by space.
    vector<string> tokens = tokenizeBySpace(line);

    //Generates the arguments.
    char** argv = createArgv(tokens);
    int argc = (int) tokens.size();

    //Processes the command line args.
    po::positional_options_description positionalOptions;
    positionalOptions.add("source", -1);

    po::variables_map vm;
    bool regex = false;
    try {
        po::store(po::parse_command_line(argc, (const char *const *) argv, desc), vm);

        if (vm.count("regex")){
            regex = true;
        }

        //Check for processing errors.
        if (vm.count("source") && vm.count("regex")){
            throw po::error("The --source and --regex options cannot be used together!");
        }
        if (!vm.count("source") && !regex) {
            throw po::error("You must include at least one file or directory to remove from.");
        }
    } catch(po::error& e) {
        cerr << "Error: " << e.what() << endl;
        cerr << desc;
        for (int i = 0; i < argc; i++) delete[] argv[i];
        return;
    }

    //Checks what operation to perform.
    if (!regex){
        vector<string> removeFiles = vm["source"].as<vector<string>>();

        //Next, we loop through to remove files.
        for (int i = 1; i < removeFiles.size(); i++){
            path curPath = removeFiles.at((unsigned int) i);

            int numRemoved = driver.removeByPath(curPath);
            if (!is_directory(curPath) && numRemoved == 0){
                cerr << "The file " << curPath.filename() << " is not in the list." << endl;
            } else if (is_directory(curPath)){
                cout << numRemoved << " files have been removed for directory " << curPath.filename() << "!" << endl;
            } else {
                cout << "The file " << curPath.filename() << " has been removed!" << endl;
            }
        }
    } else {
        string regexString = vm["regex"].as<std::string>();

        //Simply remove by regex.
        int numRemoved = driver.removeByRegex(regexString);
        if (!numRemoved){
            cerr << "The regular expression " << regexString << " did not match any files." << endl;
        } else {
            cout << "The regular expression " << regexString << " removed " << numRemoved << " file(s)." << endl;
        }
    }

    changed = true;
    for (int i = 0; i < argc; i++) delete[] argv[i];
}

/**
 * Processes the list option. Shows the status of the program.
 * @param line The line entered.
 * @param desc The options configured.
 */
void processList(string line, po::options_description desc){
    bool listAll = true;
    bool listGraphs = false;
    bool listFiles = false;
    bool listToggle = false;

    //First we tokenize.
    vector<string> tokens = tokenizeBySpace(line);

    //Next, we split into arguments.
    char** argv = createArgv(tokens);
    int argc = (int) tokens.size();

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, (const char *const *) argv, desc), vm);

        //Checks if help was enabled.
        if (vm.count("help")) {
            cout << "Usage: list [options]" << endl << desc;
            for (int i = 0; i < argc; i++) delete[] argv[i];
            return;
        }

        //Sets up what's getting listed.
        if (vm.count("num-graphs")){
            listAll = false;
            listGraphs = true;
        }
        if (vm.count("files")){
            listAll = false;
            listFiles = true;
        }
        if (vm.count("toggle")){
            listAll = false;
            listToggle = true;
        }
    } catch(po::error& e) {
        cerr << "Error: " << e.what() << endl;
        cerr << desc;
        for (int i = 0; i < argc; i++) delete[] argv[i];
        return;
    }

    string listItem = "";
    if (listAll){
        listItem = driver.printStatus(true, true, true);
    } else {
        listItem = driver.printStatus(listFiles, listGraphs, listToggle);
    }
    cout << listItem;

    for (int i = 0; i < argc; i++) delete[] argv[i];
}

/**
 * Processes the enable option. Enables language features.
 * @param line The line entered.
 */
void processEnable(string line){
    //First we tokenize.
    vector<string> tokens = tokenizeBySpace(line);

    //Check the size.
    if (tokens.size() == 1){
        cerr << "Error: You must specify at least one language feature to enable.\nAvailable language features:\n"
             << driver.getLanguageString();
        return;
    }

    //Handles each.
    for (int i = 1; i < tokens.size(); i++){
        bool succ = driver.enableFeature(tokens.at(i));

        if (!succ){
            cerr << "Error: The token " << tokens.at(i) << " is not a valid language feature. Ignored..." << endl;
        }
    }
}

/**
 * Processes the disable option. Disables language features.
 * @param line
 */
void processDisable(string line){
    //First we tokenize.
    vector<string> tokens = tokenizeBySpace(line);

    //Check the size.
    if (tokens.size() == 1){
        cerr << "Error: You must specify at least one language feature to disable.\nAvailable language features:\n"
             << driver.getLanguageString();
        return;
    }

    //Handles each.
    for (int i = 1; i < tokens.size(); i++){
        bool succ = driver.disableFeature(tokens.at(i));

        if (!succ){
            cerr << "Error: The token " << tokens.at(i) << " is not a valid language feature. Ignored..." << endl;
        }
    }
}

/**
 * Processes the generate option. Generates a TA file for the user.
 * @param line The line entered.
 * @param desc The options configured.
 */
void processGenerate(string line, po::options_description desc){
    //Generates the arguments.
    vector<string> tokens = tokenizeBySpace(line);
    char** argv = createArgv(tokens);
    int argc = (int) tokens.size();

    bool blobMode = false;
    string mergeFile = "";
    bool lowMemory = false;
    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, (const char *const *) argv, desc), vm);

        //Checks if help was enabled.
        if (vm.count("help")) {
            cout << "Usage: generate [options]" << endl << desc;
            for (int i = 0; i < argc; i++) delete[] argv[i];
            return;
        }

        //Sets up what's getting listed.
        if (vm.count("blob")){
            blobMode = true;
        }
        if (vm.count("initial")){
            mergeFile = vm["initial"].as<std::string>();
        }
        if (vm.count("low")){
            lowMemory = true;
        }
    } catch(po::error& e) {
        cerr << "Error: " << e.what() << endl;
        cerr << desc;
        for (int i = 0; i < argc; i++) delete[] argv[i];
        return;
    }

    //Checks whether we can generate.
    int numFiles = driver.getNumFiles();
    if (numFiles == 0) {
        cerr << "No files are in the queue to be processed. Add some before you continue." << endl;
        for (int i = 0; i < argc; i++) delete[] argv[i];
        return;
    }

    //Next, tells ClangEx to generate them.
    cout << "Processing " << numFiles << " file(s)..." << endl << "This may take some time!" << endl << endl;
    bool success = driver.processAllFiles(blobMode, mergeFile, lowMemory);

    //Checks the success of the operation.
    if (success) {
        cout << "ClangEx contribution graph was created successfully!" << endl
             << "Graph number is #" << driver.getNumGraphs() - 1 << "." << endl;
        changed = true;
    }

    for (int i = 0; i < argc; i++) delete[] argv[i];
    delete[] argv;
}

/**
 * Processes the output option.
 * @param line The line entered.
 * @param desc The options configured.
 */
void processOutput(string line, po::options_description desc){
    //Generates the arguments.
    vector<string> tokens = tokenizeBySpace(line);
    char** argv = createArgv(tokens);
    int argc = (int) tokens.size();

    string outputValues = string();;
    vector<int> outputIndex;

    //Processes the command line args.
    po::positional_options_description positionalOptions;
    positionalOptions.add("outputFile", 1);

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, (const char* const*) argv).options(desc)
                          .positional(positionalOptions).run(), vm);
        po::notify(vm);

        //Checks if help was enabled.
        if (vm.count("help")){
            cout << "Usage: output [options] OutputName" << endl << desc;
            for (int i = 0; i < argc; i++) delete[] argv[i];
            delete[] argv;
            return;
        }

        //Check the number of graphs.
        if (driver.getNumGraphs() == 0){
            cerr << "Error: There are no graphs to output!" << endl;
            for (int i = 0; i < argc; i++) delete[] argv[i];
            delete[] argv;
            return;
        }

        //Checks if the user specified an output directory.
        if (!vm.count("outputFile")){
            throw po::error("You must specify an output base name!");
        }

        //Checks if selective output was enabled.
        if (vm.count("select")){
            outputValues = vm["select"].as<std::string>();

            //Now, parses the values.
            stringstream ss(outputValues);
            int i;
            while (ss >> i){
                outputIndex.push_back(i);
            }

            //Check for validity.
            if (outputIndex.size() == 0){
                throw po::error("Format the --select argument as 1,2,3,5.");
            }
        }

        po::notify(vm);
    } catch(po::error& e) {
        cerr << "Error: " << e.what() << endl;
        cerr << desc;
        for (int i = 0; i < argc; i++) delete[] argv[i];
        delete[] argv;
        return;
    }

    vector<string> outputVec = vm["outputFile"].as<std::vector<std::string>>();
    string output = outputVec.at(0);
    bool success = false;

    //Now, outputs the graphs.
    if (outputValues.compare(string()) == 0){
        //We output all the graphs.
        if (driver.getNumGraphs() == 1){
            success = driver.outputIndividualModel(0, output);
        } else {
            success = driver.outputAllModels(output);
        }
    } else {
        //We selectively output the graphs.
        for (int indexNum : outputIndex){
            if (indexNum < 0 || indexNum >= driver.getNumGraphs()){
                cerr << "Error: There are only " << driver.getNumGraphs()
                     << " graphs! " << indexNum << " is out of bounds." << endl;
                for (int i = 0; i < argc; i++) delete[] argv[i];
                delete[] argv;
                return;
            }

            success = driver.outputIndividualModel(indexNum, output);
        }
    }

    if (!success) {
        cerr << "There was an error outputting all graphs to TA models." << endl;
    } else {
        cout << "Contribution networks created successfully." << endl;
        changed = false;
    }

    for (int i = 0; i < argc; i++) delete[] argv[i];
    delete[] argv;
}

/**
 * Processes the script option. Runs a script on the program.
 * @param line The line entered.
 * @param desc The program options.
 */
void processScript(string line, po::options_description desc){
    //First we tokenize.
    vector<string> tokens = tokenizeBySpace(line);

    //Generates the arguments.
    char** argv = createArgv(tokens);
    int argc = (int) tokens.size();

    //Processes the command line args.
    po::positional_options_description positionalOptions;
    positionalOptions.add("script", 1);

    po::variables_map vm;
    string filename = "";
    try {
        po::store(po::command_line_parser(argc, (const char* const*) argv).options(desc)
                          .positional(positionalOptions).run(), vm);
        po::notify(vm);

        if (vm.count("help")) {
            cout << "Usage: script [script]" << endl << desc;
            for (int i = 0; i < argc; i++) delete[] argv[i];
            delete[] argv;
            return;
        }

        if (!vm.count("script")) throw po::error("No script file was supplied!");
        filename = vm["script"].as<std::string>();
    } catch(po::error& e) {
        cerr << "Error: " << e.what() << endl;
        cerr << desc;
        for (int i = 0; i < argc; i++) delete[] argv[i];
        delete[] argv;
        return;
    }

    //Process the filename.
    std::ifstream scriptFile;
    scriptFile.open(filename);

    //Loop until we hit eof.
    string curLine;
    bool continueLoop = true;
    while(!scriptFile.eof() && continueLoop) {
        getline(scriptFile, curLine);
        continueLoop = !processCommand(curLine);
    }

    scriptFile.close();
    if (!continueLoop){
        for (int i = 0; i < argc; i++) delete[] argv[i];
        delete[] argv;
        _exit(1);
    }
    for (int i = 0; i < argc; i++) delete[] argv[i];
    delete[] argv;
}

void processRecover(string line, po::options_description desc){
    //Generates the arguments.
    vector<string> tokens = tokenizeBySpace(line);
    char** argv = createArgv(tokens);
    int argc = (int) tokens.size();

    //Processes the command line args.
    po::variables_map vm;
    string initial = ".";
    bool compact = false;
    try {
        po::store(po::command_line_parser(argc, (const char* const*) argv).options(desc)
                          .run(), vm);
        po::notify(vm);

        if (vm.count("help")) {
            cout << "Usage: recover [options]" << endl << desc;
            for (int i = 0; i < argc; i++) delete[] argv[i];
            delete[] argv;
            return;
        }

        if (vm.count("compact")){
            compact = true;
        }

        if (vm.count("initial")){
            initial = vm["initial"].as<std::string>();
        }
    } catch(po::error& e) {
        cerr << "Error: " << e.what() << endl;
        cerr << desc;
        for (int i = 0; i < argc; i++) delete[] argv[i];
        delete[] argv;
        return;
    }

    //Next, prepares the recovery system.
    if (compact){
        driver.recoverCompact(initial);
    } else {
        driver.recoverFull(initial);
    }
}

void processOutputLoc(string line, po::options_description desc){
    //Tokenize by space.
    vector<string> tokens = tokenizeBySpace(line);

    //Next, we check for errors.
    if (driver.getNumGraphs() > 0) {
        //TODO: Move graphs in processing.
        cerr << "Error: You cannot change the current low memory graph output location now." << endl;
        return;
    } else if (tokens.size() != 2) {
        cerr << "Error: You must include at least one file or directory to process." << endl;
        return;
    }

    //Now we get the directory.
    string directory = tokens.at(1);
    bool status = driver.changeLowMemoryLoc(directory);

    if (!status){
        cerr << "Error: Graph location did not change! Please supply a different filename." << endl;
    } else {
        cout << "Low memory graph location successfully changed to " << directory << "!" << endl;
    }
}

/**
 * Processes an individual command.
 * @param line The line entered.
 * @return Whether the program is quit or not.
 */
bool processCommand(string line) {
    if (line.compare("") == 0) return true;

    //Checks the commands
    if (!line.compare(0, HELP_ARG.size(), HELP_ARG) &&
        (line[HELP_ARG.size()] == ' ' || line.size() == HELP_ARG.size())) {
        processHelp(line, helpString);
    } else if (!line.compare(0, ABOUT_ARG.size(), ABOUT_ARG) &&
               (line[ABOUT_ARG.size()] == ' ' || line.size() == ABOUT_ARG.size())) {
        processAbout();
    } else if (!line.compare(0, EXIT_ARG.size(), EXIT_ARG) &&
               (line[EXIT_ARG.size()] == ' ' || line.size() == EXIT_ARG.size())) {
        return processQuit(line);
    } else if (!line.compare(0, ADD_ARG.size(), ADD_ARG) &&
               (line[ADD_ARG.size()] == ' ' || line.size() == ADD_ARG.size())) {
        processAdd(line);
    } else if (!line.compare(0, REMOVE_ARG.size(), REMOVE_ARG) &&
               (line[REMOVE_ARG.size()] == ' ' || line.size() == REMOVE_ARG.size())) {
        processRemove(line, *(helpInfo.at(REMOVE_ARG).desc.get()));
    } else if (!line.compare(0, LIST_ARG.size(), LIST_ARG) &&
               (line[LIST_ARG.size()] == ' ' || line.size() == LIST_ARG.size())) {
        processList(line, *(helpInfo.at(LIST_ARG).desc.get()));
    } else if (!line.compare(0, ENABLE_ARG.size(), ENABLE_ARG) &&
               (line[ENABLE_ARG.size()] == ' ' || line.size() == ENABLE_ARG.size())) {
        processEnable(line);
    } else if (!line.compare(0, DISABLE_ARG.size(), DISABLE_ARG) &&
               (line[DISABLE_ARG.size()] == ' ' || line.size() == DISABLE_ARG.size())) {
        processDisable(line);
    } else if (!line.compare(0, GEN_ARG.size(), GEN_ARG) &&
               (line[GEN_ARG.size()] == ' ' || line.size() == GEN_ARG.size())) {
        processGenerate(line, *(helpInfo.at(GEN_ARG).desc.get()));
    } else if (!line.compare(0, OUT_ARG.size(), OUT_ARG) &&
               (line[OUT_ARG.size()] == ' ' || line.size() == OUT_ARG.size())) {
        processOutput(line, *(helpInfo.at(OUT_ARG).desc.get()));
    } else if (!line.compare(0, SCRIPT_ARG.size(), SCRIPT_ARG) &&
               (line[SCRIPT_ARG.size()] == ' ' || line.size() == SCRIPT_ARG.size())) {
        processScript(line, *(helpInfo.at(SCRIPT_ARG).desc.get()));
    } else if (!line.compare(0, RECOVER_ARG.size(), RECOVER_ARG) &&
               (line[RECOVER_ARG.size()] == ' ' || line.size() == RECOVER_ARG.size())) {
        processRecover(line, *(helpInfo.at(RECOVER_ARG).desc.get()));
    } else if (!line.compare(0, OLOC_ARG.size(), OLOC_ARG) &&
               (line[OLOC_ARG.size()] == ' ' || line.size() == OLOC_ARG.size())) {
        processOutputLoc(line, *(helpInfo.at(OLOC_ARG).desc.get()));
    } else {
        cerr << "No such command: " << line << "\nType \'help\' for more information." << endl;
    }

    return true;
}

/**
 * The main method. Drives the entire program.
 * @param argc The number of arguments.
 * @param argv The arguments.
 * @return Status code.
 */
int main(int argc, const char **argv){
    //Starts by printing the header.
    printHeader();

    //Checks if we have arguments.
    if (argc > 1){
        cerr << "Run " << argv[0] << " with no arguments!" << endl;
        return 1;
    }

    //Gets the username.
    string username;
    try {
        struct passwd *pass;
        pass = getpwuid(getuid());
        username = pass->pw_name;
    } catch(exception& e){
        //If the lookup fails.
        username = "user";
    }

    //Initializes the help system.
    generateCommandSystem(&helpInfo, &helpString);

    //Initiates main loop.
    string line;
    bool contLoop = true;
    while(contLoop){
        cout << username << " > ";
        getline(cin, line);

        //Processes the command.
        contLoop = processCommand(line);
    }

    return 0;
}