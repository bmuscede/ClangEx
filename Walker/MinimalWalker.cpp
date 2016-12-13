//
// Created by bmuscede on 06/12/16.
//

#include <iostream>
#include "MinimalWalker.h"

using namespace std;

MinimalWalker::MinimalWalker(){
    ClangArgParse::ClangExclude exclude;
    this->exclusions = exclude;
}

MinimalWalker::MinimalWalker(ClangArgParse::ClangExclude exclusions){
    this->exclusions = exclusions;
}

MinimalWalker::~MinimalWalker(){ }

void MinimalWalker::run(const MatchFinder::MatchResult &result){
    //Check if the current result fits any of our match criteria.
    if (const DeclaratorDecl *dec = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[FUNC_DEC])) {
        //Get whether we have a system header.
        if (isInSystemHeader(result, dec)) return;

        addFunctionDec(result, dec);
    } else if (const VarDecl *dec = result.Nodes.getNodeAs<clang::VarDecl>(types[VAR_DEC])){
         //Get whether we have a system header.
        if (isInSystemHeader(result, dec)) return;
        if (dec->getQualifiedNameAsString().compare("") == 0) return;

        addVariableDec(result, dec);
    } else if (const FieldDecl *dec = result.Nodes.getNodeAs<clang::FieldDecl>(types[FIELD_DEC])){
        //Get whether we have a system header.
        if (isInSystemHeader(result, dec)) return;

        addVariableDec(result, dec);
    } else if (const CallExpr *expr = result.Nodes.getNodeAs<clang::CallExpr>(types[FUNC_CALLEE])){
        if (expr->getCalleeDecl() == NULL || !(isa<const clang::FunctionDecl>(expr->getCalleeDecl()))) return;
        auto callee = expr->getCalleeDecl()->getAsFunction();
        auto caller = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[FUNC_CALLER]);

        //Get whether this call expression is a system header.
        if (isInSystemHeader(result, callee)) return;

        addFunctionCall(result, caller, callee);
    } else if (const VarDecl *callee = result.Nodes.getNodeAs<clang::VarDecl>(types[VAR_CALLEE])) {
        //If a variable reference has been found.
        auto *caller = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[VAR_CALLER]);
        auto *expr = result.Nodes.getNodeAs<clang::Expr>(types[VAR_EXPR]);

        //Get whether this call expression is in the system header.
        if (isInSystemHeader(result, callee)) return;

        addVariableCall(result, callee, caller, expr);
    } else if (const FieldDecl *callee = result.Nodes.getNodeAs<clang::FieldDecl>(types[FIELD_CALLEE])){
        //If a variable reference has been found.
        auto *caller = result.Nodes.getNodeAs<clang::DeclaratorDecl>(types[VAR_CALLER]);
        auto *expr = result.Nodes.getNodeAs<clang::Expr>(types[VAR_EXPR]);

        //Get whether this call expression is in the system header.
        if (isInSystemHeader(result, callee)) return;

        addVariableCall(result, callee, caller, expr);
    } else if (const CXXRecordDecl *classRec = result.Nodes.getNodeAs<clang::CXXRecordDecl>(types[CLASS_DEC])){
        //Get whether this call expression is in the system header.
        if (isInSystemHeader(result, classRec)) return;

        addClassDec(result, classRec);
    } else if (const EnumDecl *dec = result.Nodes.getNodeAs<clang::EnumDecl>(types[ENUM_DEC])){
        //Get whether this call expression is in the system header.
        if (isInSystemHeader(result, dec)) return;

        addEnumDec(result, dec);
    }
}

void MinimalWalker::generateASTMatches(MatchFinder *finder){
    //Function methods.
    if (!exclusions.cFunction){
        //Finds function declarations for current C/C++ file.
        finder->addMatcher(functionDecl().bind(types[FUNC_DEC]), this);

        //Finds function calls from one function to another.
        finder->addMatcher(callExpr(hasAncestor(functionDecl().bind(types[FUNC_CALLER]))).bind(types[FUNC_CALLEE]), this);
    }

    //Variable methods.
    if (!exclusions.cVariable){
        //Finds variable declarations in functions AND in class decs.
        finder->addMatcher(varDecl().bind(types[VAR_DEC]), this);
        finder->addMatcher(fieldDecl().bind(types[FIELD_DEC]), this);

        //Finds variable uses amongst functions.
        finder->addMatcher(declRefExpr(hasDeclaration(varDecl().bind(types[VAR_CALLEE])),
                           hasAncestor(functionDecl().bind(types[VAR_CALLER])),
                           hasParent(expr().bind(types[VAR_EXPR]))), this);
        finder->addMatcher(declRefExpr(hasDeclaration(fieldDecl().bind(types[FIELD_CALLEE])),
                                       hasAncestor(functionDecl().bind(types[VAR_CALLER])),
                                       hasParent(expr().bind(types[FIELD_EXPR]))), this);

    }

    //Class methods.
    if (!exclusions.cClass){
        //Finds class declarations.
        finder->addMatcher(cxxRecordDecl(isClass()).bind(types[CLASS_DEC]), this);

        //TODO: References not implemented.
    }

    //Enum methods.
    if (!exclusions.cEnum){
        finder->addMatcher(enumDecl().bind(types[ENUM_DEC]), this);
    }
}

