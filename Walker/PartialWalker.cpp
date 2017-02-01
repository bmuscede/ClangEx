//
// Created by bmuscede on 06/12/16.
//

#include <iostream>
#include "PartialWalker.h"
#include "../File/ClangArgParse.h"

using namespace std;
using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;
using namespace llvm;

PartialWalker::PartialWalker(bool md5, ClangArgParse::ClangExclude exclusions, TAGraph* graph) :
        ASTWalker(exclusions, md5, graph){ }

PartialWalker::~PartialWalker() { }

void PartialWalker::run(const MatchFinder::MatchResult &result) {
    //Look for the AST matcher being triggered.
    if (const DeclaratorDecl *dec = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[FUNC_DEC])) {
        //If a function has been found.
        addFunctionDecl(result, dec);

        //Adds class declarations/references.
        manageClasses(result, dec, ClangNode::FUNCTION);
    } else if (const CallExpr *expr = result.Nodes.getNodeAs<clang::CallExpr>(types[FUNC_CALL])){
        //If a function call has been found.
        auto caller = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[CALLER]);
        if (expr->getCalleeDecl() == nullptr || expr->getCalleeDecl()->getAsFunction() == nullptr) return;
        auto callee = expr->getCalleeDecl()->getAsFunction();

        addFunctionCall(result, caller, callee);
    } else if (const VarDecl *dec = result.Nodes.getNodeAs<clang::VarDecl>(types[VAR_DEC])){
        //If a variable declaration has been found.
        addVariableDecl(result, dec);

        //Adds class declarations/references.
        manageClasses(result, dec, ClangNode::VARIABLE);
    } else if (const VarDecl *dec = result.Nodes.getNodeAs<clang::VarDecl>(types[VAR_CALL])){
        //If a variable reference has been found.
        auto *caller = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[CALLER_VAR]);
        auto *expr = result.Nodes.getNodeAs<clang::Expr>(types[VAR_EXPR]);

        addVariableCall(result, caller, expr, dec);
    } else if (const DeclaratorDecl *dec = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[CLASS_DEC_FUNC])){
        //Get the variable declaration.
        auto *var = result.Nodes.getNodeAs<clang::VarDecl>(types[CLASS_DEC_VAR]);

        //Add the class reference.
        manageClasses(result, dec, ClangNode::VARIABLE, var);
    } else if (const RecordDecl *rec = result.Nodes.getNodeAs<clang::RecordDecl>(types[STRUCT_DEC])){
        cout << "The struct found is: " << rec->getQualifiedNameAsString() << endl;
    } else if (const RecordDecl *rec = result.Nodes.getNodeAs<clang::RecordDecl>(types[UNION_DEC])){
        cout << "The union found is: " << rec->getQualifiedNameAsString() << endl;
    } else if (const EnumDecl *dec = result.Nodes.getNodeAs<clang::EnumDecl>(types[ENUM_DEC])){
        //Get the parent type.
        auto *parent = result.Nodes.getNodeAs<clang::VarDecl>(types[ENUM_VAR]);

        //Adds the enum decl.
        addEnumDecl(result, dec);

        //Add the parent relationship.
        if (parent != nullptr) addEnumRef(result, dec, parent);
    } else if (const EnumConstantDecl *dec = result.Nodes.getNodeAs<clang::EnumConstantDecl>(types[ENUM_CONST])){
        auto *parent = result.Nodes.getNodeAs<clang::FunctionDecl>(types[ENUM_CONST_PARENT]);

        //Adds the enum constant.
        addEnumConstDecl(result, dec);

        //Next, adds the reference between parent and constant.
        addEnumConstRef(result, dec, parent);
    }
}

void PartialWalker::generateASTMatches(MatchFinder *finder) {
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
        finder->addMatcher(varDecl(isExpansionInMainFile(), hasAncestor(functionDecl().bind(types[CLASS_DEC_FUNC])))
                                   .bind(types[CLASS_DEC_VAR]), this);
    }

    if (!exclusions.cEnum){
        //Finds enums in the program.
        finder->addMatcher(enumDecl(isExpansionInMainFile()).bind(types[ENUM_DEC]), this);
        //finder->addMatcher(varDecl(isExpansionInMainFile(),
        //                           hasType(enumType(hasDeclaration(enumDecl().bind(types[ENUM_DEC]))))).bind(types[ENUM_VAR]), this);

        //Finds enum constants and their usage.
        //finder->addMatcher(enumConstantDecl(isExpansionInMainFile(),
        //                                    hasAncestor(functionDecl().bind(types[ENUM_CONST_PARENT]))).bind(types[ENUM_CONST]), this);
    }

    if (!exclusions.cStruct){
        //TODO: Implement
    }

    if (!exclusions.cUnion){
        //TODO: Implement
    }
}

void PartialWalker::manageClasses(const MatchFinder::MatchResult result,
                                  const clang::DeclaratorDecl *decl, ClangNode::NodeType type,
                                  const clang::DeclaratorDecl *innerDecl){
    const DeclaratorDecl* labelDecl = innerDecl;
    if (innerDecl == nullptr) labelDecl = decl;

    //Checks if we need to process.
    if (exclusions.cClass) return;

    //Get the filename.
    string filename = generateFileName(result, labelDecl->getInnerLocStart(), true);
    string declLabel = generateLabel(labelDecl, type);

    //Get the class.
    CXXRecordDecl* record = extractClass(decl->getQualifier());
    if (record == nullptr) return;

    //First, we add the class.
    addClassDecl(result, record, filename);

    //Next, we add the reference.
    addClassCall(result, record, declLabel);
}

void PartialWalker::addUnStrcDecl(const MatchFinder::MatchResult result, const clang::RecordDecl *decl){
    //Get the type.
    ClangNode::NodeType type;
    if (decl->isUnion()){
        type = ClangNode::UNION;
    } else {
        type = ClangNode::STRUCT;
    }

    //TODO: Stuff.
}

void PartialWalker::addEnumRef(const MatchFinder::MatchResult result, const EnumDecl *decl,
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

void PartialWalker::addEnumConstDecl(const MatchFinder::MatchResult result, const EnumConstantDecl *decl) {
    //TODO
}

void PartialWalker::addEnumConstRef(const MatchFinder::MatchResult result, const EnumConstantDecl *decl,
                                const FunctionDecl *func) {
    //TODO
}