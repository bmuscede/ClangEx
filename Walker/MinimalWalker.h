//
// Created by bmuscede on 06/12/16.
//

#ifndef CLANGEX_MINIMALWALKER_H
#define CLANGEX_MINIMALWALKER_H

#include "ASTWalker.h"

class MinimalWalker : public ASTWalker {
public:
    MinimalWalker();
    MinimalWalker(ClangArgParse::ClangExclude exclusions);
    virtual ~MinimalWalker();

    virtual void run(const MatchFinder::MatchResult &result);
    virtual void generateASTMatches(MatchFinder *finder);

private:
    enum {FUNC_DEC = 0, VAR_DEC, FIELD_DEC, FUNC_CALLER, FUNC_CALLEE, VAR_CALLER, VAR_CALLEE, VAR_EXPR, FIELD_CALLEE};
    const char* types[9] = {"func_dec", "var_dec", "field_dec", "caller", "callee", "v_caller", "v_callee", "v_expr",
                            "field_callee"};

    void addFunctionDec(const MatchFinder::MatchResult results, const clang::DeclaratorDecl *dec);
    void addVariableDec(const MatchFinder::MatchResult results, const clang::VarDecl *dec);
    void addVariableDec(const MatchFinder::MatchResult results, const clang::FieldDecl *dec);
    void addFunctionCall(const MatchFinder::MatchResult results, const clang::DeclaratorDecl *caller, const clang::FunctionDecl *callee);
    void addVariableCall(const MatchFinder::MatchResult result, const clang::VarDecl *callee, const clang::DeclaratorDecl *caller,
                        const clang::DeclRefExpr* expr);
};


#endif //CLANGEX_MINIMALWALKER_H
