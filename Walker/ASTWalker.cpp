//
// Created by bmuscede on 05/11/16.
//

//TODO:
// - Get unions and structs working
// - Verify correctness of union, struct, and enums.

#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <openssl/md5.h>
#include "ASTWalker.h"
#include "clang/AST/Mangle.h"
#include "../File/ClangArgParse.h"
#include "../Graph/ClangNode.h"

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
    bool assumeValid = true;
    vector<ClangNode*> fileNodes = vector<ClangNode*>();
    vector<ClangEdge*> fileEdges = vector<ClangEdge*>();

    //Gets all the associated clang nodes.
    fileParser.processPaths(fileNodes, fileEdges, md5Flag);

    //Adds them to the graph.
    for (ClangNode *file : fileNodes) {
        if ((file->getType() == ClangNode::NodeType::SUBSYSTEM && !exclusions.cSubSystem) ||
                (file->getType() == ClangNode::NodeType::FILE && !exclusions.cFile)) {
            graph->addNode(file, assumeValid);
        }
    }

    //Adds the edges to the graph.
    map<string, ClangNode*> fileSkip;
    for (ClangEdge *edge : fileEdges) {
        //Surpasses.
        if (exclusions.cFile && edge->getDst()->getType() == ClangNode::FILE){
            fileSkip[edge->getDst()->getID()] = edge->getSrc();
        } else {
            graph->addEdge(edge, assumeValid);
        }
    }

    //Next, for each item in the graph, add it to a file.
    graph->addNodesToFile(fileSkip, md5Flag);
}

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

    if (Entry == nullptr) return string();

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

string ASTWalker::generateID(string fileName, string qualifiedName){
    string genID = fileName + "[" + qualifiedName + "]";

    //Check what flag we're operating on.
    if (md5Flag){
        genID = generateMD5(genID);
    } else {
        genID = removeInvalidIDSymbols(genID);
    }

    return genID;
}

