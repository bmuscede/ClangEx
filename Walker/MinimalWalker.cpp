//
// Created by bmuscede on 06/12/16.
//

#include "MinimalWalker.h"

MinimalWalker::MinimalWalker(){
    ClangArgParse::ClangExclude exclude;
    this->exclusions = exclude;
}

MinimalWalker::MinimalWalker(ClangArgParse::ClangExclude exclusions){
    this->exclusions = exclusions;
}

MinimalWalker::~MinimalWalker(){

}

void MinimalWalker::run(const MatchFinder::MatchResult &result){

}

void MinimalWalker::generateASTMatches(MatchFinder *finder){

}
