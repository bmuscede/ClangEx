/////////////////////////////////////////////////////////////////////////////////////////////////////////
// FileParse.cpp
//
// Created By: Bryan J Muscedere
// Date: 07/11/16.
//
// File parsing system that maintains information about encountered
// files while processing the source code. Creates nodes and edges once
// the compliation has completed.
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

#include <boost/filesystem.hpp>
#include "FileParse.h"
#include "../Walker/ASTWalker.h"

using namespace std;

/**
 * Constructor. Creates a default file parse object.
 */
FileParse::FileParse() {}

/**
 * Destructor. Destroys the file parse object.
 */
FileParse::~FileParse() {}

/**
 * Adds a singular file to the list.
 * @param path The path to add.
 */
void FileParse::addPath(string path) {
    //Check if path already exists.
    for (int i = 0; i < paths.size(); i++) {
        if (paths.at(i).compare(path) == 0) return;
    }

    //Add it to the vector if it doesn't.
    paths.push_back(path);
}

/**
 * Creates nodes and edges for every single path that was added to the list.
 * @param nodes The created nodes. (Should be empty on invokation).
 * @param edges The created edges. (Should be empty on invokation).
 */
void FileParse::processPaths(vector<ClangNode*>& nodes, vector<ClangEdge*>& edges) {
    //Iterate through all the paths.
    for (int i = 0; i < paths.size(); i++){
        processPath(paths.at(i), nodes, edges);
    }
}

/**
 * Processes an individual path in the path list. Creates the associated nodes
 * and edges.
 * @param path The path to create.
 * @param curPath The set of created nodes.
 * @param curContains The set of created edges.
 */
void FileParse::processPath(string path, vector<ClangNode*>& curPath, vector<ClangEdge*>& curContains) {
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
        int existsIndex = doesNodeExist(ASTWalker::generateMD5(pathComponents.at(i)), curPath);
        if (existsIndex == -1){
            //Determines the type of node.
            ClangNode::NodeType type;
            if (i + 1 == pathComponents.size()){
                type = ClangNode::FILE;
            } else {
                type = ClangNode::SUBSYSTEM;
            }

            //Creates the node.
            string current = ASTWalker::generateMD5(pathComponents.at(i));
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

/**
 * Checks if a node already exists in the associated list.
 * @param ID The ID to check for.
 * @param nodes The list of nodes to check in.
 * @return Index of where the node is in the list.
 */
int FileParse::doesNodeExist(string ID, const vector<ClangNode*>& nodes){
    //Iterates through and checks.
    for (int i = 0; i < nodes.size(); i++){
        ClangNode* curNode = nodes.at(i);
        if (curNode->getID().compare(ID) == 0) return i;
    }

    return -1;
}

/**
 * Checks if an edge already exists in the associated list.
 * @param src The source ID.
 * @param dst The destination ID.
 * @param edges The edge list to check in.
 * @return The index of where the node is in the list.
 */
bool FileParse::doesEdgeExist(ClangNode* src, ClangNode* dst, const vector<ClangEdge*>& edges){
    //Iterates through and checks.
    for (auto curEdge : edges){
        if (curEdge->getSrc() == src && curEdge->getDst() == dst) return true;
    }

    return false;
}
