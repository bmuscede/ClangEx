//
// Created by bmuscede on 05/11/16.
//

#ifndef CLANGEX_TAGRAPH_H
#define CLANGEX_TAGRAPH_H

#include <vector>
#include <string>
#include "ClangNode.h"
#include "ClangEdge.h"

class TAGraph {
public:
    TAGraph();
    ~TAGraph();

    bool addNode(ClangNode* node);
    bool addEdge(ClangEdge* edge);

    ClangNode* findNodeByID(std::string ID);
    std::vector<ClangNode*> findNodeByName(std::string name);
    ClangEdge* findEdgeByIDs(std::string IDOne, std::string IDTwo);
    std::vector<ClangNode*> findSrcNodesByEdge(ClangNode* dst, ClangEdge::EdgeType type);
    std::vector<ClangNode*> findDstNodesByEdge(ClangNode* src, ClangEdge::EdgeType type);

    bool nodeExists(std::string ID);
    bool edgeExists(std::string IDOne, std::string IDTwo);

    bool addSingularAttribute(std::string ID, std::string key, std::string value);
    bool addSinuglarAttribute(std::string IDSrc, std::string IDDst, std::string key, std::string value);
    bool addAttribute(std::string ID, std::string key, std::string value);
    bool addAttribute(std::string IDSrc, std::string IDDst, std::string key, std::string value);

    std::string generateTAFormat();
    void addNodesToFile();

    bool isPartOfClass(ClangNode* node);

    static const std::string FILE_ATTRIBUTE;

private:
    std::vector<ClangNode*> nodeList;
    std::vector<ClangEdge*> edgeList;

    std::string const TA_SCHEMA = "//Generated TA File\n//Author: Jingwei Wu & Bryan J Muscedere\n\nSCHEME TUPLE :\n//No"
            "des\n$INHERIT\tcArchitecturalNds\tcRoot\n$INHERIT\tcAsgNds\t\t\tcRoot\n$INHERIT\tcSubSystem\t\tcArchitectur"
            "alNds\n$INHERIT\tcFile\t\t\tcArchitecturalNds\n$INHERIT\tcClass\t\t\tcAsgNds\n$INHERIT\tcFunction\t\tcAsgNd"
            "s\n$INHERIT\tcVariable\t\tcAsgNds\n$INHERIT\tcLang\t\t\tcAsgNds\n$INHERIT\tcEnum\t\t\tcLang\n$INHERIT\tcStr"
            "uct\t\t\tcLang\n$INHERIT\tcUnion\t\t\tcLang\n\n//Relationships\ncontain\t\tcRoot\t\tcRoot\ncall\t\tcFunctio"
            "n\tcFunction\nreference\tcAsgNds\t\tcAsgNds\ninherits\tcClass\t\tcClass\n\nSCHEME ATTRIBUTE :\n$ENTITY {\n\t"
            "x\n\ty\n\twidth\n\theight\n\tlabel\n}\n\ncRoot {\n\telision = contain\n\tcolor = (0.0 0.0 0.0)\n\tfile\n\tl"
            "ine\n\tname\n}\n\ncAsgNds {\n\tbeg\n\tend\n\tfile\n\tline\n\tvalue\n\tcolor = (0.0 0.0 0.0)\n}\n\ncArchitec"
            "turalNds {\n\tclass_style = 4\n\tcolor = (0.0 0.0 1.0)\n\tcolor = (0.0 0.0 0.0)\n}\n\ncSubSystem {\n\tclass"
            "_style = 4\n\tcolor = (0.0 0.0 1.0)\n}\n\ncFile {\n\tclass_style = 2\n\tcolor = (0.9 0.9 0.9)\n\tlabelcolor"
            " = (0.0 0.0 0.0)\n}\n\ncFunction {\n\tfilename\n\tisStatic\n\tisConst\n\tisVolatile\n\tisVariadic\n\tvisibi"
            "lity\n\tcolor = (1.0 0.0 0.0)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\ncVariable {\n\tfilename\n}\n\ncClass {\n\t"
            "filename\n\tbaseNum\n\tcolor = (0.2 0.4 0.1)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\ncEnum {\n\tfilename\n\tcol"
            "or = (0.9 0.2 0.5)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\n(reference) {\n\taccess\n}\n\n";

    std::string generateInstances();
    std::string generateRelationships();
    std::string generateAttributes();
};


#endif //CLANGEX_TAGRAPH_H
