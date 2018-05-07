/////////////////////////////////////////////////////////////////////////////////////////////////////////
// LowMemoryTAGraph.cpp
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

#include <fstream>
#include <sys/stat.h>
#include <boost/algorithm/string.hpp>
#include <cstdio>
#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "LowMemoryTAGraph.h"

using namespace std;
namespace bs = boost::filesystem;

/** Const Methods */
int LowMemoryTAGraph::currentNumber = 0;
const string LowMemoryTAGraph::CUR_FILE_LOC = "curFile.txt";
const string LowMemoryTAGraph::CUR_SETTING_LOC = "curSetting.txt";
const string LowMemoryTAGraph::BASE_INSTANCE_FN = "instances.ta";
const string LowMemoryTAGraph::BASE_RELATION_FN = "relations.ta";
const string LowMemoryTAGraph::BASE_MV_RELATION_FN = "old.relations.ta";
const string LowMemoryTAGraph::BASE_ATTRIBUTE_FN = "attributes.ta";

/**
 * Creates a graph with a base and a specific number.
 * @param basePath The base path to dump to.
 * @param curNum The graph number.
 */
LowMemoryTAGraph::LowMemoryTAGraph(string basePath, int curNum) : TAGraph() {
    purge = true;
    fileNumber = curNum;

    instanceFN = bs::weakly_canonical(bs::path(basePath + "/" + to_string(fileNumber) + "-" + BASE_INSTANCE_FN)).string();
    relationFN = bs::weakly_canonical(bs::path(basePath + "/" + to_string(fileNumber) + "-" + BASE_RELATION_FN)).string();
    mvRelationFN = bs::weakly_canonical(bs::path(basePath + "/" + to_string(fileNumber) + "-" + BASE_MV_RELATION_FN)).string();
    attributeFN = bs::weakly_canonical(bs::path(basePath + "/" + to_string(fileNumber) + "-" + BASE_ATTRIBUTE_FN)).string();
    settingFN = bs::weakly_canonical(bs::path(basePath + "/" + to_string(fileNumber) + "-" + CUR_SETTING_LOC)).string();
    curFileFN = bs::weakly_canonical(bs::path(basePath + "/" + to_string(fileNumber) + "-" + CUR_FILE_LOC)).string();
}

/**
 * Creates a graph with a base.
 * @param basePath The base path to dump to.
 */
LowMemoryTAGraph::LowMemoryTAGraph(string basePath) : TAGraph() {
    purge = true;
    fileNumber = LowMemoryTAGraph::currentNumber;
    LowMemoryTAGraph::currentNumber++;

    instanceFN = bs::weakly_canonical(bs::path(basePath + "/" + to_string(fileNumber) + "-" + BASE_INSTANCE_FN)).string();
    relationFN = bs::weakly_canonical(bs::path(basePath + "/" + to_string(fileNumber) + "-" + BASE_RELATION_FN)).string();
    mvRelationFN = bs::weakly_canonical(bs::path(basePath + "/" + to_string(fileNumber) + "-" + BASE_MV_RELATION_FN)).string();
    attributeFN = bs::weakly_canonical(bs::path(basePath + "/" + to_string(fileNumber) + "-" + BASE_ATTRIBUTE_FN)).string();
    settingFN = bs::weakly_canonical(bs::path(basePath + "/" + to_string(fileNumber) + "-" + CUR_SETTING_LOC)).string();
    curFileFN = bs::weakly_canonical(bs::path(basePath + "/" + to_string(fileNumber) + "-" + CUR_FILE_LOC)).string();

    if (doesFileExist(instanceFN)) deleteFile(instanceFN);
    if (doesFileExist(relationFN)) deleteFile(relationFN);
    if (doesFileExist(attributeFN)) deleteFile(attributeFN);
    std::ofstream f = std::ofstream{ instanceFN };
    f.close();
    f = ofstream{ relationFN };
    f.close();
    f = ofstream{ attributeFN };
    f.close();
    f = ofstream{ settingFN };
    f.close();
    f = ofstream{ curFileFN };
    f.close();
}

/**
 * Generates a default LowMemoryTAGraph.
 */
