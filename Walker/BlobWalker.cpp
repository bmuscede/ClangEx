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

void BlobWalker::run(const MatchFinder::MatchResult &result) {
    //Check if the current result fits any of our match criteria.
    if (const FunctionDecl *functionDecl = result.Nodes.getNodeAs<clang::FunctionDecl>(types[FUNC_DEC])) {
        //Get whether we have a system header.
        if (isInSystemHeader(result, functionDecl)) return;

        //Adds a function decl.
        addFunctionDecl(result, functionDecl);

        //Adds a class reference.
        performAddClassCall(result, functionDecl, ClangNode::FUNCTION);
    } else if (const VarDecl *variableDecl = result.Nodes.getNodeAs<clang::VarDecl>(types[VAR_DEC])) {
        //Get whether we have a system header.
        if (isInSystemHeader(result, variableDecl) || variableDecl->getQualifiedNameAsString().compare("") == 0) return;

        //Adds a variable decl.
        addVariableDecl(result, variableDecl);

        //Adds a class reference.
        performAddClassCall(result, variableDecl, ClangNode::VARIABLE);
    } else if (const FieldDecl *fieldDecl = result.Nodes.getNodeAs<clang::FieldDecl>(types[FIELD_DEC])) {
        //Get whether we have a system header.
        if (isInSystemHeader(result, fieldDecl) || fieldDecl->getQualifiedNameAsString().compare("") == 0) return;

        //Adds a field decl.
        addVariableDecl(result, nullptr, fieldDecl);

        //Adds a class reference.
        performAddClassCall(result, fieldDecl, ClangNode::VARIABLE);
    } else if (const VarDecl *varInside = result.Nodes.getNodeAs<clang::VarDecl>(types[VAR_INSIDE])) {
        //Get the parent function.
        auto *parentFunc = result.Nodes.getNodeAs<clang::FunctionDecl>(types[INSIDE_FUNC]);

        //Get whether this call expression is a system header.
        if (isInSystemHeader(result, varInside) || varInside->getQualifiedNameAsString().compare("") == 0) return;

        //Adds the function call.
        addVariableInsideCall(result, parentFunc, varInside);
    } else if (const FieldDecl *fieldInside = result.Nodes.getNodeAs<clang::FieldDecl>(types[FIELD_INSIDE])) {
        //Get the parent function.
        auto *parentFunc = result.Nodes.getNodeAs<clang::FunctionDecl>(types[INSIDE_FUNC]);

        //Get whether this call expression is a system header.
        if (isInSystemHeader(result, fieldInside) || fieldInside->getQualifiedNameAsString().compare("") == 0) return;

        //Adds the function call.
        addVariableInsideCall(result, parentFunc, nullptr, fieldInside);
    } else if (const VarDecl *varParam = result.Nodes.getNodeAs<clang::VarDecl>(types[VAR_PARAM])) {
        //Get the parent function.
        auto *parentFunc = result.Nodes.getNodeAs<clang::FunctionDecl>(types[FUNC_PARAM]);

        //Get whether this call expression is a system header.
        if (isInSystemHeader(result, varParam) || varParam->getQualifiedNameAsString().compare("") == 0) return;

        //Adds the function call.
        addVariableInsideCall(result, parentFunc, varParam);
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
    } else if (const EnumDecl *enumDecl = result.Nodes.getNodeAs<clang::EnumDecl>(types[ENUM_DEC])){
        //Get whether this call expression is in the system header.
        if (isInSystemHeader(result, enumDecl)) return;

        //Adds the enum declaration.
        addEnumDecl(result, enumDecl);
    } else if (const EnumConstantDecl *enumConstDecl = result.Nodes.getNodeAs<clang::EnumConstantDecl>(types[ENUM_CONST_DECL])){
        //Get whether this call expression is in the system header.
        if (isInSystemHeader(result, enumConstDecl)) return;

        //Adds the enum constant declarations.
        addEnumConstantDecl(result, enumConstDecl);

        auto *parent = result.Nodes.getNodeAs<clang::EnumDecl>(types[ENUM_PARENT]);
        if (parent == nullptr) return;

        addEnumConstantCall(result, parent, enumConstDecl);
    } else if (const VarDecl *varEnumRef = result.Nodes.getNodeAs<clang::VarDecl>(types[VAR_REF_ENUM])){
        auto *enumRef = result.Nodes.getNodeAs<clang::EnumDecl>(types[ENUM_DEC_REF]);

        //Get whether this call expression is in the system header.
        if (isInSystemHeader(result, varEnumRef) || isInSystemHeader(result, enumRef)) return;

        addEnumCall(result, enumRef, varEnumRef);
    } else if (const FieldDecl *fieldEnumRef = result.Nodes.getNodeAs<clang::FieldDecl>(types[FIELD_REF_ENUM])){
        auto *enumRef = result.Nodes.getNodeAs<clang::EnumDecl>(types[ENUM_DEC_REF]);

        //Get whether this call expression is in the system header.
        if (isInSystemHeader(result, fieldEnumRef) || isInSystemHeader(result, enumRef)) return;

        addEnumCall(result, enumRef, nullptr, fieldEnumRef);
    } else if (const RecordDecl *structDecl = result.Nodes.getNodeAs<clang::RecordDecl>(types[STRUCT_DECL])){
        //Get whether this call expression is in the system header.
        if (isInSystemHeader(result, structDecl)) return;

        addStructDecl(result, structDecl);
    } else if (const DeclaratorDecl *itemDecl = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[STRUCT_REF_ITEM])){
        //Get the struct being referenced.
        auto *structDecl = result.Nodes.getNodeAs<clang::RecordDecl>(types[STRUCT_REF]);

        //Checks if the expression is in the system header.
        if (isInSystemHeader(result, itemDecl) || isInSystemHeader(result, structDecl)) return;

        addStructCall(result, structDecl, itemDecl);
    } else if (const VarDecl *varStruct = result.Nodes.getNodeAs<clang::VarDecl>(types[VAR_BOUND_STRUCT])){
        //Get the struct being referenced.
        auto *structDecl = result.Nodes.getNodeAs<clang::RecordDecl>(types[STRUCT_DECL]);

        //Get whether this call expression is in the system header.
        if (isInSystemHeader(result, varStruct) || isInSystemHeader(result, structDecl)) return;

        cout << "trigger" << endl;
        addStructUseCall(result, structDecl, varStruct);
    } else if (const FieldDecl *fieldStruct = result.Nodes.getNodeAs<clang::FieldDecl>(types[FIELD_BOUND_STRUCT])){
        //Get the struct being referenced.
        auto *structDecl = result.Nodes.getNodeAs<clang::RecordDecl>(types[STRUCT_DECL]);

        //Get whether this call expression is in the system header.
        if (isInSystemHeader(result, fieldStruct) || isInSystemHeader(result, structDecl)) return;

        cout << "trigger" << endl;
        addStructUseCall(result, structDecl, nullptr, fieldStruct);
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

        //Adds scope for variables.
        finder->addMatcher(varDecl(hasAncestor(functionDecl().bind(types[INSIDE_FUNC]))).bind(types[VAR_INSIDE]), this);
        finder->addMatcher(fieldDecl(hasAncestor(functionDecl().bind(types[INSIDE_FUNC]))).bind(types[FIELD_INSIDE]), this);
        finder->addMatcher(parmVarDecl(hasAncestor(functionDecl()
                                                           .bind(types[FUNC_PARAM]))).bind(types[VAR_PARAM]), this);

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

        //Finds enum constant declarations.
        //Also deals with their connections to enums.
        finder->addMatcher(enumConstantDecl().bind(types[ENUM_CONST_DECL]), this);
        finder->addMatcher(enumConstantDecl(hasAncestor(enumDecl().bind(types[ENUM_PARENT])))
                .bind(types[ENUM_CONST_DECL]), this);

        //Looks for enum references.
        finder->addMatcher(varDecl(hasType(enumDecl().bind(types[ENUM_DEC_REF]))).bind(types[VAR_REF_ENUM]), this);
        finder->addMatcher(fieldDecl(hasType(enumDecl().bind(types[ENUM_DEC_REF]))).bind(types[FIELD_REF_ENUM]), this);
    }

    //Struct methods.
    if (!exclusions.cStruct){
        //Builds the struct definition.
        finder->addMatcher(recordDecl(isStruct()).bind(types[STRUCT_DECL]), this);

        //Builds up struct.
        finder->addMatcher(varDecl(hasAncestor(recordDecl(isStruct()).bind(types[STRUCT_REF])))
                                   .bind(types[STRUCT_REF_ITEM]), this);
        finder->addMatcher(fieldDecl(hasAncestor(recordDecl(isStruct()).bind(types[STRUCT_REF])))
                                   .bind(types[STRUCT_REF_ITEM]), this);
        finder->addMatcher(functionDecl(hasAncestor(recordDecl(isStruct()).bind(types[STRUCT_REF])))
                                   .bind(types[STRUCT_REF_ITEM]), this);
        //TODO: Add more items.

        //Builds the struct reference.
        //TODO: This doesn't work!
        finder->addMatcher(varDecl(hasType(recordDecl().bind(types[STRUCT_DECL])))
                                   .bind(types[VAR_BOUND_STRUCT]), this);
        finder->addMatcher(fieldDecl(hasType(recordDecl().bind(types[STRUCT_DECL])))
                                   .bind(types[FIELD_BOUND_STRUCT]), this);
    }

    //Union methods.
    if (!exclusions.cUnion){
        //TODO: Implement unions.
    }
}

