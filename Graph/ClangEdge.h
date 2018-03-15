/////////////////////////////////////////////////////////////////////////////////////////////////////////
// ClangEdge.h
//
// Created By: Bryan J Muscedere
// Date: 05/11/16.
//
// Represents an edge in the TA graph system. Edges are elements of the AST that
// are relationships between entities. Also contains a set of operations
// for the attributes that each type of edge expects.
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

#ifndef CLANGEX_CLANGEDGE_H
#define CLANGEX_CLANGEDGE_H

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <boost/tokenizer.hpp>
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Lex/Lexer.h"
#include "ClangNode.h"

using namespace clang;
using namespace clang::ast_matchers;

class ClangEdge {
private:
    /** Struct for Variable Reads/Writes */
    typedef struct {
        const std::string attrName = "access";
        const std::string READ_FLAG = "read";
        const std::string WRITE_FLAG = "write";
        const char* assignmentOperators[17] = {"=", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=",
                                               "&=", "^=", "|=", "&", "|", "^", "~", "<<", ">>"};
        const char* incDecOperators[2] = {"++", "--"};

        /**
         * Gets the access type of variables. Can be either read or writes.
         * @param expr The expression to investigate.
         * @param varName The name of the variable to add.
         */
        std::string getVariableAccess(const clang::Expr *expr,
                                      const std::string varName){
            //Generate the printing policy.
            clang::LangOptions LangOpts;
            clang::PrintingPolicy Policy(LangOpts);

            //Gets the string.
            std::string TypeS;
            llvm::raw_string_ostream s(TypeS);
            if (expr == nullptr) return ClangEdge::ACCESS_ATTRIBUTE.READ_FLAG;
            expr->printPretty(s, nullptr, Policy);

            if (s.str().compare("") == 0) return ClangEdge::ACCESS_ATTRIBUTE.READ_FLAG;

            //Get associated strings.
            std::string varStatement = s.str();

            //First, check if the name of the variable actually appears.
            if (varStatement.find(varName) == std::string::npos) return ClangEdge::ACCESS_ATTRIBUTE.READ_FLAG;

            //Next, tokenize statement.
            typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
            boost::char_separator<char> sep{" "};
            tokenizer tok{varStatement, sep};
            std::vector<std::string> statementTokens;
            for (const auto &t : tok) {
                statementTokens.push_back(t);
            }

            //Check if we have increment and decrement operators.
            for (std::string token : statementTokens){
                //We found our variable.
                if (token.find(varName) != std::string::npos){
                    //Next, check for the increment and decrement operators.
                    for (std::string op : ClangEdge::ACCESS_ATTRIBUTE.incDecOperators){
                        if (token.compare(op + varName) == 0) return ClangEdge::ACCESS_ATTRIBUTE.WRITE_FLAG;
                        if (token.compare(varName + op) == 0) return ClangEdge::ACCESS_ATTRIBUTE.WRITE_FLAG;
                    }
                }
            }

            //Get the position of the variable name.
            int pos = find(statementTokens.begin(), statementTokens.end(), varName) - statementTokens.begin();

            //Search vector for C/C++ assignment operators.
            int i = 0;
            bool foundAOp = false;
            for (std::string token : statementTokens) {
                //Check for a particular assignment operator.
                for (std::string op : ClangEdge::ACCESS_ATTRIBUTE.assignmentOperators) {
                    if (token.compare(op) == 0) {
                        foundAOp = true;
                        break;
                    }
                }

                //See if we found the operator.
                if (foundAOp) break;
                i++;
            }

            //Now, we see what side of the operator our variable is on.
            if (!foundAOp){
                return ClangEdge::ACCESS_ATTRIBUTE.READ_FLAG;
            } else if (pos <= i){
                return ClangEdge::ACCESS_ATTRIBUTE.WRITE_FLAG;
            }
            return ClangEdge::ACCESS_ATTRIBUTE.READ_FLAG;
        }
    } AccessStruct;

public:
    /** Edge Type Members */
    enum EdgeType {CALLS, REFERENCES, CONTAINS, INHERITS, FILE_CONTAIN};
    static std::string getTypeString(EdgeType type);
    static ClangEdge::EdgeType getTypeEdge(std::string name);

    /** Constructor/Destructor */
    ClangEdge(ClangNode* src, ClangNode* dst, EdgeType type);
    ClangEdge(ClangNode* src, std::string dst, EdgeType type);
    ClangEdge(std::string src, ClangNode* dst, EdgeType type);
    ClangEdge(std::string src, std::string dst, EdgeType type);
    ~ClangEdge();

    /** Getters */
    ClangNode* getSrc();
    ClangNode* getDst();
    std::string getSrcID();
    std::string getDstID();
    ClangEdge::EdgeType getType();

    /** Resolution System */
    bool isResolved();

    /** Setters */
    void setSrc(ClangNode* newSrc);
    void setDst(ClangNode* newDst);

    /** Attribute Getters/Setters */
    bool addAttribute(std::string key, std::string value);
    bool clearAttribute(std::string key);
    std::vector<std::string> getAttribute(std::string key);
    bool doesAttributeExist(std::string key, std::string value);
    std::map<std::string, std::vector<std::string>> getAttributes();

    /** TA Helper Methods */
    std::string generateRelationship();
    std::string generateAttribute();

    /** Attribute Variables */
    static AccessStruct ACCESS_ATTRIBUTE;

private:
    /** Member Variables */
    ClangNode* src;
    ClangNode* dst;
    std::string srcID;
    std::string dstID;
    EdgeType type;
    bool unresolved;
    std::map<std::string, std::vector<std::string>> edgeAttributes;

    /** TA Helper Helper Methods */
    std::string printSingleAttribute(std::string key, std::vector<std::string> value);
    std::string printSetAttribute(std::string key, std::vector<std::string> value);
};


#endif //CLANGEX_CLANGEDGE_H
