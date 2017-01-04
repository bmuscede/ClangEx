//
// Created by bmuscede on 05/11/16.
//

#include "ClangEdge.h"

using namespace std;

ClangEdge::AccessStruct ClangEdge::ACCESS_ATTRIBUTE;

string ClangEdge::getTypeString(EdgeType type) {
    //Goes through and checks for type.
    if (type == CONTAINS){
        return "contain";
    } else if (type == CALLS){
        return "call";
    } else if (type == REFERENCES){
        return "reference";
    } else if (type == INHERITS){
        return "inherit";
    }

    //Default if the type isn't defined.
    return "cRef";
}

ClangEdge::EdgeType ClangEdge::getTypeEdge(string name){
    //Goes through and checks for type.
    if (){
        return CONTAINS;
    } else if (name.compare("call") == 0){
        return CALLS;
    } else if (name.compare("reference") == 0){
        return REFERENCES;
    } else if (name.compare("inherit") == 0){
        return INHERITS;
    }

    //Default if the type isn't defined.
    return REFERENCES;
}

ClangEdge::ClangEdge(ClangNode *src, ClangNode *dst, EdgeType type) {
    this->src = src;
    this->dst = dst;

    this->type = type;

    edgeAttributes = map<string, vector<string>>();
}

ClangEdge::~ClangEdge() { }

ClangNode* ClangEdge::getSrc() {
    return src;
}

ClangNode* ClangEdge::getDst() {
    return dst;
}

ClangEdge::EdgeType ClangEdge::getType(){
    return type;
}

bool ClangEdge::addAttribute(string key, string value){
    //Add the attribute by key.
    edgeAttributes[key].push_back(value);

    //Return true on new value entry.
    if (edgeAttributes[key].size() == 1) return true;
    return false;
}

bool ClangEdge::clearAttribute(string key){
    //Check if the key has attributes.
    if (edgeAttributes[key].size() == 0) return false;

    //Next, we clear it.
    edgeAttributes[key] = vector<string>();
    return true;
}

vector<string> ClangEdge::getAttribute(string key) {
    return edgeAttributes[key];
}

bool ClangEdge::doesAttributeExist(string key, string value) {
    //Check if the attribute key exists.
    vector<string> attr = edgeAttributes[key];
    if (attr.size() == 0) return false;

    //Now, look for the value.
    for (string attrVal : attr){
        if (value.compare(attrVal) == 0) return true;
    }

    return false;
}
string ClangEdge::generateRelationship() {
    return getTypeString(type) + " " + src->getID() + " " + dst->getID();
}

string ClangEdge::generateAttribute() {
    //Choose not to proceed.
    if (edgeAttributes.size() == 0) return "";

    //Starts the string.
    string attributeList = "(" + ClangEdge::getTypeString(type) + " " + src->getID() + " " + dst->getID() + ") { ";

    //Loop through and add all KVs.
    bool nBegin = false;
    for (auto const& it : edgeAttributes){
        //Check if we have a blank attribute.
        if (it.second.size() == 0) continue;

        //Add the attribute to the string.
        if (nBegin) attributeList += " ";

        //Check the type of vector we have.
        if (it.second.size() == 1) attributeList += printSingleAttribute(it.first, it.second);
        else if (it.second.size() > 1) attributeList += printSetAttribute(it.first, it.second);

        nBegin = true;
    }
    attributeList += " }";

    return attributeList;
}

std::string ClangEdge::printSingleAttribute(string key, vector<string> value){
    return key + " = " + value.at(0);
}

std::string ClangEdge::printSetAttribute(string key, vector<string> value){
    string attribute = key + " = ( ";

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
