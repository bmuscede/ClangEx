//
// Created by bmuscede on 06/12/16.
//

#ifndef CLANGEX_MINIMALWALKER_H
#define CLANGEX_MINIMALWALKER_H

#include "ASTWalker.h"

class BlobWalker : public ASTWalker {
public:
    BlobWalker(bool md5, ClangArgParse::ClangExclude exclusions = ClangArgParse::ClangExclude(),
                  TAGraph* graph = new TAGraph());
    virtual ~BlobWalker();

    virtual void run(const MatchFinder::MatchResult &result);
    virtual void generateASTMatches(MatchFinder *finder);

private:
    enum {FUNC_DEC = 0, VAR_DEC, FIELD_DEC, FUNC_CALLER, FUNC_CALLEE, VAR_CALLER, VAR_CALLEE, VAR_EXPR,
        FIELD_CALLEE, FIELD_EXPR, CLASS_DEC, ENUM_DEC, ENUM_CONST_DECL, ENUM_PARENT, ENUM_DEC_REF, VAR_REF_ENUM,
        FIELD_REF_ENUM, STRUCT_DECL, STRUCT_REF_ITEM, STRUCT_REF};
    const char* types[20] = {"func_dec", "var_dec", "field_dec", "caller", "callee", "v_caller", "v_callee", "v_expr",
                            "field_callee", "field_expr", "class_dec", "enum_dec", "enum_const_decl", "enum_parent",
                            "enum_dec_ref", "var_ref_enum", "field_ref_enum", "struct_decl", "struct_ref_item",
                            "struct_ref"};
};


#endif //CLANGEX_MINIMALWALKER_H
