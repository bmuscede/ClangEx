/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TAGraph.cpp
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

#include <ctime>
#include "TAGraph.h"
#include "../Walker/ASTWalker.h"

using namespace std;

/** File Attribute */
const string TAGraph::FILE_ATTRIBUTE = "filename";

/**
 * Constructor. Creates all the member variables.
 * @param print The printer type to be used.
 */
TAGraph::TAGraph(Printer* print) : clangPrinter(print) {
    nodeList = unordered_map<string, ClangNode*>();
    nodeNameList = unordered_map<string, vector<string>>();
    edgeSrcList = unordered_map<string, vector<ClangEdge*>>();
    edgeDstList = unordered_map<string, vector<ClangEdge*>>();
}

/**
 * Destructor. Deletes all nodes and edges in the graph.
 */
TAGraph::~TAGraph() {
    //Iterate through node list to delete.
    for (auto it = nodeList.begin(); it != nodeList.end(); it++)
        delete it->second;

    //Next, iterate through the edge list.
    for (auto it = edgeSrcList.begin(); it != edgeSrcList.end(); it++)
        for (ClangEdge* edge : it->second)
            delete edge;
}

/**
 * Sets the current printer.
 * @param newPrint The new printer to use.
 */
void TAGraph::setPrinter(Printer* newPrint){
    delete clangPrinter;
    clangPrinter = newPrint;
}

/**
 * Adds a node to the TA graph.
 * @param node The node to add.
 * @param assumeValid Flag that assumes the node is already valid.
 * @return Whether the node was added or not.
 */
bool TAGraph::addNode(ClangNode *node, bool assumeValid) {
    //Check if the node ID exists.
    if (!assumeValid && nodeExists(node->getID())){
        return false;
    }

    //Now, we simply add to the node list.
    nodeList[node->getID()] = node;
    nodeNameList[node->getName()].push_back(node->getID());
    return true;
}

/**
 * Adds an edge to the TA graph.
 * @param edge The edge to add.
 * @param assumeValid Flag that assumes the edge is already valid.
 * @return Whether the edge was added or not.
 */
bool TAGraph::addEdge(ClangEdge *edge, bool assumeValid) {
    //Check if the edge already exists.
    if (!assumeValid && edgeExists(edge->getSrc()->getID(), edge->getDst()->getID(), edge->getType())){
        return false;
    } else if (edge->getSrc()->getID().compare(edge->getDst()->getID()) == 0 && edge->getType() == ClangEdge::EdgeType::CONTAINS){
        return false;
    }

    //Now, check if we already have a contains edge for the source node.
    if (edge->getType() == ClangEdge::EdgeType::CONTAINS) {
        vector<ClangEdge *> edges = edgeDstList[edge->getDst()->getID()];
        for (ClangEdge *curEdge : edges) {
            if (curEdge->getType() == ClangEdge::EdgeType::CONTAINS) {
                removeEdge(curEdge);
            }
        }
    }

    //Now, we add the edge.
    edgeSrcList[edge->getSrc()->getID()].push_back(edge);
    edgeDstList[edge->getDst()->getID()].push_back(edge);
    return true;
}

/**
 * Removes a node from the graph.
 * @param node The node to remove.
 * @param unsafe Whether we remove the node from the graph yet keep edges that reference it.
 */
void TAGraph::removeNode(ClangNode *node, bool unsafe) {
    //First, goes through and deletes the node from the map.
    nodeList[node->getID()] = nullptr;

    //Gets the vector with the name for the node.
    vector<string> nodeString = nodeNameList[node->getName()];
    for (int i = 0; i < nodeString.size(); i++){
        if (nodeString.at(i).compare(node->getID())) {
            nodeString.erase(nodeString.begin() + i);
            break;
        }
    }
    nodeNameList[node->getName()] = nodeString;

    //Checks if we've got unsafe deletion.
    if (!unsafe){
        //Gets a list of all edges that pertain.
        vector<ClangEdge*> edges = findEdgesBySrcID(node);
        for (ClangEdge* edge : edges) {
            removeEdge(edge);
        }

    }

    delete node;
}

/**
 * Removes an edge from the graph.
 * @param edge The edge to remove
 */
