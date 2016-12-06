//
// Created by bmuscede on 06/12/16.
//

#include <iostream>
#include "MinimalWalker.h"

using namespace std;

MinimalWalker::MinimalWalker(){
    ClangArgParse::ClangExclude exclude;
    this->exclusions = exclude;
}

MinimalWalker::MinimalWalker(ClangArgParse::ClangExclude exclusions){
    this->exclusions = exclusions;
}

MinimalWalker::~MinimalWalker(){ }

void MinimalWalker::run(const MatchFinder::MatchResult &result){
    //Check if the current result fits any of our match criteria.
    if (const DeclaratorDecl *dec = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[FUNC_DEC])) {
        //Get whether we have a system header.
        if (isInSystemHeader(result, dec)) return;

        cout << dec->getQualifiedNameAsString() << " -> " << this->generateFileName(result, dec->getInnerLocStart()) << endl;
    }
}

void MinimalWalker::generateASTMatches(MatchFinder *finder){
    //Function methods.
    if (!exclusions.cFunction){
        //Finds function declarations for current C/C++ file.
        finder->addMatcher(functionDecl().bind(types[FUNC_DEC]), this);
    }
}
