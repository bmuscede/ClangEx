#include <iostream>
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "ASTWalker.h"

using namespace std;
using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;
using namespace llvm;

static const string DEFAULT_OUT = "a.out";

static const string HELP_MESSAGE = "---------------------------------------\n\n"
                                   "ClangEx is a modified Clang compiler used to extract essential\n"
                                   "program details from C/C++ source code. The program will go through\n"
                                   "all supplied source files and pull out essential details into a\n"
                                   "tuple-attribute (TA) file.\n";
static const string OUTPUT_HELP = "Specifies the output file for the generated Tuple-Attribute\n"
                                  "file. If no filename is specified, a Tuple-Attribute file\n"
                                  "will be created with the filename " + DEFAULT_OUT;

static llvm::cl::OptionCategory ClangExCategory("ClangEx Options");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp ClangExHelp(HELP_MESSAGE.c_str());
static cl::opt<string> TAOutCommand("out", cl::cat(ClangExCategory));

int main(int argc, const char **argv) {
    //Sets up command line options.
    TAOutCommand.setInitialValue(DEFAULT_OUT);
    TAOutCommand.setDescription(OUTPUT_HELP);

    //Develops an option parser for Clang.
    CommonOptionsParser OptionsParser(argc, argv, ClangExCategory);
    ClangTool tool(OptionsParser.getCompilations(),
                   OptionsParser.getSourcePathList());

    //Gets the Clang command for output (if specified).
    string output = TAOutCommand.getValue();

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
    return code;
}