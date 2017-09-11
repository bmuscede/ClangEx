//
// Created by bmuscede on 05/11/16.
//

#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <openssl/md5.h>
#include "ASTWalker.h"
#include "clang/AST/Mangle.h"
#include "../Graph/ClangNode.h"

using namespace std;
using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;
using namespace llvm;

ASTWalker::~ASTWalker() { }

TAGraph* ASTWalker::getGraph(){
    return graph;
}

void ASTWalker::resolveExternalReferences() {
    graph->resolveExternalReferences(false);
}

void ASTWalker::resolveFiles(){
    bool assumeValid = true;
    vector<ClangNode*> fileNodes = vector<ClangNode*>();
    vector<ClangEdge*> fileEdges = vector<ClangEdge*>();

    //Gets all the associated clang nodes.
    fileParser.processPaths(fileNodes, fileEdges);

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
    graph->addNodesToFile(fileSkip);
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

ASTWalker::ASTWalker(ClangDriver::ClangExclude ex, Printer *print, TAGraph* existing = new TAGraph()) :
        clangPrinter(print){
    //Sets the current file name to blank.
    curFileName = "";

    //Creates the graph system.
    graph = existing;
    graph->setPrinter(print);

    //Sets up the exclusions.
    exclusions = ex;
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

string ASTWalker::generateID(const MatchFinder::MatchResult result, const NamedDecl *dec){
    //Generates the ID.
    string name = generateIDString(result, dec);
    name = generateMD5(name);
    return name;
}

string ASTWalker::generateLabel(const MatchFinder::MatchResult result, const NamedDecl* curDecl) {
    string name = curDecl->getNameAsString();
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
    string ID = generateID(results, dec);
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
    string ID = generateID(result, enumDecl);
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
    string ID = generateID(result, enumDecl);
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
    string ID = generateID(result, structDecl);
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
    string callerID = generateID(results, caller);
    string callerLabel = generateLabel(results, caller);
    string calleeID = generateID(results, callee);
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

void ASTWalker::addClassCall(const MatchFinder::MatchResult result, const CXXRecordDecl *classDecl, string declID,
                             string declLabel){
    string classID = generateID(result, classDecl);
    string classLabel = generateLabel(result, classDecl);

    processEdge(classID, classLabel, declID, declLabel, ClangEdge::CONTAINS);
}

void ASTWalker::addClassInheritance(const MatchFinder::MatchResult result,
                                    const CXXRecordDecl *childClass, const CXXRecordDecl *parentClass) {
    string classID = generateID(result, childClass);
    string baseID = generateID(result, parentClass);
    string classLabel = generateLabel(result, childClass);
    string baseLabel = generateLabel(result, parentClass);

    processEdge(classID, classLabel, baseID, baseLabel, ClangEdge::INHERITS);
}

void ASTWalker::addEnumConstantCall(const MatchFinder::MatchResult result, const clang::EnumDecl *enumDecl,
                                    const clang::EnumConstantDecl *enumConstantDecl){
    //Gets the labels.
    string enumID = generateID(result, enumDecl);
    string enumConstID = generateID(result, enumConstantDecl);
    string enumLabel = generateLabel(result, enumDecl);
    string enumConstLabel = generateLabel(result, enumConstantDecl);

    processEdge(enumID, enumLabel, enumConstID, enumConstLabel, ClangEdge::CONTAINS);
}

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

void ASTWalker::addStructCall(const MatchFinder::MatchResult result, const clang::RecordDecl *structDecl,
                              const clang::DeclaratorDecl *itemDecl){
    //Generate the labels and ID.
    string structID = generateID(result, structDecl);
    string structLabel = generateLabel(result, structDecl);
    string refID = generateID(result, itemDecl);
    string refLabel = generateLabel(result, itemDecl);

    processEdge(structID, structLabel, refID, refLabel, ClangEdge::CONTAINS);
}

void ASTWalker::addStructUseCall(const MatchFinder::MatchResult result, const RecordDecl *structDecl,
                                 const VarDecl *varDecl, const FieldDecl *fieldDecl){
    string structID = generateID(result, structDecl);
    string structLabel = generateLabel(result, structDecl);

    //Determine whether we are using a field or variable.
    string refID;
    string refLabel;
    if (fieldDecl == nullptr){
        refID = generateID(result, fieldDecl);
        refLabel = generateLabel(result, fieldDecl);
    } else {
        refID = generateID(result, varDecl);
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

string ASTWalker::generateIDString(const MatchFinder::MatchResult result, const NamedDecl *dec) {
    //Gets the canonical decl.
    dec = dyn_cast<NamedDecl>(dec->getCanonicalDecl());
    string name = "";

    //Generates a special name for function overloading.
    if (isa<FunctionDecl>(dec) || isa<CXXMethodDecl>(dec)){
        const FunctionDecl* cur = dec->getAsFunction();
        name = cur->getReturnType().getAsString() + "-" + dec->getNameAsString();
        for (int i = 0; i < cur->getNumParams(); i++){
            name += "-" + cur->parameters().data()[i]->getType().getAsString();
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

bool ASTWalker::isSource(std::string fileName){
    //Iterate through looking.
    for (string item : FILE_EXT){
        if (item.size() > fileName.size()) continue;
        bool curr = std::equal(item.rbegin(), item.rend(), fileName.rbegin());

        if (curr) return true;
    }

    return false;
}