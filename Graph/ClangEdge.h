//
// Created by bmuscede on 05/11/16.
//

#ifndef CLANGEX_CLANGEDGE_H
#define CLANGEX_CLANGEDGE_H

#include <string>
#include <vector>
#include <map>
#include "ClangNode.h"

class ClangEdge {
public:
    enum EdgeType {CALLS, REFERENCES, CONTAINS};
    static std::string getTypeString(EdgeType type);

    ClangEdge(ClangNode* src, ClangNode* dst, EdgeType type);
    ~ClangEdge();

    ClangNode* getSrc();
    ClangNode* getDst();

    bool addAttribute(std::string key, std::string value);
    bool clearAttribute(std::string key);
    std::vector<std::string> getAttribute(std::string key);

    std::string generateRelationship();
    std::string generateAttribute();

private:
    ClangNode* src;
    ClangNode* dst;
    EdgeType type;
    std::map<std::string, std::vector<std::string>> edgeAttributes;

    std::string printSingleAttribute(std::string key, std::vector<std::string> value);
    std::string printSetAttribute(std::string key, std::vector<std::string> value);
};


#endif //CLANGEX_CLANGEDGE_H
