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

    std::string generateTAFormat();
    void addNodesToFile();

    static const std::string FILE_ATTRIBUTE;

private:
    std::vector<ClangNode*> nodeList;
    std::vector<ClangEdge*> edgeList;

    //TODO: Alter this to reflect ClangEx schema.
    std::string const TA_SCHEMA = "//Generated TA File\n//Author: Jingwei Wu & Bryan J Muscedere\n\nSCHEME TUPLE :\n"
            "cLinks		cRoot		cRoot\ncontain		cRoot		cRoot\ncall             cRoot           cRoot\n"
            "reference      cRoot       cRoot\n\n"
            "$INHERIT	cArchitecturalNds	cRoot\n"
            "$INHERIT	cAsgNds			cRoot\n$INHERIT	cSubSystem		cArchitecturalNds\n$INHERIT	cFile			cArchitecturalNds\n"
            "$INHERIT	cExecutable		cFile\n$INHERIT	cObjectFile		cFile\n$INHERIT	cArchiveFile		cFile\n"
            "$INHERIT	cFunction		cAsgNds\n$INHERIT	cObject			cAsgNds\n\nSCHEME ATTRIBUTE :\n"
            "$ENTITY {\n\tx\n\ty\n\twidth\n\theight\n\tlabel\n}\n\ncRoot {\n\telision = contain\n\tcolor = (0.0 0.0 0.0)\n\t"
            "file\n\tline\n\tname\n}\n\ncAsgNds {\n\tbeg\n\tend\n\tfile\n\tline\n\tvalue\n\tcolor = (0.0 0.0 0.0)\n}\n\n"
            "cArchitecturalNds {\n\tclass_style = 4\n\tcolor = (0.0 0.0 1.0)\n\tcolor = (0.0 0.0 0.0)\n}\n\n"
            "cSubSystem {\n\tclass_style = 4\n\tcolor = (0.0 0.0 1.0)\n}\n\ncFile {\n\tclass_style = 2\n\tcolor = (0.9 0.9 0.9)\n\t"
            "labelcolor = (0.0 0.0 0.0)\n}\n\ncExecutable {\n\tclass_style = 4\n\tcolor = (0.8 0.9 0.9)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\n"
            "cObjectFile {\n\tclass_style = 4\n\tcolor = (0.6 0.8 0.6)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\n"
            "cArchiveFile {\n\tclass_style = 4\n\tcolor = (0.5 0.5 0.1)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\n"
            "cFunction {\n\tfilename\n\tcolor = (1.0 0.0 0.0)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\n"
            "cObject {\n\tfilename\n}\n\n(cLinks) {\n\tcolor = (0.0 0.0 0.0)\n}\n\n";

    std::string generateInstances();
    std::string generateRelationships();
    std::string generateAttributes();
};


#endif //CLANGEX_TAGRAPH_H
