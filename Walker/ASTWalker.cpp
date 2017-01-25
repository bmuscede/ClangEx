//
// Created by bmuscede on 05/11/16.
//

//TODO:
// - Create attribute structs in Node and Edge classes (Improve with functions in struct).
// - Get unions and structs working
// - Verify correctness of union, struct, and enums.

#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>
#include "ASTWalker.h"

using namespace std;
using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;
using namespace llvm;

ASTWalker::~ASTWalker() {
    delete graph;
}

void ASTWalker::buildGraph(string fileName) {
    //First, runs the graph builder process.
    string tupleAttribute = graph->generateTAFormat();

    //Next, writes to disk.
    ofstream taFile;
    taFile.open(fileName.c_str());

    //Check if the file is opened.
    if (!taFile.is_open()){
        cout << "The TA file could not be written to " << fileName << "!" << endl;
        return;
    }

    taFile << tupleAttribute;
    taFile.close();

    cout << "TA file successfully written to " << fileName << "!" << endl;
}

void ASTWalker::resolveExternalReferences() {
    graph->resolveExternalReferences(false);
}

void ASTWalker::resolveFiles(){
    vector<ClangNode*> fileNodes = vector<ClangNode*>();
    vector<ClangEdge*> fileEdges = vector<ClangEdge*>();

    //Gets all the associated clang nodes.
    fileParser.processPaths(fileNodes, fileEdges);

    //Adds them to the graph.
    if (!exclusions.cSubSystem) {
        for (ClangNode *file : fileNodes) {
            if (file->getType() != ClangNode::NodeType::SUBSYSTEM) {
                graph->addNode(file);
            }
        }

        //Adds the edges to the graph.
        for (ClangEdge *edge : fileEdges) {
            if (edge->getType() != ClangEdge::EdgeType::CONTAINS) {
                graph->addEdge(edge);
            }
        }
    }
    //Next, for each item in the graph, add it to a file.
    graph->addNodesToFile();
}

ASTWalker::ASTWalker(ClangArgParse::ClangExclude ex, TAGraph* existing = new TAGraph()){
    //Sets the current file name to blank.
    curFileName = "";

    //Creates the graph system.
    graph = existing;

    //Sets up the exclusions.
    exclusions = ex;
}

string ASTWalker::generateFileName(const MatchFinder::MatchResult result,
                                   SourceLocation loc, bool suppressFileOutput){
    //Gets the file name.
    SourceManager& SrcMgr = result.Context->getSourceManager();
    const FileEntry* Entry = SrcMgr.getFileEntryForID(SrcMgr.getFileID(loc));

    if (Entry == NULL) return string();

    string fileName(Entry->getName());

    //Use boost to get the absolute path.
    boost::filesystem::path fN = boost::filesystem::path(fileName);
    string newPath = fN.normalize().string();

    //Adds the file path.
    fileParser.addPath(newPath);

    //Checks if we have a output suppression in place.
    if (!suppressFileOutput && newPath.compare("") != 0) printFileName(newPath);
    return newPath;
}

//TODO: This currently doesn't deal with classes.
//TODO: This also is problematic. Find a unique way to resolve!
string ASTWalker::generateID(string fileName, string qualifiedName){
    return fileName + "[" + qualifiedName + "]";
}

string ASTWalker::generateLabel(const Decl* decl, ClangNode::NodeType type){
    string label;

    //Determines how we populate the string.
    switch (type) {
        case ClangNode::FUNCTION: {
            label = decl->getAsFunction()->getQualifiedNameAsString();
            break;
        }
        case ClangNode::VARIABLE: {

            const VarDecl *var = static_cast<const VarDecl *>(decl);
            label = var->getQualifiedNameAsString();

            //We need to get the parent function.
            const DeclContext *parentContext = var->getParentFunctionOrMethod();

            //If we have NULL, get the parent function.
            if (parentContext != NULL) {
                string parentQualName = static_cast<const FunctionDecl *>(parentContext)->getQualifiedNameAsString();
                label = parentQualName + "::" + label;
            }
            break;
        }
        case ClangNode::CLASS: {
            label = static_cast<const CXXRecordDecl *>(decl)->getQualifiedNameAsString();
            break;
        }
        case ClangNode::ENUM: {
            label = static_cast<const EnumDecl *>(decl)->getQualifiedNameAsString();
            break;
        }
        default: {
            label = "";
        }
    }

    //Check if we need to remove a symbol.
    for (int i = 0; i < ANON_LIST->size(); i++){
        string item = ANON_LIST[i];
        if (label.find(item) != string::npos)
            label = replaceLabel(label, item, ANON_REPLACE[i]);

    }
    replace(label.begin(), label.end(), '=', 'e');
    return label;
}

string ASTWalker::generateClassName(string qualifiedName){
    string cpyQual = qualifiedName;
    string qualifier = "::";
    vector<string> quals = vector<string>();

    size_t pos = 0;
    while ((pos = cpyQual.find(qualifier)) != std::string::npos) {
        quals.push_back(cpyQual.substr(0, pos));
        cpyQual.erase(0, pos + qualifier.length());
    }
    quals.push_back(cpyQual);

    //Check if we have a class.
    if (quals.size() == 1) return string();

    return quals.at(quals.size() - 2);
}

bool ASTWalker::isInSystemHeader(const MatchFinder::MatchResult &result, const clang::Decl *decl){
    if (decl == NULL) return false;
    bool isIn;

    //Some system headers cause Clang to segfault.
    try {
        //Gets where this item is located.
        auto &SourceManager = result.Context->getSourceManager();
        auto ExpansionLoc = SourceManager.getExpansionLoc(decl->getLocStart());

        //Checks if we have an invalid location.
        if (ExpansionLoc.isInvalid()) {
            return false;
        }

        //Now, checks if we don't have a system header.
        isIn = SourceManager.isInSystemHeader(ExpansionLoc);
    } catch ( ... ){
        return false;
    }

    return isIn;
}

/********************************************************************************************************************/
// START AST TO GRAPH PARAMETERS
/********************************************************************************************************************/

/********************************************************************************************************************/
// END AST TO GRAPH PARAMETERS
/********************************************************************************************************************/

void ASTWalker::printFileName(std::string curFile){
    if (curFile.compare(curFileName) != 0){
        cout << "\tCurrently processing: " << curFile << endl;
        curFileName = curFile;
    }
}

std::string ASTWalker::replaceLabel(std::string label, std::string init, std::string aft){
    size_t index = 0;
    while (true) {
        //Locate the substring to replace.
        index = label.find(init, index);
        if (index == string::npos) break;

        //Make the replacement.
        label.replace(index, init.size(), aft);

        //Advance index forward so the next iteration doesn't pick it up as well.
        index += aft.size();
    }

    return label;
}