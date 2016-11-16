//
// Created by bmuscede on 05/11/16.
//

//TODO:
// - Add class nodes.
// - Update schema.
// - Create attribute structs in Node and Edge classes (Improve with functions in struct).
// - Fix undefined references to allow for attributes.

#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <clang/Lex/Lexer.h>
#include <boost/tokenizer.hpp>
#include "ASTWalker.h"
#include "Graph/ClangNode.h"
#include "Graph/ClangEdge.h"

using namespace std;
using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;
using namespace llvm;

ASTWalker::ASTWalker(){ }

ASTWalker::~ASTWalker() { }

void ASTWalker::run(const MatchFinder::MatchResult &result) {
    //Look for the AST matcher being triggered.
    if (const DeclaratorDecl *dec = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[FUNC_DEC])) {
        //If a function has been found.
        addFunctionDecl(result, dec);
    } else if (const CallExpr *expr = result.Nodes.getNodeAs<clang::CallExpr>(types[FUNC_CALL])){
        //If a function call has been found.
        auto *caller = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[CALLER]);
        addFunctionCall(result, expr, caller);
    } else if (const VarDecl *dec = result.Nodes.getNodeAs<clang::VarDecl>(types[VAR_DEC])){
        //If a variable declaration has been found.
        addVariableDecl(result, dec);
    } else if (const VarDecl *dec = result.Nodes.getNodeAs<clang::VarDecl>(types[VAR_CALL])){
        //If a variable reference has been found.
        auto *caller = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[CALLER_VAR]);
        auto *expr = result.Nodes.getNodeAs<clang::DeclRefExpr>(types[VAR_EXPR]);

        addVariableRef(result, dec, caller, expr);
    } else if (const CXXRecordDecl *dec = result.Nodes.getNodeAs<clang::CXXRecordDecl>(types[CLASS_DEC])){
        cout << dec->getQualifiedNameAsString() << endl;

        //If a class declaration was found.
        //addClassDecl(result, dec);
    }
}

void ASTWalker::buildGraph(string fileName) {
    //First, runs the graph builder process.
    string tupleAttribute = graph.generateTAFormat();

    //Next, writes to disk.
    ofstream taFile;
    taFile.open(fileName.c_str());

    //Check if the file is opened.
    if (!taFile.is_open()){
        cout << "The TA file could not be written to " << fileName << "!" << endl;
        return;
    }

    taFile << tupleAttribute;
    taFile.close();

    cout << "TA file successfully written to " << fileName << "!" << endl;
}

void ASTWalker::generateASTMatches(MatchFinder *finder) {
    //Finds function declarations for current C/C++ file.
    finder->addMatcher(functionDecl(isExpansionInMainFile()).bind(types[FUNC_DEC]), this);

    //Finds function calls from one function to another.
    finder->addMatcher(callExpr(isExpansionInMainFile(), hasAncestor(functionDecl().bind(types[CALLER]))).bind(types[FUNC_CALL]), this);

    //Finds variables in functions or in class declaration.
    finder->addMatcher(varDecl(isExpansionInMainFile()).bind(types[VAR_DEC]), this);

    //Finds variable uses from a function to a variable.
    finder->addMatcher(declRefExpr(hasDeclaration(varDecl(isExpansionInMainFile()).bind(types[VAR_CALL])),
                                   hasAncestor(functionDecl().bind(types[CALLER_VAR]))).bind(types[VAR_EXPR]), this);

    //Finds any class declarations.
    finder->addMatcher(cxxRecordDecl(isExpansionInMainFile()).bind(types[CLASS_DEC]), this);
}

