//
// Created by bmuscede on 05/11/16.
//

#ifndef CLANGEX_CLANGNODE_H
#define CLANGEX_CLANGNODE_H

#include <string>
#include <map>
#include <vector>
#include <boost/filesystem/path.hpp>

class ClangNode {
private:
    /** ATTRIBUTE STRUCTS */
    typedef struct {
        const std::string attrName = "filename";

        std::string processFileName(std::string path){
            boost::filesystem::path p(path);
            return p.filename().string();
        }
    } AttributeStruct;

public:
    enum NodeType {FILE, OBJECT, FUNCTION, SUBSYSTEM, CLASS};
    static std::string getTypeString(NodeType type);

    ClangNode(std::string ID, std::string name, NodeType type);
    ~ClangNode();

    std::string getID();
    std::string getName();
    ClangNode::NodeType getType();

    bool addAttribute(std::string key, std::string value);
    bool clearAttributes(std::string key);
    std::vector<std::string> getAttribute(std::string key);

    bool doesAttributeExist(std::string key, std::string value);

    std::string generateInstance();
    std::string generateAttribute();

    /** ATTRIBUTE VARS */
    static AttributeStruct FILE_ATTRIBUTE;

private:
    const std::string INSTANCE_FLAG = "$INSTANCE";
    const std::string NAME_FLAG = "label";

    std::string ID;
    std::map<std::string, std::vector<std::string>> nodeAttributes;
    NodeType type;

    std::string printSingleAttribute(std::string key, std::vector<std::string> value);
    std::string printSetAttribute(std::string key, std::vector<std::string> value);
};


#endif //CLANGEX_CLANGNODE_H