void TAGraph::removeEdge(ClangEdge* edge){
    //We need to delete this edge from both arrays.
    for (int i = 0; i < edgeSrcList[edge->getSrc()->getID()].size(); i++){
        ClangEdge* ex = edgeSrcList[edge->getSrc()->getID()].at(i);
        if (ex->getSrc()->getID().compare(edge->getSrc()->getID()) == 0 && ex->getDst()->getID().compare(edge->getDst()->getID()) == 0 &&
            ex->getType() == edge->getType()) {
            edgeSrcList[edge->getSrc()->getID()].erase(edgeSrcList[edge->getSrc()->getID()].begin() + i);
        }
    }
    for (int i = 0; i < edgeDstList[edge->getDst()->getID()].size(); i++){
        ClangEdge* ex = edgeDstList[edge->getDst()->getID()].at(i);
        if (ex->getSrc()->getID().compare(edge->getSrc()->getID()) == 0 && ex->getDst()->getID().compare(edge->getDst()->getID()) == 0 &&
            ex->getType() == edge->getType()) {
            edgeDstList[edge->getDst()->getID()].erase(edgeDstList[edge->getDst()->getID()].begin() + i);
        }
    }
    delete edge;

}

/**
 * Adds an attribute to a node in the graph.
 * @param ID The ID of the node.
 * @param key The key of the attribute.
 * @param value The value of the attribute.
 * @return Whether the value was added successfully.
 */
bool TAGraph::addAttribute(string ID, string key, string value){
    //Get the node.
    ClangNode* node = findNodeByID(ID);
    if (node == nullptr) return false;

    //Check if the attribute exists.
    if (node->doesAttributeExist(key, value)) return true;

    //Add the node attribute.
    return node->addAttribute(key, value);
}

/**
 * Adds an attribute to an edge in the graph.
 * @param IDSrc The source ID.
 * @param IDDst The destination ID.
 * @param type The type of edge to add.
 * @param key The key of the attribute.
 * @param value The edge of the attribute.
 * @return Whether the value was added successfully.
 */
bool TAGraph::addAttribute(string IDSrc, string IDDst, ClangEdge::EdgeType type, string key, string value){
    //Get the edge.
    ClangEdge* edge = findEdgeByIDs(IDSrc, IDDst, type);
    if (edge == nullptr) return false;

    //Check if the attribute exists.
    if (edge->doesAttributeExist(key, value)) return true;

    //Add the attribute.
    edge->addAttribute(key, value);
    return true;
}

/**
 * Returns a list of all nodes in the graph.
 * @return All nodes in the graph.
 */
vector<ClangNode*> TAGraph::getNodes(){
    vector<ClangNode*> nodes;

    //Copies the items in the map to the vector.
    for (auto it = nodeList.begin(); it != nodeList.end(); it++)
        nodes.push_back(it->second);

    return nodes;
}

/**
 * Returns a list of all edges in the graph.
 * @return All edges in the graph.
 */
vector<ClangEdge*> TAGraph::getEdges(){
    vector<ClangEdge*> edges;

    //Copies the item in the map over to the vector.
    for (auto it = edgeSrcList.begin(); it != edgeSrcList.end(); it++)
        for (ClangEdge* curEdge : it->second)
            edges.push_back(curEdge);

    return edges;
}

/**
 * Finds a node by a given ID.
 * @param ID The ID of the node.
 * @return The node that was found.
 */
ClangNode* TAGraph::findNodeByID(string ID) {
    //We iterate through until we find the node.
    return nodeList[ID];
}

/**
 * Finds a node by a specific name.
 * @param name The name of the node.
 * @return Vector of all nodes with that given name.
 */
vector<ClangNode*> TAGraph::findNodeByName(string name) {
    vector<ClangNode*> nodes;

    //Searches for the node.
    vector<string> iDRep = nodeNameList[name];
    for (string curr : iDRep){
        nodes.push_back(findNodeByID(curr));
    }

    return nodes;
}

/**
 * Finds an edge by a set of given IDs.
 * @param IDOne The ID of the source node.
 * @param IDTwo The ID of the destination node.
 * @param type The type of edge.
 * @return The edge that was found.
 */
ClangEdge* TAGraph::findEdgeByIDs(string IDOne, string IDTwo, ClangEdge::EdgeType type) {
    //First, find the entry.
    vector<ClangEdge*> edges = edgeSrcList[IDOne];
    if (edges.size() == 0) return nullptr;

    //Next, iterate through to find the edge.
    for (ClangEdge* edge : edges){
        if (edge->getDst()->getID().compare(IDTwo) == 0 && edge->getType() == type) return edge;
    }

    return nullptr;
}

/**
 * Gets all edges based on a destination node.
 * @param dst The destination node to look for.
 * @param type The type of node.
 * @return A set of all nodes that participate with that destination.
 */
