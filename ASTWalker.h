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
#include "ClangArgParse.h"

using namespace clang::ast_matchers;

class ASTWalker : public MatchFinder::MatchCallback {
public:
    ASTWalker(ClangArgParse::ClangExclude exclusions);
    virtual ~ASTWalker();

    virtual void run(const MatchFinder::MatchResult &result);
    void buildGraph(std::string fileName);

    void generateASTMatches(MatchFinder *finder);
    void resolveExternalReferences();
    void resolveFiles();

private:
    std::string curFileName;
    ClangArgParse::ClangExclude exclusions;

    enum {FUNC_DEC = 0, FUNC_CALL, CALLER, VAR_DEC, VAR_CALL, CALLER_VAR,
        VAR_EXPR, CLASS_DEC_FUNC, CLASS_DEC_VAR, CLASS_DEC_VAR_TWO, CLASS_DEC_VAR_THREE};
    const char* types[11] = {"func_dec", "func_call", "caller", "var_dec", "var_call", "caller_var",
                            "expr_var", "class_dec_func", "class_dec_var", "class_dec_var_two", "class_dec_var_three"};

    const char* assignmentOperators[17] = {"=", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=",
                                           "&=", "^=", "|=", "&", "|", "^", "~", "<<", ">>"};
    const char* incDecOperators[2] = {"++", "--"};
    const std::string CLASS_PREPEND = "class-";

    TAGraph graph;
    FileParse fileParser;
    std::vector<std::pair<std::pair<std::string, std::string>, ClangEdge::EdgeType>> unresolvedRef;
    std::vector<std::pair<std::pair<std::string, std::string>, std::pair<std::string, std::vector<std::string>>>*> unresolvedRefAttr;

    void addUnresolvedRef(std::string callerID, std::string calleeID, ClangEdge::EdgeType type);
    void addUnresolvedRefAttr(std::string callerID, std::string calleeID, std::string attrName, std::string attrValue);
    std::vector<std::pair<std::string, std::vector<std::string>>> findAttributes(std::string callerID, std::string calleeID);

    std::string generateID(std::string fileName, std::string signature, ClangNode::NodeType type);
    std::string generateID(const MatchFinder::MatchResult result, const clang::DeclaratorDecl *decl, ClangNode::NodeType type);
    std::string generateFileName(const MatchFinder::MatchResult result, clang::SourceLocation loc);

    void addVariableDecl(const MatchFinder::MatchResult result, const clang::VarDecl *decl);
    void addFunctionDecl(const MatchFinder::MatchResult result, const clang::DeclaratorDecl *decl);
    void addFunctionCall(const MatchFinder::MatchResult result,
                         const clang::CallExpr *expr, const clang::DeclaratorDecl* decl);
    void addVariableRef(const MatchFinder::MatchResult result,
                        const clang::VarDecl *decl, const clang::DeclaratorDecl* caller, const clang::DeclRefExpr* expr);
    void addClassDecl(const MatchFinder::MatchResult result, const clang::CXXRecordDecl *classDec, std::string fileName);
    void addClassRef(const MatchFinder::MatchResult result,
                     const clang::CXXRecordDecl* classRec, const clang::DeclaratorDecl* funcRec);
    void addClassRef(const MatchFinder::MatchResult result,
                     const clang::CXXRecordDecl* classRec, const clang::VarDecl* varRec);
    void addClassRef(std::string srcLabel, std::string dstLabel);
    void addClassInheritanceRef(const clang::CXXRecordDecl* classDec, const clang::CXXRecordDecl* baseDec);

    std::string generateLabel(const clang::Decl* decl, ClangNode::NodeType type);
    std::string getVariableAccess(const MatchFinder::MatchResult result, const clang::VarDecl *var);
    void printFileName(std::string curFile);
};


#endif //CLANGEX_ASTWALKER_H