LowMemoryTAGraph::LowMemoryTAGraph() : TAGraph() {
    purge = true;
    fileNumber = LowMemoryTAGraph::currentNumber;
    LowMemoryTAGraph::currentNumber++;

    instanceFN = bs::weakly_canonical(bs::path(to_string(fileNumber) + "-" + BASE_INSTANCE_FN)).string();
    relationFN = bs::weakly_canonical(bs::path(to_string(fileNumber) + "-" + BASE_RELATION_FN)).string();
    mvRelationFN = bs::weakly_canonical(bs::path(to_string(fileNumber) + "-" + BASE_MV_RELATION_FN)).string();
    attributeFN = bs::weakly_canonical(bs::path(to_string(fileNumber) + "-" + BASE_ATTRIBUTE_FN)).string();
    settingFN = bs::weakly_canonical(bs::path(to_string(fileNumber) + "-" + CUR_SETTING_LOC)).string();
    curFileFN = bs::weakly_canonical(bs::path(to_string(fileNumber) + "-" + CUR_FILE_LOC)).string();

    if (doesFileExist(instanceFN)) deleteFile(instanceFN);
    if (doesFileExist(relationFN)) deleteFile(relationFN);
    if (doesFileExist(attributeFN)) deleteFile(attributeFN);
    std::ofstream f = std::ofstream{ instanceFN };
    f.close();
    f = ofstream{ relationFN };
    f.close();
    f = ofstream{ attributeFN };
    f.close();
    f = ofstream{ settingFN };
    f.close();
    f = ofstream{ curFileFN };
    f.close();

}

/**
 * Deletes all the files to disk.
 */
LowMemoryTAGraph::~LowMemoryTAGraph() {
    if (doesFileExist(instanceFN)) deleteFile(instanceFN);
    if (doesFileExist(relationFN)) deleteFile(relationFN);
    if (doesFileExist(attributeFN)) deleteFile(attributeFN);
    if (doesFileExist(settingFN)) deleteFile(settingFN);
    if (doesFileExist(curFileFN)) deleteFile(curFileFN);
}

/**
 * Changes where the graph is located.
 * @param basePath The base path to change the location.
 */
void LowMemoryTAGraph::changeRoot(std::string basePath){
    instanceFN = bs::weakly_canonical(bs::path(basePath + "/" + to_string(fileNumber) + "-" + BASE_INSTANCE_FN)).string();
    relationFN = bs::weakly_canonical(bs::path(basePath + "/" + to_string(fileNumber) + "-" + BASE_RELATION_FN)).string();
    mvRelationFN = bs::weakly_canonical(bs::path(basePath + "/" + to_string(fileNumber) + "-" + BASE_MV_RELATION_FN)).string();
    attributeFN = bs::weakly_canonical(bs::path(basePath + "/" + to_string(fileNumber) + "-" + BASE_ATTRIBUTE_FN)).string();
    settingFN = bs::weakly_canonical(bs::path(basePath + "/" + to_string(fileNumber) + "-" + CUR_SETTING_LOC)).string();
    curFileFN = bs::weakly_canonical(bs::path(basePath + "/" + to_string(fileNumber) + "-" + CUR_FILE_LOC)).string();
}

/**
 * Adds a node to the graph.
 * @param node The node to add.
 * @param assumeValid Whether we assume it's valid.
 * @return Whether it was added.
 */
bool LowMemoryTAGraph::addNode(ClangNode* node, bool assumeValid){
    //Check the number of entities.
    int amt = getNumberEntities();
    if (amt > PURGE_AMOUNT){
        purgeCurrentGraph();
    }

    //Add the graph.
    return TAGraph::addNode(node, assumeValid);
}

/**
 * Adds an edge to the graph.
 * @param edge The edge to add.
 * @param assumeValid Whether we assume it's valid.
 * @return Whether it was added.
 */
bool LowMemoryTAGraph::addEdge(ClangEdge* edge, bool assumeValid){
    //Check the number of entities.
    int amt = getNumberEntities();
    if (amt > PURGE_AMOUNT){
        purgeCurrentGraph();
    }

    //Add the graph.
    return TAGraph::addEdge(edge, assumeValid);
}

/**
 * Generates the TA for this graph.
 * @return The TA format.
 */
string LowMemoryTAGraph::generateTAFormat() {
    string format = generateTAHeader();
    string curLine;

    //Generate the instances.
    format += "FACT TUPLE :\n";
    ifstream instances(instanceFN);
    if (instances.is_open()) while(getline(instances, curLine)) format += curLine + "\n";
    instances.close();

    //Generate the relations.
    ifstream relations(relationFN);
    if (relations.is_open()) while(getline(relations, curLine)) format += curLine + "\n";
    relations.close();
    format += "\n";

    //Generate the attributes.
    format += "FACT ATTRIBUTE :\n";
    ifstream attributes(attributeFN);
    if (attributes.is_open()) while(getline(attributes, curLine)) format += curLine + "\n";
    attributes.close();

    return format;
}

/**
 * Resolves files on disk.
 * @param exclusions The exclusions to process.
 */