string ASTWalker::generateLabel(const Decl* decl, ClangNode::NodeType type){
    string label;

    //Determines how we populate the string.
    if (decl == nullptr) return string();
    switch (type) {
        case ClangNode::FUNCTION: {
            auto funcDecl = decl->getAsFunction();
            if (funcDecl == nullptr) return string();

            label = funcDecl->getQualifiedNameAsString();

            break;
        }
        case ClangNode::VARIABLE: {
            const VarDecl *var = static_cast<const VarDecl *>(decl);
            if (var == nullptr) return string();

            label = var->getQualifiedNameAsString();

            //We need to get the parent function.
            const DeclContext *parentContext = var->getParentFunctionOrMethod();

            //If we have nullptr, get the parent function.
            if (parentContext != nullptr) {
                string parentQualName = static_cast<const FunctionDecl *>(parentContext)->getQualifiedNameAsString();
                label = parentQualName + "::" + label;
            }
            break;
        }
        case ClangNode::CLASS: {
            auto classDecl = static_cast<const CXXRecordDecl *>(decl);
            if (classDecl == nullptr) return string();

            label = classDecl->getQualifiedNameAsString();
            break;
        }
        case ClangNode::ENUM: {
            auto enumDecl = static_cast<const EnumDecl *>(decl);
            if (enumDecl == nullptr) return string();

            label = enumDecl->getQualifiedNameAsString();
            break;
        }
        case ClangNode::ENUM_CONST: {
            auto enumDecl = static_cast<const EnumConstantDecl*>(decl);
            if (enumDecl == nullptr) return string();

            label = enumDecl->getQualifiedNameAsString();
            break;
        }
        case ClangNode::UNION:
        case ClangNode::STRUCT: {
            auto recordDecl = static_cast<const RecordDecl*>(decl);
            if (recordDecl == nullptr) return string();

            label = recordDecl->getQualifiedNameAsString();
            break;
        }
        default: {
            label = string();
        }
    }

    //Removes all invalid symbols.
    label = removeInvalidSymbols(label);
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

void ASTWalker::performAddClassCall(const MatchFinder::MatchResult result, const DeclaratorDecl *decl,
                                    ClangNode::NodeType type){
    //Checks if we can add a class reference.
    CXXRecordDecl* classDecl = extractClass(decl->getQualifier());
    if (classDecl != nullptr && !exclusions.cClass) {
        string declLabel = generateLabel(decl, type);
        addClassCall(result, classDecl, declLabel);
    }
}

CXXRecordDecl* ASTWalker::extractClass(NestedNameSpecifier* name){
    //Checks if the qualifier is null.
    if (name == nullptr || name->getAsType() == nullptr) return nullptr;
    return name->getAsType()->getAsCXXRecordDecl();
}

/********************************************************************************************************************/
// START AST TO GRAPH PARAMETERS
/********************************************************************************************************************/
void ASTWalker::addFunctionDecl(const MatchFinder::MatchResult results, const DeclaratorDecl *dec) {
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

void ASTWalker::addClassDecl(const MatchFinder::MatchResult results, const CXXRecordDecl *classDecl, string fName){
    //Get the definition.
    auto def = classDecl->getDefinition();
    if (def == nullptr) return;

    //Check if we're dealing with a class.
    if (!classDecl->isClass()) return;

    //Generate the fields for the node.
    string filename = (fName.compare("") == 0) ? generateFileName(results, classDecl->getInnerLocStart(), true) : fName;
    string ID = generateID(filename, classDecl->getQualifiedNameAsString());
    string className = generateLabel(classDecl, ClangNode::CLASS);
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
    graph->addNode(node);

    //Process attributes.
    graph->addSingularAttribute(node->getID(),
                                ClangNode::FILE_ATTRIBUTE.attrName,
                                ClangNode::FILE_ATTRIBUTE.processFileName(filename));
    graph->addSingularAttribute(node->getID(),
                                ClangNode::BASE_ATTRIBUTE.attrName,
                                std::to_string(numBases));

    //Get base classes.
    if (classDecl->getNumBases() > 0) {
        for (auto base = classDecl->bases_begin(); base != classDecl->bases_end(); base++) {
            if (base->getType().getTypePtr() == nullptr) continue;
            CXXRecordDecl *baseClass = base->getType().getTypePtr()->getAsCXXRecordDecl();
            if (baseClass == nullptr) continue;

            //Add a linkage in our graph->
            addClassInheritance(classDecl, baseClass);
        }
    }
}

void ASTWalker::addEnumDecl(const MatchFinder::MatchResult result, const EnumDecl *enumDecl, string spoofFilename){
    //Generate the fields for the node.
    string filename = (spoofFilename.compare(string()) == 0) ?
                      generateFileName(result, enumDecl->getInnerLocStart()) : spoofFilename;
    string ID = generateID(filename, enumDecl->getQualifiedNameAsString());
    string enumName = generateLabel(enumDecl, ClangNode::ENUM);
    if (ID.compare("") == 0 || filename.compare("") == 0) return;

    //Creates a enum entry.
    ClangNode* node = new ClangNode(ID, enumName, ClangNode::ENUM);
    graph->addNode(node);

    //Process attributes.
    graph->addSingularAttribute(node->getID(),
                                ClangNode::FILE_ATTRIBUTE.attrName,
                                ClangNode::FILE_ATTRIBUTE.processFileName(filename));
}

void ASTWalker::addEnumConstantDecl(const MatchFinder::MatchResult result, const clang::EnumConstantDecl *enumDecl,
                                    string filenameSpoof){
    //Generate the fields for the node.
    string filename = (filenameSpoof.compare(string()) == 0) ?
                      generateFileName(result, enumDecl->getLocStart()) : filenameSpoof;
    string ID = generateID(filename, enumDecl->getQualifiedNameAsString());
    string enumName = generateLabel(enumDecl, ClangNode::ENUM_CONST);
    if (ID.compare("") == 0 || filename.compare("") == 0) return;

    //Creates a new enum entry.
    ClangNode* node = new ClangNode(ID, enumName, ClangNode::ENUM_CONST);
    graph->addNode(node);

    //Process attributes.
    graph->addSingularAttribute(node->getID(),
                                ClangNode::FILE_ATTRIBUTE.attrName,
                                ClangNode::FILE_ATTRIBUTE.processFileName(filename));
}

void ASTWalker::addStructDecl(const MatchFinder::MatchResult result, const clang::RecordDecl *structDecl){
    bool isAnonymous = false;

    //First, generates a specialized name.
    string qualifiedName = structDecl->getQualifiedNameAsString();
    for (string currList : ANON_LIST){
        if (qualifiedName.find(currList) != std::string::npos) {
            qualifiedName += "::" + std::to_string(result.SourceManager->getSpellingLineNumber(structDecl->getLocStart()));
            isAnonymous = true;
            break;
        }
    }

    //With that, generates the ID, label, and filename.
    string filename = generateFileName(result, structDecl->getInnerLocStart());
    string ID = generateID(filename, qualifiedName);
    string label = generateLabel(structDecl, ClangNode::STRUCT);

    //Next, generates the node.
    ClangNode* node = new ClangNode(ID, label, ClangNode::STRUCT);
    graph->addNode(node);

    //Process the attributes.
    graph->addSingularAttribute(node->getID(),
                                ClangNode::FILE_ATTRIBUTE.attrName,
                                ClangNode::FILE_ATTRIBUTE.processFileName(filename));
    graph->addSingularAttribute(node->getID(),
                                ClangNode::STRUCT_ATTRIBUTE.anonymousName,
                                ClangNode::STRUCT_ATTRIBUTE.processAnonymous(isAnonymous));
}

void ASTWalker::addFunctionCall(const MatchFinder::MatchResult results, const DeclaratorDecl* caller,
                                const FunctionDecl* callee){
    //Generate a label for the two functions.
    string callerLabel = generateLabel(caller, ClangNode::FUNCTION);
    string calleeLabel = generateLabel(callee, ClangNode::FUNCTION);

    //Next, we find by ID.
    vector<ClangNode*> callerNode = graph->findNodeByName(callerLabel);
    vector<ClangNode*> calleeNode = graph->findNodeByName(calleeLabel);

    //TODO: Function call attributes?

    //Check if we have an already known reference.
    if (callerNode.size() == 0 || calleeNode.size() == 0){
        //Add to unresolved reference list.
        graph->addUnresolvedRef(callerLabel, calleeLabel, ClangEdge::CALLS);
    } else {
        //We finally add the edge.
        ClangEdge* edge = new ClangEdge(callerNode.at(0), calleeNode.at(0), ClangEdge::CALLS);
        graph->addEdge(edge);

    }
}

void ASTWalker::addVariableCall(const MatchFinder::MatchResult result, const DeclaratorDecl *caller,
                                const Expr* expr, const VarDecl *varCallee,
                                const FieldDecl *fieldCallee){
    string variableName;
    string variableShortName;

    //Generate the information associated with the caller.
    string callerName = generateLabel(caller, ClangNode::FUNCTION);

    //Decide how we should process.
    if (fieldCallee == nullptr){
        variableName = generateLabel(varCallee, ClangNode::VARIABLE);
        variableShortName = varCallee->getName();
    } else {
        variableName = generateLabel(fieldCallee, ClangNode::VARIABLE);
        variableShortName = fieldCallee->getName();
    }

    //Generate the attributes.
    auto variableAccessName = ClangEdge::ACCESS_ATTRIBUTE.attrName;
    auto variableAccess = ClangEdge::ACCESS_ATTRIBUTE.getVariableAccess(expr, variableShortName);

    //Next, we find their IDs.
    vector<ClangNode*> callerNode = graph->findNodeByName(callerName);
    vector<ClangNode*> varNode = graph->findNodeByName(variableName);

    //Check to see if we have these entries already done.
    if (callerNode.size() == 0 || varNode.size() == 0){
        //Add to unresolved reference list.
        graph->addUnresolvedRef(callerName, variableName, ClangEdge::REFERENCES);

        //Add attributes.
        graph->addUnresolvedRefAttr(callerName, variableName, variableAccessName, variableAccess);
    } else {
        //Add the edge.
        ClangEdge* edge = new ClangEdge(callerNode.at(0), varNode.at(0), ClangEdge::REFERENCES);
        graph->addEdge(edge);

        //Process attributes.
        graph->addAttribute(callerNode.at(0)->getID(), varNode.at(0)->getID(), ClangEdge::REFERENCES,
                            variableAccessName, variableAccess);
    }
}

void ASTWalker::addClassCall(const MatchFinder::MatchResult result, const CXXRecordDecl *classDecl, string declLabel){
    string classLabel = generateLabel(classDecl, ClangNode::CLASS);

    //Looks up the entities being added.
    vector<ClangNode*> src = graph->findNodeByName(classLabel);
    vector<ClangNode*> dst = graph->findNodeByName(declLabel);

    //Check if we found the entity.
    if (src.size() == 0 || dst.size() == 0){
        //Add to unresolved reference list.
        graph->addUnresolvedRef(classLabel, declLabel, ClangEdge::REFERENCES);
    } else {
        //Add the edge.
        ClangEdge* edge = new ClangEdge(src.at(0), dst.at(0), ClangEdge::REFERENCES);
        graph->addEdge(edge);
    }
}

void ASTWalker::addClassInheritance(const CXXRecordDecl *childClass, const CXXRecordDecl *parentClass) {
    string classLabel = generateLabel(childClass, ClangNode::CLASS);
    string baseLabel = generateLabel(parentClass, ClangNode::CLASS);

    //Get the nodes by their label.
    vector<ClangNode*> classNode = graph->findNodeByName(classLabel);
    vector<ClangNode*> baseNode = graph->findNodeByName(baseLabel);

    //Check to see if we don't have these entries.
    if (classNode.size() == 0 || baseNode.size() == 0){
        graph->addUnresolvedRef(classLabel, baseLabel, ClangEdge::INHERITS);
    } else {
        //Add the edge.
        ClangEdge* edge = new ClangEdge(classNode.at(0), baseNode.at(0), ClangEdge::INHERITS);
        graph->addEdge(edge);
    }
}

void ASTWalker::addEnumConstantCall(const MatchFinder::MatchResult result, const clang::EnumDecl *enumDecl,
                                    const clang::EnumConstantDecl *enumConstantDecl){
    //Gets the labels.
    string enumLabel = generateLabel(enumDecl, ClangNode::ENUM);
    string enumConstLabel = generateLabel(enumConstantDecl, ClangNode::ENUM_CONST);

    //Looks up the nodes by label.
    vector<ClangNode*> enumNode = graph->findNodeByName(enumLabel);
    vector<ClangNode*> enumConstNode = graph->findNodeByName(enumConstLabel);

    if (enumNode.size() == 0 || enumConstNode.size() == 0){
        graph->addUnresolvedRef(enumLabel, enumConstLabel, ClangEdge::CONTAINS);
    }  else {
        //Add the edge.
        ClangEdge* edge = new ClangEdge(enumNode.at(0), enumConstNode.at(0), ClangEdge::CONTAINS);
        graph->addEdge(edge);
    }
}

void ASTWalker::addEnumCall(const MatchFinder::MatchResult result, const EnumDecl *enumDecl, const VarDecl *varDecl,
                            const FieldDecl *fieldDecl){
    //Generate the labels.
    string enumLabel = generateLabel(enumDecl, ClangNode::ENUM);
    string refLabel = (fieldDecl == nullptr) ?
                      generateLabel(varDecl, ClangNode::VARIABLE) : generateLabel(fieldDecl, ClangNode::VARIABLE);

    //Looks up the nodes by label.
    vector<ClangNode*> enumNode = graph->findNodeByName(enumLabel);
    vector<ClangNode*> refNode = graph->findNodeByName(refLabel);

    if (enumNode.size() == 0 || refNode.size() == 0){
        graph->addUnresolvedRef(enumLabel, refLabel, ClangEdge::REFERENCES);
    }  else {
        //Add the edge.
        ClangEdge* edge = new ClangEdge(enumNode.at(0), refNode.at(0), ClangEdge::REFERENCES);
        graph->addEdge(edge);
    }
}

void ASTWalker::addStructCall(const MatchFinder::MatchResult result, const clang::RecordDecl *structDecl,
                              const clang::DeclaratorDecl *itemDecl, ClangNode::NodeType inputType){
    //First, determine the declarator's type.
    ClangNode::NodeType type = (inputType == ClangNode::NodeType::SUBSYSTEM) ?
                                    ClangNode::convertToNodeType(itemDecl->getKind()) : inputType;

    //Generate the labels.
    string structLabel = generateLabel(structDecl, ClangNode::STRUCT);
    string refLabel = generateLabel(itemDecl, type);

    //Looks up the nodes by label.
    vector<ClangNode*> structNode = graph->findNodeByName(structLabel);
    vector<ClangNode*> refNode = graph->findNodeByName(refLabel);

    if (structNode.size() == 0 || refNode.size() == 0){
        graph->addUnresolvedRef(structLabel, refLabel, ClangEdge::CONTAINS);
    }  else {
        //Add the edge.
        ClangEdge* edge = new ClangEdge(structNode.at(0), refNode.at(0), ClangEdge::CONTAINS);
        graph->addEdge(edge);
    }
}

void ASTWalker::addStructUseCall(const MatchFinder::MatchResult result, const RecordDecl *structDecl,
                                 const VarDecl *varDecl, const FieldDecl *fieldDecl){
    string structLabel = generateLabel(structDecl, ClangNode::STRUCT);
    string refLabel;

    //Determine whether we are using a field or variable.
    if (fieldDecl == nullptr){
        refLabel = generateLabel(fieldDecl, ClangNode::VARIABLE);
    } else {
        refLabel = generateLabel(varDecl, ClangNode::VARIABLE);
    }

    //Looks up the nodes by label.
    vector<ClangNode*> structNode = graph->findNodeByName(structLabel);
    vector<ClangNode*> refNode = graph->findNodeByName(refLabel);

    if (structNode.size() == 0 || refNode.size() == 0){
        graph->addUnresolvedRef(structLabel, refLabel, ClangEdge::REFERENCES);
    }  else {
        //Add the edge.
        ClangEdge* edge = new ClangEdge(structNode.at(0), refNode.at(0), ClangEdge::REFERENCES);
        graph->addEdge(edge);
    }
}
/********************************************************************************************************************/
// END AST TO GRAPH PARAMETERS
/********************************************************************************************************************/

void ASTWalker::printFileName(string curFile){
    if (curFile.compare(curFileName) != 0){
        cout << "\tCurrently processing: " << curFile << endl;
        curFileName = curFile;
    }
}

string ASTWalker::replaceLabel(string label, string init, string aft){
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

string ASTWalker::removeInvalidIDSymbols(string label) {
    replace(label.begin(), label.end(), ':', '-');
    return removeInvalidSymbols(label);
}

string ASTWalker::removeInvalidSymbols(string label) {
    //Check if we need to remove a symbol.
    for (int i = 0; i < ANON_SIZE; i++){
        string item = ANON_LIST[i];
        if (label.find(item) != string::npos)
            label = replaceLabel(label, item, ANON_REPLACE);

    }
    replace(label.begin(), label.end(), '=', 'e');

    return label;
}