//
// Created by bmuscede on 05/11/16.
//

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
    /** ATTRIBUTE STRUCTS */
    typedef struct {
        const std::string attrName = "filename";

        std::string processFileName(std::string path){
            boost::filesystem::path p(path);
            return p.filename().string();
        }
    } AttributeStruct;
    typedef struct {
        const std::string attrName = "baseNum";
    } BaseStruct;
    typedef struct {
        const std::string staticName = "isStatic";
        const std::string constName = "isConst";
        const std::string volName = "isVolatile";
        const std::string varName = "isVariadic";
    } FuncIsAStruct;
    typedef struct {
        const std::string attrName = "visibility";

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
    typedef struct {
        const std::string scopeName = "scopeType";
        const std::string staticName = "isStatic";

        const std::string GLOBAL_KEY = "global";
        const std::string LOCAL_KEY = "local";
        const std::string PARAM_KEY = "parameter";

        //NOTE: This returns static function vars as "local".
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

        std::string getScope(const clang::FieldDecl* decl){
            return "todo";
        }

        std::string getStatic(const clang::VarDecl* decl){
            return std::to_string(decl->isStaticDataMember());
        }

        std::string getStatic(const clang::FieldDecl* decl){
            return "todo";
        }
    } VarStruct;
public:
    enum NodeType {FILE, VARIABLE, FUNCTION, SUBSYSTEM, CLASS, UNION, STRUCT, ENUM};
    static std::string getTypeString(NodeType type);
    static ClangNode::NodeType getTypeNode(std::string name);

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
    static BaseStruct BASE_ATTRIBUTE;
    static FuncIsAStruct FUNC_IS_ATTRIBUTE;
    static AccessStruct VIS_ATTRIBUTE;
    static VarStruct VAR_ATTRIBUTE;
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
