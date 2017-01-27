//
// Created by bmuscede on 07/11/16.
//

#ifndef CLANGEX_FILEPARSE_H
#define CLANGEX_FILEPARSE_H

#include <string>
#include <vector>
#include "../Graph/ClangNode.h"
#include "../Graph/ClangEdge.h"

class FileParse {
public:
    FileParse();
    ~FileParse();

    void addPath(std::string path);
    void processPaths(std::vector<ClangNode*>& nodes, std::vector<ClangEdge*>& edges, bool md5);

private:
    std::vector<std::string> paths;

    std::vector<ClangNode*> processPath(std::string path, std::vector<ClangNode*>& curPath,
                                        std::vector<ClangEdge*>& curContains, bool md5);
};


#endif //CLANGEX_FILEPARSE_H
