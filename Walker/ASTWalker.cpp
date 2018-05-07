/////////////////////////////////////////////////////////////////////////////////////////////////////////
// ASTWalker.h
//
// Created By: Bryan J Muscedere
// Date: 05/11/16.
//
// Parent class that supports the other two walkers. Provides methods
// that add each of the AST elements. Also provides information for
// ID and name generation. Basically, this method is a catch-all for
// operations.
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
#include <boost/filesystem.hpp>
#include <openssl/md5.h>
#include "ASTWalker.h"
#include "clang/AST/Mangle.h"
#include "../Graph/ClangNode.h"
#include "../Graph/LowMemoryTAGraph.h"

using namespace std;
using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;
using namespace llvm;

/**
 * Default Destructor
 */
ASTWalker::~ASTWalker() { }

/**
 * Gets the graph for the current AST.
 * @return The graph currenly being used.
 */
TAGraph* ASTWalker::getGraph(){
    return graph;
}

/**
 * Generates an MD5 hash of the current string.
 * @param text The string to convert.
 * @return The MD5 of the current string.
 */
string ASTWalker::generateMD5(string text){
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

/**
 * Constructor. Sets fields used by the other two walkers.
 * @param ex The AST elements to exclude.
 * @param print The printer to use.
 * @param existing The existing TA graph (if any).
 */
ASTWalker::ASTWalker(TAGraph::ClangExclude ex, bool lowMemory, Printer *print, TAGraph* existing) :
        clangPrinter(print){
    //Sets the current file name to blank.
    curFileName = "";

    //Creates the graph system.
    if (existing == nullptr){
        if (lowMemory == true) graph = new LowMemoryTAGraph();
        else graph = new TAGraph();
    } else {
        graph = existing;
    }

    //Sets up the exclusions.
    exclusions = ex;
}

/**
 * Generates a file name from a given source location.
 * @param result The match result.
 * @param loc The source location of the item.
 * @param suppressFileOutput Whether we print the file being processed or not.
 * @return The filename.
 */
string ASTWalker::generateFileName(const MatchFinder::MatchResult result,
                                   SourceLocation loc, bool suppressFileOutput){
    //Gets the file name.
    SourceManager& SrcMgr = result.Context->getSourceManager();
    const FileEntry* Entry = SrcMgr.getFileEntryForID(SrcMgr.getFileID(loc));

    if (Entry == nullptr) return string();

    string fileName(Entry->getName());

    //Use boost to get the absolute path.
    boost::filesystem::path fN = boost::filesystem::path(fileName);
    string newPath = canonical(fN.normalize()).string();

    //Adds the file path.
    graph->addPath(newPath);

    //Checks if we have a output suppression in place.
    if (!suppressFileOutput && newPath.compare("") != 0) printFileName(newPath);
    return newPath;
}

/**
 * Generates an ID of a declaration.
 * @param result The match result.
 * @param dec The declaration.
 * @return The ID of the declaration.
 */
string ASTWalker::generateID(const MatchFinder::MatchResult result, const NamedDecl *dec){
    //Generates the ID.
    string name = generateIDString(result, dec);
    name = generateMD5(name);
    return name;
}

string ASTWalker::generateLabel(const MatchFinder::MatchResult result, const NamedDecl* curDecl) {
    string name = curDecl->getNameAsString();
    if (isa<RecordDecl>(curDecl) && (dyn_cast<RecordDecl>(curDecl)->isStruct()
                                 || dyn_cast<RecordDecl>(curDecl)->isUnion())
                                 && isAnonymousRecord(curDecl->getQualifiedNameAsString())) {
        name = ANON_REPLACE + "-" + generateLineNumber(result, curDecl->getSourceRange().getBegin());
    }

    bool getParent = true;
    bool recurse = false;
    const NamedDecl *originalDecl = curDecl;

    //Get the parent.
    auto parent = result.Context->getParents(*curDecl);
    while (getParent) {
        //Check if it's empty.
        if (parent.empty()) {
            getParent = false;
            continue;
        }

        //Get the current decl as named.
        curDecl = parent[0].get<clang::NamedDecl>();
        if (curDecl) {
            name = generateLabel(result, curDecl) + "::" + name;
            recurse = true;
            getParent = false;
            continue;
        }

        parent = result.Context->getParents(parent[0]);
    }

    //Sees if no true qualified name was used.
    Decl::Kind kind = originalDecl->getKind();
    if (!recurse) {
        if (kind == Decl::Function || kind == Decl::CXXMethod) {
            name = originalDecl->getQualifiedNameAsString();
        } else {
            //We need to get the parent function.
            const DeclContext *parentContext = originalDecl->getParentFunctionOrMethod();

            //If we have nullptr, get the parent function.
            if (parentContext != nullptr) {
                string parentQualName = generateLabel(result, static_cast<const FunctionDecl *>(parentContext));
                name = parentQualName + "::" + originalDecl->getNameAsString();
            }
        }
    }

    return name;
}

/**
 * Whether or not a declaration is in a system header.
 * @param result The match result.
 * @param decl The decl.
 * @return A boolean indicating whether the declaration is or not.
 */
bool ASTWalker::isInSystemHeader(const MatchFinder::MatchResult &result, const Decl *decl){
    if (decl == nullptr) return false;
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

/**
 * Extracts the CXXRecordDecl from a NestedNameSpecifier.
 * @param name The NestedNameSpecifier
 * @return A CXXRecordDecl or nullptr on failure.
 */
CXXRecordDecl* ASTWalker::extractClass(NestedNameSpecifier* name){
    //Checks if the qualifier is null.
    if (name == nullptr || name->getAsType() == nullptr) return nullptr;
    return name->getAsType()->getAsCXXRecordDecl();
}

/********************************************************************************************************************/
// START AST TO GRAPH PARAMETERS
/********************************************************************************************************************/
/**
 * Adds a function decl to the graph.
 * @param results The match result.
 * @param dec The function decl to add.
 */
void ASTWalker::addFunctionDecl(const MatchFinder::MatchResult results, const FunctionDecl *dec) {
    //Generate the fields for the node.
    string label = generateLabel(results, dec);
    string filename = generateFileName(results, dec->getInnerLocStart());
    string ID = generateID(results, dec);
    if (ID.compare("") == 0 || filename.compare("") == 0) return;


    //Creates a new function entry.
    ClangNode* node = new ClangNode(ID, label, ClangNode::FUNCTION);
    bool succ = graph->addNode(node);
    if (!succ) return;

    //Adds parameters.
    graph->addAttribute(node->getID(),
                                ClangNode::FILE_ATTRIBUTE.attrName,
                                ClangNode::FILE_ATTRIBUTE.processFileName(filename));


    //Check if we have a CXXMethodDecl.
    auto func = dec->getAsFunction();
    if (func != nullptr && isa<CXXMethodDecl>(func)){
        //Perform a static cast.
        const CXXMethodDecl* methDecl = static_cast<const CXXMethodDecl*>(dec->getAsFunction());

        //Process method decls.
        bool isStatic = methDecl->isStatic();
        bool isConst = methDecl ->isConst();
        bool isVol = methDecl->isVolatile();
        bool isVari = methDecl->isVariadic();
        AccessSpecifier spec = methDecl->getAccess();

        //Add these types of attributes.
        graph->addAttribute(node->getID(),
                                    ClangNode::FUNC_IS_ATTRIBUTE.staticName,
                                    std::to_string(isStatic));
        graph->addAttribute(node->getID(),
                                    ClangNode::FUNC_IS_ATTRIBUTE.constName,
                                    std::to_string(isConst));
        graph->addAttribute(node->getID(),
                                    ClangNode::FUNC_IS_ATTRIBUTE.volName,
                                    std::to_string(isVol));
        graph->addAttribute(node->getID(),
                                    ClangNode::FUNC_IS_ATTRIBUTE.varName,
                                    std::to_string(isVari));
        graph->addAttribute(node->getID(),
                                    ClangNode::VIS_ATTRIBUTE.attrName,
                                    ClangNode::VIS_ATTRIBUTE.processAccessSpec(spec));
    }
}

/**
 * Adds a variable decl (or field decl) to the graph.
 * @param results The match result.
 * @param varDec The var decl to add.
 * @param fieldDec The field decl to add.
 */
void ASTWalker::addVariableDecl(const MatchFinder::MatchResult results,
                                const VarDecl *varDec, const FieldDecl *fieldDec){
    string label;
    string filename;
    string ID;
    string scopeInfo;
    string staticInfo;

    //Check which one we use.
    bool useField = false;
    if (varDec == nullptr) useField = true;

    //Next, generate the fields for the decl.
    if (useField){
        label = generateLabel(results, fieldDec);
        filename = generateFileName(results, fieldDec->getInnerLocStart());
        ID = generateID(results, fieldDec);
        scopeInfo = ClangNode::VAR_ATTRIBUTE.getScope(fieldDec);
        staticInfo = ClangNode::VAR_ATTRIBUTE.getStatic(fieldDec);
    } else {
        label = generateLabel(results, varDec);
        filename = generateFileName(results, varDec->getInnerLocStart());
        ID = generateID(results, varDec);
        scopeInfo = ClangNode::VAR_ATTRIBUTE.getScope(varDec);
        staticInfo = ClangNode::VAR_ATTRIBUTE.getStatic(varDec);
    }
    if (ID.compare("") == 0 || filename.compare("") == 0) return;

    //Creates a variable entry.
    ClangNode* node = new ClangNode(ID, label, ClangNode::VARIABLE);
    bool succ = graph->addNode(node);
    if (!succ) return;

    //Process attributes.
    graph->addAttribute(node->getID(),
                                ClangNode::FILE_ATTRIBUTE.attrName,
                                ClangNode::FILE_ATTRIBUTE.processFileName(filename));

    //Get the scope of the decl.
    graph->addAttribute(node->getID(),
                                ClangNode::VAR_ATTRIBUTE.scopeName,
                                scopeInfo);
    graph->addAttribute(node->getID(),
                                ClangNode::VAR_ATTRIBUTE.staticName,
                                staticInfo);
}

/**
 * Adds a class decl to the graph.
 * @param results The match result.
 * @param classDecl The class decl to add.
 * @param fName The name of the file.
 */
void ASTWalker::addClassDecl(const MatchFinder::MatchResult results, const CXXRecordDecl *classDecl, string fName){
    //Get the definition.
    auto def = classDecl->getDefinition();
    if (def == nullptr) return;

    //Check if we're dealing with a class.
    if (!classDecl->isClass()) return;

    //Generate the fields for the node.
    string filename = (fName.compare("") == 0) ? generateFileName(results, classDecl->getInnerLocStart(), true) : fName;
    string ID = generateID(results, classDecl);
    string className = generateLabel(results, classDecl);
    if (ID.compare("") == 0 || filename.compare("") == 0) return;

    //Try to get the number of bases.
    int numBases = 0;
    try {
        numBases = classDecl->getNumBases();
    } catch (...){
        return;
    }

    //Creates a class entry.
    ClangNode* node = new ClangNode(ID, className, ClangNode::CLASS);
    bool succ = graph->addNode(node);
    if (!succ) return;

    //Process attributes.
    graph->addAttribute(node->getID(),
                                ClangNode::FILE_ATTRIBUTE.attrName,
                                ClangNode::FILE_ATTRIBUTE.processFileName(filename));
    graph->addAttribute(node->getID(),
                                ClangNode::BASE_ATTRIBUTE.attrName,
                                std::to_string(numBases));

    //Get base classes.
    if (classDecl->getNumBases() > 0) {
        for (auto base = classDecl->bases_begin(); base != classDecl->bases_end(); base++) {
            if (base->getType().getTypePtr() == nullptr) continue;
            CXXRecordDecl *baseClass = base->getType().getTypePtr()->getAsCXXRecordDecl();
            if (baseClass == nullptr) continue;

            //Add a linkage in our graph->
            addClassInheritance(results, classDecl, baseClass);
        }
    }
}

/**
 * Adds an enum decl to the graph.
 * @param result The match result.
 * @param enumDecl The enum decl to add.
 * @param spoofFilename A potential false filename to add it under.
 */
void ASTWalker::addEnumDecl(const MatchFinder::MatchResult result, const EnumDecl *enumDecl, string spoofFilename){
    //Generate the fields for the node.
    string filename = (spoofFilename.compare(string()) == 0) ?
                      generateFileName(result, enumDecl->getInnerLocStart()) : spoofFilename;
    string ID = generateID(result, enumDecl);
    string enumName = generateLabel(result, enumDecl);
    if (ID.compare("") == 0 || filename.compare("") == 0) return;

    //Creates a enum entry.
    ClangNode* node = new ClangNode(ID, enumName, ClangNode::ENUM);
    bool succ = graph->addNode(node);
    if (!succ) return;

    //Process attributes.
    graph->addAttribute(node->getID(),
                                ClangNode::FILE_ATTRIBUTE.attrName,
                                ClangNode::FILE_ATTRIBUTE.processFileName(filename));
}

/**
 * Adds an enum constant to the graph.
 * @param result The match result.
 * @param enumDecl The enum decl to add.
 * @param filenameSpoof A potential false filename to add it under.
 */
void ASTWalker::addEnumConstantDecl(const MatchFinder::MatchResult result, const clang::EnumConstantDecl *enumDecl,
                                    string filenameSpoof){
    //Generate the fields for the node.
    string filename = (filenameSpoof.compare(string()) == 0) ?
                      generateFileName(result, enumDecl->getLocStart()) : filenameSpoof;
    string ID = generateID(result, enumDecl);
    string enumName = generateLabel(result, enumDecl);
    if (ID.compare("") == 0 || filename.compare("") == 0) return;

    //Creates a new enum entry.
    ClangNode* node = new ClangNode(ID, enumName, ClangNode::ENUM_CONST);
    bool succ = graph->addNode(node);
    if (!succ) return;

    //Process attributes.
    graph->addAttribute(node->getID(),
                                ClangNode::FILE_ATTRIBUTE.attrName,
                                ClangNode::FILE_ATTRIBUTE.processFileName(filename));
}

/**
 * Adds a struct decl to the graph.
 * @param result The match result.
 * @param structDecl The struct decl to add.
 * @param filename A spoof filename to add it under.
 */
void ASTWalker::addStructDecl(const MatchFinder::MatchResult result, const clang::RecordDecl *structDecl, string filename){
    //Checks whether the function is anonymous.
    bool isAnonymous = isAnonymousRecord(structDecl->getQualifiedNameAsString());

    //With that, generates the ID, label, and filename.
    string fileName = generateFileName(result, structDecl->getInnerLocStart());
    string ID = generateID(result, structDecl);
    string label = generateLabel(result, structDecl);

    //Next, generates the node.
    ClangNode* node = new ClangNode(ID, label, ClangNode::STRUCT);
    bool succ = graph->addNode(node);
    if (!succ) return;

    //Process the attributes.
    graph->addAttribute(node->getID(),
                                ClangNode::FILE_ATTRIBUTE.attrName,
                                ClangNode::FILE_ATTRIBUTE.processFileName(filename));
    graph->addAttribute(node->getID(),
                                ClangNode::STRUCT_ATTRIBUTE.anonymousName,
                                ClangNode::STRUCT_ATTRIBUTE.processAnonymous(isAnonymous));
}

/**
 * Adds a union decl to the graph.
 * @param result The match result.
 * @param unionDecl The union decl to add.
 * @param filename A spoof filename to add it under.
 */
void ASTWalker::addUnionDecl(const MatchFinder::MatchResult result, const RecordDecl *unionDecl, string filename){
    //Checks whether the function is anonymous.
    bool isAnonymous = isAnonymousRecord(unionDecl->getQualifiedNameAsString());

    //With that, generates the ID, label, and filename.
    string fileName = generateFileName(result, unionDecl->getInnerLocStart());
    string ID = generateID(result, unionDecl);
    string label = generateLabel(result, unionDecl);

    //Next, generates the node.
    ClangNode* node = new ClangNode(ID, label, ClangNode::UNION);
    bool succ = graph->addNode(node);
    if (!succ) return;

    //Process the attributes.
    graph->addAttribute(node->getID(),
                        ClangNode::FILE_ATTRIBUTE.attrName,
                        ClangNode::FILE_ATTRIBUTE.processFileName(filename));
    graph->addAttribute(node->getID(),
                        ClangNode::STRUCT_ATTRIBUTE.anonymousName,
                        ClangNode::STRUCT_ATTRIBUTE.processAnonymous(isAnonymous));
}

/**
 * Adds a function call to the graph.
 * @param results The match result.
 * @param caller The caller.
 * @param callee The callee.
 */
void ASTWalker::addFunctionCall(const MatchFinder::MatchResult results, const DeclaratorDecl* caller,
                                const FunctionDecl* callee){
    //Generate a label for the two functions.
    string callerID = generateID(results, caller);
    string callerLabel = generateLabel(results, caller);
    string calleeID = generateID(results, callee);
    string calleeLabel = generateLabel(results, callee);

    processEdge(callerID, callerLabel, calleeID, calleeLabel, ClangEdge::CALLS);
}

/**
 * Adds a variable call to the graph.
 * @param result The match result.
 * @param caller The caller.
 * @param expr The expression that the call takes place in.
 * @param varCallee The var callee.
 * @param fieldCallee The field callee.
 */
void ASTWalker::addVariableCall(const MatchFinder::MatchResult result, const DeclaratorDecl *caller,
                                const Expr* expr, const VarDecl *varCallee,
                                const FieldDecl *fieldCallee){
    string variableID;
    string variableLabel;
    string variableShortName;

    //Generate the information associated with the caller.
    string callerID = generateID(result, caller);
    string callerLabel = generateLabel(result, caller);

    //Decide how we should process.
    if (fieldCallee == nullptr){
        variableID = generateID(result, varCallee);
        variableLabel = generateLabel(result, varCallee);
        variableShortName = varCallee->getName();
    } else {
        variableID = generateID(result, fieldCallee);
        variableLabel = generateLabel(result, fieldCallee);
        variableShortName = fieldCallee->getName();
    }

    //Generate the attributes.
    pair<string, string> accessVar = pair<string, string>();
    accessVar.first = ClangEdge::ACCESS_ATTRIBUTE.attrName;
    accessVar.second = ClangEdge::ACCESS_ATTRIBUTE.getVariableAccess(expr, variableShortName);

    vector<pair<string, string>> attributes = vector<pair<string, string>>();
    attributes.push_back(accessVar);

    //Adds the edge.
    processEdge(callerID, callerLabel, variableID, variableLabel, ClangEdge::REFERENCES, attributes);
}

/**
 * Adds a variable inside call.
 * @param result The match result.
 * @param functionParent The function parent.
 * @param varChild The variable child.
 * @param fieldChild The field child.
 */
void ASTWalker::addVariableInsideCall(const MatchFinder::MatchResult result, const clang::FunctionDecl *functionParent,
                                      const clang::VarDecl *varChild, const clang::FieldDecl *fieldChild){
    string functionID = generateID(result, functionParent);
    string functionLabel = generateLabel(result, functionParent);
    string varID;
    string varLabel;

    //Checks whether we have a field or var.
    if (fieldChild == nullptr){
        varID = generateID(result, varChild);
        varLabel = generateLabel(result, varChild);
    } else {
        varID = generateID(result, fieldChild);
        varLabel = generateLabel(result, fieldChild);
    }

    processEdge(functionID, functionLabel, varID, varLabel, ClangEdge::CONTAINS);
}

/**
 * Adds a class call from a class to a decl.
 * @param result The match result.
 * @param classDecl The class decl.
 * @param declID The ID of the decl.
 * @param declLabel The label of the decl.
 */
void ASTWalker::addClassCall(const MatchFinder::MatchResult result, const CXXRecordDecl *classDecl, string declID,
                             string declLabel){
    string classID = generateID(result, classDecl);
    string classLabel = generateLabel(result, classDecl);

    processEdge(classID, classLabel, declID, declLabel, ClangEdge::CONTAINS);
}

/**
 * Adds a class inheritance call.
 * @param result The match result.
 * @param childClass The child class.
 * @param parentClass The parent class.
 */
void ASTWalker::addClassInheritance(const MatchFinder::MatchResult result,
                                    const CXXRecordDecl *childClass, const CXXRecordDecl *parentClass) {
    string classID = generateID(result, childClass);
    string baseID = generateID(result, parentClass);
    string classLabel = generateLabel(result, childClass);
    string baseLabel = generateLabel(result, parentClass);

    processEdge(classID, classLabel, baseID, baseLabel, ClangEdge::INHERITS);
}

/**
 * Adds an enum constant to the graph.
 * @param result The match result.
 * @param enumDecl The enum being added.
 * @param enumConstantDecl The enum constant.
 */
void ASTWalker::addEnumConstantCall(const MatchFinder::MatchResult result, const clang::EnumDecl *enumDecl,
                                    const clang::EnumConstantDecl *enumConstantDecl){
    //Gets the labels.
    string enumID = generateID(result, enumDecl);
    string enumConstID = generateID(result, enumConstantDecl);
    string enumLabel = generateLabel(result, enumDecl);
    string enumConstLabel = generateLabel(result, enumConstantDecl);

    processEdge(enumID, enumLabel, enumConstID, enumConstLabel, ClangEdge::CONTAINS);
}

/**
 * Adds an enum call to the graph.
 * @param result The match result.
 * @param enumDecl The enum decl being added.
 * @param varDecl The var decl.
 * @param fieldDecl The field decl.
 */
void ASTWalker::addEnumCall(const MatchFinder::MatchResult result, const EnumDecl *enumDecl, const VarDecl *varDecl,
                            const FieldDecl *fieldDecl){
    //Generate the labels.
    string enumID = generateID(result, enumDecl);
    string enumLabel = generateLabel(result, enumDecl);
    string refID = (fieldDecl == nullptr) ?
                        generateID(result, varDecl) : generateID(result, fieldDecl);
    string refLabel = (fieldDecl == nullptr) ?
                        generateLabel(result, varDecl) : generateLabel(result, fieldDecl);

    processEdge(enumID, enumLabel, refID, refLabel, ClangEdge::REFERENCES);
}

/**
 * Adds a record call from one record to a declaration.
 * @param result The match result.
 * @param recordDecl The record decl.
 * @param itemDecl The item decl.
 */
void ASTWalker::addRecordCall(const MatchFinder::MatchResult result, const clang::RecordDecl *recordDecl,
                              const clang::DeclaratorDecl *itemDecl){
    //Generate the labels and ID.
    string recordID = generateID(result, recordDecl);
    string recordLabel = generateLabel(result, recordDecl);
    string refID = generateID(result, itemDecl);
    string refLabel = generateLabel(result, itemDecl);

    processEdge(recordID, recordLabel, refID, refLabel, ClangEdge::CONTAINS);
}

/**
 * Adds the usage of a record.
 * @param result The match result.
 * @param recordDecl The record decl.
 * @param varDecl The var decl.
 * @param fieldDecl The field decl.
 */
void ASTWalker::addRecordUseCall(const MatchFinder::MatchResult result, const RecordDecl *recordDecl,
                                 const VarDecl *varDecl, const FieldDecl *fieldDecl){
    string recordID = generateID(result, recordDecl);
    string recordLabel = generateLabel(result, recordDecl);

    //Determine whether we are using a field or variable.
    string refID;
    string refLabel;
    if (fieldDecl == nullptr){
        refID = generateID(result, varDecl);
        refLabel = generateLabel(result, varDecl);
    } else {
        refID = generateID(result, fieldDecl);
        refLabel = generateLabel(result, fieldDecl);
    }

    processEdge(recordID, recordLabel, refID, refLabel, ClangEdge::REFERENCES);
}
/********************************************************************************************************************/
// END AST TO GRAPH PARAMETERS
/********************************************************************************************************************/

/**
 * Processes the edge. This is a generic method.
 * @param srcID The source ID.
 * @param srcLabel The source label.
 * @param dstID The destination ID.
 * @param dstLabel The destination label.
 * @param type The edge type.
 * @param attributes A collection of attributes.
 */
void ASTWalker::processEdge(string srcID, string srcLabel, string dstID, string dstLabel, ClangEdge::EdgeType type,
                            vector<pair<string, string>> attributes){
    //Looks up the nodes by label.
    ClangNode* sourceNode = graph->findNodeByID(srcID);
    ClangNode* destNode = graph->findNodeByID(dstID);

    //Add the edge.
    ClangEdge* edge;
    if (sourceNode && destNode){
        edge = new ClangEdge(sourceNode, destNode, type);
    } else if (!sourceNode && destNode){
        edge = new ClangEdge(srcID, destNode, type);
    } else if (sourceNode && !destNode){
        edge = new ClangEdge(sourceNode, dstID, type);
    } else {
        edge = new ClangEdge(srcID, dstID, type);
    }
    bool succ = graph->addEdge(edge);
    if (!succ) return;

    //Iterate through our vector and add.
    for (auto mapItem : attributes) {
        graph->addAttribute(edge->getSrcID(), edge->getDstID(), type,
                            mapItem.first, mapItem.second);
    }
}

/**
 * Prints the currently processed filename.
 * @param curFile The current file being processed.
 */
void ASTWalker::printFileName(string curFile){
    if (curFile.compare(curFileName) != 0){
        //Ensure we're only outputting a source file.
        if (!isSource(curFile)) return;

        clangPrinter->printFileName(curFile);
        curFileName = curFile;
    }
}

/**
 * Generates an ID string for a given decl.
 * @param result The match result.
 * @param dec The decl.
 * @return ID string.
 */
string ASTWalker::generateIDString(const MatchFinder::MatchResult result, const NamedDecl *dec) {
    //Gets the canonical decl.
    dec = dyn_cast<NamedDecl>(dec->getCanonicalDecl());
    string name = "";

    if (isa<FunctionDecl>(dec) || isa<CXXMethodDecl>(dec)){
        //Generates a special name for function overloading.
        const FunctionDecl* cur = dec->getAsFunction();
        name = cur->getReturnType().getAsString() + "-" + dec->getNameAsString();
        for (int i = 0; i < cur->getNumParams(); i++){
            name += "-" + cur->parameters().data()[i]->getType().getAsString();
        }
    } else if (isa<RecordDecl>(dec) && (dyn_cast<RecordDecl>(dec)->isStruct() || dyn_cast<RecordDecl>(dec)->isUnion())) {
        //Generates a special name for structs and unions (especially anonymous ones).
        const RecordDecl* cur = dyn_cast<RecordDecl>(dec);
        if (isAnonymousRecord(cur->getQualifiedNameAsString())){
            name = ANON_REPLACE + "-" + generateLineNumber(result, cur->getSourceRange().getBegin()) + "-" +
                    generateFileName(result, cur->getSourceRange().getBegin());
        } else {
            name = dec->getNameAsString();
        }
    } else {
        name = dec->getNameAsString();
    }


    bool getParent = true;
    bool recurse = false;
    const NamedDecl* originalDecl = dec;

    //Get the parent.
    auto parent = result.Context->getParents(*dec);
    while(getParent){
        //Check if it's empty.
        if (parent.empty()){
            getParent = false;
            continue;
        }

        //Get the current decl as named.
        dec = parent[0].get<clang::NamedDecl>();
        if (dec) {
            name = generateIDString(result, dec) + "::" + name;
            recurse = true;
            getParent = false;
            continue;
        }

        parent = result.Context->getParents(parent[0]);
    }

    //Sees if no true qualified name was used.
    Decl::Kind kind = originalDecl->getKind();
    if (!recurse) {
        if (kind != Decl::Function && kind == Decl::CXXMethod){
            //We need to get the parent function.
            const DeclContext *parentContext = originalDecl->getParentFunctionOrMethod();

            //If we have nullptr, get the parent function.
            if (parentContext != nullptr) {
                string parentQualName = generateIDString(result, static_cast<const FunctionDecl *>(parentContext));
                name = parentQualName + "::" + originalDecl->getNameAsString();
            }
        }
    }

    return name;
}

/**
 * Generates the line number for the current source location.
 * @param result The match result.
 * @param loc The source location.
 * @return The line number.
 */
string ASTWalker::generateLineNumber(const MatchFinder::MatchResult result, SourceLocation loc){
    int lineNum = result.SourceManager->getSpellingLineNumber(loc);
    return std::to_string(lineNum);
}

/**
 * Checks if the item currently is a source file.
 * @param fileName The file name.
 * @return Whether it is a source file.
 */
bool ASTWalker::isSource(std::string fileName){
    //Iterate through looking.
    for (string item : FILE_EXT){
        if (item.size() > fileName.size()) continue;
        bool curr = std::equal(item.rbegin(), item.rend(), fileName.rbegin());

        if (curr) return true;
    }

    return false;
}

/**
 * Checks whether the record is anonymous.
 * @param qualName The qualified name.
 * @return Whether it is an anonymous record.
 */
bool ASTWalker::isAnonymousRecord(string qualName){
    //Iterate through the anonymous names.
    for (int i = 0; i < ANON_LIST->length(); i++){
        if (ANON_LIST[i].compare(qualName) == 0) return true;
    }

    return false;
}