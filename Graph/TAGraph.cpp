//
// Created by bmuscede on 05/11/16.
//

#include "TAGraph.h"

using namespace std;

const string TAGraph::FILE_ATTRIBUTE = "filename";

TAGraph::TAGraph() {
    nodeList = vector<ClangNode*>();
    edgeList = vector<ClangEdge*>();
}

TAGraph::~TAGraph() {
    //Iterate through node list to delete.
    for (int i = 0; i < nodeList.size(); i++)
        delete nodeList.at(i);

    //Next, iterate through the edge list.
    for (int i = 0; i < edgeList.size(); i++)
        delete edgeList.at(i);
}

bool TAGraph::addNode(ClangNode *node) {
    //Check if the node ID exists.
    if (nodeExists(node->getID())){
        return false;
    }

    //Now, we simply add to the node list.
    nodeList.push_back(node);
    return true;
}

bool TAGraph::addEdge(ClangEdge *edge) {
    //Check if the edge already exists.
    if (edgeExists(edge->getSrc()->getID(), edge->getDst()->getID())){
        return false;
    }

    //Now, we add the edge.
    edgeList.push_back(edge);
    return true;
}

ClangNode* TAGraph::findNodeByID(string ID) {
    //We iterate through until we find the node.
    for (int i = 0; i < nodeList.size(); i++){
        if (ID.compare(nodeList.at(i)->getID()) == 0){
            return nodeList.at(i);
        }
    }

    //If we don't find it, return nullptr.
    return nullptr;
}

vector<ClangNode*> TAGraph::findNodeByName(string name) {
    vector<ClangNode*> nodes = vector<ClangNode*>();

    //Iterate through and add ALL relevant nodes.
    for (int i = 0; i < nodeList.size(); i++){
        if (name.compare(nodeList.at(i)->getName()) == 0){
            nodes.push_back(nodeList.at(i));
        }
    }

    return nodes;
}

ClangEdge* TAGraph::findEdgeByIDs(string IDOne, string IDTwo, ClangEdge::EdgeType type) {
    //Iterate through the edge list.
    for (int i = 0; i < edgeList.size(); i++){
        //Get the nodes.
        ClangNode* src = edgeList.at(i)->getSrc();
        ClangNode* dst = edgeList.at(i)->getDst();
        ClangEdge::EdgeType curType = edgeList.at(i)->getType();

        //Check if we found it.
        if (IDOne.compare(src->getID()) == 0 &&
            IDTwo.compare(dst->getID()) == 0 &&
            curType == type){
            return edgeList.at(i);
        }
    }

    return nullptr;
}

vector<ClangNode*> TAGraph::findSrcNodesByEdge(ClangNode* dst, ClangEdge::EdgeType type){
    vector<ClangNode*> srcNodes;

    //Iterate through our edge list.
    for (auto edge : edgeList){
        if (edge->getType() == type && edge->getDst() == dst)
            srcNodes.push_back(edge->getSrc());
    }

    return srcNodes;
}

vector<ClangNode*> TAGraph::findDstNodesByEdge(ClangNode* src, ClangEdge::EdgeType type){
    vector<ClangNode*> dstNodes;

    //Iterate through our edge list.
    for (auto edge : edgeList){
        if (edge->getType() == type && edge->getSrc() == src)
            dstNodes.push_back(edge->getDst());
    }

    return dstNodes;
}

bool TAGraph::nodeExists(string ID) {
    //Iterate through the node list.
    for (int i = 0; i < nodeList.size(); i++){
        if (ID.compare(nodeList.at(i)->getID()) == 0){
            return true;
        }
    }

    return false;
}

bool TAGraph::edgeExists(string IDOne, string IDTwo) {
    //Iterate through the edge list.
    for (int i = 0; i < edgeList.size(); i++){
        //Get the nodes.
        ClangNode* src = edgeList.at(i)->getSrc();
        ClangNode* dst = edgeList.at(i)->getDst();

        //Check if we found it.
        if (IDOne.compare(src->getID()) == 0 &&
                IDTwo.compare(dst->getID()) == 0){
            return true;
        }
    }

    return false;
}

bool TAGraph::addSingularAttribute(string ID, string key, string value){
    //Get the ClangNode.
    ClangNode* node = findNodeByID(ID);
    if (node == nullptr) return false;

    //Clears the vector and adds the attribute.
    node->clearAttributes(key);
    return node->addAttribute(key, value);
}

bool TAGraph::addSingularAttribute(string IDSrc, string IDDst, ClangEdge::EdgeType type, string key, string value){
    //Get the ClangEdge.
    ClangEdge* edge = findEdgeByIDs(IDSrc, IDDst, type);
    if (edge == nullptr) return false;

    //Clears the vector and adds the attribute.
    edge->clearAttribute(key);
    edge->addAttribute(key, value);
    return true;
}

