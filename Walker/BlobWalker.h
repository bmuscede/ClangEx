//
// Created by bmuscede on 06/12/16.
//

#ifndef CLANGEX_MINIMALWALKER_H
#define CLANGEX_MINIMALWALKER_H

#include "../Driver/ClangDriver.h"
#include "ASTWalker.h"

class BlobWalker : public ASTWalker {
public:
    BlobWalker(bool md5, Printer* print, ClangDriver::ClangExclude exclusions = ClangDriver::ClangExclude(),
                  TAGraph* graph = new TAGraph());
    virtual ~BlobWalker();

    virtual void run(const MatchFinder::MatchResult &result);
    virtual void generateASTMatches(MatchFinder *finder);

private:
    enum {FUNC_DEC = 0, VAR_DEC, FIELD_DEC, VAR_INSIDE, FIELD_INSIDE, INSIDE_FUNC, VAR_PARAM, FUNC_PARAM,
        FUNC_CALLER, FUNC_CALLEE, VAR_CALLER, VAR_CALLEE, VAR_EXPR,
        FIELD_CALLEE, FIELD_EXPR, CLASS_DEC, ENUM_DEC, ENUM_CONST_DECL, ENUM_PARENT, ENUM_DEC_REF, VAR_REF_ENUM,
        FIELD_REF_ENUM, STRUCT_DECL, STRUCT_REF_ITEM, STRUCT_REF, VAR_BOUND_STRUCT, FIELD_BOUND_STRUCT};
    const char* types[27] = {"func_dec", "var_dec", "field_dec", "var_inside", "field_inside", "inside_func",
                             "var_param", "func_param", "caller", "callee", "v_caller", "v_callee", "v_expr",
                            "field_callee", "field_expr", "class_dec", "enum_dec", "enum_const_decl", "enum_parent",
                            "enum_dec_ref", "var_ref_enum", "field_ref_enum", "struct_decl", "struct_ref_item",
                            "struct_ref", "var_bound_struct", "field_bound_struct"};

    void performAddClassCall(const MatchFinder::MatchResult result, const clang::DeclaratorDecl *decl,
                             ClangNode::NodeType type);
};


#endif //CLANGEX_MINIMALWALKER_H
