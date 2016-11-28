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

static const string DEFAULT_OUT = "a.ta";
static const string CLANG_SYS_LIB = "include";

static llvm::cl::OptionCategory ClangExCategory("ClangEx Options");

enum Test{LOL, LOL_YOURSELF};

int main(int argc, const char **argv) {
    //Starts by creating an argument parser.
    ClangArgParse parser = ClangArgParse();
    bool succ = parser.parseArguments(argc, argv);

    //Check return code.
    if (!succ) return 1;

    //Generates a specialized version of arguments for Clang.
    int genArgC = 0;
    const char** genArgV = parser.generateClangArgv(genArgC);

    //Get the exclusions.
    ClangArgParse::ClangExclude exclude = parser.generateExclusions();

    //Now, runs Clang.
    CommonOptionsParser OptionsParser(genArgC, genArgV, ClangExCategory);
    ClangTool tool(OptionsParser.getCompilations(),
                   OptionsParser.getSourcePathList());

    //Generates a matcher system.
    ASTWalker walker = ASTWalker(exclude);
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
    cout << endl << "Resolving external references..." << endl;
    walker.resolveExternalReferences();

    //Generates file paths.
    walker.resolveFiles();

    //Processes the TA file.
    vector<string> outputFiles = parser.getOption(ClangArgParse::OUT_LONG);
    cout << endl;
    if (outputFiles.size() == 0){
        walker.buildGraph(DEFAULT_OUT);
    } else {
        for (string file : outputFiles)
            walker.buildGraph(file);
    }
    return code;
}