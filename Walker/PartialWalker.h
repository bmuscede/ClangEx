//
// Created by bmuscede on 06/12/16.
//

#ifndef CLANGEX_FULLASTWALKER_H
#define CLANGEX_FULLASTWALKER_H

#include "ASTWalker.h"

class FullWalker : public ASTWalker {
public:
    FullWalker(ClangArgParse::ClangExclude exclusions = ClangArgParse::ClangExclude(), TAGraph* graph = new TAGraph());
    virtual ~FullWalker();

    virtual void run(const MatchFinder::MatchResult &result);
    virtual void generateASTMatches(MatchFinder *finder);

private:
    enum {FUNC_DEC = 0, FUNC_CALL, CALLER, VAR_DEC, VAR_CALL, CALLER_VAR,
        VAR_EXPR, CLASS_DEC_FUNC, CLASS_DEC_VAR, CLASS_DEC_VAR_TWO, CLASS_DEC_VAR_THREE, STRUCT_DEC, UNION_DEC, ENUM_DEC,
        ENUM_VAR, ENUM_CONST, ENUM_CONST_PARENT};
    const char* types[17] = {"func_dec", "func_call", "caller", "var_dec", "var_call", "caller_var",
                             "expr_var", "class_dec_func", "class_dec_var", "class_dec_var_two", "class_dec_var_three",
                             "struct_dec", "union_dec", "enum_dec", "enum_var", "enum_const", "enum_const_parent"};

    void addVariableDecl(const MatchFinder::MatchResult result, const clang::VarDecl *decl);
    void addFunctionDecl(const MatchFinder::MatchResult result, const clang::DeclaratorDecl *decl);
    void addFunctionCall(const MatchFinder::MatchResult result,
                         const clang::CallExpr *expr, const clang::DeclaratorDecl* decl);
    void addVariableRef(const MatchFinder::MatchResult result,
                        const clang::VarDecl *decl, const clang::DeclaratorDecl* caller, const clang::Expr* expr);
    void addClassDecl(const MatchFinder::MatchResult result, const clang::CXXRecordDecl *classDec, std::string fileName);
    void addClassRef(const MatchFinder::MatchResult result,
                     const clang::CXXRecordDecl* classRec, const clang::DeclaratorDecl* funcRec);
    void addClassRef(const MatchFinder::MatchResult result,
                     const clang::CXXRecordDecl* classRec, const clang::VarDecl* varRec);
    void addClassRef(std::string srcLabel, std::string dstLabel);
    void addClassInheritanceRef(const clang::CXXRecordDecl* classDec, const clang::CXXRecordDecl* baseDec);
    void addUnStrcDecl(const MatchFinder::MatchResult result, const clang::RecordDecl *decl);
    void addEnumDecl(const MatchFinder::MatchResult result, const clang::EnumDecl *decl, const clang::VarDecl *parent);
    void addEnumClassRef(const MatchFinder::MatchResult result, const clang::EnumDecl *decl, const clang::CXXRecordDecl *record);
    void addEnumRef(const MatchFinder::MatchResult result, const clang::EnumDecl *decl, const clang::VarDecl *parent);
    void addEnumConstDecl(const MatchFinder::MatchResult result, const clang::EnumConstantDecl *decl);
    void addEnumConstRef(const MatchFinder::MatchResult result, const clang::EnumConstantDecl *decl, const clang::FunctionDecl *func);
};


#endif //CLANGEX_FULLASTWALKER_H
