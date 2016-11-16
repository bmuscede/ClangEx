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

    //If we don't find it, return NULL.
    return NULL;
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

ClangEdge* TAGraph::findEdgeByIDs(string IDOne, string IDTwo) {
    //Iterate through the edge list.
    for (int i = 0; i < edgeList.size(); i++){
        //Get the nodes.
        ClangNode* src = edgeList.at(i)->getSrc();
        ClangNode* dst = edgeList.at(i)->getDst();

        //Check if we found it.
        if (IDOne.compare(src->getID()) == 0 &&
            IDTwo.compare(dst->getID()) == 0){
            return edgeList.at(i);
        }
    }

    return NULL;
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
    if (node == NULL) return false;

    //Clears the vector and adds the attribute.
    node->clearAttributes(key);
    return node->addAttribute(key, value);
}

bool TAGraph::addSinuglarAttribute(string IDSrc, string IDDst, string key, string value){
    //Get the ClangEdge.
    ClangEdge* edge = findEdgeByIDs(IDSrc, IDDst);
    if (edge == NULL) return false;

    //Clears the vector and adds the attribute.
    edge->clearAttribute(key);
    edge->addAttribute(key, value);
    return true;
}

bool TAGraph::addAttribute(string ID, string key, string value){
    //Get the node.
    ClangNode* node = findNodeByID(ID);
    if (node == NULL) return false;

    //Check if the attribute exists.
    if (node->doesAttributeExist(key, value)) return true;

    //Add the node attribute.
    return node->addAttribute(key, value);
}

bool TAGraph::addAttribute(string IDSrc, string IDDst, string key, string value){
    //Get the edge.
    ClangEdge* edge = findEdgeByIDs(IDSrc, IDDst);
    if (edge == NULL) return false;

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
    format += generateInstances() + "\n";
    format += generateRelationships() + "\n";
    format += generateAttributes() + "\n";

    return format;
}

void TAGraph::addNodesToFile() {
    //Iterate through all our nodes and find the appropriate file.
    for (ClangNode* node : nodeList){
        vector<string> fileAttrVec = node->getAttribute(FILE_ATTRIBUTE);
        if (fileAttrVec.size() != 1) continue;

        string file = fileAttrVec.at(0);
        if (file.compare("") != 0){
            //Find the appropriate node.
            ClangNode* fileNode = findNodeByID(file);
            if (fileNode == NULL) continue;

            //Add it to the graph.
            ClangEdge* edge = new ClangEdge(fileNode, node, ClangEdge::CONTAINS);
            addEdge(edge);
        }
    }
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