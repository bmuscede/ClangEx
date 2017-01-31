//
// Created by bmuscede on 07/11/16.
//

#include <boost/filesystem.hpp>
#include "FileParse.h"
#include "../Walker/ASTWalker.h"

using namespace std;

FileParse::FileParse() {}

FileParse::~FileParse() {}

void FileParse::addPath(string path) {
    //Check if path already exists.
    for (int i = 0; i < paths.size(); i++) {
        if (paths.at(i).compare(path) == 0) return;
    }

    //Add it to the vector if it doesn't.
    paths.push_back(path);
}

void FileParse::processPaths(vector<ClangNode*>& nodes, vector<ClangEdge*>& edges, bool md5) {
    //Iterate through all the paths.
    for (int i = 0; i < paths.size(); i++){
        processPath(paths.at(i), nodes, edges, md5);
    }
}

void FileParse::processPath(string path, vector<ClangNode*>& curPath, vector<ClangEdge*>& curContains,
                                          bool md5) {
    //Start by iterating at each path element.
    vector<string> pathComponents = vector<string>();
    vector<string> pathLabels = vector<string>();
    int i = 0;
    for (auto& curr : boost::filesystem::path(path)){
        if (i == 0) {
            pathComponents.push_back(curr.string());
        } else {
            string craft = (i == 1) ? "" : "/";
            pathComponents.push_back(pathComponents.at(i - 1) + craft + curr.string());
        }

        pathLabels.push_back(curr.string());
        i++;
    }

    //Next, we iterate until we hit the end.
    ClangNode* prevNode = nullptr;
    for (int i = 0; i < pathComponents.size(); i++){
        ClangNode* currentNode;

        //Check if a path component exists.
        int existsIndex = doesNodeExist((md5) ? ASTWalker::generateMD5(pathComponents.at(i)) : pathComponents.at(i)
                , curPath);
        if (existsIndex == -1){
            //Determines the type of node.
            ClangNode::NodeType type;
            if (i + 1 == pathComponents.size()){
                type = ClangNode::FILE;
            } else {
                type = ClangNode::SUBSYSTEM;
            }

            //Creates the node.
            string current = (md5) ? ASTWalker::generateMD5(pathComponents.at(i)) : pathComponents.at(i);
            currentNode = new ClangNode(current, pathLabels.at(i), type);
            curPath.push_back(currentNode);
        } else {
            currentNode = curPath.at(existsIndex);
        }

        //Next, deals with contains.
        if (prevNode != nullptr && !doesEdgeExist(prevNode, currentNode, curContains)){
            //Adds a new link.
            curContains.push_back(new ClangEdge(prevNode, currentNode, ClangEdge::CONTAINS));
        }

        prevNode = currentNode;
    }
}

int FileParse::doesNodeExist(string ID, const vector<ClangNode*>& nodes){
    //Iterates through and checks.
    for (int i = 0; i < nodes.size(); i++){
        ClangNode* curNode = nodes.at(i);
        if (curNode->getID().compare(ID) == 0) return i;
    }

    return -1;
}

bool FileParse::doesEdgeExist(ClangNode* src, ClangNode* dst, const vector<ClangEdge*>& edges){
    //Iterates through and checks.
    for (auto curEdge : edges){
        if (curEdge->getSrc() == src && curEdge->getDst() == dst) return true;
    }

    return false;
}
