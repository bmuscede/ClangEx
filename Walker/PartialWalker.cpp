//
// Created by bmuscede on 06/12/16.
//

#include <iostream>
#include "PartialWalker.h"

using namespace std;
using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;
using namespace llvm;

FullWalker::FullWalker(ClangArgParse::ClangExclude exclusions, TAGraph* graph) : ASTWalker(exclusions, graph){ }

FullWalker::~FullWalker() { }

void FullWalker::run(const MatchFinder::MatchResult &result) {
    //Look for the AST matcher being triggered.
    if (const DeclaratorDecl *dec = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[FUNC_DEC])) {
        //If a function has been found.
        addFunctionDecl(result, dec);
    } else if (const CallExpr *expr = result.Nodes.getNodeAs<clang::CallExpr>(types[FUNC_CALL])){
        //If a function call has been found.
        auto *caller = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[CALLER]);
        addFunctionCall(result, expr, caller);
    } else if (const VarDecl *dec = result.Nodes.getNodeAs<clang::VarDecl>(types[VAR_DEC])){
        //If a variable declaration has been found.
        addVariableDecl(result, dec);
    } else if (const VarDecl *dec = result.Nodes.getNodeAs<clang::VarDecl>(types[VAR_CALL])){
        //If a variable reference has been found.
        auto *caller = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[CALLER_VAR]);
        auto *expr = result.Nodes.getNodeAs<clang::Expr>(types[VAR_EXPR]);

        addVariableRef(result, dec, caller, expr);
    } else if (const DeclaratorDecl *dec = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[CLASS_DEC_FUNC])){
        //Get the CXXRecordDecl.
        if (dec->getQualifier() == NULL || dec->getQualifier()->getAsType() == NULL) return;
        CXXRecordDecl *record = dec->getQualifier()->getAsType()->getAsCXXRecordDecl();
        if (record == NULL) return;

        //Get the filename.
        string filename = generateFileName(result, dec->getInnerLocStart());

        //If a class declaration was found.
        addClassDecl(result, record, filename);
        addClassRef(result, record, dec);
    } else if (const VarDecl *var = result.Nodes.getNodeAs<clang::VarDecl>(types[CLASS_DEC_VAR])){
        //Get the CXXRecordDecl.
        if (var->getQualifier() == NULL || var->getQualifier()->getAsType() == NULL) return;
        CXXRecordDecl *record = var->getQualifier()->getAsType()->getAsCXXRecordDecl();
        if (record == NULL) return;

        //Get the filename.
        string filename = generateFileName(result, var->getInnerLocStart());

        //If a class declaration was found.
        addClassDecl(result, record, filename);
        addClassRef(result, record, var);
    } else if (const DeclaratorDecl *dec = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[CLASS_DEC_VAR_TWO])){
        //Get the variable declaration.
        auto *var = result.Nodes.getNodeAs<clang::VarDecl>(types[CLASS_DEC_VAR_THREE]);

        //Get the CXXRecordDecl.
        if (dec->getQualifier() == NULL || dec->getQualifier()->getAsType() == NULL) return;
        CXXRecordDecl *record = dec->getQualifier()->getAsType()->getAsCXXRecordDecl();
        if (record == NULL) return;

        //Get the filename.
        string filename = generateFileName(result, dec->getInnerLocStart());

        //If a class declaration was found.
        addClassDecl(result, record, filename);
        addClassRef(result, record, var);
    } else if (const RecordDecl *rec = result.Nodes.getNodeAs<clang::RecordDecl>(types[STRUCT_DEC])){
        cout << "The struct found is: " << rec->getQualifiedNameAsString() << endl;
    } else if (const RecordDecl *rec = result.Nodes.getNodeAs<clang::RecordDecl>(types[UNION_DEC])){
        cout << "The union found is: " << rec->getQualifiedNameAsString() << endl;
    } else if (const EnumDecl *dec = result.Nodes.getNodeAs<clang::EnumDecl>(types[ENUM_DEC])){
        //Get the parent type.
        auto *parent = result.Nodes.getNodeAs<clang::VarDecl>(types[ENUM_VAR]);

        //Adds the enum decl.
        addEnumDecl(result, dec, parent);

        //Add the parent relationship.
        if (parent != NULL) addEnumRef(result, dec, parent);
    } else if (const EnumConstantDecl *dec = result.Nodes.getNodeAs<clang::EnumConstantDecl>(types[ENUM_CONST])){
        auto *parent = result.Nodes.getNodeAs<clang::FunctionDecl>(types[ENUM_CONST_PARENT]);

        //Adds the enum constant.
        addEnumConstDecl(result, dec);

        //Next, adds the reference between parent and constant.
        addEnumConstRef(result, dec, parent);
    }
}

