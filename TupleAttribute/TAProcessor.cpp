/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TAProcessor.cpp
//
// Created By: Bryan J Muscedere
// Date:22/12/16.
//
// Processor system that is capable of reading and writing a TA file. Allows for
// the conversion of a TA graph to TA file or from a TA file to TA graph. This is
// primarily used for merging an already created TA file with newly processed source
// code.
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

#include <fstream>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "TAProcessor.h"

using namespace std;

/**
 * Left trim string.
 * @param s The string to trim.
 * @return The trimmed string.
 */
static inline string &ltrim(string &s) {
    s.erase(s.begin(), find_if(s.begin(), s.end(),
                                    not1(ptr_fun<int, int>(isspace))));
    return s;
}

/**
 * Right trim string.
 * @param s The string to trim.
 * @return The trimmed string.
 */
static inline string &rtrim(string &s) {
    s.erase(find_if(s.rbegin(), s.rend(),
                         not1(ptr_fun<int, int>(isspace))).base(), s.end());
    return s;
}

/**
 * Overall trim string.
 * @param s The string to trim.
 * @return The trimmed string.
 */
static inline string &trim(string &s) {
    return ltrim(rtrim(s));
}

/**
 * Constructor. Sets the entity flag name. By default, it is $INSTANCE.
 * @param entityRelName The entity relationship name.
 * @param print The ClangEx printer.
 */
TAProcessor::TAProcessor(string entityRelName, Printer* print) : clangPrinter(print) {
    this->entityString = entityRelName;
}

/**
 * Default Destructor
 */
TAProcessor::~TAProcessor(){ }

/**
 * Reads the TA file from a given file name.
 * @param fileName The file name to read from.
 * @return Whether it was read successfully.
 */
bool TAProcessor::readTAFile(string fileName){
    //Starts by creating the file stream.
    ifstream modelStream(fileName);

    //Check if the file opens.
    if (!modelStream.is_open()){
        clangPrinter->printErrorTAProcessRead(fileName);
        return false;
    }

    //Next starts the main loop.
    bool success = readGeneric(modelStream, fileName);

    modelStream.close();
    return success;
}

/**
 * Writes a TA file to a given file name.
 * @param fileName The location to write to.
 * @return Whether or not the file was written successfully.
 */
bool TAProcessor::writeTAFile(string fileName){
    //Open up a file pointer.
    ofstream taFile;
    taFile.open(fileName.c_str());

    //Check if it opened.
    if (!taFile.is_open()){
        clangPrinter->printErrorTAProcessWrite(fileName);
        return false;
    }

    //Generates the TA file.
    taFile << generateTAString();
    taFile.close();

    clangPrinter->printGenTADone(fileName, true);
    return true;
}

/**
 * Reads a TA graph.
 * @param graph The graph to read.
 * @return Whether the graph was read successfully.
 */
bool TAProcessor::readTAGraph(TAGraph* graph){
    if (graph == nullptr){
        clangPrinter->printErrorTAProcessGraph();
        return false;
    }

    //We simply read through the nodes and edges.
    processNodes(graph->getNodes());
    processEdges(graph->getEdges());

    return true;
}

/**
 * Writes to a TA graph.
 * @return The graph that was written.
 */
TAGraph* TAProcessor::writeTAGraph(){
    //Create a new graph.
    TAGraph* graph = new TAGraph();

    //We now iterate through the facts first.
    bool succ = writeRelations(graph);
    if (!succ) return nullptr;
    succ = writeAttributes(graph);
    if (!succ) return nullptr;

    return graph;
}

/**
 * From a file, reads each line. This method decides how to proceed.
 * @param modelStream The stream of the filename.
 * @param fileName The filename being read from.
 * @return Whether it was successful.
 */
