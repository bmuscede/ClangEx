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

PartialWalker::PartialWalker(Printer* print, ClangDriver::ClangExclude exclusions, TAGraph* graph) :
        ASTWalker(exclusions, print, graph){ }

PartialWalker::~PartialWalker() { }

void PartialWalker::run(const MatchFinder::MatchResult &result) {
    //Look for the AST matcher being triggered.
    if (const FunctionDecl *functionDecl = result.Nodes.getNodeAs<clang::FunctionDecl>(types[FUNC_DEC])) {
        //If a function has been found.
        addFunctionDecl(result, functionDecl);

        //Adds class declarations/references.
        manageClasses(result, functionDecl, ClangNode::FUNCTION);
    } else if (const CallExpr *expr = result.Nodes.getNodeAs<clang::CallExpr>(types[FUNC_CALL])) {
        //If a function call has been found.
        auto caller = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[CALLER]);
        if (expr->getCalleeDecl() == nullptr || expr->getCalleeDecl()->getAsFunction() == nullptr) return;
        auto callee = expr->getCalleeDecl()->getAsFunction();

        addFunctionCall(result, caller, callee);
    } else if (const VarDecl *varDecl = result.Nodes.getNodeAs<clang::VarDecl>(types[VAR_DEC])) {
        //If a variable declaration has been found.
        addVariableDecl(result, varDecl);

        //Adds class declarations/references.
        manageClasses(result, varDecl, ClangNode::VARIABLE);
    } else if (const VarDecl *varInside = result.Nodes.getNodeAs<clang::VarDecl>(types[VAR_INSIDE])) {
        //Gets the parent function.
        auto *functionDecl = result.Nodes.getNodeAs<clang::FunctionDecl>(types[INSIDE_FUNC]);

        //Adds the function.
        addVariableInsideCall(result, functionDecl, varInside);
    } else if (const ParmVarDecl *paramInside = result.Nodes.getNodeAs<clang::ParmVarDecl>(types[PARAM_INSIDE])) {
        //Gets the parent function.
        auto *functionDecl = result.Nodes.getNodeAs<clang::FunctionDecl>(types[INSIDE_FUNC]);

        //Adds the function.
        addVariableInsideCall(result, functionDecl, paramInside);
    } else if (const VarDecl *varDeclExpr = result.Nodes.getNodeAs<clang::VarDecl>(types[VAR_CALL])) {
        //If a variable reference has been found.
        auto *caller = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[CALLER_VAR]);
        auto *expr = result.Nodes.getNodeAs<clang::Expr>(types[VAR_EXPR]);

        addVariableCall(result, caller, expr, varDeclExpr);
    } else if (const FunctionDecl *functionDeclClass = result.Nodes.getNodeAs<clang::FunctionDecl>(
            types[CLASS_DEC_FUNC])) {
        //Get the variable declaration.
        auto *var = result.Nodes.getNodeAs<clang::VarDecl>(types[CLASS_DEC_VAR]);

        //Add the class reference.
        manageClasses(result, functionDeclClass, ClangNode::VARIABLE, var);
    } else if (const VarDecl *varRefDecl = result.Nodes.getNodeAs<clang::VarDecl>(types[ENUM_VAR])) {
        auto *enumDecl = result.Nodes.getNodeAs<clang::EnumDecl>(types[ENUM_DEC]);

        //Get the file name of the varRef.
        string filename = generateFileName(result, varRefDecl->getInnerLocStart());

        //Add the enum decl first.
        addEnumDecl(result, enumDecl, filename);

        //Add the variable reference.
        addEnumCall(result, enumDecl, varRefDecl);

        //Finally, add the enum constants.
        addEnumConstants(result, enumDecl, filename);
    } else if (const EnumDecl *enumDecl = result.Nodes.getNodeAs<clang::EnumDecl>(types[ENUM_DEC])) {
        //Add the enum.
        addEnumDecl(result, enumDecl);

        //Add the enum constants.
        addEnumConstants(result, enumDecl);
    } else if (const RecordDecl *structDecl = result.Nodes.getNodeAs<clang::RecordDecl>(types[STRUCT_DECL])){
        //Adds the struct.
        addStructDecl(result, structDecl);
    } else if (const DeclaratorDecl *itemDecl = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[STRUCT_REF_ITEM])) {
        //Get the struct being referenced.
        auto *structDecl = result.Nodes.getNodeAs<clang::RecordDecl>(types[STRUCT_REF]);

        //Adds the structure.
        addStructDecl(result, structDecl, generateFileName(result, itemDecl->getInnerLocStart()));
        //addStructCall(result, structDecl, itemDecl);
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

        //Adds scope for variables.
        /*finder->addMatcher(varDecl(isExpansionInMainFile(),
                                   hasAncestor(functionDecl().bind(types[INSIDE_FUNC]))).bind(types[VAR_INSIDE]), this);
        finder->addMatcher(parmVarDecl(isExpansionInMainFile(),
                                       hasAncestor(functionDecl()
                                                           .bind(types[INSIDE_FUNC]))).bind(types[PARAM_INSIDE]), this);
        */
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
        //Finds enum declarations (assuming they're defined in the source file).
        finder->addMatcher(enumDecl(isExpansionInMainFile()).bind(types[ENUM_DEC]), this);

        //Finds enums, adds them, and adds their associated references.
        finder->addMatcher(varDecl(isExpansionInMainFile(),
                                   hasType(enumType(hasDeclaration(enumDecl().bind(types[ENUM_DEC]))))).bind(types[ENUM_VAR]), this);
    }

    if (!exclusions.cStruct){
        //Finds struct declarations.
        finder->addMatcher(recordDecl(isStruct(), isExpansionInMainFile()).bind(types[STRUCT_DECL]), this);

        //Finds items that are part of structs.
        finder->addMatcher(varDecl(isExpansionInMainFile(),
                                   hasAncestor(recordDecl(isStruct()).bind(types[STRUCT_REF]))).bind(types[STRUCT_REF_ITEM]), this);
        finder->addMatcher(fieldDecl(isExpansionInMainFile(),
                                     hasAncestor(recordDecl(isStruct()).bind(types[STRUCT_REF]))).bind(types[STRUCT_REF_ITEM]), this);
        finder->addMatcher(functionDecl(isExpansionInMainFile(),
                                        hasAncestor(recordDecl(isStruct()).bind(types[STRUCT_REF]))).bind(types[STRUCT_REF_ITEM]), this);
        //TODO: Add more items.

        //TODO: Add matcher for variable referencing struct.
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
    if (labelDecl == nullptr) return;

    //Checks if we need to process.
    if (exclusions.cClass) return;

    //Get the filename.
    string filename = generateFileName(result, labelDecl->getInnerLocStart(), true);
    string declID = generateID(result, labelDecl);
    string declLabel = generateLabel(result, labelDecl);

    //Get the class.
    CXXRecordDecl* record = extractClass(decl->getQualifier());
    if (record == nullptr) return;

    //First, we add the class.
    addClassDecl(result, record, filename);

    //Next, we add the reference.
    addClassCall(result, record, declID, declLabel);
}

void PartialWalker::addEnumConstants(const MatchFinder::MatchResult result, const EnumDecl *enumDecl,
                                     string filename){
    //Iterate through the values in the enum.
    for (DeclContext::specific_decl_iterator<EnumConstantDecl> constant = enumDecl->enumerator_begin();
         constant != enumDecl->enumerator_end(); constant++) {
        //Get the iterator pointer.
        EnumConstantDecl* constDecl = constant->getFirstDecl();
        if (constDecl == nullptr) continue;

        //Add the enum constant.
        if (filename.compare(string()) == 0)
            addEnumConstantDecl(result, constDecl);
        else
            addEnumConstantDecl(result, constDecl, filename);

        //Finally, add the reference between enum and enum constant.
        addEnumConstantCall(result, enumDecl, constDecl);
    }
}