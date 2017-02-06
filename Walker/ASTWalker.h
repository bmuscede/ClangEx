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

    static std::string generateMD5(std::string text);

protected:
    ClangArgParse::ClangExclude exclusions;

    ASTWalker(ClangArgParse::ClangExclude ex, bool md5, TAGraph* existing);

    std::string generateFileName(const MatchFinder::MatchResult result,
                                 clang::SourceLocation loc, bool suppressOutput = false);
    std::string generateID(std::string fileName, std::string qualifiedName);
    std::string generateLabel(const clang::Decl* decl, ClangNode::NodeType type);
    std::string generateClassName(std::string qualifiedName);

    bool isInSystemHeader(const MatchFinder::MatchResult &result, const clang::Decl *decl);

    void performAddClassCall(const MatchFinder::MatchResult result, const clang::DeclaratorDecl *decl,
                             ClangNode::NodeType type);
    clang::CXXRecordDecl* extractClass(clang::NestedNameSpecifier* name);

/********************************************************************************************************************/
    /** Node Insertion Functions */
    void addFunctionDecl(const MatchFinder::MatchResult results, const clang::DeclaratorDecl *dec);
    void addVariableDecl(const MatchFinder::MatchResult results, const clang::VarDecl *varDec = nullptr,
                         const clang::FieldDecl *fieldDec = nullptr);
    void addClassDecl(const MatchFinder::MatchResult results, const clang::CXXRecordDecl *classDecl,
                      std::string fName = "");
    void addEnumDecl(const MatchFinder::MatchResult results, const clang::EnumDecl *enumDecl,
                     std::string spoofFilename = std::string());
    void addEnumConstantDecl(const MatchFinder::MatchResult result, const clang::EnumConstantDecl *enumDecl,
                             std::string filenameSpoof = std::string());
    void addStructDecl(const MatchFinder::MatchResult result, const clang::RecordDecl *structDecl);

    /** Relation Insertion Functions */
    void addFunctionCall(const MatchFinder::MatchResult results, const clang::DeclaratorDecl* caller,
                         const clang::FunctionDecl* callee);
    void addVariableCall(const MatchFinder::MatchResult result, const clang::DeclaratorDecl *caller,
                         const clang::Expr* expr, const clang::VarDecl *varCallee, const clang::FieldDecl *fieldCallee = nullptr);
    void addClassCall(const MatchFinder::MatchResult result, const clang::CXXRecordDecl *classDecl, std::string declLabel);
    void addClassInheritance(const clang::CXXRecordDecl *childClass, const clang::CXXRecordDecl *parentClass);
    void addEnumConstantCall(const MatchFinder::MatchResult result, const clang::EnumDecl *enumDecl,
                             const clang::EnumConstantDecl *enumConstantDecl);
    void addEnumCall(const MatchFinder::MatchResult result, const clang::EnumDecl *enumDecl,
                     const clang::VarDecl *varDecl, const clang::FieldDecl *fieldDecl = nullptr);
    void addStructCall(const MatchFinder::MatchResult result, const clang::RecordDecl *structDecl,
                       const clang::DeclaratorDecl *itemDecl, ClangNode::NodeType inputType = ClangNode::NodeType::SUBSYSTEM);
    void addStructUseCall(const MatchFinder::MatchResult result, const clang::RecordDecl *structDecl,
                          const clang::VarDecl *varDecl, const clang::FieldDecl *fieldDecl = nullptr);
/********************************************************************************************************************/

private:
    const static int ANON_SIZE = 3;
    const std::string ANON_LIST[ANON_SIZE] = {"(anonymous struct)", "(union struct)", "(anonymous)"};
    const std::string ANON_REPLACE = "Anonymous";
    const static int MD5_LENGTH = 33;

    std::string curFileName;
    FileParse fileParser;
    bool md5Flag;
    TAGraph* graph;

    void printFileName(std::string curFile);
    std::string replaceLabel(std::string label, std::string init, std::string aft);
    std::string removeInvalidIDSymbols(std::string label);
    std::string removeInvalidSymbols(std::string label);
};


#endif //CLANGEX_ASTWALKER_H
