/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TAProcessor.h
//
// Created By: Bryan J Muscedere
// Date:22/12/16.
//
// Processor system that is capable of reading and writing a TA file. Allows for
// the conversion of a TA graph to TA file or from a TA file to TA graph. This is
// primarily used for merging an already created TA file with newly processed source
// code.
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

#ifndef CLANGEX_TAPROCESSOR_H
#define CLANGEX_TAPROCESSOR_H

#include <string>
#include <set>
#include "../Graph/TAGraph.h"

class TAProcessor {
public:
    /** Constructor/Destructor */
    TAProcessor(std::string entityRelName, Printer* print);
    ~TAProcessor();

    /** TA File I/O */
    bool readTAFile(std::string fileName);
    bool writeTAFile(std::string fileName);

    /** TA Graph I/O */
    bool readTAGraph(TAGraph* graph);
    TAGraph* writeTAGraph();

private:
    /** Private Flags and Strings */
    const char COMMENT_CHAR = '/';
    const char COMMENT_BLOCK_CHAR = '*';
    const std::string UNEXPECTED_FLAG = "Unexpected flag.";
    const std::string RSF_INVALID = "Line should contain a single tuple in RSF format.";
    const std::string ATTRIBUTE_SHORT = "Attribute line is too short to be valid!";
    const std::string RELATION_FLAG = "FACT TUPLE :";
    const std::string ATTRIBUTE_FLAG = "FACT ATTRIBUTE :";
    const std::string SCHEME_FLAG = "SCHEME TUPLE :";
    const std::string SCHEMA_HEADER = "//TAProcessor TA File Created by ClangEx";

    /** Private Variables */
    std::string entityString;
    Printer *clangPrinter;
    std::vector<std::pair<std::string, std::set<std::pair<std::string, std::string>>>> relations;
    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::vector<std::string>>>>> attributes;
    std::vector<std::pair<std::vector<std::string>,
            std::vector<std::pair<std::string, std::vector<std::string>>>>> relAttributes;

    /** TA Readers */
    bool readGeneric(std::ifstream& modelStream, std::string fileName);
    bool readScheme(std::ifstream& modelStream, int* lineNum);
    bool readRelations(std::ifstream& modelStream, int* lineNum);
    bool readAttributes(std::ifstream& modelStream, int* lineNum);

    /** TA Writers */
    bool writeRelations(TAGraph* graph);
    bool writeAttributes(TAGraph* graph);

    /** TA Component Generators */
    std::string generateTAString();
    std::string generateRelationString();
    std::string generateAttributeString();
    std::string generateAttributeStringFromKVs(std::vector<std::pair<std::string, std::vector<std::string>>> attr);
    std::vector<std::pair<std::string, std::vector<std::string>>> generateAttributes(int lineNum,
                                                                                     bool& succ,
                                                                                     std::vector<std::string> line);

    /** Helper Methods */
    std::vector<std::string> prepareLine(std::string line, bool &blockComment);
    std::string removeStandardComment(std::string line);
    std::string removeBlockComment(std::string line, bool &blockComment);
    int findRelEntry(std::string name);
    void createRelEntry(std::string name);
    int findAttrEntry(std::string attrName);
    int findAttrEntry(std::string relName, std::string src, std::string dst);
    void createAttrEntry(std::string attrName);
    void createAttrEntry(std::string relName, std::string src, std::string dst);

    /** Node / Edge Processors */
    void processNodes(std::vector<ClangNode*> nodes);
    void processEdges(std::vector<ClangEdge*> edges);
};


#endif //CLANGEX_TAPROCESSOR_H