void MinimalWalker::addFunctionDec(const MatchFinder::MatchResult results, const DeclaratorDecl *dec){
    //Generate the fields for the node.
    string ID = generateID(results, dec, ClangNode::FUNCTION);
    string label = generateLabel(dec, ClangNode::FUNCTION);
    string filename = generateFunctionFileName(results, dec->getAsFunction(),
                                               generateFileName(results, dec->getInnerLocStart()));
    if (ID.compare("") == 0 || filename.compare("") == 0) return;


    //Creates a new function entry.
    ClangNode* node = new ClangNode(ID, label, ClangNode::FUNCTION);
    graph.addNode(node);

    //Adds parameters.
    graph.addSingularAttribute(node->getID(),
                               ClangNode::FILE_ATTRIBUTE.attrName,
                               ClangNode::FILE_ATTRIBUTE.processFileName(filename));


    //Check if we have a CXXMethodDecl.
    if (isa<CXXMethodDecl>(dec->getAsFunction())){
        //Perform a static cast.
        const CXXMethodDecl* methDecl = static_cast<const CXXMethodDecl*>(dec->getAsFunction());

        //Process method decls.
        bool isStatic = methDecl->isStatic();
        bool isConst = methDecl ->isConst();
        bool isVol = methDecl->isVolatile();
        bool isVari = methDecl->isVariadic();
        AccessSpecifier spec = methDecl->getAccess();

        //Add these types of attributes.
        graph.addSingularAttribute(node->getID(),
                                   ClangNode::FUNC_IS_ATTRIBUTE.staticName,
                                   std::to_string(isStatic));
        graph.addSingularAttribute(node->getID(),
                                   ClangNode::FUNC_IS_ATTRIBUTE.constName,
                                   std::to_string(isConst));
        graph.addSingularAttribute(node->getID(),
                                   ClangNode::FUNC_IS_ATTRIBUTE.volName,
                                   std::to_string(isVol));
        graph.addSingularAttribute(node->getID(),
                                   ClangNode::FUNC_IS_ATTRIBUTE.varName,
                                   std::to_string(isVari));
        graph.addSingularAttribute(node->getID(),
                                   ClangNode::VIS_ATTRIBUTE.attrName,
                                   ClangNode::VIS_ATTRIBUTE.processAccessSpec(spec));
    }
}

void MinimalWalker::addVariableDec(const MatchFinder::MatchResult results, const VarDecl *dec){
    //Generate the fields for the node.
    string ID = generateID(results, dec, ClangNode::VARIABLE);
    string label = generateLabel(dec, ClangNode::VARIABLE);
    string filename = generateFileName(results, dec->getInnerLocStart());
    if (ID.compare("") == 0 || filename.compare("") == 0) return;

    //Creates a variable entry.
    ClangNode* node = new ClangNode(ID, label, ClangNode::VARIABLE);
    graph.addNode(node);

    //Process attributes.
    graph.addSingularAttribute(node->getID(),
                               ClangNode::FILE_ATTRIBUTE.attrName,
                               ClangNode::FILE_ATTRIBUTE.processFileName(filename));

    //Get the scope of the decl.
    graph.addSingularAttribute(node->getID(),
                               ClangNode::VAR_ATTRIBUTE.scopeName,
                               ClangNode::VAR_ATTRIBUTE.getScope(dec));
    graph.addSingularAttribute(node->getID(),
                               ClangNode::VAR_ATTRIBUTE.staticName,
                               ClangNode::VAR_ATTRIBUTE.getStatic(dec));
}

