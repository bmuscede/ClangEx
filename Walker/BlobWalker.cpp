//
// Created by bmuscede on 06/12/16.
//

#include <iostream>
#include "BlobWalker.h"

using namespace std;

BlobWalker::BlobWalker(bool md5, ClangArgParse::ClangExclude exclusions, TAGraph* graph) :
        ASTWalker(exclusions, md5, graph){ }

BlobWalker::~BlobWalker(){ }

void BlobWalker::run(const MatchFinder::MatchResult &result){
    //Check if the current result fits any of our match criteria.
    if (const DeclaratorDecl *functionDecl = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[FUNC_DEC])) {
        //Get whether we have a system header.
        if (isInSystemHeader(result, functionDecl)) return;

        addFunctionDecl(result, functionDecl);
    } else if (const VarDecl *variableDecl = result.Nodes.getNodeAs<clang::VarDecl>(types[VAR_DEC])){
         //Get whether we have a system header.
        if (isInSystemHeader(result, variableDecl)) return;
        if (variableDecl->getQualifiedNameAsString().compare("") == 0) return;

        addVariableDecl(result, variableDecl);
    } else if (const FieldDecl *fieldDecl = result.Nodes.getNodeAs<clang::FieldDecl>(types[FIELD_DEC])){
        //Get whether we have a system header.
        if (isInSystemHeader(result, fieldDecl)) return;
        if (fieldDecl->getQualifiedNameAsString().compare("") == 0) return;

        addVariableDecl(result, nullptr, fieldDecl);
    } else if (const CallExpr *expr = result.Nodes.getNodeAs<clang::CallExpr>(types[FUNC_CALLEE])){
        if (expr->getCalleeDecl() == nullptr || !(isa<const clang::FunctionDecl>(expr->getCalleeDecl()))) return;
        auto callee = expr->getCalleeDecl()->getAsFunction();
        auto caller = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[FUNC_CALLER]);

        //Get whether this call expression is a system header.
        if (isInSystemHeader(result, callee)) return;

        addFunctionCall(result, caller, callee);
    } else if (const VarDecl *callee = result.Nodes.getNodeAs<clang::VarDecl>(types[VAR_CALLEE])) {
        //If a variable reference has been found.
        auto *caller = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[VAR_CALLER]);
        auto *expr = result.Nodes.getNodeAs<clang::Expr>(types[VAR_EXPR]);

        //Get whether this call expression is in the system header.
        if (isInSystemHeader(result, callee)) return;

        addVariableCall(result, caller, expr, callee);
    } else if (const FieldDecl *callee = result.Nodes.getNodeAs<clang::FieldDecl>(types[FIELD_CALLEE])){
        //If a variable reference has been found.
        auto *caller = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[VAR_CALLER]);
        auto *expr = result.Nodes.getNodeAs<clang::Expr>(types[VAR_EXPR]);

        //Get whether this call expression is in the system header.
        if (isInSystemHeader(result, callee)) return;

        addVariableCall(result, caller, expr, nullptr, callee);
    } else if (const CXXRecordDecl *classRec = result.Nodes.getNodeAs<clang::CXXRecordDecl>(types[CLASS_DEC])){
        //Get whether this call expression is in the system header.
        if (isInSystemHeader(result, classRec)) return;

        addClassDec(result, classRec);
    } else if (const EnumDecl *dec = result.Nodes.getNodeAs<clang::EnumDecl>(types[ENUM_DEC])){
        //Get whether this call expression is in the system header.
        if (isInSystemHeader(result, dec)) return;

        addEnumDec(result, dec);
    }
}

void BlobWalker::generateASTMatches(MatchFinder *finder){
    //Function methods.
    if (!exclusions.cFunction){
        //Finds function declarations for current C/C++ file.
        finder->addMatcher(functionDecl(isDefinition()).bind(types[FUNC_DEC]), this);

        //Finds function calls from one function to another.
        finder->addMatcher(callExpr(hasAncestor(functionDecl().bind(types[FUNC_CALLER]))).bind(types[FUNC_CALLEE]), this);
    }

    //Variable methods.
    if (!exclusions.cVariable){
        //Finds variable declarations in functions AND in class decs.
        finder->addMatcher(varDecl().bind(types[VAR_DEC]), this);
        finder->addMatcher(fieldDecl().bind(types[FIELD_DEC]), this);

        //Finds variable uses amongst functions.
        finder->addMatcher(declRefExpr(hasDeclaration(varDecl().bind(types[VAR_CALLEE])),
                           hasAncestor(functionDecl().bind(types[VAR_CALLER])),
                           hasParent(expr().bind(types[VAR_EXPR]))), this);
        finder->addMatcher(declRefExpr(hasDeclaration(fieldDecl().bind(types[FIELD_CALLEE])),
                                       hasAncestor(functionDecl().bind(types[VAR_CALLER])),
                                       hasParent(expr().bind(types[FIELD_EXPR]))), this);

    }

    //Class methods.
    if (!exclusions.cClass){
        //Finds class declarations.
        finder->addMatcher(cxxRecordDecl(isClass()).bind(types[CLASS_DEC]), this);

        //TODO: References not implemented.
    }

    //Enum methods.
    if (!exclusions.cEnum){
        finder->addMatcher(enumDecl().bind(types[ENUM_DEC]), this);
    }
}

void BlobWalker::addClassDec(const MatchFinder::MatchResult result, const CXXRecordDecl *classRec) {
    //Get the definition.
    auto def = classRec->getDefinition();
    if (def == nullptr) return;

    //Check if we're dealing with a class.
    if (!classRec->isClass()) return;

    //Generate the fields for the node.
    string filename = generateFileName(result, classRec->getInnerLocStart());
    string ID = generateID(filename, classRec->getQualifiedNameAsString());
    string className = generateLabel(classRec, ClangNode::CLASS);
    if (ID.compare("") == 0 || filename.compare("") == 0) return;

    //Try to get the number of bases.
    int numBases = 0;
    try {
        numBases = classRec->getNumBases();
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
    if (classRec->getNumBases() > 0) {
        for (auto base = classRec->bases_begin(); base != classRec->bases_end(); base++) {
            if (base->getType().getTypePtr() == nullptr) continue;
            CXXRecordDecl *baseClass = base->getType().getTypePtr()->getAsCXXRecordDecl();
            if (baseClass == nullptr) continue;

            //Add a linkage in our graph->
            addClassInheritanceRef(classRec, baseClass);
        }
    }
}

void BlobWalker::addClassInheritanceRef(const CXXRecordDecl *childClass, const CXXRecordDecl *parentClass) {
    string classLabel = generateLabel(childClass, ClangNode::CLASS);
    string baseLabel = generateLabel(parentClass, ClangNode::CLASS);

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

void BlobWalker::addEnumDec(const MatchFinder::MatchResult result, const EnumDecl *dec){
    //Generate the fields for the node.
    string filename = generateFileName(result, dec->getInnerLocStart());
    string ID = generateID(filename, dec->getQualifiedNameAsString());
    string enumName = generateLabel(dec, ClangNode::ENUM);
    if (ID.compare("") == 0 || filename.compare("") == 0) return;

    //Creates a class entry.
    ClangNode* node = new ClangNode(ID, enumName, ClangNode::ENUM);
    graph->addNode(node);

    //Process attributes.
    graph->addSingularAttribute(node->getID(),
                               ClangNode::FILE_ATTRIBUTE.attrName,
                               ClangNode::FILE_ATTRIBUTE.processFileName(filename));
}