vector<ClangNode*> TAGraph::findSrcNodesByEdge(ClangNode* dst, ClangEdge::EdgeType type){
    vector<ClangNode*> srcNodes;

    vector<ClangEdge*> edges = edgeDstList[dst->getID()];
    for (ClangEdge* curEdge : edges){
        if (curEdge->getType() == type) srcNodes.push_back(curEdge->getSrc());
    }

    return srcNodes;
}

/**
 * Gets all edges based on a source node.
 * @param dst The source node to look for.
 * @param type The type of node.
 * @return A set of all nodes that particpate with that source.
 */
vector<ClangNode*> TAGraph::findDstNodesByEdge(ClangNode* src, ClangEdge::EdgeType type){
    vector<ClangNode*> dstNodes;

    vector<ClangEdge*> edges = edgeSrcList[src->getID()];
    for (ClangEdge* curEdge : edges){
        if (curEdge->getType() == type) dstNodes.push_back(curEdge->getSrc());
    }

    return dstNodes;
}

/**
 * Finds all edges based on some some source node.
 * @param src The source node to find.
 * @return A set of all edges.
 */
vector<ClangEdge*> TAGraph::findEdgesBySrcID(ClangNode* src){
    return edgeSrcList[src->getID()];
}

/**
 * Finds all edges based on some some destination node.
 * @param src The destination node to find.
 * @return A set of all edges.
 */
vector<ClangEdge*> TAGraph::findEdgesByDstID(ClangNode* dst){
    return edgeDstList[dst->getID()];
}

/**
 * Checks whether a node exists.
 * @param ID The ID of the node.
 * @return Whether it exists or not.
 */
bool TAGraph::nodeExists(string ID) {
    if (nodeList[ID] == nullptr) return false;
    return true;
}

/**
 * Checks whether an edge exists.
 * @param IDOne The ID of the source node.
 * @param IDTwo The ID of the destination node.
 * @param type The type of edge.
 * @return Whether the edge exists or not.
 */
bool TAGraph::edgeExists(string IDOne, string IDTwo, ClangEdge::EdgeType type) {
    //First check if the entry for the source exists.
    vector<ClangEdge*> edges = edgeSrcList[IDOne];
    if (edges.size() == 0) return false;

    //Iterate through the list.
    for (ClangEdge* edge : edges){
        if (edge->getDst()->getID().compare(IDTwo) == 0 && edge->getType() == type) return true;
    }

    return false;
}

/**
 * Generates a string representation of the graph using the Tuple-Attribute format.
 * @return The string of the TA representation.
 */
string TAGraph::generateTAFormat() {
    string format = "";

    //Get the time.
    char tString[1000];
    time_t now = time(0);
    struct tm * p = localtime(&now);
    strftime(tString, 1000, "%A, %B %d %Y", p);

    //Generate the instances, relationships, and attributes.
    format += TA_HEADER + " (" + tString + ")" + "\n";
    format += TA_SCHEMA;
    format += generateInstances();
    format += generateRelationships() + "\n";
    format += generateAttributes() + "\n";

    return format;
}

/**
 * Adds nodes in the graph to a file node.
 * @param fileSkip Whether we're going to skip a certain component.
 */
void TAGraph::addNodesToFile(map<string, ClangNode*> fileSkip) {
    //Iterate through all our nodes and find the appropriate file.
    for (auto it = nodeList.begin(); it != nodeList.end(); it++) {
        ClangNode* node = it->second;
        if (!node) continue;

        vector<string> fileAttrVec = node->getAttribute(FILE_ATTRIBUTE);
        if (fileAttrVec.size() != 1) continue;

        for (string file : fileAttrVec) {
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
                ClangEdge *edge = new ClangEdge(fileNode, node, ClangEdge::FILE_CONTAIN);
                addEdge(edge);
            }
        }
    }
}

/**
 * Adds a reference edge between two components. Used when one node doesn't yet exist.
 * @param callerID The source ID.
 * @param calleeID The destination ID.
 * @param type The type of edge to add.
 */
void TAGraph::addUnresolvedRef(string callerID, string calleeID, ClangEdge::EdgeType type) {
    //Creates the entry for the vector.
    pair<pair<string, string>, ClangEdge::EdgeType> entry = pair<pair<string, string>, ClangEdge::EdgeType>();
    entry.first.first = callerID;
    entry.first.second = calleeID;
    entry.second = type;

    //Finally, adds it to the unresolved vector.
    unresolvedRef.push_back(entry);
}

/**
 * Adds an attribute for some undefined edge.
 * @param callerID The source ID.
 * @param calleeID The destination ID.
 * @param attrName The attribute key.
 * @param attrValue The attribute value.
 */