void MinimalWalker::addVariableDec(const MatchFinder::MatchResult results, const FieldDecl *dec){
    //Generate the fields for the node.
    string ID = generateID(results, dec, ClangNode::VARIABLE);
    string label = generateLabel(dec, ClangNode::VARIABLE);
    string filename = generateFileName(results, dec->getInnerLocStart());
    if (ID.compare("") == 0 || filename.compare("") == 0) return;

    //Creates a variable entry.
    ClangNode* node = new ClangNode(ID, label, ClangNode::VARIABLE);
    graph.addNode(node);

    //Process attributes.
    graph.addSingularAttribute(node->getID(),
                               ClangNode::FILE_ATTRIBUTE.attrName,
                               ClangNode::FILE_ATTRIBUTE.processFileName(filename));

    //Get the scope of the decl.
    graph.addSingularAttribute(node->getID(),
                               ClangNode::VAR_ATTRIBUTE.scopeName,
                               ClangNode::VAR_ATTRIBUTE.getScope(dec));
    graph.addSingularAttribute(node->getID(),
                               ClangNode::VAR_ATTRIBUTE.staticName,
                               ClangNode::VAR_ATTRIBUTE.getStatic(dec));
}

void MinimalWalker::addFunctionCall(const MatchFinder::MatchResult results, const DeclaratorDecl* caller,
                                    const FunctionDecl* callee){
    //Generate a label for the two functions.
    string callerLabel = generateLabel(caller, ClangNode::FUNCTION);
    string calleeLabel = generateLabel(callee, ClangNode::FUNCTION);

    //Next, we find by ID.
    vector<ClangNode*> callerNode = graph.findNodeByName(callerLabel);
    vector<ClangNode*> calleeNode = graph.findNodeByName(calleeLabel);

    //Check if we have an already known reference.
    if (callerNode.size() == 0 || calleeNode.size() == 0){
        //Add to unresolved reference list.
        addUnresolvedRef(callerLabel, calleeLabel, ClangEdge::CALLS);

        //Add the attributes.
        //TODO: Function call attributes?
        return;
    }

    //We finally add the edge.
    ClangEdge* edge = new ClangEdge(callerNode.at(0), calleeNode.at(0), ClangEdge::CALLS);
    graph.addEdge(edge);

    //Process attributes.
    //TODO: Function call attributes?
}

void MinimalWalker::addVariableCall(const MatchFinder::MatchResult result, const VarDecl *callee, const DeclaratorDecl *caller,
                                    const Expr* expr){
    //Start by generating the ID of the caller and callee.
    string callerLabel = generateLabel(caller, ClangNode::FUNCTION);
    string calleeLabel = generateLabel(callee, ClangNode::VARIABLE);

    addVariableCall(result, callee->getName(), callerLabel, calleeLabel, expr);

}

void MinimalWalker::addVariableCall(const MatchFinder::MatchResult result, const clang::FieldDecl *callee,
                                    const clang::DeclaratorDecl *caller, const clang::Expr *expr) {
    //Start by generating the ID of the caller and callee.
    string callerLabel = generateLabel(caller, ClangNode::FUNCTION);
    string calleeLabel = generateLabel(callee, ClangNode::VARIABLE);

    addVariableCall(result, callee->getName(), callerLabel, calleeLabel, expr);
}

void MinimalWalker::addVariableCall(const MatchFinder::MatchResult result, string calleeName,
                                    string callerLabel, string calleeLabel, const clang::Expr* expr){
    //Next, we find their IDs.
    vector<ClangNode*> callerNode = graph.findNodeByName(callerLabel);
    vector<ClangNode*> calleeNode = graph.findNodeByName(calleeLabel);

    //Check to see if we have these entries already done.
    if (callerNode.size() == 0 || calleeNode.size() == 0){
        //Add to unresolved reference list.
        addUnresolvedRef(callerLabel, calleeLabel, ClangEdge::REFERENCES);

        //Add attributes.
        addUnresolvedRefAttr(callerLabel, calleeLabel,
                             ClangEdge::ACCESS_ATTRIBUTE.attrName, ClangEdge::ACCESS_ATTRIBUTE.getVariableAccess(result, expr, calleeName));
        return;
    }

    //Add the edge.
    ClangEdge* edge = new ClangEdge(callerNode.at(0), calleeNode.at(0), ClangEdge::REFERENCES);
    graph.addEdge(edge);

    //Process attributes.
    graph.addAttribute(callerNode.at(0)->getID(), calleeNode.at(0)->getID(),
                       ClangEdge::ACCESS_ATTRIBUTE.attrName, ClangEdge::ACCESS_ATTRIBUTE.getVariableAccess(result, expr, calleeName));
}