bool TAProcessor::readGeneric(ifstream& modelStream, string fileName){
    bool running = true;
    bool tupleEncountered = false;

    //Starts by iterating until complete.
    string curLine;
    int line = 1;
    while(running){
        //We've hit the end.
        if (!getline(modelStream, curLine)){
            running = false;
            continue;
        }

        //We now check the line.
        if (!curLine.compare(0, SCHEME_FLAG.size(), SCHEME_FLAG)){
            //Fast forward.
            bool success = readScheme(modelStream, &line);
            if (!success) return false;

        } else if (!curLine.compare(0, RELATION_FLAG.size(), RELATION_FLAG)){
            tupleEncountered = true;

            //Reads the relations.
            bool success = readRelations(modelStream, &line);
            if (!success) return false;
        } else if (!curLine.compare(0, ATTRIBUTE_FLAG.size(), ATTRIBUTE_FLAG)){
            if (tupleEncountered == false){
                clangPrinter->printErrorTAProcess(line, ATTRIBUTE_FLAG + " encountered before " + RELATION_FLAG + "!");
                return false;
            }

            //Reads the attributes.
            bool success = readAttributes(modelStream, &line);
            if (!success) return false;
        }

        line++;
    }

    //Checks whether we've encountered a "fact tuple" section.
    if (tupleEncountered){
        return true;
    }

    return false;
}

/**
 * Reader that reads the schema section of the file.
 * @param modelStream The model stream.
 * @param lineNum The current line number.
 * @return Whether or not it was successful.
 */
bool TAProcessor::readScheme(ifstream& modelStream, int* lineNum){
    string line;

    //Start iterating through
    auto pos = modelStream.tellg();
    while(getline(modelStream, line)){
        //Check the line.
        if (!line.compare(0, SCHEME_FLAG.size(), SCHEME_FLAG)){
            //Invalid input.
            clangPrinter->printErrorTAProcess(*lineNum, UNEXPECTED_FLAG);

            return false;
        } else if (!line.compare(0, RELATION_FLAG.size(), RELATION_FLAG) ||
                !line.compare(0, ATTRIBUTE_FLAG.size(), ATTRIBUTE_FLAG)) {
            //Breaks out of the loop.
            break;
        }

        //Get the current line.
        pos = modelStream.tellg();
        (*lineNum)++;
    }

    //Seeks backward.
    modelStream.seekg(pos);
    return true;
}

/**
 * Reads the relation section from the TA file.
 * @param modelStream The model stream.
 * @param lineNum The current line number.
 * @return Whether or not it was successful.
 */
bool TAProcessor::readRelations(ifstream& modelStream, int* lineNum){
    string line;
    bool blockComment = false;
    (*lineNum)--;

    //Start iterating through
    auto pos = modelStream.tellg();
    while(getline(modelStream, line)){
        if (!line.compare(0, SCHEME_FLAG.size(), SCHEME_FLAG) ||
            !line.compare(0, ATTRIBUTE_FLAG.size(), ATTRIBUTE_FLAG)) {
            //Breaks out of the loop.
            break;
        } else if (!line.compare(0, RELATION_FLAG.size(), RELATION_FLAG)) {
            //Invalid input.
            clangPrinter->printErrorTAProcess(*lineNum, UNEXPECTED_FLAG);

            return false;
        }

        (*lineNum)++;

        //Tokenize.
        vector<string> entry = prepareLine(line, blockComment);
        if ((blockComment && entry.size() == 0) || entry.size() == 0) continue;

        //Check whether the entry is valid.
        if (entry.size() != 3) {
            clangPrinter->printErrorTAProcess(*lineNum, RSF_INVALID);
            return false;
        }

        //Next, gets the relation name.
        auto relName = entry.at(0);
        auto toName = entry.at(1);
        auto fromName = entry.at(2);

        //Finds if a pair exists.
        int pos = findRelEntry(relName);
        if (pos == -1) {
            createRelEntry(relName);
            pos = (int) relations.size() - 1;
        }

        //Creates a to from pair.
        pair<string, string> edge = pair<string, string>();
        edge.first = toName;
        edge.second = fromName;

        //Inserts it.
        relations.at(pos).second.insert(edge);

    }

    //Seeks backward.
    modelStream.seekg(pos);

    return true;
}