bool TAGraph::addAttribute(string ID, string key, string value){
    //Get the node.
    ClangNode* node = findNodeByID(ID);
    if (node == nullptr) return false;

    //Check if the attribute exists.
    if (node->doesAttributeExist(key, value)) return true;

    //Add the node attribute.
    return node->addAttribute(key, value);
}

//TODO: FIX THIS!
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

string TAGraph::generateTAFormat() {
    string format = "";

    //Generate the instances, relationships, and attributes.
    format += TA_SCHEMA;
    format += generateInstances();
    format += generateRelationships() + "\n";
    format += generateAttributes() + "\n";

    return format;
}

void TAGraph::addNodesToFile() {
    //Iterate through all our nodes and find the appropriate file.
    for (ClangNode* node : nodeList){
        vector<string> fileAttrVec = node->getAttribute(FILE_ATTRIBUTE);
        if (fileAttrVec.size() != 1) continue;

        //First, get all the current edges for this attribute.
        if (isPartOfClass(node)) continue;

        string file = fileAttrVec.at(0);
        if (file.compare("") != 0){
            //Find the appropriate node.
            ClangNode* fileNode = findNodeByID(file);
            if (fileNode == nullptr) continue;

            //Add it to the graph.
            ClangEdge* edge = new ClangEdge(fileNode, node, ClangEdge::CONTAINS);
            addEdge(edge);
        }
    }
}

bool TAGraph::isPartOfClass(ClangNode* node){
    //First, find all source edges with contains.
    vector<ClangNode*> srcNodes = findSrcNodesByEdge(node, ClangEdge::CONTAINS);

    //Look for a class node in the src list.
    for (auto srcNode : srcNodes)
        if (srcNode->getType() == ClangNode::CLASS) return true;

    return false;
}

void TAGraph::addUnresolvedRef(string callerID, string calleeID, ClangEdge::EdgeType type) {
    //Creates the entry for the vector.
    pair<pair<string, string>, ClangEdge::EdgeType> entry = pair<pair<string, string>, ClangEdge::EdgeType>();
    entry.first.first = callerID;
    entry.first.second = calleeID;
    entry.second = type;

    //Finally, adds it to the unresolved vector.
    unresolvedRef.push_back(entry);
}

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

void TAGraph::resolveExternalReferences(bool silent) {
    int resolved = 0;
    int unresolved = 0;

    //Iterate through and resolve.
    for (int i = 0; i < unresolvedRef.size(); i++){
        pair<pair<string, string>, ClangEdge::EdgeType> entry = unresolvedRef.at(i);

        //Find the two items.
        vector<ClangNode*> srcs = findNodeByName(entry.first.first);
        vector<ClangNode*> dsts = findNodeByName(entry.first.second);

        //See if they could be resolved.
        if (srcs.size() == 0 || dsts.size() == 0){
            unresolved++;
        } else {
            ClangEdge* edge = new ClangEdge(srcs.at(0), dsts.at(0), entry.second);
            addEdge(edge);

            //Now, find all associated attributes.
            vector<pair<string, vector<string>>> attributes = findAttributes(entry.first.first, entry.first.second);
            for (auto attribute : attributes){
                for (auto attVal : attribute.second){
                    addAttribute(srcs.at(0)->getID(), dsts.at(0)->getID(), entry.second, attribute.first, attVal);
                }
            }

            resolved++;
        }
    }

    //Afterwards, notify of success.
    if (!silent){
        cout << "Overall, " << resolved << " references were resolved and " << unresolved
             << " references could not be resolved." << endl;
    }
}

vector<ClangNode*> TAGraph::getNodes(){
    return nodeList;
}

vector<ClangEdge*> TAGraph::getEdges(){
    return edgeList;
}

string TAGraph::generateInstances() {
    string instances = "FACT TUPLE : \n";

    //Iterate through our node list to generate.
    for (int i = 0; i < nodeList.size(); i++){
        instances += nodeList.at(i)->generateInstance() + "\n";
    }

    return instances;
}

string TAGraph::generateRelationships() {
    string relationships = "";

    //Iterate through our edge list to generate.
    for (int i = 0; i < edgeList.size(); i++){
        relationships += edgeList.at(i)->generateRelationship() + "\n";
    }

    return relationships;
}

string TAGraph::generateAttributes() {
    string attributes = "FACT ATTRIBUTE : \n";

    //Iterate through our node list again to generate.
    for (int i = 0; i < nodeList.size(); i++){
        string attribute = nodeList.at(i)->generateAttribute();
        if (attribute.compare("") == 0) continue;
        attributes += attribute + "\n";
    }

    //Next, iterate through our edge list.
    for (int i = 0; i < edgeList.size(); i++){
        string attribute = edgeList.at(i)->generateAttribute();
        if (attribute.compare("") == 0) continue;
        attributes += attribute + "\n";
    }

    return attributes;
}