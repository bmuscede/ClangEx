//
// Created by bmuscede on 06/12/16.
//

#ifndef CLANGEX_FULLASTWALKER_H
#define CLANGEX_FULLASTWALKER_H

#include "../Driver/ClangDriver.h"
#include "ASTWalker.h"

class PartialWalker : public ASTWalker {
public:
    PartialWalker(bool md5, Printer* print, ClangDriver::ClangExclude exclusions = ClangDriver::ClangExclude(),
               TAGraph* graph = new TAGraph());
    virtual ~PartialWalker();

    virtual void run(const MatchFinder::MatchResult &result);
    virtual void generateASTMatches(MatchFinder *finder);

private:
    enum {FUNC_DEC = 0, FUNC_CALL, CALLER, VAR_DEC, INSIDE_FUNC, VAR_INSIDE, PARAM_INSIDE, VAR_CALL, CALLER_VAR,
        VAR_EXPR, CLASS_DEC_FUNC, CLASS_DEC_VAR, ENUM_DEC, ENUM_VAR, STRUCT_DECL, STRUCT_REF, STRUCT_REF_ITEM};
    const char* types[17] = {"func_dec", "func_call", "caller", "var_dec", "inside_func", "var_inside", "param_inside",
                             "var_call", "caller_var", "expr_var", "class_dec_func", "class_dec_var", "enum_dec",
                             "enum_var", "struct_decl", "struct_ref", "struct_ref_item"};

    void manageClasses(const MatchFinder::MatchResult result, const clang::DeclaratorDecl *decl,
                       ClangNode::NodeType type, const clang::DeclaratorDecl *innerDecl = nullptr);
    void addEnumConstants(const MatchFinder::MatchResult result, const clang::EnumDecl *enumDecl,
                          std::string filename = std::string());

};


#endif //CLANGEX_FULLASTWALKER_H
