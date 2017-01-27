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
#include <openssl/md5.h>
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
    fileParser.processPaths(fileNodes, fileEdges, md5Flag);

    //Adds them to the graph.
    if (!exclusions.cSubSystem) {
        for (ClangNode *file : fileNodes) {
            if (file->getType() == ClangNode::NodeType::SUBSYSTEM) {
                graph->addNode(file);
            }
        }

        //Adds the edges to the graph.
        for (ClangEdge *edge : fileEdges) {
            if (edge->getType() == ClangEdge::EdgeType::CONTAINS) {
                graph->addEdge(edge);
            }
        }
    }
    //Next, for each item in the graph, add it to a file.
    graph->addNodesToFile();
}

std::string ASTWalker::generateMD5(std::string text){
    //Creates a digest buffer.
    unsigned char digest[MD5_DIGEST_LENGTH];
    const char* cText = text.c_str();

    //Initializes the MD5 string.
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, cText, strlen(cText));
    MD5_Final(digest, &ctx);

    //Fills it with characters.
    char mdString[MD5_LENGTH];
    for (int i = 0; i < 16; i++)
        sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);

    return string(mdString);
}

ASTWalker::ASTWalker(ClangArgParse::ClangExclude ex, bool md5, TAGraph* existing = new TAGraph()){
    //Sets the current file name to blank.
    curFileName = "";

    //Creates the graph system.
    graph = existing;

    //Sets up the exclusions.
    exclusions = ex;

    //Sets the MD5 name generate flag.
    md5Flag = md5;
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
string ASTWalker::generateID(string fileName, string qualifiedName){
    string genID = fileName + "[" + qualifiedName + "]";

    //Check what flag we're operating on.
    if (md5Flag){
        genID = generateMD5(genID);
    }

    return genID;
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
    for (int i = 0; i < ANON_SIZE; i++){
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
void ASTWalker::addFunctionDecl(const MatchFinder::MatchResult results, const clang::DeclaratorDecl *dec) {
    //Generate the fields for the node.
    string label = generateLabel(dec, ClangNode::FUNCTION);
    string filename = generateFileName(results, dec->getInnerLocStart());
    string ID = generateID(filename, dec->getQualifiedNameAsString());
    if (ID.compare("") == 0 || filename.compare("") == 0) return;


    //Creates a new function entry.
    ClangNode* node = new ClangNode(ID, label, ClangNode::FUNCTION);
    graph->addNode(node);

    //Adds parameters.
    graph->addSingularAttribute(node->getID(),
                                ClangNode::FILE_ATTRIBUTE.attrName,
                                ClangNode::FILE_ATTRIBUTE.processFileName(filename));


    //Check if we have a CXXMethodDecl.
    if (isa<CXXMethodDecl>(dec->getAsFunction())){
        //Perform a static cast.
        const CXXMethodDecl* methDecl = static_cast<const CXXMethodDecl*>(dec->getAsFunction());

        //Process method decls.
        bool isStatic = methDecl->isStatic();
        bool isConst = methDecl ->isConst();
        bool isVol = methDecl->isVolatile();
        bool isVari = methDecl->isVariadic();
        AccessSpecifier spec = methDecl->getAccess();

        //Add these types of attributes.
        graph->addSingularAttribute(node->getID(),
                                    ClangNode::FUNC_IS_ATTRIBUTE.staticName,
                                    std::to_string(isStatic));
        graph->addSingularAttribute(node->getID(),
                                    ClangNode::FUNC_IS_ATTRIBUTE.constName,
                                    std::to_string(isConst));
        graph->addSingularAttribute(node->getID(),
                                    ClangNode::FUNC_IS_ATTRIBUTE.volName,
                                    std::to_string(isVol));
        graph->addSingularAttribute(node->getID(),
                                    ClangNode::FUNC_IS_ATTRIBUTE.varName,
                                    std::to_string(isVari));
        graph->addSingularAttribute(node->getID(),
                                    ClangNode::VIS_ATTRIBUTE.attrName,
                                    ClangNode::VIS_ATTRIBUTE.processAccessSpec(spec));
    }
}

void ASTWalker::addVariableDecl(const MatchFinder::MatchResult results,
                                const clang::VarDecl *varDec, const clang::FieldDecl *fieldDec){
    string label;
    string filename;
    string ID;
    string scopeInfo;
    string staticInfo;

    //Check which one we use.
    bool useField = false;
    if (varDec == NULL) useField = true;

    //Next, generate the fields for the decl.
    if (useField){
        label = generateLabel(fieldDec, ClangNode::VARIABLE);
        filename = generateFileName(results, fieldDec->getInnerLocStart());
        ID = generateID(filename, fieldDec->getQualifiedNameAsString());
        scopeInfo = ClangNode::VAR_ATTRIBUTE.getScope(fieldDec);
        staticInfo = ClangNode::VAR_ATTRIBUTE.getStatic(fieldDec);
    } else {
        label = generateLabel(varDec, ClangNode::VARIABLE);
        filename = generateFileName(results, varDec->getInnerLocStart());
        ID = generateID(filename, varDec->getQualifiedNameAsString());
        scopeInfo = ClangNode::VAR_ATTRIBUTE.getScope(varDec);
        staticInfo = ClangNode::VAR_ATTRIBUTE.getStatic(varDec);
    }
    if (ID.compare("") == 0 || filename.compare("") == 0) return;

    //Creates a variable entry.
    ClangNode* node = new ClangNode(ID, label, ClangNode::VARIABLE);
    graph->addNode(node);

    //Process attributes.
    graph->addSingularAttribute(node->getID(),
                                ClangNode::FILE_ATTRIBUTE.attrName,
                                ClangNode::FILE_ATTRIBUTE.processFileName(filename));

    //Get the scope of the decl.
    graph->addSingularAttribute(node->getID(),
                                ClangNode::VAR_ATTRIBUTE.scopeName,
                                scopeInfo);
    graph->addSingularAttribute(node->getID(),
                                ClangNode::VAR_ATTRIBUTE.staticName,
                                staticInfo);
}
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