void FullWalker::generateASTMatches(MatchFinder *finder) {
    //Function methods.
    if (!exclusions.cFunction){
        //Finds function declarations for current C/C++ file.
        finder->addMatcher(functionDecl(isExpansionInMainFile()).bind(types[FUNC_DEC]), this);

        //Finds function calls from one function to another.
        finder->addMatcher(callExpr(isExpansionInMainFile(), hasAncestor(functionDecl().bind(types[CALLER]))).bind(types[FUNC_CALL]), this);
    }

    //Variable methods.
    if (!exclusions.cVariable){
        //Finds variables in functions or in class declaration.
        finder->addMatcher(varDecl(isExpansionInMainFile()).bind(types[VAR_DEC]), this);

        //Finds variable uses from a function to a variable.
        finder->addMatcher(declRefExpr(hasDeclaration(varDecl(isExpansionInMainFile()).bind(types[VAR_CALL])),
                                       hasAncestor(functionDecl().bind(types[CALLER_VAR])),
                                       hasParent(expr().bind(types[VAR_EXPR]))), this);
    }

    //Class methods.
    if (!exclusions.cClass){
        //Finds any class declarations.
        finder->addMatcher(functionDecl(isExpansionInMainFile()).bind(types[CLASS_DEC_FUNC]), this);
        finder->addMatcher(varDecl(isExpansionInMainFile()).bind(types[CLASS_DEC_VAR]), this);
        finder->addMatcher(varDecl(isExpansionInMainFile(), hasAncestor(functionDecl().bind(types[CLASS_DEC_VAR_TWO])))
                                   .bind(types[CLASS_DEC_VAR_THREE]), this);
    }

    //Finds unions in the program.
    //finder->addMatcher(recordDecl(isExpansionInMainFile(), isStruct()).bind(types[STRUCT_DEC]), this);
    //finder->addMatcher(varDecl(isExpansionInMainFile(),
    //                           hasType(recordDecl(isStruct()).bind(types[STRUCT_DEC]))), this);

    //Finds structs in the program.
    //finder->addMatcher(recordDecl(isExpansionInMainFile(), isUnion()).bind(types[UNION_DEC]), this);
    //finder->addMatcher(varDecl(isExpansionInMainFile(),
    //                           hasType(recordDecl(isUnion()).bind(types[UNION_DEC]))), this);

    if (!exclusions.cEnum){
        //Finds enums in the program.
        finder->addMatcher(enumDecl(isExpansionInMainFile()).bind(types[ENUM_DEC]), this);
        finder->addMatcher(varDecl(isExpansionInMainFile(),
                                   hasType(enumType(hasDeclaration(enumDecl().bind(types[ENUM_DEC]))))).bind(types[ENUM_VAR]), this);

        //Finds enum constants and their usage.
        finder->addMatcher(enumConstantDecl(isExpansionInMainFile(),
                                            hasAncestor(functionDecl().bind(types[ENUM_CONST_PARENT]))).bind(types[ENUM_CONST]), this);
    }
}