void LowMemoryTAGraph::resolveFiles(ClangExclude exclusions){
    //Disable purging.
    setPurgeStatus(false);

    //Recover files.
    TAGraph::resolveFiles(exclusions);
    setPurgeStatus(true);

    //Purge the results.
    purgeCurrentGraph();
}

/**
 * Resolves external references.
 * @param print The printer that prints messages.
 * @param silent Whether we should print silently.
 */
void LowMemoryTAGraph::resolveExternalReferences(Printer* print, bool silent) {
    //First, purge the current graph.
    purgeCurrentGraph();

    //Generate a map of the instances.
    unordered_map<string, string> instanceMap;
    ifstream instances(instanceFN);
    if (!instances.is_open()) return;

    string curLine;
    while (getline(instances, curLine)){
        vector<string> lineSplit = tokenize(curLine);
        if (lineSplit.size() != 3) continue;

        //Checks whether instance already exists.
        if (instanceMap.find(lineSplit.at(1)) == instanceMap.end()){
            instanceMap[lineSplit.at(1)] = lineSplit.at(2);
        }
    }
    instances.close();
    deleteFile(instanceFN);

    //Next, resolves the relations.
    bs::path org = relationFN;
    bs::path dst = mvRelationFN;
    rename(org, dst);

    vector<string> removedRels;

    ifstream original(mvRelationFN);
    ofstream destination(relationFN, std::ios_base::out);
    if (!original.is_open() || !destination.is_open()) return;
    while(getline(original, curLine)){
        vector<string> lineSplit = tokenize(curLine);
        if (lineSplit.size() != 3) continue;

        //Checks if the relation can be resolved.
        if (instanceMap.find(lineSplit.at(1)) == instanceMap.end() ||
                instanceMap.find(lineSplit.at(2)) == instanceMap.end()){
            removedRels.push_back(lineSplit.at(0) + " " + lineSplit.at(1) + " " + lineSplit.at(2));
            continue;
        }

        destination << curLine << "\n";
    }
    original.close();
    destination.close();
    deleteFile(mvRelationFN);

    //Compress attributes.
    unordered_map<string, vector<pair<string, vector<string>>>> attrMap;
    ifstream attributes(attributeFN);
    if (!attributes.is_open()) return;

    while(getline(attributes, curLine)) {
        //Prepare the line.
        vector<string> entry = tokenize(curLine);

        //Checks for what type of system we're dealing with.
        if (entry.at(0).compare("(") == 0 || entry.at(0).find("(") == 0) {
            //Relation attribute.
            string relID = entry.at(0) + " " + entry.at(1) + " " + entry.at(2);
            relID.erase(0, 1);
            relID.erase(relID.size() - 1);

            //Check if the relationID matches.
            if (find(removedRels.begin(), removedRels.end(), relID) != removedRels.end()){
                continue;
            }

            //Generates the attribute list.
            relID = "(" + relID + ")";
            entry.erase(entry.begin(), entry.begin() + 3);
            auto attrs = generateStrAttributes(entry);
            if (attrs.size() == 0) continue;

            //Find the attributes.
            if (attrMap.find(relID) == attrMap.end()){
                attrMap[relID] = attrs;
            } else {
                //Compact the attributes.
                auto currentAttrs = attrMap[relID];

                for (auto curA : attrs){
                    bool found = false;
                    for (auto cAI : currentAttrs){
                        if (cAI.first == curA.first){
                            found = true;

                            auto cAIVal = cAI.second;
                            for (auto val : curA.second){
                                if (find(cAIVal.begin(), cAIVal.end(), val) == cAIVal.end()){
                                    cAIVal.push_back(val);
                                }
                            }
                            break;
                        }
                    }

                    if (!found){
                        currentAttrs.push_back(curA);
                    }
                }

                attrMap[relID] = currentAttrs;
            }
        } else {
            //Regular attribute.
            //Gets the name and trims down the vector.
            string attrName = entry.at(0);
            entry.erase(entry.begin());

            //Generates the attribute list.
            auto attrs = generateStrAttributes(entry);
            if (attrs.size() == 0) continue;

            //Find the attributes.
            if (attrMap.find(attrName) == attrMap.end()){
                attrMap[attrName] = attrs;
            } else {
                //Compact the attributes.
                auto currentAttrs = attrMap[attrName];

                for (auto curA : attrs){
                    bool found = false;
                    for (auto cAI : currentAttrs){
                        if (cAI.first == curA.first){
                            found = true;

                            auto cAIVal = cAI.second;
                            for (auto val : curA.second){
                                if (find(cAIVal.begin(), cAIVal.end(), val) == cAIVal.end()){
                                    cAIVal.push_back(val);
                                }
                            }
                            break;
                        }
                    }

                    if (!found){
                        currentAttrs.push_back(curA);
                    }
                }

                attrMap[attrName] = currentAttrs;
            }
        }
    }
    attributes.close();

    //Write the attributes.
    ofstream destAttr(attributeFN, std::ios_base::out);
    if (!destAttr.is_open()) return;
    for (auto entry : attrMap){
        string attrLine = entry.first + " { ";

        for (auto entries : entry.second){
            attrLine += entries.first + " = ";
            if (entries.second.size() == 1) attrLine += entries.second.at(0) + " ";
            else {
                attrLine += "( ";
                for (auto curVal : entries.second) attrLine += curVal + " ";
                attrLine += ") ";
            }
        }
        destAttr << attrLine + " }\n";
    }
    attrMap.clear();
    destAttr.close();

    //Write the instances.
    ofstream outI(instanceFN);
    if (!outI.is_open()) return;

    for (auto it : instanceMap) {
        outI << INSTANCE_FLAG << " " << it.first << " " << it.second << "\n";
    }
    outI.close();
}

