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
    const std::string RELATION_FLAG = "FACT TUPLE";
    const std::string ATTRIBUTE_FLAG = "FACT ATTRIBUTE";
    std::string entityString;

    std::pair<std::string, std::set<std::string>> relations;
    //std::pair<std::string,
};


#endif //CLANGEX_TAPROCESSOR_H