/**
 * Reads the attributes from the TA file.
 * @param modelStream The model stream.
 * @param lineNum The current line number.
 * @return Whether or not it was successful.
 */
bool TAProcessor::readAttributes(ifstream& modelStream, int* lineNum){
    string line;
    bool blockComment = false;
    (*lineNum)--;

    //Start iterating through
    auto pos = modelStream.tellg();
    while(getline(modelStream, line)) {
        (*lineNum)++;

        //Prepare the line.
        vector<string> entry = prepareLine(line, blockComment);
        if ((blockComment && entry.size() == 0) || entry.size() == 0) continue;

        //Checks for what type of system we're dealing with.
        bool succ = true;
        if (entry.at(0).compare("(") == 0 || entry.at(0).find("(") == 0) {
            //Relation attribute.
            if (entry.at(0).compare("(") == 0){
                entry.erase(entry.begin());
            } else {
                entry.at(0).erase(0, 1);
            }

            //Check for valid entry.
            if (entry.size() < 3 || entry.at(1).compare(")") == 0 || entry.at(2).compare(")") == 0){
                clangPrinter->printErrorTAProcess(*lineNum, ATTRIBUTE_SHORT);
                return false;
            }

            //Gets the relation name.
            string relName = entry.at(0);
            entry.erase(entry.begin());

            //Gets the IDs.
            string srcID = entry.at(0);
            entry.erase(entry.begin());
            string dstID = entry.at(0);
            if (dstID.back() == ')'){
                dstID.erase(dstID.size() - 1, 1);
                entry.erase(entry.begin());
            } else {
                entry.erase(entry.begin());
                entry.erase(entry.begin());
            }


            //Generates the attribute list.
            auto attrs = generateAttributes(*lineNum, succ, entry);
            if (!succ) return false;

            //Next, we insert
            int pos = findAttrEntry(relName, srcID, dstID);
            if (pos == -1) {
                createAttrEntry(relName, srcID, dstID);
                pos = (int) relAttributes.size() - 1;
            }
            this->relAttributes.at(pos).second = attrs;
        } else {
            //Regular attribute.
            //Gets the name and trims down the vector.
            string attrName = entry.at(0);
            entry.erase(entry.begin());

            //Generates the attribute list.
            auto attrs = generateAttributes(*lineNum, succ, entry);
            if (!succ) return false;

            //Next, we insert
            int pos = findAttrEntry(attrName);
            if (pos == -1) {
                createAttrEntry(attrName);
                pos = (int) attributes.size() - 1;
            }
            this->attributes.at(pos).second = attrs;
        }
    }

    return true;
}

/**
 * Writes relations to a TA graph.
 * @param graph The graph to write to.
 * @return Whether or not it was successful.
 */
bool TAProcessor::writeRelations(TAGraph* graph){
    //First, finds the instance relation.
    int pos = findRelEntry(entityString);
    if (pos == -1){
        clangPrinter->printErrorTAProcess(Printer::RELATION_FIND, entityString);
        return false;
    }

    //Gets the entity relation.
    auto entity = relations.at(pos).second;
    for (auto entry : entity){
        //Gets the name.
        string ID = entry.first;

        //Gets the ClangNode enum.
        ClangNode::NodeType type = ClangNode::getTypeNode(entry.second);

        //Creates a new node.
        ClangNode* node = new ClangNode(ID, ID, type);
        graph->addNode(node);
    }

    //Next, processes the other relationships.
    int i = 0;
    for (auto rels : relations){
        if (i == pos) continue;

        string relName = rels.first;
        ClangEdge::EdgeType type = ClangEdge::getTypeEdge(relName);

        std::set<pair<string, string>>::iterator it;
        for (it = rels.second.begin(); it != rels.second.end(); it++) {
            auto nodes = *it;

            //Gets the nodes.
            ClangNode* src = graph->findNodeByID(nodes.first);
            ClangNode* dst = graph->findNodeByID(nodes.second);

            if (src == nullptr || dst == nullptr){
                graph->addUnresolvedRef(nodes.first, nodes.second, type);
                continue;
            }

            //Creates a new edge.
            ClangEdge* edge = new ClangEdge(src, dst, type);
        }
    }

    return true;
}

