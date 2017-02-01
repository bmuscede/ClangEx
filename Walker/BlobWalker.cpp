//
// Created by bmuscede on 06/12/16.
//

#include <iostream>
#include "BlobWalker.h"
#include "../File/ClangArgParse.h"

using namespace std;

BlobWalker::BlobWalker(bool md5, ClangArgParse::ClangExclude exclusions, TAGraph* graph) :
        ASTWalker(exclusions, md5, graph){ }

BlobWalker::~BlobWalker(){ }

void BlobWalker::run(const MatchFinder::MatchResult &result){
    //Check if the current result fits any of our match criteria.
    if (const DeclaratorDecl *functionDecl = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[FUNC_DEC])) {
        //Get whether we have a system header.
        if (isInSystemHeader(result, functionDecl)) return;

        //Adds a function decl.
        addFunctionDecl(result, functionDecl);

        //Adds a class reference.
        performAddClassCall(result, functionDecl, ClangNode::FUNCTION);
    } else if (const VarDecl *variableDecl = result.Nodes.getNodeAs<clang::VarDecl>(types[VAR_DEC])){
         //Get whether we have a system header.
        if (isInSystemHeader(result, variableDecl) || variableDecl->getQualifiedNameAsString().compare("") == 0) return;

        //Adds a variable decl.
        addVariableDecl(result, variableDecl);

        //Adds a class reference.
        performAddClassCall(result, variableDecl, ClangNode::VARIABLE);
    } else if (const FieldDecl *fieldDecl = result.Nodes.getNodeAs<clang::FieldDecl>(types[FIELD_DEC])){
        //Get whether we have a system header.
        if (isInSystemHeader(result, fieldDecl) || fieldDecl->getQualifiedNameAsString().compare("") == 0) return;

        //Adds a field decl.
        addVariableDecl(result, nullptr, fieldDecl);

        //Adds a class reference.
        performAddClassCall(result, fieldDecl, ClangNode::VARIABLE);
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

        addClassDecl(result, classRec);
    } else if (const EnumDecl *dec = result.Nodes.getNodeAs<clang::EnumDecl>(types[ENUM_DEC])){
        //Get whether this call expression is in the system header.
        if (isInSystemHeader(result, dec)) return;

        //Adds the enum declaration.
        addEnumDecl(result, dec);

        //Adds a class reference.
        performAddClassCall(result, fieldDecl, ClangNode::ENUM);
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
    }

    //Enum methods.
    if (!exclusions.cEnum){
        //Finds enum declarations.
        finder->addMatcher(enumDecl().bind(types[ENUM_DEC]), this);

        //TODO: References not implemented.
    }

    //Struct methods.
    if (!exclusions.cStruct){
        //TODO: Implement structs.
    }

    //Union methods.
    if (!exclusions.cUnion){
        //TODO: Implement unions.
    }
}