void ASTWalker::resolveExternalReferences() {
    int resolved = 0;
    int unresolved = 0;

    //Iterate through and resolve.
    for (int i = 0; i < unresolvedRef.size(); i++){
        pair<pair<string, string>, ClangEdge::EdgeType> entry = unresolvedRef.at(i);

        //Find the two items.
        vector<ClangNode*> srcs = graph.findNodeByName(entry.first.first);
        vector<ClangNode*> dsts = graph.findNodeByName(entry.first.second);

        //See if they could be resolved.
        if (srcs.size() == 0 || dsts.size() == 0){
            unresolved++;
        } else {
            ClangEdge* edge = new ClangEdge(srcs.at(0), dsts.at(0), entry.second);
            graph.addEdge(edge);

            //Now, find all associated attributes.
            vector<pair<string, vector<string>>> attributes = findAttributes(entry.first.first, entry.first.second);
            for (auto attribute : attributes){
                for (auto attVal : attribute.second){
                    graph.addAttribute(srcs.at(0)->getID(), dsts.at(0)->getID(), attribute.first, attVal);
                }
            }

            resolved++;
        }
    }

    //Afterwards, notify of success.
    cout << "Overall, " << resolved << " references were resolved and " << unresolved
         << " references could not be resolved." << endl;
}

void ASTWalker::resolveFiles(){
    vector<ClangNode*> fileNodes = vector<ClangNode*>();
    vector<ClangEdge*> fileEdges = vector<ClangEdge*>();

    //Gets all the associated clang nodes.
    fileParser.processPaths(fileNodes, fileEdges);

    //Adds them to the graph.
    for (ClangNode* file : fileNodes){
        graph.addNode(file);
    }

    //Adds the edges to the graph.
    for (ClangEdge* edge : fileEdges){
        graph.addEdge(edge);
    }

    //Next, for each item in the graph, add it to a file.
    graph.addNodesToFile();
}

void ASTWalker::addUnresolvedRef(string callerID, string calleeID, ClangEdge::EdgeType type) {
    //Creates the entry for the vector.
    pair<pair<string, string>, ClangEdge::EdgeType> entry = pair<pair<string, string>, ClangEdge::EdgeType>();
    entry.first.first = callerID;
    entry.first.second = calleeID;
    entry.second = type;

    //Finally, adds it to the unresolved vector.
    unresolvedRef.push_back(entry);
}

void ASTWalker::addUnresolvedRefAttr(string callerID, string calleeID, string attrName, string attrValue) {
    //Find if we already have a attr entry for this list.
    for (auto current : unresolvedRefAttr){
        if (current->first.first.compare(callerID) == 0 &&
                current->first.second.compare(calleeID) == 0 &&
                current->second.first.compare(attrName) == 0){

            //We already have an entry.
            current->second.second.push_back(attrValue);
            return;
        }
    }

    //Creates the entry for the vector.
    pair<pair<string, string>, pair<string, vector<string>>> *entry = new pair<pair<string, string>, pair<string, vector<string>>>();

    entry->first.first = callerID;
    entry->first.second = calleeID;
    entry->second.first = attrName;
    entry->second.second.push_back(attrValue);

    //Adds it to the vector.
    unresolvedRefAttr.push_back(entry);
}

vector<pair<string, vector<string>>> ASTWalker::findAttributes(string callerID, string calleeID){
    vector<pair<string, vector<string>>> values;

    //Find all associated values.
    for (auto entry : unresolvedRefAttr){
        if (entry->first.first.compare(callerID) == 0 && entry->first.second.compare(calleeID) == 0){
            //Adds the entry.
            pair<string, vector<string>> value;
            value.first = entry->second.first;
            value.second = entry->second.second;

            values.push_back(value);
        }
    }

    return values;
}

string ASTWalker::generateID(string fileName, string signature) {
    return fileName + "[" + signature + "]";
}

string ASTWalker::generateID(const MatchFinder::MatchResult result, const DeclaratorDecl *decl){
    //Gets the file name from the source manager.
    string fileName = generateFileName(result, decl->getInnerLocStart());

    //Gets the qualified name.
    string name = decl->getNameAsString();

    return generateID(fileName, name);
}

string ASTWalker::generateFileName(const MatchFinder::MatchResult result, SourceLocation loc){
    //Gets the file name.
    SourceManager& SrcMgr = result.Context->getSourceManager();
    const FileEntry* Entry = SrcMgr.getFileEntryForID(SrcMgr.getFileID(loc));
    string fileName(Entry->getName());

    //Adds the file path.
    fileParser.addPath(fileName);

    return fileName;
}

