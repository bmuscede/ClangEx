/////////////////////////////////////////////////////////////////////////////////////////////////////////
// BlobWalker.h
//
// Created By: Bryan J Muscedere
// Date: 06/12/16.
//
// Walks through the Clang AST in a blob-like formation. Looks at all referenced
// header files and source files being processed. Allows for variables, fields, enums,
// classes, etc.
//
// Copyright (C) 2017, Bryan J. Muscedere
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CLANGEX_MINIMALWALKER_H
#define CLANGEX_MINIMALWALKER_H

#include "../Driver/ClangDriver.h"
#include "ASTWalker.h"

class BlobWalker : public ASTWalker {
public:
    /** Constructor and Destructor */
    explicit BlobWalker(Printer* print, ClangDriver::ClangExclude exclusions = ClangDriver::ClangExclude(),
                  TAGraph* graph = new TAGraph());
    ~BlobWalker() override;

    /** Methods for running the AST Walker */
    void run(const MatchFinder::MatchResult &result) override;
    void generateASTMatches(MatchFinder *finder) override;

private:
    /** Enum and Array for AST Matcher */
    enum {FUNC_DEC = 0, VAR_DEC, FIELD_DEC, VAR_INSIDE, FIELD_INSIDE, INSIDE_FUNC, VAR_PARAM, FUNC_PARAM,
        FUNC_CALLER, FUNC_CALLEE, VAR_CALLER, VAR_CALLEE, VAR_EXPR,
        FIELD_CALLEE, FIELD_EXPR, CLASS_DEC, ENUM_DEC, ENUM_CONST_DECL, ENUM_PARENT, ENUM_DEC_REF, VAR_REF_ENUM,
        FIELD_REF_ENUM, STRUCT_DECL, STRUCT_REF_ITEM, STRUCT_REF, STRUCT_REF_DECL, VAR_BOUND_STRUCT, FIELD_BOUND_STRUCT,
        UNION_DECL, UNION_REF_ITEM, UNION_REF, UNION_REF_DECL, VAR_BOUND_UNION, FIELD_BOUND_UNION};
    const char* types[34] = {"func_dec", "var_dec", "field_dec", "var_inside", "field_inside", "inside_func",
                             "var_param", "func_param", "caller", "callee", "v_caller", "v_callee", "v_expr",
                            "field_callee", "field_expr", "class_dec", "enum_dec", "enum_const_decl", "enum_parent",
                            "enum_dec_ref", "var_ref_enum", "field_ref_enum", "struct_decl", "struct_ref_item",
                            "struct_ref", "struct_ref_decl", "var_bound_struct", "field_bound_struct", "union_decl",
                             "union_ref_item", "union_ref", "union_ref_decl", "var_bound_union", "field_bound_union"};

    /** Manages Classes */
    void performAddClassCall(const MatchFinder::MatchResult result, const clang::DeclaratorDecl *decl,
                             ClangNode::NodeType type);
};


#endif //CLANGEX_MINIMALWALKER_H
