/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TAGraph.h
//
// Created By: Bryan J Muscedere
// Date: 05/11/16.
//
// Maintains a graph structure of the AST while it's being processed. Stored
// as a graph to ease the conversion of source code to the TA format.
// This file also allows for the conversion of this graph to a TA
// file.
//
// Copyright (C) 2017, Bryan J. Muscedere
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CLANGEX_TAGRAPH_H
#define CLANGEX_TAGRAPH_H

#include <vector>
#include <string>
#include <unordered_map>
#include "ClangNode.h"
#include "ClangEdge.h"
#include "../Printer/Printer.h"
#include "../File/FileParse.h"

class TAGraph {
public:
    /** Toggle System */
    typedef struct {
        bool cSubSystem = false;
        bool cFile = false;
        bool cClass = false;
        bool cFunction = false;
        bool cVariable = false;
        bool cEnum = false;
        bool cStruct = false;
        bool cUnion = false;
    } ClangExclude;

    /** Constructor/Destructor */
    TAGraph();
    virtual ~TAGraph();

    /** Node/Edge Adders */
    virtual bool addNode(ClangNode* node, bool assumeValid = false);
    virtual bool addEdge(ClangEdge* edge, bool assumeValid = false);

    /** Node/Edge Removers */
    void removeNode(ClangNode* node, bool unsafe = true);
    void removeEdge(ClangEdge* edge);

    /** Attribute Adders */
    bool addAttribute(std::string ID, std::string key, std::string value);
    bool addAttribute(std::string IDSrc, std::string IDDst, ClangEdge::EdgeType type, std::string key,
                      std::string value);

    /** Node/Edge Getters */
    std::vector<ClangNode*> getNodes();
    std::vector<ClangEdge*> getEdges();

    /** Find Operations */
    ClangNode* findNodeByID(std::string ID);
    std::vector<ClangNode*> findNodeByName(std::string name);
    ClangEdge* findEdgeByIDs(std::string IDOne, std::string IDTwo, ClangEdge::EdgeType type);
    std::vector<ClangNode*> findSrcNodesByEdge(ClangNode* dst, ClangEdge::EdgeType type);
    std::vector<ClangNode*> findDstNodesByEdge(ClangNode* src, ClangEdge::EdgeType type);
    std::vector<ClangEdge*> findEdgesBySrcID(ClangNode* src);
    std::vector<ClangEdge*> findEdgesByDstID(ClangNode* dst);

    /** Node/Edge Checkers */
    bool nodeExists(std::string ID);
    bool edgeExists(std::string IDOne, std::string IDTwo, ClangEdge::EdgeType type);

    /** TA Operations */
    virtual std::string generateTAFormat();
    virtual void addNodesToFile(std::map<std::string, ClangNode*> fileSkip);

    /** Unresolved Operations */
    virtual void resolveExternalReferences(Printer* print, bool silent = false);
    virtual void resolveFiles(ClangExclude exclusions);
    void addPath(std::string path);

    static const std::string FILE_ATTRIBUTE;

protected:
    std::string const INSTANCE_FLAG = "$INSTANCE";

    /** TA Variables */
    std::unordered_map<std::string, ClangNode*> nodeList;
    std::unordered_map<std::string, std::vector<std::string>> nodeNameList;
    std::unordered_map<std::string, std::vector<ClangEdge*>> edgeSrcList;
    std::unordered_map<std::string, std::vector<ClangEdge*>> edgeDstList;

    /** Clear Graph */
    void clearGraph();

    /** TA Helper Methods */
    std::string generateTAHeader();
    std::string generateInstances();
    std::string generateRelationships();
    std::string generateAttributes();

private:
    /** Settings */
    FileParse fileParser;

    /** TA Const Variables */
    std::string const TA_HEADER = "//Generated TA File";
    std::string const TA_SCHEMA = "//Author: Jingwei Wu & Bryan J Muscedere\n\nSCHEME TUPLE :\n//Nodes\n$INHERIT\tcArch"
            "itecturalNds\tcRoot\n$INHERIT\tcAsgNds\t\t\tcRoot\n$INHERIT\tcSubSystem\t\tcArchitecturalNds\n$INHERIT\tcF"
            "ile\t\t\tcArchitecturalNds\n$INHERIT\tcClass\t\t\tcAsgNds\n$INHERIT\tcFunction\t\tcAsgNds\n$INHERIT\tcVari"
            "able\t\tcAsgNds\n$INHERIT\tcLang\t\t\tcAsgNds\n$INHERIT\tcEnum\t\t\tcLang\n$INHERIT\tcEnumConst\t\tcLang\n"
            "$INHERIT\tcStruct\t\t\tcLang\n$INHERIT\tcUnion\t\t\tcLang\n\n//Relationships\ncontain\t\tcRoot\t\t\tcRoot\n"
            "call\t\tcFunction\t\tcFunction\nreference\tcAsgNds\t\t\tcAsgNds\ninherit\t\tcClass\t\t\tcClass\nfContain\t"
            "cArchitecturalNds\tcAsgNds\n\nSCHEME ATTRIBUTE :\n$ENTITY {\n\tx\n\ty\n\twidth\n\theight\n\tlabel\n}\n\ncR"
            "oot {\n\telision = contain\n\tcolor = (0.0 0.0 0.0)\n\tfile\n\tline\n\tname\n}\n\ncAsgNds {\n\tbeg\n\tend"
            "\n\tfile\n\tline\n\tvalue\n\tcolor = (0.0 0.0 0.0)\n}\n\ncArchitecturalNds {\n\tclass_style = 4\n\tcolor ="
            " (0.0 0.0 1.0)\n\tcolor = (0.0 0.0 0.0)\n}\n\ncSubSystem {\n\tclass_style = 4\n\tcolor = (1.0 1.0 1.0)\n\t"
            "labelcolor = (0.0 0.0 0.0)\n}\n\ncFile {\n\tclass_style = 2\n\tcolor = (1.0 0.04 1.0)\n\tlabelcolor = (0.0"
            " 0.0 0.0)\n}\n\ncFunction {\n\tfilename\n\tisStatic\n\tisConst\n\tisVolatile\n\tisVariadic\n\tvisibility\n"
            "\tcolor = (1.0 0.0 0.0)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\ncVariable {\n\tfilename\n\tscopeType\n\tisStat"
            "ic\n}\n\ncClass {\n\tfilename\n\tbaseNum\n\tcolor = (0.2 0.4 0.1)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\ncEnu"
            "m {\n\tfilename\n\tcolor = (0.9 0.2 0.5)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\ncEnumConst {\n\tfilename\n\tc"
            "olor = (0.9 0.2 0.5)\n\tlabelcolor = (0.0 0.0 0.0)\n}\n\n(reference) {\n\taccess\n}\n";
};


#endif //CLANGEX_TAGRAPH_H
