//
// Created by bmuscede on 14/03/18.
//

#include <fstream>
#include <sys/stat.h>
#include <cstdio>
#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "LowMemoryTAGraph.h"

using namespace std;
using namespace boost::filesystem;

int LowMemoryTAGraph::currentNumber = 0;

LowMemoryTAGraph::LowMemoryTAGraph(string basePath, int curNum) : TAGraph() {
    fileNumber = curNum;

    instanceFN = weakly_canonical(path(basePath + "/" + to_string(fileNumber) + "-" + BASE_INSTANCE_FN)).string();
    relationFN = weakly_canonical(path(basePath + "/" + to_string(fileNumber) + "-" + BASE_RELATION_FN)).string();
    mvRelationFN = weakly_canonical(path(basePath + "/" + to_string(fileNumber) + "-" + BASE_MV_RELATION_FN)).string();
    attributeFN = weakly_canonical(path(basePath + "/" + to_string(fileNumber) + "-" + BASE_ATTRIBUTE_FN)).string();
}

LowMemoryTAGraph::LowMemoryTAGraph() : TAGraph() {
    fileNumber = LowMemoryTAGraph::currentNumber;
    LowMemoryTAGraph::currentNumber++;

    instanceFN = weakly_canonical(path(to_string(fileNumber) + "-" + BASE_INSTANCE_FN)).string();
    relationFN = weakly_canonical(path(to_string(fileNumber) + "-" + BASE_RELATION_FN)).string();
    mvRelationFN = weakly_canonical(path(to_string(fileNumber) + "-" + BASE_MV_RELATION_FN)).string();
    attributeFN = weakly_canonical(path(to_string(fileNumber) + "-" + BASE_ATTRIBUTE_FN)).string();

    if (doesFileExist(instanceFN)) deleteFile(instanceFN);
    if (doesFileExist(relationFN)) deleteFile(relationFN);
    if (doesFileExist(attributeFN)) deleteFile(attributeFN);
    ofstream{ instanceFN };
    ofstream{ relationFN };
    ofstream{ attributeFN };
}

LowMemoryTAGraph::~LowMemoryTAGraph() {
    if (doesFileExist(instanceFN)) deleteFile(instanceFN);
    if (doesFileExist(relationFN)) deleteFile(relationFN);
    if (doesFileExist(attributeFN)) deleteFile(attributeFN);
}

bool LowMemoryTAGraph::addNode(ClangNode* node, bool assumeValid){
    //Check the number of entities.
    int amt = getNumberEntities();
    if (amt > PURGE_AMOUNT){
        purgeCurrentGraph();
    }

    //Add the graph.
    return TAGraph::addNode(node, assumeValid);
}

bool LowMemoryTAGraph::addEdge(ClangEdge* edge, bool assumeValid){
    //Check the number of entities.
    int amt = getNumberEntities();
    if (amt > PURGE_AMOUNT){
        purgeCurrentGraph();
    }

    //Add the graph.
    return TAGraph::addEdge(edge, assumeValid);
}

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
    path org = relationFN;
    path dst = mvRelationFN;
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
            continue; //TODO!
            //Relation attribute.
            string relID = entry.at(0) + " " + entry.at(1) + " " + entry.at(2);

            //Check if the relationID matches.
            if (find(removedRels.begin(), removedRels.end(), relID) != removedRels.end()){
                continue;
            }

            //Generates the attribute list.
            auto attrs = generateStrAttributes(entry);

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
        destAttr << attrLine + "\n";
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

bool LowMemoryTAGraph::doesFileExist(string fN){
    struct stat buffer;
    return (stat (fN.c_str(), &buffer) == 0);
}

void LowMemoryTAGraph::deleteFile(string fN) {
    remove(fN.c_str());
}

void LowMemoryTAGraph::purgeCurrentGraph(){
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

int LowMemoryTAGraph::getNumberEntities(){
    //Get the number of keys in the graph.
    auto curAmnt = (int) nodeList.size();
    return curAmnt;
}

vector<string> LowMemoryTAGraph::tokenize(string curString){
    vector<string> tokens;

    boost::split(tokens, curString, boost::is_any_of(" "), boost::token_compress_on);
    return tokens;
}

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