//
// Created by bmuscede on 06/12/16.
//

#ifndef CLANGEX_MINIMALWALKER_H
#define CLANGEX_MINIMALWALKER_H

#include "ASTWalker.h"

class MinimalWalker : public ASTWalker {
public:
    MinimalWalker();
    MinimalWalker(ClangArgParse::ClangExclude exclusions);
    virtual ~MinimalWalker();

    virtual void run(const MatchFinder::MatchResult &result);
    virtual void generateASTMatches(MatchFinder *finder);

private:
    enum {FUNC_DEC = 0, FUNC_DEC_SYS};
    const char* types[2] = {"func_dec", "func_dec_sys"};
};


#endif //CLANGEX_MINIMALWALKER_H
