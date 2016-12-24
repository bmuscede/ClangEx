//
// Created by bmuscede on 22/12/16.
//

#ifndef CLANGEX_TAPROCESSOR_H
#define CLANGEX_TAPROCESSOR_H

#include <string>
#include "../Graph/TAGraph.h"

class TAProcessor {
public:
    TAProcessor(std::string entityRelName);
    ~TAProcessor();

    bool readTAFile(std::string fileName);
    bool writeTAFile(std::string fileName);

    bool readTAGraph(TAGraph graph);
    TAGraph* writeTAGraph();

private:
    const std::string COMMENT_PREFIX = "//";
    const std::string COMMENT_PREFIX_2 = "/*";

    const std::string RELATION_FLAG = "FACT TUPLE";
    const std::string ATTRIBUTE_FLAG = "FACT ATTRIBUTE";
    const std::string SCHEME_FLAG = "SCHEME TUPLE";

    std::string entityString;

    std::vector<std::pair<std::string, std::set<std::pair<std::string, std::string>>>> relations;
    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> attributes;

    bool readGeneric(std::ifstream modelStream, std::string fileName);
    bool readScheme(std::ifstream modelStream, int* lineNum);
    bool readRelations(std::ifstream modelStream, int* lineNum);
    bool readAttributes(std::ifstream modelStream, int* lineNum);
};


#endif //CLANGEX_TAPROCESSOR_H
