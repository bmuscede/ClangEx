//
// Created by bmuscede on 05/11/16.
//

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
        clangPrinter->printGenTADone(fileName, false);
        return;
    }

    taFile << tupleAttribute;
    taFile.close();

    clangPrinter->printGenTADone(fileName, true);
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

ASTWalker::ASTWalker(ClangArgParse::ClangExclude ex, bool md5, Printer *print, TAGraph* existing = new TAGraph()) :
        clangPrinter(print){
    //Sets the current file name to blank.
    curFileName = "";

    //Creates the graph system.
    graph = existing;
    graph->setPrinter(print);

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

string ASTWalker::generateID(const MatchFinder::MatchResult result, const TagDecl* dec, ClangNode::NodeType type){
    return generateID(result, dec, type, dec->getLocStart());
}

string ASTWalker::generateID(const MatchFinder::MatchResult result, const DeclaratorDecl* dec, ClangNode::NodeType type){
    return generateID(result, dec, type, dec->getLocStart());
}

string ASTWalker::generateID(const MatchFinder::MatchResult result, const NamedDecl *dec,
                             const ClangNode::NodeType type, const SourceLocation loc){
    //Starts by generating fields.
    string filename = generateFileName(result, loc, true);
    string qualifiedName = generateLabel(dec, type);
    string ID;

    //Checks how we generate.
    Decl::Kind curKind = dec->getKind();
    bool success = false;
    switch(curKind){
        case Decl::Kind::Function:
        case Decl::Kind::Var:
            ID = generateMangledName(result, dec, success);
            if (success) break;
        default:
            ID = generateFileName(result, loc, true) + "[" + qualifiedName + "]";
    }

    //Check what flag we're operating on.
    if (md5Flag){
        ID = generateMD5(ID);
    } else {
        ID = removeInvalidIDSymbols(ID);
    }

    return ID;
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

            label = classDecl->getQualifiedNameAsString() + CLASS_PREPEND;
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

string ASTWalker::generateLabel(const MatchFinder::MatchResult result, const NamedDecl* curDecl){
    string name = curDecl->getNameAsString();
    bool getParent = true;

    //Get the parent.
    auto parent = result.Context->getParents(*curDecl);
    while(getParent){
        //Check if it's empty.
        if (parent.empty()){
            getParent = false;
            continue;
        }

        //Get the current decl as named.
        curDecl = parent[0].get<clang::NamedDecl>();
        if (curDecl) {
            name = curDecl->getNameAsString() + "::" + name;
        }

        parent = result.Context->getParents(parent[0]);
    }

    return removeInvalidSymbols(name);
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

string ASTWalker::generateLineNumber(const MatchFinder::MatchResult result, SourceLocation loc){
    int lineNum = result.SourceManager->getSpellingLineNumber(loc);
    return std::to_string(lineNum);
}

string ASTWalker::generateMangledName(const MatchFinder::MatchResult result, const NamedDecl *dec, bool &success){
    auto mangleContext = result.Context->createMangleContext();

    //Check whether we need to mangle.
    if (!mangleContext->shouldMangleDeclName(dec)) {
        success = false;
        delete mangleContext;
        return dec->getQualifiedNameAsString();
    }

    //Mangle the CXX name.
    string mangledName;
    raw_string_ostream stream(mangledName);

    //Check how we mangle.
    mangleContext->mangleName(dec, stream);
    stream.flush();

    success = true;
    delete mangleContext;
    return mangledName;
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

CXXRecordDecl* ASTWalker::extractClass(NestedNameSpecifier* name){
    //Checks if the qualifier is null.
    if (name == nullptr || name->getAsType() == nullptr) return nullptr;
    return name->getAsType()->getAsCXXRecordDecl();
}

/********************************************************************************************************************/
// START AST TO GRAPH PARAMETERS
/********************************************************************************************************************/
void ASTWalker::addFunctionDecl(const MatchFinder::MatchResult results, const FunctionDecl *dec) {
    //Generate the fields for the node.
    string label = generateLabel(results, dec);
    string filename = generateFileName(results, dec->getInnerLocStart());
    string ID = generateID(results, dec, ClangNode::FUNCTION);
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
        label = generateLabel(results, fieldDec);
        filename = generateFileName(results, fieldDec->getInnerLocStart());
        ID = generateID(results, fieldDec, ClangNode::VARIABLE);
        scopeInfo = ClangNode::VAR_ATTRIBUTE.getScope(fieldDec);
        staticInfo = ClangNode::VAR_ATTRIBUTE.getStatic(fieldDec);
    } else {
        label = generateLabel(results, varDec);
        filename = generateFileName(results, varDec->getInnerLocStart());
        ID = generateID(results, varDec, ClangNode::VARIABLE);
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
    string ID = generateID(results, classDecl, ClangNode::CLASS);
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
            addClassInheritance(results, classDecl, baseClass);
        }
    }
}

void ASTWalker::addEnumDecl(const MatchFinder::MatchResult result, const EnumDecl *enumDecl, string spoofFilename){
    //Generate the fields for the node.
    string filename = (spoofFilename.compare(string()) == 0) ?
                      generateFileName(result, enumDecl->getInnerLocStart()) : spoofFilename;
    string ID = generateID(result, enumDecl, ClangNode::ENUM);
    string enumName = generateLabel(result, enumDecl);
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
    string ID = generateID(result, enumDecl, ClangNode::ENUM_CONST, enumDecl->getLocStart());
    string enumName = generateLabel(result, enumDecl);
    if (ID.compare("") == 0 || filename.compare("") == 0) return;

    //Creates a new enum entry.
    ClangNode* node = new ClangNode(ID, enumName, ClangNode::ENUM_CONST);
    graph->addNode(node);

    //Process attributes.
    graph->addSingularAttribute(node->getID(),
                                ClangNode::FILE_ATTRIBUTE.attrName,
                                ClangNode::FILE_ATTRIBUTE.processFileName(filename));
}

void ASTWalker::addStructDecl(const MatchFinder::MatchResult result, const clang::RecordDecl *structDecl, string filename){
    //Checks whether the function is anonymous.
    string qualifiedName = structDecl->getQualifiedNameAsString();
    bool isAnonymous = structDecl->isInAnonymousNamespace();

    //With that, generates the ID, label, and filename.
    if (filename.compare("") == 0) generateFileName(result, structDecl->getInnerLocStart());
    string ID = generateID(result, structDecl, ClangNode::STRUCT);
    string label = generateLabel(result, structDecl);

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
    string callerID = generateID(results, caller, ClangNode::FUNCTION);
    string callerLabel = generateLabel(results, caller);
    string calleeID = generateID(results, callee, ClangNode::FUNCTION);
    string calleeLabel = generateLabel(results, callee);

    processEdge(callerID, callerLabel, calleeID, calleeLabel, ClangEdge::CALLS);
}

void ASTWalker::addVariableCall(const MatchFinder::MatchResult result, const DeclaratorDecl *caller,
                                const Expr* expr, const VarDecl *varCallee,
                                const FieldDecl *fieldCallee){
    string variableID;
    string variableLabel;
    string variableShortName;

    //Generate the information associated with the caller.
    string callerID = generateID(result, caller, ClangNode::FUNCTION);
    string callerLabel = generateLabel(result, caller);

    //Decide how we should process.
    if (fieldCallee == nullptr){
        variableID = generateID(result, varCallee, ClangNode::VARIABLE);
        variableLabel = generateLabel(result, varCallee);
        variableShortName = varCallee->getName();
    } else {
        variableID = generateID(result, fieldCallee, ClangNode::VARIABLE);
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

void ASTWalker::addVariableInsideCall(const MatchFinder::MatchResult result, const clang::FunctionDecl *functionParent,
                                      const clang::VarDecl *varChild, const clang::FieldDecl *fieldChild){
    string functionID = generateID(result, functionParent, ClangNode::FUNCTION);
    string functionLabel = generateLabel(result, functionParent);
    string varID;
    string varLabel;

    //Checks whether we have a field or var.
    if (fieldChild == nullptr){
        varID = generateID(result, varChild, ClangNode::VARIABLE);
        varLabel = generateLabel(result, varChild);
    } else {
        varID = generateID(result, fieldChild, ClangNode::VARIABLE);
        varLabel = generateLabel(result, fieldChild);
    }

    processEdge(functionID, functionLabel, varID, varLabel, ClangEdge::CONTAINS);
}

void ASTWalker::addClassCall(const MatchFinder::MatchResult result, const CXXRecordDecl *classDecl, string declID,
                             string declLabel){
    string classID = generateID(result, classDecl, ClangNode::CLASS);
    string classLabel = generateLabel(result, classDecl);

    processEdge(classID, classLabel, declID, declLabel, ClangEdge::CONTAINS);
}

void ASTWalker::addClassInheritance(const MatchFinder::MatchResult result,
                                    const CXXRecordDecl *childClass, const CXXRecordDecl *parentClass) {
    string classID = generateID(result, childClass, ClangNode::CLASS);
    string baseID = generateID(result, parentClass, ClangNode::CLASS);
    string classLabel = generateLabel(result, childClass);
    string baseLabel = generateLabel(result, parentClass);

    processEdge(classID, classLabel, baseID, baseLabel, ClangEdge::INHERITS);
}

void ASTWalker::addEnumConstantCall(const MatchFinder::MatchResult result, const clang::EnumDecl *enumDecl,
                                    const clang::EnumConstantDecl *enumConstantDecl){
    //Gets the labels.
    string enumID = generateID(result, enumDecl, ClangNode::ENUM);
    string enumConstID = generateID(result, enumConstantDecl, ClangNode::ENUM_CONST, enumConstantDecl->getLocStart());
    string enumLabel = generateLabel(result, enumDecl);
    string enumConstLabel = generateLabel(result, enumConstantDecl);

    processEdge(enumID, enumLabel, enumConstID, enumConstLabel, ClangEdge::CONTAINS);
}

void ASTWalker::addEnumCall(const MatchFinder::MatchResult result, const EnumDecl *enumDecl, const VarDecl *varDecl,
                            const FieldDecl *fieldDecl){
    //Generate the labels.
    string enumID = generateID(result, enumDecl, ClangNode::ENUM);
    string enumLabel = generateLabel(result, enumDecl);
    string refID = (fieldDecl == nullptr) ?
                        generateID(result, varDecl, ClangNode::VARIABLE) : generateID(result, fieldDecl, ClangNode::VARIABLE);
    string refLabel = (fieldDecl == nullptr) ?
                        generateLabel(result, varDecl) : generateLabel(result, fieldDecl);

    processEdge(enumID, enumLabel, refID, refLabel, ClangEdge::REFERENCES);
}

void ASTWalker::addStructCall(const MatchFinder::MatchResult result, const clang::RecordDecl *structDecl,
                              const clang::DeclaratorDecl *itemDecl){
    //Generate the labels and ID.
    string structID = generateID(result, structDecl, ClangNode::STRUCT);
    string structLabel = generateLabel(result, structDecl);
    string refID = generateID(result, itemDecl, ClangNode::convertToNodeType(itemDecl->getKind()));
    string refLabel = generateLabel(result, itemDecl);

    processEdge(structID, structLabel, refID, refLabel, ClangEdge::CONTAINS);
}

void ASTWalker::addStructUseCall(const MatchFinder::MatchResult result, const RecordDecl *structDecl,
                                 const VarDecl *varDecl, const FieldDecl *fieldDecl){
    string structID = generateID(result, structDecl, ClangNode::STRUCT);
    string structLabel = generateLabel(result, structDecl);

    //Determine whether we are using a field or variable.
    string refID;
    string refLabel;
    if (fieldDecl == nullptr){
        refID = generateID(result, fieldDecl, ClangNode::VARIABLE);
        refLabel = generateLabel(result, fieldDecl);
    } else {
        refID = generateID(result, varDecl, ClangNode::VARIABLE);
        refLabel = generateLabel(result, varDecl);
    }

    processEdge(structID, structLabel, refID, refLabel, ClangEdge::REFERENCES);
}
/********************************************************************************************************************/
// END AST TO GRAPH PARAMETERS
/********************************************************************************************************************/

void ASTWalker::processEdge(string srcID, string srcLabel, string dstID, string dstLabel, ClangEdge::EdgeType type,
                            vector<pair<string, string>> attributes){
    //Looks up the nodes by label.
    vector<ClangNode*> sourceNode = graph->findNodeByName(srcLabel);
    vector<ClangNode*> destNode = graph->findNodeByName(dstLabel);

    int srcNum = 0;
    int dstNum = 0;

    //Now processes the nodes.
    if (sourceNode.size() == 0 || destNode.size() == 0){
        graph->addUnresolvedRef(srcLabel, dstLabel, type);

        //Iterate through our vector and add.
        for (auto mapItem : attributes) {
            //Add attributes.
            graph->addUnresolvedRefAttr(srcLabel, dstLabel, mapItem.first, mapItem.second);
        }
    } else {
        //Add the edge.
        ClangEdge* edge = new ClangEdge(sourceNode.at(srcNum), destNode.at(dstNum), type);
        graph->addEdge(edge);

        //Iterate through our vector and add.
        for (auto mapItem : attributes) {
            graph->addAttribute(edge->getSrc()->getID(), edge->getDst()->getID(), type,
                                mapItem.first, mapItem.second);
        }
    }
}

void ASTWalker::printFileName(string curFile){
    if (curFile.compare(curFileName) != 0){
        //Ensure we're only outputting a source file.
        if (!isSource(curFile)) return;

        clangPrinter->printFileName(curFile);
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
    replace(label.begin(), label.end(), ' ', '_');
    replace(label.begin(), label.end(), '(', '_');
    replace(label.begin(), label.end(), ')', '_');
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

bool ASTWalker::canCollapse(vector<ClangNode*> node){
    //Check the size.
    if (node.size() != 2) return false;

    //Next, perform the collapse check.
    ClangNode* c1 = node.at(0);
    ClangNode* c2 = node.at(1);
    if (c1->getID().find("[" + c1->getName() + "]") != string::npos &&
        c2->getID().find("[" + c2->getName() + "]") != string::npos){
        //Collapse candidate is good.
        collapseDuplicates(node);
        return true;
    }

    return false;
}

void ASTWalker::collapseDuplicates(vector<ClangNode*> duplicates){
    ClangNode* master = duplicates.at(0);

    //Starts at 1 and continues.
    for (int i = 1; i < duplicates.size(); i++){
        //Start by getting all the src edges.
        vector<ClangEdge*> srcEdges = graph->findEdgesBySrcID(duplicates.at(i));
        for (ClangEdge* edge : srcEdges) edge->setSrc(master);

        //Get all the dst edges.
        vector<ClangEdge*> dstEdges = graph->findEdgesByDstID(duplicates.at(i));
        for (ClangEdge* edge : dstEdges) edge->setDst(master);

        //Delete the node.
        graph->removeNode(duplicates.at(i));
    }
}

bool ASTWalker::isSource(std::string fileName){
    //Iterate through looking.
    for (string item : FILE_EXT){
        if (item.size() > fileName.size()) continue;
        bool curr = std::equal(item.rbegin(), item.rend(), fileName.rbegin());

        if (curr) return true;
    }

    return false;
}