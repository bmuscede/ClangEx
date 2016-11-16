//
// Created by bmuscede on 05/11/16.
//

#include "ClangNode.h"

using namespace std;

ClangNode::AttributeStruct ClangNode::FILE_ATTRIBUTE;

string ClangNode::getTypeString(NodeType type) {
    //Generates switch statement.
    if (type == FILE){
        return "cFile";
    } else if (type == OBJECT){
        return "cObject";
    } else if (type == FUNCTION){
        return  "cFunction";
    } else if (type == SUBSYSTEM){
        return "cSubSystem";
    } else if (type == CLASS){
        return "cClass";
    }

    //Default value if there is no node type specified.
    return "cRoot";
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

bool ClangNode::addAttribute(string key, string value) {
    //Check if we're trying to modify the name.
    if (key.compare(NAME_FLAG) == 0){
        return false;
    }

    nodeAttributes[key].push_back(value);
    return true;
}

bool ClangNode::clearAttributes(string key){
    if (key.compare(NAME_FLAG) == 0) return false;

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