/**
 * Writes attributes to a TA graph.
 * @param graph The graph to write to.
 * @return Whether or not it was successful.
 */
bool TAProcessor::writeAttributes(TAGraph* graph){
    //We simply go through and process them.
    for (auto attr : attributes){
        string itemID = attr.first;

        //Next, we go through all the KVs.
        for (auto kv : attr.second){
            string key = kv.first;
            vector<string> values = kv.second;

            //Now, updates the attributes.
            for (auto value : values) {
                bool succ = graph->addAttribute(itemID, key, value);
                if (!succ) {
                    clangPrinter->printErrorTAProcess(Printer::ENTITY_ATTRIBUTE, itemID);
                    return false;
                }
            }
        }
    }

    //Next, we deal with relation attributes.
    for (auto attr : relAttributes){
        vector<string> items = attr.first;
        if (items.size() != 3) {
            clangPrinter->printErrorTAProcessMalformed();
            return false;
        }

        ClangEdge::EdgeType relName = ClangEdge::getTypeEdge(items.at(0));
        string srcID = items.at(1);
        string dstID = items.at(2);

        //Next, we go through all the KVs.
        for (auto kv : attr.second){
            string key = kv.first;
            vector<string> values = kv.second;

            //Now, updates the attributes.
            for (auto value : values) {
                bool succ = graph->addAttribute(srcID, dstID, relName, key, value);
                if (!succ) {
                    clangPrinter->printErrorTAProcess(Printer::RELATION_ATTRIBUTE, "(" + srcID + ", " + dstID + ")");
                    return false;
                }
            }
        }
    }

    return true;
}

/**
 * Generates a TA string based on this system's internal representation.
 * @return The TA string.
 */
string TAProcessor::generateTAString(){
    string taString = "";

    //Gets the time.
    time_t now = time(0);
    string curTime = string(ctime(&now));

    //Start by generating the header.
    taString += SCHEMA_HEADER + "\n";
    taString += "//Generated on: " + curTime + "\n";

    //Next, gets the relations.
    taString += generateRelationString() + "\n";
    taString += generateAttributeString();

    return taString;
}

/**
 * Generates the relation portion for the TA file.
 * @return The relation string.
 */
string TAProcessor::generateRelationString(){
    string relString = "";
    relString += RELATION_FLAG + "\n";

    //Iterate through the relations.
    for (auto curr : relations){
        string relName = curr.first;

        //Iterate through the entries.
        for (auto currRel : curr.second){
            relString += relName + " " + currRel.first + " " + currRel.second + "\n";
        }
    }

    relString + "\n";
    return relString;
}

/**
 * Generates the attribute portion for the TA file.
 * @return The attribute string.
 */
string TAProcessor::generateAttributeString(){
    string attrString = "";
    attrString += ATTRIBUTE_FLAG + "\n";

    //Iterate through the entity attributes first.
    for (auto curr : attributes){
        string attributeID = curr.first;
        attrString += attributeID + generateAttributeStringFromKVs(curr.second) + "\n";
    }

    //Next, deals with the relation attribute list.
    for (auto curr : relAttributes){
        vector<string> items = curr.first;
        auto kVs = curr.second;

        if (items.size() != 3) return "";

        attrString += "(" + items.at(0) + " " + items.at(1) + " " +
                items.at(2) + ")" + generateAttributeStringFromKVs(kVs) + "\n";
    }

    return attrString;
}

/**
 * Generates attributes from a given set of KV pairs.
 * @param attr The attribute KV pair map.
 * @return The attribute string.
 */