/**
 * Adds nodes to a file system.
 * @param fileSkip Whether we skip files.
 */
void LowMemoryTAGraph::addNodesToFile(std::map<std::string, ClangNode*> fileSkip){
    //Load in each attribute.
    ifstream attributes(attributeFN);
    if (!attributes.is_open()) return;

    string current;
    while (getline(attributes, current)){
        boost::algorithm::trim(current);

        //Check for a relation attribute.
        if (current.at(0) == '(') continue;

        //Get the name and attributes.
        string name = current.substr(0, current.find(" { "));
        string attrs = current.substr(current.find(" { ") + 3);
        attrs.pop_back();
        attrs.pop_back();

        //Split by space.
        vector<string> tokens;
        boost::algorithm::split(tokens, attrs, boost::is_any_of(" "));

        //Process the attributes.
        string attrValue;
        int pos = 0;
        bool doNotCheck = false;
        for (string curTok : tokens){
            boost::algorithm::trim(curTok);

            //Check the value of pos.
            if (pos == 0){
                if (curTok == FILE_ATTRIBUTE){
                    doNotCheck = false;
                } else {
                    doNotCheck = true;
                }
            } else if (pos == 2 ) {
                pos = -1;
                if (doNotCheck == true) {
                    pos = 0;
                    continue;
                }

                //Add the file node.
                string file = curTok;
                ClangNode* fileNode;
                if (file.compare("") != 0) {
                    //Find the appropriate node.
                    vector<ClangNode*> fileVec = findNodeByName(file);
                    if (fileVec.size() > 0) {
                        fileNode = fileVec.at(0);

                        //We now look up the file node.
                        auto ptrSkip = fileSkip.find(file);
                        if (ptrSkip != fileSkip.end()) {
                            ClangNode *skip = ptrSkip->second;
                            fileNode = skip;
                        }
                    } else {
                        continue;
                    }

                    //Add it to the graph.
                    ClangEdge *edge = new ClangEdge(fileNode, file, ClangEdge::FILE_CONTAIN);
                    addEdge(edge);
                }
            }

            pos++;
        }
    }

    attributes.close();
}

/**
 * Dumps the current file to disk.
 * @param fileNum The file number.
 * @param file The file to dump.
 */
void LowMemoryTAGraph::dumpCurrentFile(int fileNum, string file){
    //Opens the file list.
    std::ofstream curFile(curFileFN);
    if (!curFile.is_open()) return;

    //Writes the current file.
    curFile << fileNum << endl << file;
    curFile.close();
}

/**
 * Dumps the settings to disk.
 * @param files The files being processed.
 * @param exclude The exclusions.
 * @param blobMode Blob mode toggle.
 */
void LowMemoryTAGraph::dumpSettings(vector<bs::path> files, TAGraph::ClangExclude exclude, bool blobMode){
    //Opens the file.
    std::ofstream curSettings(settingFN);
    if (!curSettings.is_open()) return;

    //First, dump the files.
    for (int i = 0; i < files.size(); i++){
        curSettings << files.at(i).string();
        if (i != files.size() - 1) curSettings << ",";
    }
    curSettings << endl;

    //Next, dump the excludes.
    curSettings << exclude.cClass << exclude.cEnum << exclude.cFile << exclude.cFunction << exclude.cStruct <<
                exclude.cSubSystem << exclude.cUnion << exclude.cVariable;
    curSettings << blobMode;
    curSettings.close();
}

