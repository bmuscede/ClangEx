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
            "alNds\n$INHERIT\tcFile\t\t\tcArchitecturalNds\n$INHERIT\tcExecutable\t\tcFile\n$INHERIT\tcObjectFile\t\tcFi"
            "le\n$INHERIT\tcArchiveFile\t\tcFile\n$INHERIT\tcClass\t\t\tcAsgNds\n$INHERIT\tcFunction\t\tcAsgNds\n$INHERI"
            "T\tcObject\t\t\tcAsgNds\n\n//Relationships\ncontain\t\tcRoot\t\tcRoot\ncall\t\tcFunction\tcFunction\nrefere"
            "nce\tcFunction\tcObject\ninherits\tcClass\t\tcClass\n\nSCHEME ATTRIBUTE :\n$ENTITY {\n\tx\n\ty\n\twidth\n\t"
            "height\n\tlabel\n}\n\ncRoot {\n\telision = contain\n\tcolor = (0.0 0.0 0.0)\n\tfile\n\tline\n\tname\n}\n\nc"
            "AsgNds {\n\tbeg\n\tend\n\tfile\n\tline\n\tvalue\n\tcolor = (0.0 0.0 0.0)\n}\n\ncArchitecturalNds {\n\tclass"
            "_style = 4\n\tcolor = (0.0 0.0 1.0)\n\tcolor = (0.0 0.0 0.0)\n}\n\ncSubSystem {\n\tclass_style = 4\n\tcolor"
            " = (0.0 0.0 1.0)\n}\n\ncFile {\n\tclass_style = 2\n\tcolor = (0.9 0.9 0.9)\n\tlabelcolor = (0.0 0.0 0.0)\n}"
            "\n\ncExecutable {\n\tclass_style = 4\n\tcolor = (0.8 0.9 0.9)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\ncObjectFi"
            "le {\n\tclass_style = 4\n\tcolor = (0.6 0.8 0.6)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\ncArchiveFile {\n\tclas"
            "s_style = 4\n\tcolor = (0.5 0.5 0.1)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\ncFunction {\n\tfilename\n\tisStati"
            "c\n\tvisibility\n\tcolor = (1.0 0.0 0.0)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\ncObject {\n\tfilename\n}\n\ncC"
            "lass {\n\tfilename\n\tcolor = (0.2 0.4 0.1)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\n(reference) {\n\taccess\n}"
            "\n\n";

    std::string generateInstances();
    std::string generateRelationships();
    std::string generateAttributes();
};


#endif //CLANGEX_TAGRAPH_H
