#include <iostream>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem.hpp>
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "ASTWalker.h"
#include "ClangArgParse.h"

using namespace std;
using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;
using namespace llvm;
using namespace boost::filesystem;

static const string PLACEHOLDER = "PLACEHOLDER";
static const string DEFAULT_OUT = "a.out";
static const string CLANG_SYS_LIB = "include";

static const string INCLUDE_ERROR_MSG = "Error: Clang system libraries not found! Check if the " + CLANG_SYS_LIB +
                                        " directory exists and retry!";
static const string INCLUDE_WARNING_MSG = "Warning: Clang system libraries are not being used. This may lead to "
                                          "unexpected compilation errors.";

static const string HELP_MESSAGE = "---------------------------------------\n\n"
                                   "ClangEx is a modified Clang compiler used to extract essential\n"
                                   "program details from C/C++ source code. The program will go through\n"
                                   "all supplied source files and pull out essential details into a\n"
                                   "tuple-attribute (TA) file.\n";
static const string OUTPUT_HELP = "Specifies the output file for the generated Tuple-Attribute\n"
                                  "file. If no filename is specified, a Tuple-Attribute file\n"
                                  "will be created with the filename " + DEFAULT_OUT;
static const string EXCLUDE_HELP = "This option tells ClangEx not use Clang's system headers.\n"
                                  "Most projects won't build with this option.";
static const string FIND_SOURCE_HELP = "This option causes ClangEx to automatically scan for C/C++ source\n"
                                       "files within a given directory. Only files listed here will be used.";

static llvm::cl::OptionCategory ClangExCategory("ClangEx Options");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp ClangExHelp(HELP_MESSAGE.c_str());

static cl::opt<string> TAOutCommand("out", cl::cat(ClangExCategory));
static cl::opt<bool> TAExcludeCommand("exclude", cl::cat(ClangExCategory));
static cl::opt<string> SourceFindCommand("find", cl::cat(ClangExCategory));

static const string cExtensions[4] = {".c", ".cc", ".cpp", ".cxx"};

const char** injectPlaceholder(int argc, const char** argv){
    //Creates a new argv.
    char** newArgv = new char*[argc + 1];

    //Copies over.
    bool inserted = false;
    for (int i = 0; i < argc; i++){
        if (string(argv[i]).compare("--") == 0){
            //Insert the placeholder.
            const char* current = PLACEHOLDER.c_str();
            newArgv[i] = new char[strlen(current) + 1];
            strcpy(newArgv[i], current);

            //Notifies of insertion.
            inserted = true;
        }

        //Inserts the current char.
        const char* current = argv[i];
        newArgv[(inserted ? i + 1 : i)] = new char[strlen(current) + 1];
        strcpy(newArgv[(inserted ? i + 1 : i)], current);
    }

    //Check if we inserted.
    if (!inserted){
        //Insert the placeholder.
        const char* current = PLACEHOLDER.c_str();
        newArgv[argc] = new char[strlen(current) + 1];
        strcpy(newArgv[argc], current);
    }

    return (const char**) newArgv;
}

