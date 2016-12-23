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
#include "../ClangArgParse.h"

using namespace clang::ast_matchers;

class ASTWalker : public MatchFinder::MatchCallback {
public:
    virtual ~ASTWalker();

    virtual void run(const MatchFinder::MatchResult &result) = 0;
    virtual void generateASTMatches(MatchFinder *finder) = 0;

    void buildGraph(std::string fileName);

    void resolveExternalReferences();
    void resolveFiles();

    void setGraph(TAGraph* graph);

protected:
    ClangArgParse::ClangExclude exclusions;
    const std::string CLASS_PREPEND = "class-";
    TAGraph* graph;

    ASTWalker();

    void addUnresolvedRef(std::string callerID, std::string calleeID, ClangEdge::EdgeType type);
    void addUnresolvedRefAttr(std::string callerID, std::string calleeID, std::string attrName, std::string attrValue);

    std::string generateID(std::string fileName, std::string signature, ClangNode::NodeType type);
    std::string generateID(const MatchFinder::MatchResult result, const clang::DeclaratorDecl *decl, ClangNode::NodeType type);
    std::string generateFileName(const MatchFinder::MatchResult result, clang::SourceLocation loc);
    std::string generateFileNameQuietly(const MatchFinder::MatchResult result, clang::SourceLocation loc);
    std::string generateLabel(const clang::Decl* decl, ClangNode::NodeType type);

    std::string getClassNameFromQualifier(std::string qualifiedName);

    bool isInSystemHeader(const MatchFinder::MatchResult &result, const clang::Decl *decl);

private:
    const std::string ANON_STRUCT = "(anonymous struct)";
    const std::string ANON_STRUCT_SYM = "anon_struct";
    const std::string UNION_STRUCT = "(union struct)";
    const std::string UNION_STRUCT_SYM = "union_struct";
    const std::string ANON = "(anonymous)";
    const std::string ANON_SYM = "anon";

    std::string curFileName;
    FileParse fileParser;
    std::vector<std::pair<std::pair<std::string, std::string>, ClangEdge::EdgeType>> unresolvedRef;
    std::vector<std::pair<std::pair<std::string, std::string>, std::pair<std::string, std::vector<std::string>>>*> unresolvedRefAttr;

    std::vector<std::pair<std::string, std::vector<std::string>>> findAttributes(std::string callerID, std::string calleeID);

    void printFileName(std::string curFile);

    std::string replaceLabel(std::string label, std::string init, std::string aft);
};


#endif //CLANGEX_ASTWALKER_H
