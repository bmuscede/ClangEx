/////////////////////////////////////////////////////////////////////////////////////////////////////////
// ClangNode.cpp
//
// Created By: Bryan J Muscedere
// Date: 05/11/16.
//
// Represents a node in the TA graph system. Nodes are elements of the AST that
// could be variables, functions, and classes. Also contains a set of operations
// for the attributes that each type of node expects.
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

#include "ClangNode.h"

using namespace std;

/** Struct Definitions */
ClangNode::AttributeStruct ClangNode::FILE_ATTRIBUTE;
ClangNode::BaseStruct ClangNode::BASE_ATTRIBUTE;
ClangNode::FuncIsAStruct ClangNode::FUNC_IS_ATTRIBUTE;
ClangNode::AccessStruct ClangNode::VIS_ATTRIBUTE;
ClangNode::VarStruct ClangNode::VAR_ATTRIBUTE;
ClangNode::StructStruct ClangNode::STRUCT_ATTRIBUTE;

/**
 * Converts an enum to a string representation. Used for TA encoding.
 * @param type The node type to convert.
 * @return The string representation of the enum.
 */
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

/**
 * Converts a string to an enum representation. Used for TA loading.
 * @param name The string being read.
 * @return An enum representation of the string. (Defaults to SUBSYSTEM).
 */
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

/**
 * Converts a decl kind to an enum.
 * @param src The decl kind information.
 * @return The enum that represents the decl kind.
 */
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

/**
 * Constructor. Builds a node based on an ID, name and type.
 * @param ID The ID of the node.
 * @param name The name of the node.
 * @param type The type of the node.
 */
ClangNode::ClangNode(string ID, string name, NodeType type) {
    //Set the ID and type.
    this->ID = ID;
    this->type = type;

    //Next, set the attribute list.
    nodeAttributes = map<string, vector<string>>();
    nodeAttributes[NAME_FLAG] = vector<string>();
    nodeAttributes[NAME_FLAG].push_back(name);
}

/**
 * Default destructor.
 */
ClangNode::~ClangNode() { }

/**
 * Gets the ID of the node.
 * @return The ID of the node.
 */
string ClangNode::getID() {
    return ID;
}

/**
 * Gets the name of the node.
 * @return The name of the node.
 */
string ClangNode::getName() {
    return nodeAttributes.at(NAME_FLAG).at(0);
}

/**
 * Gets the type of the node.
 * @return The type of the node.
 */
ClangNode::NodeType ClangNode::getType(){
    return type;
}

/**
 * Adds an attribute to the node.
 * @param key The key of the attribute.
 * @param value The value of the attribute.
 * @return Whether the attribute was added.
 */
bool ClangNode::addAttribute(string key, string value) {
    //Check if we're trying to modify the name.
    if (key.compare(NAME_FLAG) == 0){
        return false;
    }

    nodeAttributes[key].push_back(value);
    return true;
}

/**
 * Clears all attribute values for a given key.
 * @param key The key to clear.
 * @return Whether that attribute was cleared.
 */
bool ClangNode::clearAttributes(string key){
    //Check if we already have an empty set of attributes.
    if (nodeAttributes[key].size() == 0) return false;

    //Clear the vector.
    nodeAttributes[key] = vector<string>();
    return true;
}

/**
 * Gets an attribute for a given key.
 * @param key The key to look up.
 * @return A vector with all values.
 */
vector<string> ClangNode::getAttribute(string key) {
    return nodeAttributes[key];
}

/**
 * Checks whether an attribute exists.
 * @param key The key to find.
 * @param value The value to find.
 * @return Whether or not it exists.
 */
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

/**
 * Gets the entire attribute list.
 * @return The map of all attributes for the node.
 */
map<string, vector<std::string>> ClangNode::getAttributes(){
    return nodeAttributes;
};

/**
 * Helper method that generates a line for the node in the TA encoding.
 * @return
 */
string ClangNode::generateInstance() {
    return INSTANCE_FLAG + " " + ID + " " + getTypeString(type);
}

/**
 * Generates the attribute line for the given node.
 * @return
 */
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

/**
 * Helper method that generates an attribute line for an attribute with only one value.
 * @param key The key of the attribute.
 * @param value The value of the attribute.
 * @return The string representing that attribute.
 */
string ClangNode::printSingleAttribute(string key, vector<string> value){
    return key + " = \"" + value.at(0) + "\"";
}

/**
 * Helper method that generates an attribute line for an attribute with multiple values.
 * @param key The key of the attribute.
 * @param value The value of the attribute.
 * @return The string representing that attribute.
 */
string ClangNode::printSetAttribute(string key, vector<string> value){
    string attribute = key + " = ( ";

    //Prints the value.
    for (int i = 0; i < value.size(); i++){
        string curr = "\"" + value.at(i) + "\"";

        //Get the attribute
        attribute += curr;
        if (i + 1 < value.size()) attribute += " ";
    }
    attribute += " )";

    return attribute;
}

