//
// Created by bmuscede on 05/11/16.
//

#include "ClangNode.h"

using namespace std;

ClangNode::AttributeStruct ClangNode::FILE_ATTRIBUTE;
ClangNode::BaseStruct ClangNode::BASE_ATTRIBUTE;
ClangNode::FuncIsAStruct ClangNode::FUNC_IS_ATTRIBUTE;
ClangNode::AccessStruct ClangNode::VIS_ATTRIBUTE;
ClangNode::VarStruct ClangNode::VAR_ATTRIBUTE;
ClangNode::StructStruct ClangNode::STRUCT_ATTRIBUTE;

string ClangNode::getTypeString(NodeType type) {
    //Generates switch statement.
    if (type == FILE){
        return "cFile";
    } else if (type == VARIABLE){
        return "cVariable";
    } else if (type == FUNCTION){
        return  "cFunction";
    } else if (type == SUBSYSTEM){
        return "cSubSystem";
    } else if (type == CLASS){
        return "cClass";
    } else if (type == UNION){
        return "cUnion";
    } else if (type == STRUCT){
        return "cStruct";
    } else if (type == ENUM){
        return "cEnum";
    } else if (type == ENUM_CONST){
        return "cEnumConst";
    }

    //Default value if there is no node type specified.
    return "cRoot";
}

ClangNode::NodeType ClangNode::getTypeNode(string name){
    //Generates switch statement.
    if (name.compare("cFile") == 0){
        return FILE;
    } else if (name.compare("cVariable") == 0){
        return VARIABLE;
    } else if (name.compare("cFunction") == 0){
        return  FUNCTION;
    } else if (name.compare("cSubSystem") == 0){
        return SUBSYSTEM;
    } else if (name.compare("cClass") == 0){
        return CLASS;
    } else if (name.compare("cUnion") == 0){
        return UNION;
    } else if (name.compare("cStruct") == 0){
        return STRUCT;
    } else if (name.compare("cEnum") == 0){
        return ENUM;
    } else if (name.compare("cEnumConst") == 0){
        return ENUM_CONST;
    }

    //Default value if there is no node type specified.
    return SUBSYSTEM;
}

ClangNode::NodeType ClangNode::convertToNodeType(clang::Decl::Kind src){
    if (src == clang::Decl::Kind::Var || src == clang::Decl::Kind::Field){
        return VARIABLE;
    } else if (src == clang::Decl::Kind::Function || src == clang::Decl::Kind::CXXMethod){
        return FUNCTION;
    } else if (src == clang::Decl::Kind::Enum){
        return ENUM;
    } else if (src == clang::Decl::Kind::EnumConstant){
        return ENUM_CONST;
    } else if (src == clang::Decl::Kind::Record || src == clang::Decl::Kind::CXXRecord){
        //Note, a class is returned here since there is no way to
        //distinguish between class, structs, and unions.
        return CLASS;
    }

    assert("Error! The item being passed to the conversion system is not recognized.");
    return SUBSYSTEM;
}

ClangNode::ClangNode(string ID, string name, NodeType type) {
    //Set the ID and type.
    this->ID = ID;
    this->type = type;

    //Next, set the attribute list.
    nodeAttributes = map<string, vector<string>>();
    nodeAttributes[NAME_FLAG] = vector<string>();
    nodeAttributes[NAME_FLAG].push_back(name);
}

ClangNode::~ClangNode() { }

string ClangNode::getID() {
    return ID;
}

string ClangNode::getName() {
    return nodeAttributes.at(NAME_FLAG).at(0);
}

ClangNode::NodeType ClangNode::getType(){
    return type;
}

bool ClangNode::addAttribute(string key, string value) {
    //Check if we're trying to modify the name.
    if (key.compare(NAME_FLAG) == 0){
        return false;
    }

    nodeAttributes[key].push_back(value);
    return true;
}

bool ClangNode::clearAttributes(string key){
    //Check if we already have an empty set of attributes.
    if (nodeAttributes[key].size() == 0) return false;

    //Clear the vector.
    nodeAttributes[key] = vector<string>();
    return true;
}

vector<string> ClangNode::getAttribute(string key) {
    return nodeAttributes[key];
}

bool ClangNode::doesAttributeExist(string key, string value){
    //Check if the attribute key exists.
    vector<string> attr = nodeAttributes[key];
    if (attr.size() == 0) return false;

    //Now, look for the value.
    for (string attrVal : attr){
        if (value.compare(attrVal) == 0) return true;
    }

    return false;
}

string ClangNode::generateInstance() {
    return INSTANCE_FLAG + " " + ID + " " + getTypeString(type);
}

string ClangNode::generateAttribute() {
    if (nodeAttributes.size() == 0) return "";

    //Create label with ID and opening bracket.
    string att = ID + " { ";

    //Loop through and add all KVs.
    bool nBegin = false;
    for (auto const& it : nodeAttributes){
        //Check if we have a blank attribute.
        if (it.second.size() == 0) continue;

        //Add the attribute to the string.
        if (nBegin) att += " ";

        //Check the type of vector we have.
        if (it.second.size() == 1) att += printSingleAttribute(it.first, it.second);
        else if (it.second.size() > 1) att += printSetAttribute(it.first, it.second);

        nBegin = true;
    }
    att += " }";

    return att;
}

map<string, vector<std::string>> ClangNode::getAttributes(){
    return nodeAttributes;
};

string ClangNode::printSingleAttribute(string key, vector<string> value){
    return key + " = \"" + value.at(0) + "\"";
}

string ClangNode::printSetAttribute(string key, vector<string> value){
    string attribute = key + " ( ";

    //Prints the value.
    for (int i = 0; i < value.size(); i++){
        string curr = value.at(i);

        //Get the attribute
        attribute += curr;
        if (i + 1 < value.size()) attribute += " ";
    }
    attribute += " )";

    return attribute;
}