string TAProcessor::generateAttributeStringFromKVs(vector<pair<string, vector<string>>> attr){
    string attrString = " { ";

    //Iterate through the pairs.
    for (auto currAttr : attr){
        //Check what type of string we need to generate.
        if (currAttr.second.size() == 1){
            attrString += currAttr.first + " = " + currAttr.second.at(0) + " ";
        } else {
            attrString += currAttr.first + " = (";
            for (auto value : currAttr.second)
                attrString += " " + value;

            attrString += " ) ";
        }
    }
    attrString += "}";

    return attrString;
}

/**
 * Generates attributes from a given line.
 * @param lineNum The line number.
 * @param succ Whether or not it was successful.
 * @param line The line to process.
 * @return A vector of all KV pairs for the attribute.
 */
vector<pair<string, vector<string>>> TAProcessor::generateAttributes(int lineNum, bool& succ,
                                                                     std::vector<std::string> line){
    if (line.size() < 3){
        succ = false;
        return vector<pair<string, vector<string>>>();
    }

    //Start by expecting the { symbol.
    if (line.at(0).compare("{") == 0){
        line.erase(line.begin());
    } else if (line.at(0).find("{") == 0){
        line.at(0).erase(0, 1);
    } else {
        succ = false;
        return vector<pair<string, vector<string>>>();
    }

    //Now, we iterate until we hit the end.
    int i = 0;
    bool end = false;
    string current = line.at(i);
    vector<pair<string, vector<string>>> attrList = vector<pair<string, vector<string>>>();
    do {
        //Adds in the first part of the entry.
        pair<string, vector<string>> currentEntry = pair<string, vector<string>>();
        currentEntry.first = current;

        //Checks for validity.
        if (i + 2 >= line.size() || line.at(++i).compare("=") != 0){
            succ = false;
            return vector<pair<string, vector<string>>>();
        }

        //Gets the next KV pair.
        string next = line.at(++i);
        if (next.compare("(") == 0 || next.find("(") == 0) {
            //First, remove the ( symbol.
            if (next.compare("(") == 0){
                if (i + 1 == line.size()){
                    succ = false;
                    return vector<pair<string, vector<string>>>();
                }
                next = line.at(++i);
            } else if (next.find("(") == 0){
                next.erase(0, 1);
            }

            //We now iterate through the attribute list.
            bool endList = false;
            do {
                //Check if we hit the end.
                if (next.compare(")") == 0) {
                    break;
                } else if (next.find(")") == next.size() - 1) {
                    next.erase(next.size() - 1, 1);
                    endList = true;
                }

                //Next, process the item.
                currentEntry.second.push_back(next);

                //Finally check if we've hit the conditions.
                if (endList){
                    break;
                } else if (i + 1 == line.size()){
                    succ = false;
                    return vector<pair<string, vector<string>>>();
                } else {
                    next = line.at(++i);
                }
            } while (true);
        } else {
            //Check if we have a "} symbol at the end.
            if (next.find("}") == next.size() - 1){
                end = true;
                next.erase(next.size() - 1, 1);
            }

            //Add it to the current entry.
            currentEntry.second.push_back(next);
        }

        //Increments the current string.
        if (i + 1 == line.size() && end == false){
            succ = false;
            return vector<pair<string, vector<string>>>();
        } else if (!end){
            current = line.at(++i);
        }

        //Adds the entry in.
        attrList.push_back(currentEntry);
    } while (current.compare("}") != 0 && end == false);

    return attrList;
}

/**
 * Prepares a line for being processed.
 * @param line The line to process.
 * @param blockComment Whether or not a block comment was encountered.
 * @return The tokenized string.
 */
vector<string> TAProcessor::prepareLine(string line, bool& blockComment){
    //Perform comment processing.
    line = removeStandardComment(line);
    line = removeBlockComment(line, blockComment);

    //Split into a vector.
    vector<string> stringList;
    boost::split(stringList, line, boost::is_any_of(" "));

    //Trim the strings.
    vector<string> modified;
    for (string curr : stringList){
        trim(curr);
        modified.push_back(curr);
    }

    //Sanity check for the modified system.
    if (modified.size() == 1 && modified.at(0).compare("") == 0){
        modified.erase(modified.begin());
    }

    return modified;
}

