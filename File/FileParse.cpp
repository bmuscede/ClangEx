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
        vector<ClangNode*> curr = processPath(paths.at(i), nodes, edges, md5);
        nodes.insert(nodes.end(), curr.begin(), curr.end());
    }
}

vector<ClangNode*> FileParse::processPath(string path, vector<ClangNode*>& curPath, vector<ClangEdge*>& curContains,
                                          bool md5) {
    //Start by iterating at each path element.
    vector<string> pathComponents = vector<string>();
    for (auto& curr : boost::filesystem::path(path)){
        pathComponents.push_back(curr.string());
    }

    //Next, we iterate until we hit the end.
    ClangNode* prevNode = nullptr;
    for (int i = 0; i < pathComponents.size(); i++){
        //Determines the type of node.
        ClangNode::NodeType type;
        if (i + 1 == pathComponents.size()){
            type = ClangNode::FILE;
        } else {
            type = ClangNode::SUBSYSTEM;
        }

        //Creates the node.
        string current = (md5) ? ASTWalker::generateMD5(pathComponents.at(i)) : pathComponents.at(i);
        ClangNode* currentNode = new ClangNode(current, current, type);
        curPath.push_back(currentNode);

        //Next, deals with contains.
        if (prevNode != nullptr){
            //Adds a new link.
            curContains.push_back(new ClangEdge(prevNode, currentNode, ClangEdge::CONTAINS));
        }

        prevNode = currentNode;
    }

    return curPath;
}