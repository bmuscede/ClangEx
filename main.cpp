#include <iostream>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem.hpp>
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "Walker/ASTWalker.h"
#include "File/ClangArgParse.h"
#include "Walker/PartialWalker.h"
#include "Walker/BlobWalker.h"
#include "TupleAttribute/TAProcessor.h"

using namespace std;
using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;
using namespace llvm;
using namespace boost::filesystem;

static const string DEFAULT_OUT = "a.ta";
static const string INSTANCE_FLAG = "$INSTANCE";

static llvm::cl::OptionCategory ClangExCategory("ClangEx Options");

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

    //Gets whether whether we're dealing with a merge.
    vector<string> mergeVec = parser.getOption(ClangArgParse::MERGE_LONG);
    bool merge = false;
    TAGraph* mergeGraph = nullptr;
    if (mergeVec.size() != 0){
        //We're dealing with a merge.
        merge = true;
        string mergeFile = mergeVec.at(0);

        cout << "Reading TA file " << mergeFile << "..." << endl << endl;

        //Loads the file.
        TAProcessor processor = TAProcessor(INSTANCE_FLAG);
        bool succ = processor.readTAFile(mergeFile);

        if (!succ) return 1;
        
        //Gets the graph.
        TAGraph* graph = processor.writeTAGraph();
        if (graph == nullptr) return 1;
    }

    //Check how we're dealing with IDs.
    bool useMD5 = parser.getFlag(ClangArgParse::MDFIVE_LONG);

    //Gets whether blob was set.
    ASTWalker* walker;
    if (parser.getFlag(ClangArgParse::BLOB_LONG)){
        if (merge)
            walker = new BlobWalker(useMD5, exclude, mergeGraph);
        else
            walker = new BlobWalker(useMD5, exclude);
    } else {
        if (merge)
            walker = new PartialWalker(useMD5, exclude, mergeGraph);
        else
            walker = new PartialWalker(useMD5, exclude);
    }
    //Generates a matcher system.
    MatchFinder finder;

    //Next, processes the matching conditions.
    walker->generateASTMatches(&finder);

    //Runs the Clang tool.
    cout << "Compiling the source code..." << endl;
    int code = tool.run(newFrontendActionFactory(&finder).get());
    if (code != 0) {
        return code;
    }

    //Resolves references.
    cout << endl << "Resolving external references..." << endl;
    walker->resolveExternalReferences();

    //Generates file paths.
    walker->resolveFiles();

    //Processes the TA file.
    vector<string> outputFiles = parser.getOption(ClangArgParse::OUT_LONG);
    cout << endl;
    if (outputFiles.size() == 0){
        if (merge){
            walker->buildGraph(mergeVec.at(0));
        } else {
            walker->buildGraph(DEFAULT_OUT);
        }
    } else {
        for (string file : outputFiles)
            walker->buildGraph(file);
    }

    delete walker;
    return code;
}