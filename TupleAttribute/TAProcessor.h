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

    bool readTAGraph(TAGraph* graph);
    TAGraph* writeTAGraph();

private:
    const char COMMENT_CHAR = '/';
    const char COMMENT_BLOCK_CHAR = '*';

    const std::string RELATION_FLAG = "FACT TUPLE";
    const std::string ATTRIBUTE_FLAG = "FACT ATTRIBUTE";
    const std::string SCHEME_FLAG = "SCHEME TUPLE";

    const std::string SCHEMA_HEADER = "//TAProcessor TA File Created by ClangEx";

    std::string entityString;

    std::vector<std::pair<std::string, std::set<std::pair<std::string, std::string>>>> relations;
    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::vector<std::string>>>>> attributes;
    std::vector<std::pair<std::pair<std::string, std::string>,
            std::vector<std::pair<std::string, std::vector<std::string>>>>> relAttributes;

    bool readGeneric(std::ifstream& modelStream, std::string fileName);
    bool readScheme(std::ifstream& modelStream, int* lineNum);
    bool readRelations(std::ifstream& modelStream, int* lineNum);
    bool readAttributes(std::ifstream& modelStream, int* lineNum);

    bool writeRelations(TAGraph* graph);
    bool writeAttributes(TAGraph* graph);

    std::string generateTAString();
    std::string generateRelationString();
    std::string generateAttributeString();
    std::string generateAttributeStringFromKVs(std::vector<std::pair<std::string, std::vector<std::string>>> attr);

    std::vector<std::pair<std::string, std::vector<std::string>>> generateAttributes(int lineNum,
                                                                                     bool& succ,
                                                                                     std::vector<std::string> line);
    std::vector<std::string> prepareLine(std::string line, bool &blockComment);
    std::string removeStandardComment(std::string line);
    std::string removeBlockComment(std::string line, bool &blockComment);
    int findRelEntry(std::string name);
    void createRelEntry(std::string name);
    int findAttrEntry(std::string attrName);
    int findAttrEntry(std::string src, std::string dst);
    void createAttrEntry(std::string attrName);
    void createAttrEntry(std::string src, std::string dst);

    void processNodes(std::vector<ClangNode*> nodes);
    void processEdges(std::vector<ClangEdge*> edges);
};


#endif //CLANGEX_TAPROCESSOR_H
