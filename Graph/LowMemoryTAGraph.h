/////////////////////////////////////////////////////////////////////////////////////////////////////////
// LowMemoryTAGraph.h
//
// Created By: Bryan J Muscedere
// Date: 14/03/18.
//
// Handles a run of the low-memory system by dumping items to disk.
// Adds items to the graph and manages resolution and disk dumps.
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

#ifndef CLANGEX_LOWMEMORYTAGRAPH_H
#define CLANGEX_LOWMEMORYTAGRAPH_H

#include <string>
#include <boost/filesystem.hpp>
#include "../Printer/Printer.h"
#include "TAGraph.h"

class LowMemoryTAGraph : public TAGraph {
public:
    /** Constructors / Destructor */
    LowMemoryTAGraph();
    LowMemoryTAGraph(std::string basePath);
    LowMemoryTAGraph(std::string basePath, int curNum);
    ~LowMemoryTAGraph() override;

    /** Changes the Root */
    void changeRoot(std::string basePath);

    /** Node Adders */
    bool addNode(ClangNode* node, bool assumeValid = false) override;
    bool addEdge(ClangEdge* edge, bool assumeValid = false) override;

    /** TA Generation */
    std::string generateTAFormat() override;
    void resolveFiles(ClangExclude exclusions) override;
    void resolveExternalReferences(Printer* print, bool silent = false) override;

    /** File System Adders */
    void addNodesToFile(std::map<std::string, ClangNode*> fileSkip) override;

    /** Settings/File Dumpers */
    void dumpCurrentFile(int fileNum, std::string file);
    void dumpSettings(std::vector<boost::filesystem::path> files,
                      TAGraph::ClangExclude exclude, bool blobMode);

    /** TA Dumper */
    void purgeCurrentGraph();

    static const std::string CUR_FILE_LOC;
    static const std::string CUR_SETTING_LOC;
    static const std::string BASE_INSTANCE_FN;
    static const std::string BASE_RELATION_FN;
    static const std::string BASE_MV_RELATION_FN;
    static const std::string BASE_ATTRIBUTE_FN;

private:
    const int PURGE_AMOUNT = 1000;

    std::string instanceFN;
    std::string relationFN;
    std::string mvRelationFN;
    std::string attributeFN;
    std::string settingFN;
    std::string curFileFN;

    static int currentNumber;
    int fileNumber;
    bool purge;

    /** File Operations */
    bool doesFileExist(std::string fN);
    void deleteFile(std::string fN);

    /** Helper Methods */
    void setPurgeStatus(bool purge);
    int getNumberEntities();
    std::vector<std::string> tokenize(std::string);
    std::vector<std::pair<std::string, std::vector<std::string>>> generateStrAttributes(std::vector<std::string> line);
};


#endif //CLANGEX_LOWMEMORYTAGRAPH_H