void ASTWalker::addVariableDecl(const MatchFinder::MatchResult result, const VarDecl *decl) {
    string fileName = generateFileName(result, decl->getInnerLocStart());

    //Get the ID of the variable.
    string ID = generateID(fileName, decl->getNameAsString());

    //Next, gets the qualified name.
    string qualName = generateLabel(decl, ClangNode::OBJECT);

    //Creates a variable entry.
    ClangNode* node = new ClangNode(ID, qualName, ClangNode::OBJECT);
    graph.addNode(node);

    //Process attributes.
    graph.addSingularAttribute(node->getID(),
                               ClangNode::FILE_ATTRIBUTE.attrName,
                               ClangNode::FILE_ATTRIBUTE.processFileName(fileName));
}

void ASTWalker::addFunctionDecl(const MatchFinder::MatchResult result, const DeclaratorDecl *decl) {
    string fileName = generateFileName(result, decl->getInnerLocStart());

    //First, generate the ID of the function.
    string ID = generateID(result, decl);

    //Gets the qualified name.
    string qualName = generateLabel(decl, ClangNode::FUNCTION);

    //Creates a new function entry.
    ClangNode* node = new ClangNode(ID, qualName, ClangNode::FUNCTION);
    graph.addNode(node);

    //Process attributes.
    graph.addSingularAttribute(node->getID(),
                               ClangNode::FILE_ATTRIBUTE.attrName,
                               ClangNode::FILE_ATTRIBUTE.processFileName(fileName));
}

void ASTWalker::addFunctionCall(const MatchFinder::MatchResult result,
                                const CallExpr *expr, const DeclaratorDecl *decl) {
    //Generate the ID of the caller and callee.
    string callerName = generateLabel(decl, ClangNode::FUNCTION);
    string calleeName = generateLabel(expr->getCalleeDecl()->getAsFunction(), ClangNode::FUNCTION);

    //Next, we find by ID.
    vector<ClangNode*> caller = graph.findNodeByName(callerName);
    vector<ClangNode*> callee = graph.findNodeByName(calleeName);

    //Check if we have the correct size.
    if (caller.size() == 0 || callee.size() == 0){
        //Add to unresolved reference list.
        addUnresolvedRef(callerName, calleeName, ClangEdge::CALLS);

        //Add the attributes.
        //TODO: Function call attributes?
        return;
    }

    //We finally add the edge.
    ClangEdge* edge = new ClangEdge(caller.at(0), callee.at(0), ClangEdge::CALLS);
    graph.addEdge(edge);

    //Process attributes.
    //TODO: Function call attributes?
}

void ASTWalker::addVariableRef(const MatchFinder::MatchResult result,
                               const VarDecl *decl, const DeclaratorDecl *caller, const DeclRefExpr *expr) {
    //Start by generating the ID of the caller and callee.
    string callerName = generateLabel(caller, ClangNode::FUNCTION);
    string varName = generateLabel(decl, ClangNode::OBJECT);

    //Next, we find their IDs.
    vector<ClangNode*> callerNode = graph.findNodeByName(callerName);
    vector<ClangNode*> varNode = graph.findNodeByName(varName);

    //Check to see if we have these entries already done.
    if (callerNode.size() == 0 || varNode.size() == 0){
        //Add to unresolved reference list.
        addUnresolvedRef(callerName, varName, ClangEdge::REFERENCES);

        //Add attributes.
        addUnresolvedRefAttr(callerName, varName,
                             ClangEdge::ACCESS_ATTRIBUTE.attrName, getVariableAccess(result, decl));
        return;
    }

    //Add the edge.
    ClangEdge* edge = new ClangEdge(callerNode.at(0), varNode.at(0), ClangEdge::REFERENCES);
    graph.addEdge(edge);

    //Process attributes.
    graph.addAttribute(callerNode.at(0)->getID(), varNode.at(0)->getID(),
                       ClangEdge::ACCESS_ATTRIBUTE.attrName, getVariableAccess(result, decl));
}

