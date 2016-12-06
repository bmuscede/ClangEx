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

ASTWalker::ASTWalker(){
    //Sets the current file name to blank.
    curFileName = "";
}

ASTWalker::~ASTWalker() { }

void ASTWalker::buildGraph(string fileName) {
    //First, runs the graph builder process.
    string tupleAttribute = graph.generateTAFormat();

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
    int resolved = 0;
    int unresolved = 0;

    //Iterate through and resolve.
    for (int i = 0; i < unresolvedRef.size(); i++){
        pair<pair<string, string>, ClangEdge::EdgeType> entry = unresolvedRef.at(i);

        //Find the two items.
        vector<ClangNode*> srcs = graph.findNodeByName(entry.first.first);
        vector<ClangNode*> dsts = graph.findNodeByName(entry.first.second);

        //See if they could be resolved.
        if (srcs.size() == 0 || dsts.size() == 0){
            unresolved++;
        } else {
            ClangEdge* edge = new ClangEdge(srcs.at(0), dsts.at(0), entry.second);
            graph.addEdge(edge);

            //Now, find all associated attributes.
            vector<pair<string, vector<string>>> attributes = findAttributes(entry.first.first, entry.first.second);
            for (auto attribute : attributes){
                for (auto attVal : attribute.second){
                    graph.addAttribute(srcs.at(0)->getID(), dsts.at(0)->getID(), attribute.first, attVal);
                }
            }

            resolved++;
        }
    }

    //Afterwards, notify of success.
    cout << "Overall, " << resolved << " references were resolved and " << unresolved
         << " references could not be resolved." << endl;
}

void ASTWalker::resolveFiles(){
    vector<ClangNode*> fileNodes = vector<ClangNode*>();
    vector<ClangEdge*> fileEdges = vector<ClangEdge*>();

    //Gets all the associated clang nodes.
    fileParser.processPaths(fileNodes, fileEdges);

    //Adds them to the graph.
    for (ClangNode* file : fileNodes){
        if (!(exclusions.cSubSystem && file->getType() == ClangNode::NodeType::SUBSYSTEM)){
            graph.addNode(file);
        }
    }

    //Adds the edges to the graph.
    for (ClangEdge* edge : fileEdges){
        if (!(exclusions.cSubSystem && edge->getType() == ClangEdge::EdgeType::CONTAINS)){
            graph.addEdge(edge);
        }
    }

    //Next, for each item in the graph, add it to a file.
    graph.addNodesToFile();
}

void ASTWalker::addUnresolvedRef(string callerID, string calleeID, ClangEdge::EdgeType type) {
    //Creates the entry for the vector.
    pair<pair<string, string>, ClangEdge::EdgeType> entry = pair<pair<string, string>, ClangEdge::EdgeType>();
    entry.first.first = callerID;
    entry.first.second = calleeID;
    entry.second = type;

    //Finally, adds it to the unresolved vector.
    unresolvedRef.push_back(entry);
}

void ASTWalker::addUnresolvedRefAttr(string callerID, string calleeID, string attrName, string attrValue) {
    //Find if we already have a attr entry for this list.
    for (auto current : unresolvedRefAttr){
        if (current->first.first.compare(callerID) == 0 &&
                current->first.second.compare(calleeID) == 0 &&
                current->second.first.compare(attrName) == 0){

            //We already have an entry.
            current->second.second.push_back(attrValue);
            return;
        }
    }

    //Creates the entry for the vector.
    pair<pair<string, string>, pair<string, vector<string>>> *entry = new pair<pair<string, string>, pair<string, vector<string>>>();

    entry->first.first = callerID;
    entry->first.second = calleeID;
    entry->second.first = attrName;
    entry->second.second.push_back(attrValue);

    //Adds it to the vector.
    unresolvedRefAttr.push_back(entry);
}

string ASTWalker::generateID(string fileName, string signature, ClangNode::NodeType type) {
    if (type == ClangNode::CLASS){
        return fileName + "[" + CLASS_PREPEND + signature + "]";
    }

    return fileName + "[" + signature + "]";
}

string ASTWalker::generateID(const MatchFinder::MatchResult result, const DeclaratorDecl *decl, ClangNode::NodeType type){
    //Gets the file name from the source manager.
    string fileName = generateFileName(result, decl->getInnerLocStart());

    //Gets the qualified name.
    string name = decl->getNameAsString();

    return generateID(fileName, name, type);
}

string ASTWalker::generateFileName(const MatchFinder::MatchResult result, SourceLocation loc){
    //Gets the file name.
    SourceManager& SrcMgr = result.Context->getSourceManager();
    const FileEntry* Entry = SrcMgr.getFileEntryForID(SrcMgr.getFileID(loc));
    string fileName(Entry->getName());

    //Adds the file path.
    fileParser.addPath(fileName);

    //Print file name.
    printFileName(fileName);

    return fileName;
}

string ASTWalker::generateLabel(const Decl* decl, ClangNode::NodeType type){
    string label;

    //Get qualified name.
    if (type == ClangNode::FUNCTION){
        label = decl->getAsFunction()->getQualifiedNameAsString();
    } else if (type == ClangNode::VARIABLE){
        const VarDecl* var = static_cast<const VarDecl*>(decl);
        label = var->getQualifiedNameAsString();

        //We need to get the parent function.
        const DeclContext* parentContext = var->getParentFunctionOrMethod();

        //If we have NULL, get the parent function.
        if (parentContext != NULL){
            string parentQualName = static_cast<const FunctionDecl*>(parentContext)->getQualifiedNameAsString();
            label = parentQualName + "::" + label;
        }
    } else if (type == ClangNode::CLASS){
        const CXXRecordDecl* record = static_cast<const CXXRecordDecl*>(decl);
        label = record->getQualifiedNameAsString();
    } else if (type == ClangNode::ENUM){
        const EnumDecl* enumDec = static_cast<const EnumDecl*>(decl);
        label = enumDec->getQualifiedNameAsString();
    }

    return label;
}

string ASTWalker::getClassNameFromQualifier(string qualifiedName){
    //TODO: Fix this. It may not be needed.
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

vector<pair<string, vector<string>>> ASTWalker::findAttributes(string callerID, string calleeID){
    vector<pair<string, vector<string>>> values;

    //Find all associated values.
    for (auto entry : unresolvedRefAttr){
        if (entry->first.first.compare(callerID) == 0 && entry->first.second.compare(calleeID) == 0){
            //Adds the entry.
            pair<string, vector<string>> value;
            value.first = entry->second.first;
            value.second = entry->second.second;

            values.push_back(value);
        }
    }

    return values;
}

void ASTWalker::printFileName(std::string curFile){
    if (curFile.compare(curFileName) != 0){
        cout << "\tCurrently processing: " << curFile << endl;
        curFileName = curFile;
    }
}