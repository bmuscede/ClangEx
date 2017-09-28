/////////////////////////////////////////////////////////////////////////////////////////////////////////
// PartialWalker.h
//
// Created By: Bryan J Muscedere
// Date: 06/12/16.
//
// Walks through the Clang AST in a class-based formation. Only looks at the current
// referenced class and ignores ALL header files.. Allows for variables, fields, enums,
// classes, etc. Can possibly create an incomplete model since header files are ignored.
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

#ifndef CLANGEX_FULLASTWALKER_H
#define CLANGEX_FULLASTWALKER_H

#include "../Driver/ClangDriver.h"
#include "ASTWalker.h"

class PartialWalker : public ASTWalker {
public:
    /** Constructor and Destructor */
    explicit PartialWalker(Printer* print, ClangDriver::ClangExclude exclusions = ClangDriver::ClangExclude(),
               TAGraph* graph = new TAGraph());
    ~PartialWalker() override;

    /** Methods for running the AST Walker */
    void run(const MatchFinder::MatchResult &result) override;
    void generateASTMatches(MatchFinder *finder) override;

private:
    /** Enum and Array for AST Matcher */
    enum {FUNC_DEC = 0, FUNC_CALL, CALLER, VAR_DEC, INSIDE_FUNC, VAR_INSIDE, PARAM_INSIDE, VAR_CALL, CALLER_VAR,
        VAR_EXPR, CLASS_DEC_FUNC, CLASS_DEC_VAR, ENUM_DEC, ENUM_VAR, STRUCT_DECL, STRUCT_REF, STRUCT_REF_ITEM};
    const char* types[17] = {"func_dec", "func_call", "caller", "var_dec", "inside_func", "var_inside", "param_inside",
                             "var_call", "caller_var", "expr_var", "class_dec_func", "class_dec_var", "enum_dec",
                             "enum_var", "struct_decl", "struct_ref", "struct_ref_item"};

    /** Manages Classes and Enums */
    void manageClasses(const MatchFinder::MatchResult result, const clang::DeclaratorDecl *decl,
                       ClangNode::NodeType type, const clang::DeclaratorDecl *innerDecl = nullptr);
    void addEnumConstants(const MatchFinder::MatchResult result, const clang::EnumDecl *enumDecl,
                          std::string filename = std::string());

};


#endif //CLANGEX_FULLASTWALKER_H
