/////////////////////////////////////////////////////////////////////////////////////////////////////////
// FileParse.h
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

#ifndef CLANGEX_FILEPARSE_H
#define CLANGEX_FILEPARSE_H

#include <string>
#include <vector>
#include "../Graph/ClangNode.h"
#include "../Graph/ClangEdge.h"

class FileParse {
public:
    /** Constructor and Destructor */
    FileParse();
    ~FileParse();

    /** Path Creation Operations */
    void addPath(std::string path);
    void processPaths(std::vector<ClangNode*>& nodes, std::vector<ClangEdge*>& edges);

private:
    /** Member Variables */
    std::vector<std::string> paths;

    /** Helper Methods */
    void processPath(std::string path, std::vector<ClangNode*>& curPath,
                     std::vector<ClangEdge*>& curContains);

    /** Node Search Operations */
    int doesNodeExist(std::string ID, const std::vector<ClangNode*>& nodes);
    bool doesEdgeExist(ClangNode* src, ClangNode* dst, const std::vector<ClangEdge*>& edges);
};


#endif //CLANGEX_FILEPARSE_H