/**
 * Removes a standard comment from a line.
 * @param line The line to process.
 * @return The string with the comment removed.
 */
string TAProcessor::removeStandardComment(string line){
    //Iterate through the string two characters at a time.
    for (int i = 0; i + 1 < line.size(); i++){
        //Get the next two characters.
        char first = line.at(i);
        char second = line.at(i + 1);

        //Erase the characters.
        if (first == COMMENT_CHAR && second == COMMENT_CHAR){
            //We want to purge the line of all subsequent characters.
            line.erase(i, string::npos);
            break;
        }
    }

    return line;
}

/**
 * Removes a block comment.
 * @param line The line to process.
 * @param blockComment Whether a block comment was encountered.
 * @return The line without the block comment.
 */
string TAProcessor::removeBlockComment(string line, bool& blockComment){
    string newLine = "";

    //Iterate through the string two characters at a time.
    for (int i = 0; i + 1 < line.size(); i++){
        //Get the next two characters.
        char first = line.at(i);
        char second = line.at(i + 1);

        //Check if we have a block comment ending to look for.
        if (blockComment == true && (first == COMMENT_BLOCK_CHAR && second == COMMENT_CHAR)){
            //Set block comment to false.
            blockComment = false;

            //Now, we move the pointer ahead by 1.
            i++;
        } else if (blockComment == false && (first == COMMENT_CHAR && second == COMMENT_BLOCK_CHAR)){
            //Set block comment to true.
            blockComment = true;

            //Now, we move the pointer ahead by 1.
            i++;
        } else if (blockComment == false) {
            //Adds the character.
            newLine += first;
            if (i + 2 == line.size())
                newLine += second;
        }
    }

    return newLine;
}

/**
 * Finds a relationship entry.
 * @param name The name of the relationship.
 * @return The index of the relationship.
 */
int TAProcessor::findRelEntry(string name){
    int i = 0;

    //Goes through the relation vector.
    for (auto rel : relations){
        if (rel.first.compare(name) == 0) return i;

        i++;
    }

    return -1;
}

/**
 * Creates a relationship entry in the system.
 * @param name The name of the relationship.
 */
void TAProcessor::createRelEntry(string name){
    pair<string, set<pair<string, string>>> entry = pair<string, set<pair<string, string>>>();
    entry.first = name;

    relations.push_back(entry);
}

/**
 * Finds an attribute entry.
 * @param attrName The name of the attribute.
 * @return The index of the attribute.
 */
int TAProcessor::findAttrEntry(string attrName){
    int i = 0;

    //Goes through the attribute vector.
    for (auto attr : attributes){
        if (attr.first.compare(attrName) == 0) return i;

        i++;
    }

    return -1;
}

/**
 * Finds an attribute entry.
 * @param relName The relationship name.
 * @param src The source name.
 * @param dst The destination name.
 * @return The index of the attribute
 */
int TAProcessor::findAttrEntry(string relName, string src, string dst){
    int i = 0;

    //Goes through the attribute vector
    for (auto attr : relAttributes){
        //Abort!
        if (attr.first.size() != 3) return -1;

        if (attr.first.at(0).compare(relName) == 0 &&
                attr.first.at(1).compare(src) == 0 &&
                attr.first.at(2).compare(dst) ==0) return i;

        i++;
    }

    return -1;
}

/**
 * Creates an attribute entry.
 * @param attrName The name of the attribute.
 */
void TAProcessor::createAttrEntry(string attrName){
    //Create the pair object.
    pair<string, vector<pair<string, vector<string>>>> entry = pair<string, vector<pair<string, vector<string>>>>();
    entry.first = attrName;

    attributes.push_back(entry);
}

