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
#include "Graph/TAGraph.h"
#include "File/FileParse.h"

using namespace clang::ast_matchers;

class ASTWalker : public MatchFinder::MatchCallback {
public:
    ASTWalker();
    virtual ~ASTWalker();

    virtual void run(const MatchFinder::MatchResult &result);
    void buildGraph(std::string fileName);

    void generateASTMatches(MatchFinder *finder);
    void resolveExternalReferences();
    void resolveFiles();

private:
    enum {FUNC_DEC = 0, FUNC_CALL, CALLER, VAR_DEC, VAR_CALL, CALLER_VAR, VAR_EXPR, CLASS_DEC, VAR_STMT};
    const char* types[9] = {"func_dec", "func_call", "caller", "var_dec", "var_call", "caller_var", "expr_var", "class_dec", "var_stmt"};

    const char* assignmentOperators[17] = {"=", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=", "&", "|", "^", "~", "<<", ">>"};
    const char* incDecOperators[2] = {"++", "--"};

    TAGraph graph;
    FileParse fileParser;
    std::vector<std::pair<std::pair<std::string, std::string>, ClangEdge::EdgeType>> unresolvedRef;

    std::string VAR_READ = "read";
    std::string VAR_WRITE = "write";

    void addUnresolvedRef(std::string callerID, std::string calleeID, ClangEdge::EdgeType type);
    std::string generateID(std::string fileName, std::string signature);
    std::string generateID(const MatchFinder::MatchResult result, const clang::DeclaratorDecl *decl);
    std::string generateFileName(const MatchFinder::MatchResult result, clang::SourceLocation loc);

    void addVariableDecl(const MatchFinder::MatchResult result, const clang::VarDecl *decl);
    void addFunctionDecl(const MatchFinder::MatchResult result, const clang::DeclaratorDecl *decl);
    void addFunctionCall(const MatchFinder::MatchResult result,
                         const clang::CallExpr *expr, const clang::DeclaratorDecl* decl);
    void addVariableRef(const MatchFinder::MatchResult result,
                        const clang::VarDecl *decl, const clang::DeclaratorDecl* caller, const clang::DeclRefExpr* expr);
    void addClassDecl(const MatchFinder::MatchResult result, const clang::CXXRecordDecl *decl);

    std::string processFileName(std::string path);
    std::string generateLabel(const clang::Decl* decl, ClangNode::NodeType type);
    std::string getVariableAccess(const MatchFinder::MatchResult result, const clang::VarDecl *var);
};


#endif //CLANGEX_ASTWALKER_H