void ASTWalker::addClassDecl(const MatchFinder::MatchResult result, const CXXRecordDecl *decl) {
    //Generates the filename.
    string fileName = generateFileName(result, decl->getInnerLocStart());

    //Get the name & ID of the class.
    string classID = generateID(fileName, decl->getNameAsString());
    string className = generateLabel(decl, ClangNode::CLASS);

    //Creates a new function entry.
    ClangNode* node = new ClangNode(classID, className, ClangNode::CLASS);
    node->addAttribute(ClangNode::FILE_ATTRIBUTE.attrName, ClangNode::FILE_ATTRIBUTE.processFileName(fileName));
    graph.addNode(node);
}

string ASTWalker::generateLabel(const Decl* decl, ClangNode::NodeType type){
    string label;

    //Get qualified name.
    if (type == ClangNode::FUNCTION){
        label = decl->getAsFunction()->getQualifiedNameAsString();
    } else if (type == ClangNode::OBJECT){
        const VarDecl* var = static_cast<const VarDecl*>(decl);
        label = var->getQualifiedNameAsString();

        //We need to get the parent function.
        const DeclContext* parentContext = var->getParentFunctionOrMethod();

        //If we have NULL, get the parent function.
        if (parentContext != NULL){
            string parentQualName = static_cast<const FunctionDecl*>(parentContext)->getQualifiedNameAsString();
            label = parentQualName + "::" + label;
        }
    }

    return label;
}

string ASTWalker::getVariableAccess(const MatchFinder::MatchResult result, const VarDecl *var) {
    //Get the source range and manager.
    SourceRange range = var->getSourceRange();
    const SourceManager *SM = result.SourceManager;

    //Use LLVM's lexer to get source text.
    llvm::StringRef ref = Lexer::getSourceText(CharSourceRange::getCharRange(range), *SM, result.Context->getLangOpts());
    if (ref.str().compare("") == 0) return ClangEdge::ACCESS_ATTRIBUTE.READ_FLAG;

    //Get associated strings.
    string varStatement = ref.str();
    string varName = var->getName();

    //First, check if the name of the variable actually appears.
    if (varStatement.find(varName) == std::string::npos) return ClangEdge::ACCESS_ATTRIBUTE.READ_FLAG;

    //Next, tokenize statement.
    typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
    boost::char_separator<char> sep{" "};
    tokenizer tok{varStatement, sep};
    vector<string> statementTokens;
    for (const auto &t : tok) {
        statementTokens.push_back(t);
    }

    //Check if we have increment and decrement operators.
    for (string token : statementTokens){
        //We found our variable.
        if (token.find(varName) != std::string::npos){
            //Next, check for the increment and decrement operators.
            for (string op : incDecOperators){
                if (token.compare(op + varName) == 0) return ClangEdge::ACCESS_ATTRIBUTE.WRITE_FLAG;
                if (token.compare(varName + op) == 0) return ClangEdge::ACCESS_ATTRIBUTE.WRITE_FLAG;
            }
        }
    }

    //Get the position of the variable name.
    int pos = find(statementTokens.begin(), statementTokens.end(), varName) - statementTokens.begin();

    //Search vector for C/C++ assignment operators.
    int i = 0;
    bool foundAOp = false;
    for (string token : statementTokens) {
        //Check for a particular assignment operator.
        for (string op : assignmentOperators) {
            if (token.compare(op) == 0) {
                foundAOp = true;
                break;
            }
        }

        //See if we found the operator.
        if (foundAOp) break;
        i++;
    }

    //Now, we see what side of the operator our variable is on.
    if (!foundAOp){
        return ClangEdge::ACCESS_ATTRIBUTE.READ_FLAG;
    } else if (pos <= i){
        return ClangEdge::ACCESS_ATTRIBUTE.WRITE_FLAG;
    }
    return ClangEdge::ACCESS_ATTRIBUTE.READ_FLAG;
}