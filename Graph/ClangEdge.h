//
// Created by bmuscede on 05/11/16.
//

#ifndef CLANGEX_CLANGEDGE_H
#define CLANGEX_CLANGEDGE_H

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <boost/tokenizer.hpp>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Lex/Lexer.h"
#include "ClangNode.h"

using namespace clang;
using namespace clang::ast_matchers;

class ClangEdge {
private:
    /** ATTRIBUTE FLAGS */
    typedef struct {
        const std::string attrName = "access";

        const std::string READ_FLAG = "read";
        const std::string WRITE_FLAG = "write";

        const char* assignmentOperators[17] = {"=", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=",
                                               "&=", "^=", "|=", "&", "|", "^", "~", "<<", ">>"};
        const char* incDecOperators[2] = {"++", "--"};

        std::string getVariableAccess(const MatchFinder::MatchResult result, const clang::VarDecl *var){
            //Get the source range and manager.
            SourceRange range = var->getSourceRange();
            const SourceManager *SM = result.SourceManager;

            //Use LLVM's lexer to get source text.
            llvm::StringRef ref = Lexer::getSourceText(CharSourceRange::getCharRange(range), *SM, result.Context->getLangOpts());
            if (ref.str().compare("") == 0) return ClangEdge::ACCESS_ATTRIBUTE.READ_FLAG;

            //Get associated strings.
            std::string varStatement = ref.str();
            std::string varName = var->getName();
            std::cout << varStatement << std::endl;
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
    enum EdgeType {CALLS, REFERENCES, CONTAINS, INHERITS};
    static std::string getTypeString(EdgeType type);

    ClangEdge(ClangNode* src, ClangNode* dst, EdgeType type);
    ~ClangEdge();

    ClangNode* getSrc();
    ClangNode* getDst();
    ClangEdge::EdgeType getType();

    bool addAttribute(std::string key, std::string value);
    bool clearAttribute(std::string key);
    std::vector<std::string> getAttribute(std::string key);

    bool doesAttributeExist(std::string key, std::string value);

    std::string generateRelationship();
    std::string generateAttribute();

    /** ATTRIBUTE VARS */
    static AccessStruct ACCESS_ATTRIBUTE;

private:
    ClangNode* src;
    ClangNode* dst;
    EdgeType type;
    std::map<std::string, std::vector<std::string>> edgeAttributes;

    std::string printSingleAttribute(std::string key, std::vector<std::string> value);
    std::string printSetAttribute(std::string key, std::vector<std::string> value);
};


#endif //CLANGEX_CLANGEDGE_H
