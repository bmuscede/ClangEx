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
#include "Walker/PartialWalker.h"
#include "Walker/BlobWalker.h"
#include "TupleAttribute/TAProcessor.h"
#include "Printer/MinimalPrinter.h"

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

    //Get whether we're running in verbose or not.
    bool verbose = parser.getFlag(ClangArgParse::VERBOSE_LONG);
    Printer* clangPrint;
    if (verbose) {
        clangPrint = new VerbosePrinter();
    } else {
        clangPrint = new MinimalPrinter();
    }

    //Generates a specialized version of arguments for Clang.
    int genArgC = 0;
    const char** genArgV = parser.generateClangArgv(genArgC, clangPrint);

    //Get the number of files.
    if (!verbose) static_cast<MinimalPrinter*>(clangPrint)->setNumSteps(parser.getNumFiles() + 3);

    //Check the number of files.
    if (parser.getNumFiles() == 0) {
        clangPrint->printErrorNoFiles();
        for (int i = 0; i < genArgC; i++)
            delete[] genArgV[i];
        delete[] genArgV;
        delete clangPrint; //SOMEHOW CAUSES A SEGFAULT?!
        return 1;
    }

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

        clangPrint->printMerge(mergeFile);

        //Loads the file.
        TAProcessor processor = TAProcessor(INSTANCE_FLAG, clangPrint);
        bool succ = processor.readTAFile(mergeFile);

        if (!succ) {
            for (int i = 0; i < genArgC; i++)
                delete[] genArgV[i];
            delete[] genArgV;
            delete clangPrint;
            return 1;
        }
        
        //Gets the graph.
        TAGraph* graph = processor.writeTAGraph();
        if (graph == nullptr) {
            for (int i = 0; i < genArgC; i++)
                delete[] genArgV[i];
            delete[] genArgV;
            delete clangPrint;
            return 1;
        }
    }

    //Check how we're dealing with IDs.
    bool useMD5 = parser.getFlag(ClangArgParse::MDFIVE_LONG);

    //Gets whether blob was set.
    ASTWalker* walker;
    if (parser.getFlag(ClangArgParse::BLOB_LONG)){
        if (merge)
            walker = new BlobWalker(useMD5, clangPrint, exclude, mergeGraph);
        else
            walker = new BlobWalker(useMD5, clangPrint, exclude);
    } else {
        if (merge)
            walker = new PartialWalker(useMD5, clangPrint, exclude, mergeGraph);
        else
            walker = new PartialWalker(useMD5, clangPrint, exclude);
    }
    //Generates a matcher system.
    MatchFinder finder;

    //Next, processes the matching conditions.
    walker->generateASTMatches(&finder);

    //Runs the Clang tool.
    clangPrint->printProcessStatus(Printer::COMPILING);
    int code = tool.run(newFrontendActionFactory(&finder).get());
    clangPrint->printFileNameDone();

    if (code != 0) {
        //Prompt the user if they'd like to contiune.
        bool generateModel = clangPrint->printProcessFailure();

        //Delete everything if not.
        if (!generateModel){
            delete walker;
            for (int i = 0; i < genArgC; i++)
                delete[] genArgV[i];
            delete[] genArgV;
            delete clangPrint;
            return code;
        }
    }

    //Resolves references.
    clangPrint->printProcessStatus(Printer::RESOLVE_REF);
    walker->resolveExternalReferences();

    //Generates file paths.
    clangPrint->printProcessStatus(Printer::RESOLVE_FILE);
    walker->resolveFiles();
    clangPrint->printResolvePathDone();

    //Processes the TA file.
    vector<string> outputFiles = parser.getOption(ClangArgParse::OUT_LONG);
    if (outputFiles.size() == 0){
        if (merge){
            clangPrint->printGenTA(mergeVec.at(0));
            walker->buildGraph(mergeVec.at(0));
        } else {
            clangPrint->printGenTA(DEFAULT_OUT);
            walker->buildGraph(DEFAULT_OUT);
        }
    } else {
        for (string file : outputFiles) {
            clangPrint->printGenTA(file);
            walker->buildGraph(file);
        }
    }

    for (int i = 0; i < genArgC; i++)
        delete[] genArgV[i];
    delete[] genArgV;
    delete walker;
    delete clangPrint;
    return code;
}