void MinimalWalker::addClassDec(const MatchFinder::MatchResult result, const CXXRecordDecl *classRec) {
    //Get the definition.
    auto def = classRec->getDefinition();
    if (def == NULL) return;

    //Check if we're dealing with a class.
    if (!classRec->isClass()) return;

    //Generate the fields for the node.
    string filename = generateFileName(result, classRec->getInnerLocStart());
    string ID = generateID(filename, classRec->getNameAsString(), ClangNode::CLASS);
    string className = generateLabel(classRec, ClangNode::CLASS);
    if (ID.compare("") == 0 || filename.compare("") == 0) return;

    //Try to get the number of bases.
    int numBases = 0;
    try {
        numBases = classRec->getNumBases();
    } catch (...){
        return;
    }

    //Creates a class entry.
    ClangNode* node = new ClangNode(ID, className, ClangNode::CLASS);
    graph.addNode(node);

    //Process attributes.
    graph.addSingularAttribute(node->getID(),
                               ClangNode::FILE_ATTRIBUTE.attrName,
                               ClangNode::FILE_ATTRIBUTE.processFileName(filename));
    graph.addSingularAttribute(node->getID(),
                               ClangNode::BASE_ATTRIBUTE.attrName,
                               std::to_string(numBases));

    //Get base classes.
    if (classRec->getNumBases() > 0) {
        for (auto base = classRec->bases_begin(); base != classRec->bases_end(); base++) {
            if (base->getType().getTypePtr() == NULL) continue;
            CXXRecordDecl *baseClass = base->getType().getTypePtr()->getAsCXXRecordDecl();
            if (baseClass == NULL) continue;

            //Add a linkage in our graph.
            addClassInheritanceRef(classRec, baseClass);
        }
    }
}

void MinimalWalker::addClassInheritanceRef(const CXXRecordDecl *childClass, const CXXRecordDecl *parentClass) {
    string classLabel = generateLabel(childClass, ClangNode::CLASS);
    string baseLabel = generateLabel(parentClass, ClangNode::CLASS);

    //Get the nodes by their label.
    vector<ClangNode*> classNode = graph.findNodeByName(classLabel);
    vector<ClangNode*> baseNode = graph.findNodeByName(baseLabel);

    //Check to see if we don't have these entries.
    if (classNode.size() == 0 || baseNode.size() == 0){
        addUnresolvedRef(classLabel, baseLabel, ClangEdge::INHERITS);

        //Add attributes.
        //TODO: Any inheritance attributes?

        return;
    }

    //Add the edge.
    ClangEdge* edge = new ClangEdge(classNode.at(0), baseNode.at(0), ClangEdge::INHERITS);
    graph.addEdge(edge);

    //Process  attributes.
    //TODO: Any inheritance attributes?
}

void MinimalWalker::addEnumDec(const MatchFinder::MatchResult result, const EnumDecl *dec){
    //Generate the fields for the node.
    string filename = generateFileName(result, dec->getInnerLocStart());
    string ID = generateID(filename,  dec->getNameAsString(), ClangNode::ENUM);
    string enumName = generateLabel(dec, ClangNode::ENUM);
    if (ID.compare("") == 0 || filename.compare("") == 0) return;

    //Creates a class entry.
    ClangNode* node = new ClangNode(ID, enumName, ClangNode::ENUM);
    graph.addNode(node);

    //Process attributes.
    graph.addSingularAttribute(node->getID(),
                               ClangNode::FILE_ATTRIBUTE.attrName,
                               ClangNode::FILE_ATTRIBUTE.processFileName(filename));
}

string MinimalWalker::generateFunctionFileName(const MatchFinder::MatchResult result,
                                               const FunctionDecl *dec, string fileName){
    if (dec == NULL) return fileName;

    const FunctionDecl *definition;
    const FunctionDecl *declaration;

    //First, check whether we have a definition or not.
    if (dec->isThisDeclarationADefinition()){
        definition = dec;
        declaration = dec;
    } else {
        declaration = dec;
        definition = dec->getDefinition();
    }

    //Check whether we don't have a definition.
    if (definition == NULL) return fileName;

    //Get the file name of the two.
    string decName = generateFileName(result, declaration->getInnerLocStart());
    string defName = generateFileName(result, definition->getInnerLocStart());

    //Check if we have a null filename.
    if (decName.compare("") == 0) return defName;
    else if (defName.compare("") == 0) return decName;

    //Return the name of the definition.
    return defName;
}