/**
 * Creates an attribute entry.
 * @param relName The name of the attribute.
 * @param src The source name.
 * @param dst The destination name.
 */
void TAProcessor::createAttrEntry(string relName, string src, string dst){
    //Create the pair object.
    pair<vector<string>, vector<pair<string, vector<string>>>> entry =
        pair<vector<string>, vector<pair<string, vector<string>>>>();
    entry.first.push_back(relName);
    entry.first.push_back(src);
    entry.first.push_back(dst);

    relAttributes.push_back(entry);
}

/**
 * Processes a collection of ClangNodes and adds them.
 * @param nodes The collection of ClangNodes.
 */
void TAProcessor::processNodes(vector<ClangNode*> nodes){
    //Sees if we have an entry for the current relation.
    int pos = findRelEntry(entityString);
    if (pos == -1) {
        createRelEntry(entityString);
        pos = (int) relations.size() - 1;
    }

    //Iterate through the nodes.
    for (ClangNode* curNode : nodes){
        //Adds in the node information.
        pair<string, string> relPair = pair<string, string>();
        relPair.first = curNode->getID();
        relPair.second = ClangNode::getTypeString(curNode->getType());
        relations.at(pos).second.insert(relPair);

        //Adds in the attributes.
        auto curAttr = curNode->getAttributes();
        if (curAttr.size() < 1) continue;

        //Adds in the attribute entry.
        int attrPos = findAttrEntry(curNode->getID());
        if (attrPos == -1) {
            createAttrEntry(curNode->getID());
            attrPos = (int) attributes.size() - 1;
        }

        //Iterates through them.
        typedef map<string, vector<string>>::iterator itType;
        for(itType it = curAttr.begin(); it != curAttr.end(); it++) {
            pair<string, vector<string>> kVs = pair<string, vector<string>>();
            kVs.first = it->first;

            //Iterates through all the values.
            for (string curVal : it->second){
                //Creates the entry.
                kVs.second.push_back(curVal);
            }

            //Adds the pair to the attribute list.
            attributes.at(attrPos).second.push_back(kVs);
        }
    }
}

/**
 * Processes a collection of edges and adds them.
 * @param edges The edges to add.
 */
void TAProcessor::processEdges(vector<ClangEdge*> edges){
    //Iterate over the edges.
    for (auto curEdge : edges){
        string typeName = ClangEdge::getTypeString(curEdge->getType());

        //Sees if we have an entry for the current type.
        int pos = findRelEntry(typeName);
        if (pos == -1) {
            createRelEntry(typeName);
            pos = (int) relations.size() - 1;
        }

        //Now, we simply add it.
        string srcID = curEdge->getSrc()->getID();
        string dstID = curEdge->getDst()->getID();
        string relName = ClangEdge::getTypeString(curEdge->getType());

        pair<string, string> relPair = pair<string, string>();
        relPair.first = srcID;
        relPair.second = dstID;

        //Add it to the relation list.
        relations.at(pos).second.insert(relPair);


        //Now we deal with any edge attributes;
        auto edgeAttr = curEdge->getAttributes();
        if (edgeAttr.size() < 1) continue;

        //Adds in an attribute entry.
        int attrPos = findAttrEntry(relName, srcID, dstID);
        if (attrPos == -1) {
            createAttrEntry(relName, srcID, dstID);
            attrPos = (int) relAttributes.size() - 1;
        }

        //Iterates through the attributes.
        typedef map<string, vector<string>>::iterator itType;
        for(itType it = edgeAttr.begin(); it != edgeAttr.end(); it++) {
            pair<string, vector<string>> kVs = pair<string, vector<string>>();
            kVs.first = it->first;

            //Iterates through all the values.
            for (string curVal : it->second){
                //Creates the entry.
                kVs.second.push_back(curVal);
            }

            //Adds the pair to the attribute list.
            relAttributes.at(attrPos).second.push_back(kVs);
        }
    }
}