/**
 * Checks whether a file exists.
 * @param fN The file to check.
 * @return Whether it exists.
 */
bool LowMemoryTAGraph::doesFileExist(string fN){
    struct stat buffer;
    return (stat (fN.c_str(), &buffer) == 0);
}

/**
 * Deletes a file.
 * @param fN The file to delete.
 */
void LowMemoryTAGraph::deleteFile(string fN) {
    remove(fN.c_str());
}

/**
 * Alters whether we purge.
 * @param purge The purge toggle.
 */
void LowMemoryTAGraph::setPurgeStatus(bool purge){
    this->purge = purge;
}

/**
 * Dumps the current TA to disk.
 */
void LowMemoryTAGraph::purgeCurrentGraph(){
    if (!purge) return;

    //Start by writing everything to disk.
    ofstream instances(instanceFN, std::ios::out | std::ios::app);
    if (!instances.is_open()) return;
    instances << generateInstances();
    instances.close();

    ofstream relations(relationFN, std::ios::out | std::ios::app);
    if (!relations.is_open()) return;
    relations << generateRelationships();
    relations.close();

    ofstream attributes(attributeFN, std::ios::out | std::ios::app);
    if (!attributes.is_open()) return;
    attributes << generateAttributes();
    attributes.close();

    //Clear the graph.
    clearGraph();
}

/**
 * Gets the number of entities in the graph.
 * @return The number of entities in the graph.
 */
int LowMemoryTAGraph::getNumberEntities(){
    //Get the number of keys in the graph.
    auto curAmnt = (int) nodeList.size();
    return curAmnt;
}

/**
 * Tokenizes the string based on a space.
 * @param curString The current string to split.
 * @return A vector of tokens.
 */
vector<string> LowMemoryTAGraph::tokenize(string curString){
    vector<string> tokens;

    boost::split(tokens, curString, boost::is_any_of(" "), boost::token_compress_on);
    return tokens;
}

/**
 * Generate a set of attributes based on a vector of tokens.
 * @param line The vector of tokens.
 * @return A vector of attributes.
 */
vector<pair<string, vector<string>>> LowMemoryTAGraph::generateStrAttributes(std::vector<std::string> line){
    if (line.size() < 3){
        return vector<pair<string, vector<string>>>();
    }

    //Start by expecting the { symbol.
    if (line.at(0).compare("{") == 0){
        line.erase(line.begin());
    } else if (line.at(0).find("{") == 0){
        line.at(0).erase(0, 1);
    } else {
        return vector<pair<string, vector<string>>>();
    }

    //Now, we iterate until we hit the end.
    int i = 0;
    bool end = false;
    string current = line.at(i);
    vector<pair<string, vector<string>>> attrList = vector<pair<string, vector<string>>>();
    do {
        //Adds in the first part of the entry.
        pair<string, vector<string>> currentEntry = pair<string, vector<string>>();
        currentEntry.first = current;

        //Checks for validity.
        if (i + 2 >= line.size() || line.at(++i).compare("=") != 0){
            return vector<pair<string, vector<string>>>();
        }

        //Gets the next KV pair.
        string next = line.at(++i);
        if (next.compare("(") == 0 || next.find("(") == 0) {
            //First, remove the ( symbol.
            if (next.compare("(") == 0){
                if (i + 1 == line.size()){
                    return vector<pair<string, vector<string>>>();
                }
                next = line.at(++i);
            } else if (next.find("(") == 0){
                next.erase(0, 1);
            }

            //We now iterate through the attribute list.
            bool endList = false;
            do {
                //Check if we hit the end.
                if (next.compare(")") == 0) {
                    break;
                } else if (next.find(")") == next.size() - 1) {
                    next.erase(next.size() - 1, 1);
                    endList = true;
                }

                //Next, process the item.
                currentEntry.second.push_back(next);

                //Finally check if we've hit the conditions.
                if (endList){
                    break;
                } else if (i + 1 == line.size()){
                    return vector<pair<string, vector<string>>>();
                } else {
                    next = line.at(++i);
                }
            } while (true);
        } else {
            //Check if we have a "} symbol at the end.
            if (next.find("}") == next.size() - 1){
                end = true;
                next.erase(next.size() - 1, 1);
            }

            //Add it to the current entry.
            currentEntry.second.push_back(next);
        }

        //Increments the current string.
        if (i + 1 == line.size() && end == false){
            return vector<pair<string, vector<string>>>();
        } else if (!end){
            current = line.at(++i);
        }

        //Adds the entry in.
        attrList.push_back(currentEntry);
    } while (current.compare("}") != 0 && end == false);

    return attrList;
}