void FullWalker::addVariableDecl(const MatchFinder::MatchResult result, const VarDecl *decl) {
    string fileName = generateFileName(result, decl->getInnerLocStart());

    //Get the ID of the variable.
    string ID = generateID(fileName, decl->getQualifiedNameAsString());

    //Next, gets the qualified name.
    string qualName = generateLabel(decl, ClangNode::VARIABLE);

    //Creates a variable entry.
    ClangNode* node = new ClangNode(ID, qualName, ClangNode::VARIABLE);
    graph->addNode(node);

    //Process attributes.
    graph->addSingularAttribute(node->getID(),
                               ClangNode::FILE_ATTRIBUTE.attrName,
                               ClangNode::FILE_ATTRIBUTE.processFileName(fileName));

    //Get the scope of the decl.
    graph->addSingularAttribute(node->getID(),
                               ClangNode::VAR_ATTRIBUTE.scopeName,
                               ClangNode::VAR_ATTRIBUTE.getScope(decl));
    graph->addSingularAttribute(node->getID(),
                               ClangNode::VAR_ATTRIBUTE.staticName,
                               ClangNode::VAR_ATTRIBUTE.getStatic(decl));
}

void FullWalker::addFunctionDecl(const MatchFinder::MatchResult result, const DeclaratorDecl *decl) {
    string fileName = generateFileName(result, decl->getInnerLocStart());

    //First, generate the ID of the function.
    string ID = generateID(fileName, decl->getQualifiedNameAsString());

    //Gets the qualified name.
    string qualName = generateLabel(decl, ClangNode::FUNCTION);

    //Creates a new function entry.
    ClangNode* node = new ClangNode(ID, qualName, ClangNode::FUNCTION);
    graph->addNode(node);

    //Process attributes.
    graph->addSingularAttribute(node->getID(),
                               ClangNode::FILE_ATTRIBUTE.attrName,
                               ClangNode::FILE_ATTRIBUTE.processFileName(fileName));


    //Check if we have a CXXMethodDecl.
    if (isa<CXXMethodDecl>(decl->getAsFunction())){
        //Perform a static cast.
        const CXXMethodDecl* methDecl = static_cast<const CXXMethodDecl*>(decl->getAsFunction());

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

void FullWalker::addFunctionCall(const MatchFinder::MatchResult result,
                                const CallExpr *expr, const DeclaratorDecl *decl) {
    //Generate the ID of the caller and callee.
    string callerName = generateLabel(decl, ClangNode::FUNCTION);
    string calleeName = generateLabel(expr->getCalleeDecl()->getAsFunction(), ClangNode::FUNCTION);

    //Next, we find by ID.
    vector<ClangNode*> caller = graph->findNodeByName(callerName);
    vector<ClangNode*> callee = graph->findNodeByName(calleeName);

    //Check if we have the correct size.
    if (caller.size() == 0 || callee.size() == 0){
        //Add to unresolved reference list.
        graph->addUnresolvedRef(callerName, calleeName, ClangEdge::CALLS);

        //Add the attributes.
        //TODO: Function call attributes?
        return;
    }

    //We finally add the edge.
    ClangEdge* edge = new ClangEdge(caller.at(0), callee.at(0), ClangEdge::CALLS);
    graph->addEdge(edge);

    //Process attributes.
    //TODO: Function call attributes?
}

void FullWalker::addVariableRef(const MatchFinder::MatchResult result,
                               const VarDecl *decl, const DeclaratorDecl *caller, const Expr *expr) {
    //Start by generating the ID of the caller and callee.
    string callerName = generateLabel(caller, ClangNode::FUNCTION);
    string varName = generateLabel(decl, ClangNode::VARIABLE);

    //Next, we find their IDs.
    vector<ClangNode*> callerNode = graph->findNodeByName(callerName);
    vector<ClangNode*> varNode = graph->findNodeByName(varName);

    //Check to see if we have these entries already done.
    if (callerNode.size() == 0 || varNode.size() == 0){
        //Add to unresolved reference list.
        graph->addUnresolvedRef(callerName, varName, ClangEdge::REFERENCES);

        //Add attributes.
        graph->addUnresolvedRefAttr(callerName, varName,
                             ClangEdge::ACCESS_ATTRIBUTE.attrName, ClangEdge::ACCESS_ATTRIBUTE.getVariableAccess(expr, decl->getName()));
        return;
    }

    //Add the edge.
    ClangEdge* edge = new ClangEdge(callerNode.at(0), varNode.at(0), ClangEdge::REFERENCES);
    graph->addEdge(edge);

    //Process attributes.
    graph->addAttribute(callerNode.at(0)->getID(), varNode.at(0)->getID(), ClangEdge::REFERENCES,
                       ClangEdge::ACCESS_ATTRIBUTE.attrName, ClangEdge::ACCESS_ATTRIBUTE.getVariableAccess(expr, decl->getName()));
}

void FullWalker::addClassDecl(const MatchFinder::MatchResult result, const CXXRecordDecl *classDec, string fileName) {
    //Get the name & ID of the class.
    string classID = generateID(fileName, classDec->getQualifiedNameAsString());
    string className = generateLabel(classDec, ClangNode::CLASS);

    //Creates a new function entry.
    ClangNode* node = new ClangNode(classID, className, ClangNode::CLASS);
    graph->addNode(node);

    //Process attributes.
    graph->addSingularAttribute(node->getID(),
                               ClangNode::FILE_ATTRIBUTE.attrName,
                               ClangNode::FILE_ATTRIBUTE.processFileName(fileName));
    graph->addSingularAttribute(node->getID(),
                               ClangNode::BASE_ATTRIBUTE.attrName,
                               std::to_string(classDec->getNumBases()));

    //Get base classes.
    if (classDec->getNumBases() > 0) {
        for (auto base = classDec->bases_begin(); base != classDec->bases_end(); base++) {
            if (base->getType().getTypePtr() == NULL) continue;
            CXXRecordDecl *baseClass = base->getType().getTypePtr()->getAsCXXRecordDecl();
            if (baseClass == NULL) continue;

            //Add a linkage in our graph.
            addClassInheritanceRef(classDec, baseClass);
        }
    }
}

void FullWalker::addClassRef(const MatchFinder::MatchResult result,
                            const CXXRecordDecl* classRec, const DeclaratorDecl* funcRec){
    //Generate the label of the class and the function.
    string className = generateLabel(classRec, ClangNode::CLASS);
    string funcName = generateLabel(funcRec, ClangNode::FUNCTION);

    addClassRef(className, funcName);
}

void FullWalker::addClassRef(const MatchFinder::MatchResult result,
                            const CXXRecordDecl* classRec, const VarDecl* varRec){
    //Generate the label of the class and the variable.
    string className = generateLabel(classRec, ClangNode::CLASS);
    string objName = generateLabel(varRec, ClangNode::VARIABLE);

    addClassRef(className, objName);
}

void FullWalker::addClassRef(string srcLabel, string dstLabel){
    //Get the nodes by their label.
    vector<ClangNode*> classNode = graph->findNodeByName(srcLabel);
    vector<ClangNode*> innerNode = graph->findNodeByName(dstLabel);

    //Check to see if we have these entries already done.
    if (classNode.size() == 0 || innerNode.size() == 0){
        //Add to unresolved reference list.
        graph->addUnresolvedRef(srcLabel, dstLabel, ClangEdge::CONTAINS);

        //Add attributes.
        //TODO: Any class reference attributes?

        return;
    }

    //Add the edge.
    ClangEdge* edge = new ClangEdge(classNode.at(0), innerNode.at(0), ClangEdge::CONTAINS);
    graph->addEdge(edge);

    //Process attributes.
    //TODO: Any class reference attributes?
}

void FullWalker::addClassInheritanceRef(const CXXRecordDecl* classDec, const CXXRecordDecl* baseDec){
    string classLabel = generateLabel(classDec, ClangNode::CLASS);
    string baseLabel = generateLabel(baseDec, ClangNode::CLASS);

    //Get the nodes by their label.
    vector<ClangNode*> classNode = graph->findNodeByName(classLabel);
    vector<ClangNode*> baseNode = graph->findNodeByName(baseLabel);

    //Check to see if we don't have these entries.
    if (classNode.size() == 0 || baseNode.size() == 0){
        graph->addUnresolvedRef(classLabel, baseLabel, ClangEdge::INHERITS);

        //Add attributes.
        //TODO: Any inheritance attributes?

        return;
    }

    //Add the edge.
    ClangEdge* edge = new ClangEdge(classNode.at(0), baseNode.at(0), ClangEdge::INHERITS);
    graph->addEdge(edge);

    //Process  attributes.
    //TODO: Any inheritance attributes?
}

void FullWalker::addUnStrcDecl(const MatchFinder::MatchResult result, const clang::RecordDecl *decl){
    //Get the type.
    ClangNode::NodeType type;
    if (decl->isUnion()){
        type = ClangNode::UNION;
    } else {
        type = ClangNode::STRUCT;
    }

    //TODO: Stuff.
}

void FullWalker::addEnumDecl(const MatchFinder::MatchResult result, const EnumDecl *decl, const VarDecl *parent){
    string fileName = generateFileName(result,
                                       (parent == NULL) ? decl->getInnerLocStart() : parent->getInnerLocStart());

    //First, generate the ID of the function.
    string ID = generateID(fileName, decl->getQualifiedNameAsString());

    //Gets the qualified name.
    string qualName = generateLabel(decl, ClangNode::ENUM);

    //Creates a new function entry.
    ClangNode* node = new ClangNode(ID, qualName, ClangNode::ENUM);
    graph->addNode(node);

    //Process attributes.
    graph->addSingularAttribute(node->getID(),
                               ClangNode::FILE_ATTRIBUTE.attrName,
                               ClangNode::FILE_ATTRIBUTE.processFileName(fileName));

    //Add the class reference.
    string classLabel = generateClassName(qualName);
    if (classLabel.compare(string()) == 0) return;
    addClassRef(generateClassName(qualName), generateLabel(decl, ClangNode::ENUM));
}

void FullWalker::addEnumRef(const MatchFinder::MatchResult result, const EnumDecl *decl,
                           const VarDecl *parent) {
    //First, get the names for both the parent and the enum.
    string varLabel = generateLabel(parent, ClangNode::VARIABLE);
    string enumLabel = generateLabel(decl, ClangNode::ENUM);

    //Get the nodes by their label.
    vector<ClangNode*> varNode = graph->findNodeByName(varLabel);
    vector<ClangNode*> enumNode = graph->findNodeByName(enumLabel);

    //Check to see if we don't have these entries.
    if (varNode.size() == 0 || enumNode.size() == 0){
        graph->addUnresolvedRef(varLabel, enumLabel, ClangEdge::REFERENCES);

        //Add attributes.
        //TODO: Any enum reference attributes?

        return;
    }

    //Add the edge.
    ClangEdge* edge = new ClangEdge(varNode.at(0), enumNode.at(0), ClangEdge::REFERENCES);
    graph->addEdge(edge);

    //Process  attributes.
    //TODO: Any enum reference attributes?
}

void FullWalker::addEnumConstDecl(const MatchFinder::MatchResult result, const EnumConstantDecl *decl) {
    //TODO
}

void FullWalker::addEnumConstRef(const MatchFinder::MatchResult result, const EnumConstantDecl *decl,
                                const FunctionDecl *func) {
    //TODO
}