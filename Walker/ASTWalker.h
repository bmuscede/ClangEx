/////////////////////////////////////////////////////////////////////////////////////////////////////////
// ASTWalker.h
//
// Created By: Bryan J Muscedere
// Date: 05/11/16.
//
// Parent class that supports the other two walkers. Provides methods
// that add each of the AST elements. Also provides information for
// ID and name generation. Basically, this method is a catch-all for
// operations.
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
#include "../Driver/ClangDriver.h"
#include "../File/FileParse.h"
#include "../Printer/Printer.h"

using namespace clang::ast_matchers;

class ASTWalker : public MatchFinder::MatchCallback {
public:
    /** Destructor */
    virtual ~ASTWalker();

    /** Pure Virtual Methods */
    virtual void run(const MatchFinder::MatchResult &result) = 0;
    virtual void generateASTMatches(MatchFinder *finder) = 0;

    /** Graph Operations */
    TAGraph* getGraph();

    /** MD5 Operations */
    static std::string generateMD5(std::string text);

protected:
    /** Protected Variables */
    TAGraph::ClangExclude exclusions;

    /** Constructor */
    ASTWalker(TAGraph::ClangExclude ex, bool lowMemory, Printer* print, TAGraph* existing = nullptr);

    /** Item Qualifiers */
    std::string generateFileName(const MatchFinder::MatchResult result,
                                 clang::SourceLocation loc, bool suppressOutput = false);
    std::string generateID(const MatchFinder::MatchResult result, const clang::NamedDecl *dec);
    std::string generateLabel(const MatchFinder::MatchResult result, const clang::NamedDecl *dec);

    /** Protected Helper Methods */
    bool isInSystemHeader(const MatchFinder::MatchResult &result, const clang::Decl *decl);
    clang::CXXRecordDecl* extractClass(clang::NestedNameSpecifier* name);

/********************************************************************************************************************/
    /** Node Insertion Functions */
    void addFunctionDecl(const MatchFinder::MatchResult results, const clang::FunctionDecl *dec);
    void addVariableDecl(const MatchFinder::MatchResult results, const clang::VarDecl *varDec = nullptr,
                         const clang::FieldDecl *fieldDec = nullptr);
    void addClassDecl(const MatchFinder::MatchResult results, const clang::CXXRecordDecl *classDecl,
                      std::string fName = "");
    void addEnumDecl(const MatchFinder::MatchResult results, const clang::EnumDecl *enumDecl,
                     std::string spoofFilename = std::string());
    void addEnumConstantDecl(const MatchFinder::MatchResult result, const clang::EnumConstantDecl *enumDecl,
                             std::string filenameSpoof = std::string());
    void addStructDecl(const MatchFinder::MatchResult result, const clang::RecordDecl *structDecl, std::string filename = "");
    void addUnionDecl(const MatchFinder::MatchResult result, const clang::RecordDecl *unionDecl, std::string filename = "");

    /** Relation Insertion Functions */
    void addFunctionCall(const MatchFinder::MatchResult results, const clang::DeclaratorDecl* caller,
                         const clang::FunctionDecl* callee);
    void addVariableCall(const MatchFinder::MatchResult result, const clang::DeclaratorDecl *caller,
                         const clang::Expr* expr, const clang::VarDecl *varCallee, const clang::FieldDecl *fieldCallee = nullptr);
    void addVariableInsideCall(const MatchFinder::MatchResult result, const clang::FunctionDecl *functionParent,
                               const clang::VarDecl *varChild, const clang::FieldDecl *fieldChild = nullptr);
    void addClassCall(const MatchFinder::MatchResult result, const clang::CXXRecordDecl *classDecl, std::string declID,
                      std::string declLabel);
    void addClassInheritance(const MatchFinder::MatchResult result,
                             const clang::CXXRecordDecl *childClass, const clang::CXXRecordDecl *parentClass);
    void addEnumConstantCall(const MatchFinder::MatchResult result, const clang::EnumDecl *enumDecl,
                             const clang::EnumConstantDecl *enumConstantDecl);
    void addEnumCall(const MatchFinder::MatchResult result, const clang::EnumDecl *enumDecl,
                     const clang::VarDecl *varDecl, const clang::FieldDecl *fieldDecl = nullptr);
    void addRecordCall(const MatchFinder::MatchResult result, const clang::RecordDecl *recordDecl,
                       const clang::DeclaratorDecl *itemDecl);
    void addRecordUseCall(const MatchFinder::MatchResult result, const clang::RecordDecl *recordDecl,
                          const clang::VarDecl *varDecl, const clang::FieldDecl *fieldDecl = nullptr);
/********************************************************************************************************************/

private:
    /** Private Const Variables */
    const static int ANON_SIZE = 4;
    const std::string ANON_LIST[ANON_SIZE] = {"(anonymous struct)", "(union struct)", "(anonymous)", "(anonymous union)"};
    const std::string FILE_EXT[7] = {".C", ".cc", ".cpp", ".CPP", ".c++", ".cp", ".cxx"};
    const std::string ANON_REPLACE = "Anonymous";
    const static int MD5_LENGTH = 33;

    /** Private Variables */
    std::string curFileName;
    TAGraph* graph;
    Printer *clangPrinter;

    /** Edge Processor */
    void processEdge(std::string srcID, std::string srcLabel, std::string dstID, std::string dstLabel,
                     ClangEdge::EdgeType type, std::vector<std::pair<std::string, std::string>> attributes =
                     std::vector<std::pair<std::string, std::string>>());

    /** Helper Methods */
    void printFileName(std::string curFile);
    std::string generateIDString(const MatchFinder::MatchResult result, const clang::NamedDecl* dec);
    std::string generateLineNumber(const MatchFinder::MatchResult result, const SourceLocation loc);
    bool isSource(std::string fileName);
    bool isAnonymousRecord(std::string qualName);
};


#endif //CLANGEX_ASTWALKER_H
