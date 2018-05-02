//
// Created by bmuscede on 14/03/18.
//

#ifndef CLANGEX_LOWMEMORYTAGRAPH_H
#define CLANGEX_LOWMEMORYTAGRAPH_H

#include <string>
#include <boost/filesystem.hpp>
#include "../Printer/Printer.h"
#include "TAGraph.h"

class LowMemoryTAGraph : public TAGraph {
public:
    LowMemoryTAGraph();
    LowMemoryTAGraph(std::string basePath);
    LowMemoryTAGraph(std::string basePath, int curNum);
    ~LowMemoryTAGraph() override;

    bool addNode(ClangNode* node, bool assumeValid = false) override;
    bool addEdge(ClangEdge* edge, bool assumeValid = false) override;

    std::string generateTAFormat() override;
    void resolveFiles(ClangExclude exclusions) override;
    void resolveExternalReferences(Printer* print, bool silent = false) override;

    void addNodesToFile(std::map<std::string, ClangNode*> fileSkip) override;

    void dumpCurrentFile(int fileNum, std::string file);
    void dumpSettings(std::vector<boost::filesystem::path> files,
                      TAGraph::ClangExclude exclude, bool blobMode);

    void purgeCurrentGraph();

    static const std::string CUR_FILE_LOC;
    static const std::string CUR_SETTING_LOC;

private:
    const int PURGE_AMOUNT = 1000;
    const std::string BASE_INSTANCE_FN = "instances.ta";
    const std::string BASE_RELATION_FN = "relations.ta";
    const std::string BASE_MV_RELATION_FN = "old.relations.ta";
    const std::string BASE_ATTRIBUTE_FN = "attributes.ta";

    std::string instanceFN;
    std::string relationFN;
    std::string mvRelationFN;
    std::string attributeFN;
    std::string settingFN;
    std::string curFileFN;

    static int currentNumber;
    int fileNumber;
    bool purge;

    bool doesFileExist(std::string fN);
    void deleteFile(std::string fN);

    void setPurgeStatus(bool purge);

    int getNumberEntities();

    std::vector<std::string> tokenize(std::string);
    std::vector<std::pair<std::string, std::vector<std::string>>> generateStrAttributes(std::vector<std::string> line);
};


#endif //CLANGEX_LOWMEMORYTAGRAPH_H
