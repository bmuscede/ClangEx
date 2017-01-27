//
// Created by bmuscede on 05/11/16.
//

#ifndef CLANGEX_ASTWALKER_H
#define CLANGEX_ASTWALKER_H

#include <vector>
#include <tuple>
#include <string>
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "../Graph/TAGraph.h"
#include "../File/FileParse.h"
#include "../File/ClangArgParse.h"

using namespace clang::ast_matchers;

class ASTWalker : public MatchFinder::MatchCallback {
public:
    virtual ~ASTWalker();

    virtual void run(const MatchFinder::MatchResult &result) = 0;
    virtual void generateASTMatches(MatchFinder *finder) = 0;

    void buildGraph(std::string fileName);

    void resolveExternalReferences();
    void resolveFiles();

protected:
    ClangArgParse::ClangExclude exclusions;
    TAGraph* graph;

    ASTWalker(ClangArgParse::ClangExclude ex, bool md5, TAGraph* existing);

    std::string generateFileName(const MatchFinder::MatchResult result,
                                 clang::SourceLocation loc, bool suppressOutput = false);
    std::string generateID(std::string fileName, std::string qualifiedName);
    std::string generateLabel(const clang::Decl* decl, ClangNode::NodeType type);
    std::string generateClassName(std::string qualifiedName);

    bool isInSystemHeader(const MatchFinder::MatchResult &result, const clang::Decl *decl);

/********************************************************************************************************************/

    void addFunctionDecl(const MatchFinder::MatchResult results, const clang::DeclaratorDecl *dec);
    void addVariableDecl(const MatchFinder::MatchResult results, const clang::VarDecl *varDec = NULL,
                         const clang::FieldDecl *fieldDec = NULL);

/********************************************************************************************************************/

private:
    const static int ANON_SIZE = 3;
    const std::string ANON_LIST[ANON_SIZE] = {"(anonymous struct)", "(union struct)", "(anonymous)"};
    const std::string ANON_REPLACE[ANON_SIZE] = {"anon_struct", "union_struct", "anon"};
    const static int MD5_LENGTH = 33;

    std::string curFileName;
    FileParse fileParser;
    bool md5Flag;

    void printFileName(std::string curFile);
    std::string replaceLabel(std::string label, std::string init, std::string aft);
    std::string generateMD5(std::string text);
};


#endif //CLANGEX_ASTWALKER_H
