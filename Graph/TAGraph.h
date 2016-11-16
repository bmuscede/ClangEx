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

    bool nodeExists(std::string ID);
    bool edgeExists(std::string IDOne, std::string IDTwo);

    bool addSingularAttribute(std::string ID, std::string key, std::string value);
    bool addSinuglarAttribute(std::string IDSrc, std::string IDDst, std::string key, std::string value);
    bool addAttribute(std::string ID, std::string key, std::string value);
    bool addAttribute(std::string IDSrc, std::string IDDst, std::string key, std::string value);

    std::string generateTAFormat();
    void addNodesToFile();

    static const std::string FILE_ATTRIBUTE;

private:
    std::vector<ClangNode*> nodeList;
    std::vector<ClangEdge*> edgeList;

    //TODO: Alter this to reflect ClangEx schema.
    std::string const TA_SCHEMA = "//Generated TA File\n//Author: Jingwei Wu & Bryan J Muscedere\n\nSCHEME TUPLE :\n//N"
            "odes\n$INHERIT\tcArchitecturalNds\tcRoot\n$INHERIT\tcAsgNds\t\t\tcRoot\n$INHERIT\tcSubSystem\t\tcArchitect"
            "uralNds\n$INHERIT\tcFile\t\t\tcArchitecturalNds\n$INHERIT\tcExecutable\t\tcFile\n$INHERIT\tcObjectFile\t\t"
            "cFile\n$INHERIT\tcArchiveFile\t\tcFile\n$INHERIT\tcClass\t\t\tcAsgNds\n$INHERIT\tcFunction\t\tcAsgNds\n$IN"
            "HERIT\tcObject\t\t\tcAsgNds\n\n//Relationships\ncontain\t\tcRoot\t\tcRoot\ncall\t\tcFunction\tcFunction\nr"
            "eference\tcFunction\tcObject\n\nSCHEME ATTRIBUTE :\n$ENTITY {\n\tx\n\ty\n\twidth\n\theight\n\tlabel\n}\n\n"
            "cRoot {\n\telision = contain\n\tcolor = (0.0 0.0 0.0)\n\tfile\n\tline\n\tname\n}\n\ncAsgNds {\n\tbeg\n\ten"
            "d\n\tfile\n\tline\n\tvalue\n\tcolor = (0.0 0.0 0.0)\n}\n\ncArchitecturalNds {\n\tclass_style = 4\n\tcolor "
            "= (0.0 0.0 1.0)\n\tcolor = (0.0 0.0 0.0)\n}\n\ncSubSystem {\n\tclass_style = 4\n\tcolor = (0.0 0.0 1.0)\n}"
            "\n\ncFile {\n\tclass_style = 2\n\tcolor = (0.9 0.9 0.9)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\ncExecutable {"
            "\n\tclass_style = 4\n\tcolor = (0.8 0.9 0.9)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\ncObjectFile {\n\tclass_st"
            "yle = 4\n\tcolor = (0.6 0.8 0.6)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\ncArchiveFile {\n\tclass_style = 4\n\t"
            "color = (0.5 0.5 0.1)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\ncFunction {\n\tfilename\n\tcolor = (1.0 0.0 0.0)"
            "\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\ncObject {\n\tfilename\n}\n\ncClass {\n\tfilename\n\tcolor = (0.2 0.4 "
            "0.1)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\n(reference) {\n\taccess\n}\n\n";

    std::string generateInstances();
    std::string generateRelationships();
    std::string generateAttributes();
};


#endif //CLANGEX_TAGRAPH_H