void TAGraph::addUnresolvedRefAttr(string callerID, string calleeID, string attrName, string attrValue) {
    //Find if we already have a attr entry for this list.
    for (auto current : unresolvedRefAttr){
        if (current->first.first.compare(callerID) == 0 &&
            current->first.second.compare(calleeID) == 0 &&
            current->second.first.compare(attrName) == 0){

            //We already have an entry.
            current->second.second.push_back(attrValue);
            return;
        }
    }

    //Creates the entry for the vector.
    pair<pair<string, string>, pair<string, vector<string>>> *entry = new pair<pair<string, string>, pair<string, vector<string>>>();

    entry->first.first = callerID;
    entry->first.second = calleeID;
    entry->second.first = attrName;
    entry->second.second.push_back(attrValue);

    //Adds it to the vector.
    unresolvedRefAttr.push_back(entry);
}

/**
 * Goes through the undefined edges and tries to resolve them. If not, deletes the reference.
 * @param silent Whether we output the results or not.
 */
void TAGraph::resolveExternalReferences(bool silent) {
    int resolved = 0;
    int unresolved = 0;

    //Iterate through and resolve.
    for (int i = 0; i < unresolvedRef.size(); i++){
        pair<pair<string, string>, ClangEdge::EdgeType> entry = unresolvedRef.at(i);

        //Find the two items.
        vector<ClangNode*> srcVec = findNodeByName(entry.first.first);
        vector<ClangNode*> dstVec = findNodeByName(entry.first.second);

        //See if they could be resolved.
        if (srcVec.size() == 0 || dstVec.size() == 0){
            unresolved++;
        } else {
            ClangNode* src = srcVec.at(0);
            ClangNode* dst = dstVec.at(0);

            ClangEdge* edge = new ClangEdge(src, dst, entry.second);
            addEdge(edge);

            //Now, find all associated attributes.
            vector<pair<string, vector<string>>> attributes = findAttributes(entry.first.first, entry.first.second);
            for (auto attribute : attributes){
                for (auto attVal : attribute.second){
                    addAttribute(src->getID(), dst->getID(), entry.second, attribute.first, attVal);
                }
            }

            resolved++;
        }
    }

    //Afterwards, notify of success.
    if (!silent){
        clangPrinter->printResolveRefDone(resolved, unresolved);
    }
}

/**
 * Generates the set of nodes for the TA file.
 * @return A string containing the list of instances.
 */
string TAGraph::generateInstances() {
    string instances = "FACT TUPLE : \n";

    //Iterate through our node list to generate.
    for (auto it = nodeList.begin(); it != nodeList.end(); it++){
        if (!it->second) continue;

        instances += it->second->generateInstance() + "\n";
    }

    return instances;
}

/**
 * Generates a set of edges for the TA file.
 * @return A string containing the list of relationships.
 */
string TAGraph::generateRelationships() {
    string relationships = "";

    //Iterate through our edge list to generate.
    for (auto it = edgeSrcList.begin(); it != edgeSrcList.end(); it++){
        for (ClangEdge* edge : it->second)
            relationships += edge->generateRelationship() + "\n";
    }

    return relationships;
}

/**
 * Generates a set of attributes for the TA file.
 * @return A string containing the list of attributes.
 */
string TAGraph::generateAttributes() {
    string attributes = "FACT ATTRIBUTE : \n";

    //Iterate through our node list again to generate.
    for (auto it = nodeList.begin(); it != nodeList.end(); it++){
        if (!it->second) continue;

        string attribute = it->second->generateAttribute();
        if (attribute.compare("") == 0) continue;
        attributes += attribute + "\n";
    }

    //Next, iterate through our edge list.
    for (auto it = edgeSrcList.begin(); it != edgeSrcList.end(); it++){
        for (ClangEdge* edge : it->second) {
            string attribute = edge->generateAttribute();
            if (attribute.compare("") == 0) continue;
            attributes += attribute + "\n";
        }
    }

    return attributes;
}

/**
 * Finds all attributes for an edge based on some caller ID and destination ID. This is for undefined edges.
 * @param callerID The source ID.
 * @param calleeID The destination ID.
 * @return A collection of attributes for that edges.
 */
vector<pair<string, vector<string>>> TAGraph::findAttributes(string callerID, string calleeID){
    vector<pair<string, vector<string>>> values;

    //Find all associated values.
    for (auto entry : unresolvedRefAttr){
        if (entry->first.first.compare(callerID) == 0 && entry->first.second.compare(calleeID) == 0){
            //Adds the entry.
            pair<string, vector<string>> value;
            value.first = entry->second.first;
            value.second = entry->second.second;

            values.push_back(value);
        }
    }

    return values;
}