vector<string> findSourceCode(path curr){
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

const char** generateArguments(const char** oldArgv, int argc, vector<string> args, vector<string> files){
    //Check if we have empty args and files.
    if (args.size() == 0 && files.size() == 0) return oldArgv;

    //Converts argv to vector.
    vector<string> argVector;
    bool inserted = false;
    for (int i = 0; i < argc; i++){
        //Push the argument.
        argVector.push_back(string(oldArgv[i]));
        if (i == argc - 1) break;

        //See where we are.
        if (i == 0){
            //Copy the argument vector.
            argVector.insert(argVector.end(), args.begin(), args.end());
        } else if (string(oldArgv[i + 1]).compare("--") == 0){
            //Copy the file vector.
            argVector.insert(argVector.end(), files.begin(), files.end());
            inserted = true;
        }
    }

    //Check if we added the files.
    if (!inserted) argVector.insert(argVector.end(), files.begin(), files.end());

    //Next, generates a new argv.
    char** returnArgv = new char*[(int) argVector.size()];

    //Iterate through our master vector.
    int pos = 0;
    for (string currentElement : argVector){
        //Copy in the element.
        const char* current = currentElement.c_str();
        returnArgv[pos] = new char[strlen(current) + 1];
        strcpy(returnArgv[pos], current);
        pos++;
    }

    return (const char**) returnArgv;
}

int main(int argc, const char **argv) {
    //Starts by creating an argument parser.
    ClangArgParse parser = ClangArgParse();
    bool succ = parser.parseArguments(argc, argv);

    //Check return code.
    if (!succ) return 1;

    //Generates a specialized version of arguments for Clang.
    int genArgC = 0;
    const char** genArgV = parser.generateClangArgv(genArgC);

    //Now, runs Clang.
    CommonOptionsParser OptionsParser(genArgC, genArgV, ClangExCategory);
    ClangTool tool(OptionsParser.getCompilations(),
                   OptionsParser.getSourcePathList());

    //Generates a matcher system.
    ASTWalker walker = ASTWalker();
    MatchFinder finder;

    //Next, processes the matching conditions.
    walker.generateASTMatches(&finder);

    //Runs the Clang tool.
    cout << "Compiling the source code..." << endl;
    int code = tool.run(newFrontendActionFactory(&finder).get());
    if (code != 0) {
        return code;
    }

    //Resolves references.
    cout << "Resolving external references..." << endl;
    walker.resolveExternalReferences();

    //Generates file paths.
    walker.resolveFiles();

    //Processes the TA file.
    vector<string> outputFiles = parser.getOption(ClangArgParse::OUT_LONG);
    if (outputFiles.size() == 0){
        walker.buildGraph(DEFAULT_OUT);
    } else {
        for (string file : outputFiles)
            walker.buildGraph(file);
    }
    return code;

    /* OLD CLANG MANAGEMENT
    //Sets up command line options.
    TAOutCommand.setInitialValue(DEFAULT_OUT);
    TAOutCommand.setDescription(OUTPUT_HELP);
    TAExcludeCommand.setInitialValue(false);
    TAExcludeCommand.setDescription(EXCLUDE_HELP);
    SourceFindCommand.setInitialValue("");
    SourceFindCommand.setDescription(FIND_SOURCE_HELP);

    //Develops an option parser for Clang.
    int fakeArgc = argc + 1;
    CommonOptionsParser OptionsParser(fakeArgc, injectPlaceholder(argc, argv), ClangExCategory);

    //Create a vector of arguments.
    vector<string> arguments;

    //Gets the Clang command for output (if specified).
    string output = TAOutCommand.getValue();

    //Check if the include directory is found.
    if (!TAExcludeCommand.getValue()){
        path path(system_complete(CLANG_SYS_LIB));
        if (!is_directory(path)){
            cerr << INCLUDE_ERROR_MSG << endl << endl;
            return 1;
        } else {
            //Generate the command to add this library.
            arguments.push_back("--extra-arg=-I" + system_complete(CLANG_SYS_LIB).string());
        }
    } else {
        //Notify the user.
        cerr << INCLUDE_WARNING_MSG << endl << endl;
    }

    //Check if we're searching for files on our own.
    vector<string> files;
    if (SourceFindCommand.getValue().compare("") != 0){
        cout << "Searching automatically for C/C++ files with base directory: " << SourceFindCommand.getValue()
             << "..." << endl;
        files = findSourceCode(path(SourceFindCommand.getValue()));
        cout << endl;
    }

    //Next, modify argc and argv to allow for new arguments.
    const char **modArgv = generateArguments(argv, argc, arguments, files);
    argc = argc + (int) (arguments.size() + files.size());

    //Reruns the options parser.
    CommonOptionsParser UpdatedOptionsParser(argc, modArgv, ClangExCategory);
    ClangTool tool(UpdatedOptionsParser.getCompilations(),
                   UpdatedOptionsParser.getSourcePathList());

    //Generates a matcher system.
    ASTWalker walker = ASTWalker();
    MatchFinder finder;

    //Next, processes the matching conditions.
    walker.generateASTMatches(&finder);

    //Runs the Clang tool.
    cout << "Compiling the source code..." << endl;
    int code = tool.run(newFrontendActionFactory(&finder).get());
    if (code != 0) {
        return code;
    }

    //Resolves references.
    cout << "Resolving external references..." << endl;
    walker.resolveExternalReferences();

    //Generates file paths.
    walker.resolveFiles();

    //Processes the TA file.
    walker.buildGraph(output);
    return code; */
}