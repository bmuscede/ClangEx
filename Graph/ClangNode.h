/////////////////////////////////////////////////////////////////////////////////////////////////////////
// ClangNode.h
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

#ifndef CLANGEX_CLANGNODE_H
#define CLANGEX_CLANGNODE_H

#include <string>
#include <map>
#include <vector>
#include <boost/filesystem/path.hpp>
#include <clang/Basic/Specifiers.h>
#include <clang/Sema/Scope.h>

class ClangNode {
private:
    /** Struct For All Nodes */
    typedef struct {
        const std::string attrName = "filename";

        /**
         * Gets the file name for the object.
         * @param path The boost path being used.
         */
        std::string processFileName(std::string path){
            boost::filesystem::path p(path);
            return p.filename().string();
        }
    } AttributeStruct;

    /** Struct for Classes */
    typedef struct {
        const std::string attrName = "baseNum";
    } BaseStruct;

    /** Structs for Functions */
    typedef struct {
        const std::string staticName = "isStatic";
        const std::string constName = "isConst";
        const std::string volName = "isVolatile";
        const std::string varName = "isVariadic";
    } FuncIsAStruct;
    typedef struct {
        const std::string attrName = "visibility";

        /**
         * Gets the access specifier information for the function.
         * @param spec The access specifier.
         */
        std::string processAccessSpec(clang::AccessSpecifier spec){
            //Check the enum value.
            if (spec == clang::AccessSpecifier::AS_private){
                return "private";
            } else if (spec == clang::AccessSpecifier::AS_protected){
                return "protected";
            } else if (spec == clang::AccessSpecifier::AS_public){
                return "public";
            }

            return "none";
        }
    } AccessStruct;

    /** Struct for Variables */
    typedef struct {
        const std::string scopeName = "scopeType";
        const std::string staticName = "isStatic";

        const std::string GLOBAL_KEY = "global";
        const std::string LOCAL_KEY = "local";
        const std::string PARAM_KEY = "parameter";

        /**
         * Gets the scope of the variable.
         * @param decl The variable decl being used.
         */
        std::string getScope(const clang::VarDecl* decl){
            //Checks first if local and param.
            if (!decl->isLocalVarDeclOrParm())
                return VAR_ATTRIBUTE.GLOBAL_KEY;

            //Next, we check if it's a parameter.
            if (decl->isLocalVarDeclOrParm() && !decl->isLocalVarDecl())
                return VAR_ATTRIBUTE.PARAM_KEY;

            //If we have gotten to this point, we assume it's local.
            return VAR_ATTRIBUTE.LOCAL_KEY;
        }

        /**
         * Gets the scope for the field.
         * @param decl The field decl being used.
         */
        std::string getScope(const clang::FieldDecl* decl){
            return "todo"; //TODO
        }

        /**
         * Gets whether a variable is static or not.
         * @param decl The variable decl being used.
         */
        std::string getStatic(const clang::VarDecl* decl){
            return std::to_string(decl->isStaticDataMember());
        }

        /**
         * Gets whether a field is static or not.
         * @param decl The field decl being used.
         */
        std::string getStatic(const clang::FieldDecl* decl){
            return "todo"; //TODO
        }
    } VarStruct;

    /** Struct for Structs */
    typedef struct {
        const std::string anonymousName = "isAnonymous";

        /**
         * Whether a struct is anonymous or not.
         * @param anonymous If a struct is anonymous.
         */
        std::string processAnonymous(bool anonymous){
            if (anonymous){
                return "1";
            }

            return "0";
        }
    } StructStruct;

public:
    /** Node Type Members */
    enum NodeType {FILE, VARIABLE, FUNCTION, SUBSYSTEM, CLASS, UNION, STRUCT, ENUM, ENUM_CONST};
    static std::string getTypeString(NodeType type);
    static ClangNode::NodeType getTypeNode(std::string name);
    static ClangNode::NodeType convertToNodeType(clang::Decl::Kind src);

    /** Constructor and Destructor */
    ClangNode(std::string ID, std::string name, NodeType type);
    ~ClangNode();

    /** Getters */
    std::string getID();
    std::string getName();
    ClangNode::NodeType getType();

    /** Attribute Getters/Setters */
    bool addAttribute(std::string key, std::string value);
    bool clearAttributes(std::string key);
    std::vector<std::string> getAttribute(std::string key);
    bool doesAttributeExist(std::string key, std::string value);
    std::map<std::string, std::vector<std::string>> getAttributes();

    /** TA Operations */
    std::string generateInstance();
    std::string generateAttribute();


    /** Attribute Variables */
    static AttributeStruct FILE_ATTRIBUTE;
    static BaseStruct BASE_ATTRIBUTE;
    static FuncIsAStruct FUNC_IS_ATTRIBUTE;
    static AccessStruct VIS_ATTRIBUTE;
    static VarStruct VAR_ATTRIBUTE;
    static StructStruct STRUCT_ATTRIBUTE;

private:
    /** TA Flags */
    const std::string INSTANCE_FLAG = "$INSTANCE";
    const std::string NAME_FLAG = "label";

    /** Member Variables */
    std::string ID;
    std::map<std::string, std::vector<std::string>> nodeAttributes;
    NodeType type;

    /** TA Helper Methods */
    std::string printSingleAttribute(std::string key, std::vector<std::string> value);
    std::string printSetAttribute(std::string key, std::vector<std::string> value);
};


#endif //CLANGEX_CLANGNODE_H
