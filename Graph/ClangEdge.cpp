/////////////////////////////////////////////////////////////////////////////////////////////////////////
// ClangEdge.cpp
//
// Created By: Bryan J Muscedere
// Date: 05/11/16.
//
// Represents an edge in the TA graph system. Edges are elements of the AST that
// are relationships between entities. Also contains a set of operations
// for the attributes that each type of edge expects.
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

#include "ClangEdge.h"

using namespace std;

/** Edge Attribute Variables */
ClangEdge::AccessStruct ClangEdge::ACCESS_ATTRIBUTE;

/**
 * Gets the string representation for a type.
 * @param type The type to get the representation.
 * @return The string representation.
 */
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
    } else if (type == FILE_CONTAIN){
        return "fContain";
    }

    //Default if the type isn't defined.
    return "cRef";
}

/**
 * Gets the enum representation for the string.
 * @param name The string to convert.
 * @return The enum representation.
 */
ClangEdge::EdgeType ClangEdge::getTypeEdge(string name){
    //Goes through and checks for type.
    if (name.compare("contains") == 0){
        return CONTAINS;
    } else if (name.compare("call") == 0){
        return CALLS;
    } else if (name.compare("reference") == 0){
        return REFERENCES;
    } else if (name.compare("inherit") == 0){
        return INHERITS;
    } else if (name.compare("fContain") == 0){
        return FILE_CONTAIN;
    }

    //Default if the type isn't defined.
    return REFERENCES;
}

/**
 * Constructor. Creates an edge with a source, destination, and edge type.
 * @param src The source node.
 * @param dst The destination node.
 * @param type The edge type.
 */
ClangEdge::ClangEdge(ClangNode *src, ClangNode *dst, EdgeType type) {
    this->src = src;
    this->dst = dst;

    this->type = type;

    edgeAttributes = map<string, vector<string>>();
}

/**
 * Default Destructor
 */
ClangEdge::~ClangEdge() { }

/**
 * Gets the source node.
 * @return The source node.
 */
ClangNode* ClangEdge::getSrc() {
    return src;
}

/**
 * Gets the destination node.
 * @return The destination node.
 */
ClangNode* ClangEdge::getDst() {
    return dst;
}

/**
 * Gets the type of the edge.
 * @return The edge type.
 */
ClangEdge::EdgeType ClangEdge::getType(){
    return type;
}

/**
 * Sets the source node.
 * @param newSrc The new source node to add.
 */
void ClangEdge::setSrc(ClangNode* newSrc){
    src = newSrc;
}

/**
 * Sets the desination node.
 * @param newDst The new destination node to add.
 */
void ClangEdge::setDst(ClangNode* newDst){
    dst = newDst;
}

/**
 * Adds an attribute.
 * @param key The key to add.
 * @param value The value to add.
 * @return Returns whether the attribute was added.
 */
bool ClangEdge::addAttribute(string key, string value){
    //Add the attribute by key.
    edgeAttributes[key].push_back(value);

    //Return true on new value entry.
    if (edgeAttributes[key].size() == 1) return true;
    return false;
}

/**
 * Clears all attributes from the attribute list.
 * @param key The key to clear.
 * @return Returns whether the attribute was cleared.
 */
bool ClangEdge::clearAttribute(string key){
    //Check if the key has attributes.
    if (edgeAttributes[key].size() == 0) return false;

    //Next, we clear it.
    edgeAttributes[key] = vector<string>();
    return true;
}

/**
 * Gets an attribute based on a key.
 * @param key The key to add.
 * @return Returns a list of all values for that key.
 */
vector<string> ClangEdge::getAttribute(string key) {
    return edgeAttributes[key];
}

/**
 * Checks whether an attribute exists given a key and value.
 * @param key The key to check.
 * @param value The value to check.
 * @return Whether the value exists or not.
 */
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

/**
 * Gets all attributes in the list.
 * @return A map of all attributes.
 */
map<string, vector<string>> ClangEdge::getAttributes(){
    return edgeAttributes;
}

/**
 * Generates the relationship string for this edge.
 * @return The relationship string.
 */
string ClangEdge::generateRelationship() {
    return getTypeString(type) + " " + src->getID() + " " + dst->getID();
}

/**
 * Generates the attribute string for this edge.
 * @return The attribute string.
 */
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

/**
 * Helper method that prints a single attribute.
 * @param key The key to print.
 * @param value The value to print.
 * @return The string for the attribute line.
 */
std::string ClangEdge::printSingleAttribute(string key, vector<string> value){
    return key + " = \"" + value.at(0) + "\"";
}

/**
 * Helper method that prints multiple attributes.
 * @param key The key to print.
 * @param value The values to print.
 * @return The string for the attribute line.
 */
std::string ClangEdge::printSetAttribute(string key, vector<string> value){
    string attribute = key + " = ( ";

    //Prints the value.
    for (int i = 0; i < value.size(); i++){
        string curr = value.at(i);

        //Get the attribute
        attribute += "\"" + curr + "\"";
        if (i + 1 < value.size()) attribute += " ";
    }
    attribute += " )";

    return attribute;
}
