//
// Created by bmuscede on 06/12/16.
//

#ifndef CLANGEX_FULLASTWALKER_H
#define CLANGEX_FULLASTWALKER_H

#include "ASTWalker.h"

class PartialWalker : public ASTWalker {
public:
    PartialWalker(bool md5, ClangArgParse::ClangExclude exclusions = ClangArgParse::ClangExclude(),
               TAGraph* graph = new TAGraph());
    virtual ~PartialWalker();

    virtual void run(const MatchFinder::MatchResult &result);
    virtual void generateASTMatches(MatchFinder *finder);

private:
    enum {FUNC_DEC = 0, FUNC_CALL, CALLER, VAR_DEC, VAR_CALL, CALLER_VAR,
        VAR_EXPR, CLASS_DEC_FUNC, CLASS_DEC_VAR, ENUM_DEC, ENUM_VAR, STRUCT_DECL};
    const char* types[12] = {"func_dec", "func_call", "caller", "var_dec", "var_call", "caller_var",
                             "expr_var", "class_dec_func", "class_dec_var", "enum_dec", "enum_var", "struct_decl"};

    void manageClasses(const MatchFinder::MatchResult result, const clang::DeclaratorDecl *decl,
                       ClangNode::NodeType type, const clang::DeclaratorDecl *innerDecl = nullptr);
    void addEnumConstants(const MatchFinder::MatchResult result, const clang::EnumDecl *enumDecl,
                          std::string filename = std::string());

};


#endif //CLANGEX_FULLASTWALKER_H
