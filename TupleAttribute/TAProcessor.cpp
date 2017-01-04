//
// Created by bmuscede on 22/12/16.
//

#include <fstream>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "TAProcessor.h"

using namespace std;

/*
 * Trim Operations
 * Taken From: http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
 */
static inline string &ltrim(string &s) {
    s.erase(s.begin(), find_if(s.begin(), s.end(),
                                    not1(ptr_fun<int, int>(isspace))));
    return s;
}
static inline string &rtrim(string &s) {
    s.erase(find_if(s.rbegin(), s.rend(),
                         not1(ptr_fun<int, int>(isspace))).base(), s.end());
    return s;
}
static inline string &trim(string &s) {
    return ltrim(rtrim(s));
}

TAProcessor::TAProcessor(string entityRelName){
    this->entityString = entityRelName;
}

TAProcessor::~TAProcessor(){ }

bool TAProcessor::readTAFile(string fileName){
    //Starts by creating the file stream.
    ifstream modelStream(fileName);

    //Check if the file opens.
    if (!modelStream.is_open()){
        cerr << "The TA file " << fileName << " does not exist!" << endl;
        cerr << "Exiting program..." << endl;
        return false;
    }

    //Next starts the main loop.
    bool success = readGeneric(modelStream, fileName);

    modelStream.close();
    return success;
}

bool TAProcessor::writeTAFile(string fileName){
    return true;
}

bool TAProcessor::readTAGraph(TAGraph graph){

    return true;
}

TAGraph* TAProcessor::writeTAGraph(){
    //Create a new graph.
    TAGraph* graph = new TAGraph();

    //We now iterate through the facts first.
    bool succ = writeRelations(graph);
    if (!succ){
        cerr << "Error converting TA file into TA graph!" << endl;
        cerr << "Check the format of your graph. Are you missing a " << entityString << " flag?" << endl;
        return NULL;
    }
    writeAttributes(graph);

    return graph;
}

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
                cerr << "Error on line " << line << "." << endl;
                cerr << ATTRIBUTE_FLAG << " encountered before " << RELATION_FLAG << "!" << endl;
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

bool TAProcessor::readScheme(ifstream& modelStream, int* lineNum){
    string line;

    //Start iterating through
    auto pos = modelStream.tellg();
    while(getline(modelStream, line)){
        //Check the line.
        if (!line.compare(0, SCHEME_FLAG.size(), SCHEME_FLAG)){
            //Invalid input.
            cerr << "Invalid input on line " << *lineNum << "." << endl;
            cerr << "Unexpected flag." << endl;

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

bool TAProcessor::readRelations(ifstream& modelStream, int* lineNum){
    string line;
    bool blockComment = false;
    (*lineNum)--;

    //Start iterating through
    auto pos = modelStream.tellg();
    while(getline(modelStream, line)){
        (*lineNum)++;
        if (line.compare("") == 0 || line.find_first_not_of(' ') != std::string::npos) continue;

        //Tokenize.
        vector<string> entry = prepareLine(line);

        //Check whether we've reached the end of the block comment.
        if (blockComment) {
            int blockEnd = checkForBlockEnd(entry, blockComment);
            if (blockComment) continue;

            //Trim the entry vector to start at block end.
            entry.erase(entry.begin(), entry.begin() + blockEnd);
            if (entry.size() == 0) continue;
        }

        //Check the comment content.
        int commentNum = checkForComment(entry, blockComment);

        //First, check if we have a block comment.
        if (blockComment && commentNum == 0){
            continue;
        } else if (blockComment && commentNum == 3) {
            continue;
        } else if (blockComment){
            cerr << "Invalid input on line " << *lineNum << "." << endl;
            cerr << "Line should contain a single tuple in RSF format." << endl;
            return false;
        }

        //Check whether the entry is valid.
        if ((entry.size() < 3 && commentNum < 3) ||
                (entry.size() > 3 && commentNum > 3)) {
            cerr << "Invalid input on line " << *lineNum << "." << endl;
            cerr << "Line should contain a single tuple in RSF format." << endl;
            return false;
        } else if (commentNum == 0) continue;

        //Next, gets the relation name.
        auto relName = entry.at(0);
        auto toName = entry.at(1);
        auto fromName = entry.at(2);

        //Finds if a pair exists.
        int pos = findRelEntry(relName);
        if (pos == -1) {
            //Creates a new pair.
            pair<string, set<pair<string, string>>> relEntry = pair<string, set<pair<string, string>>>();
            relEntry.first = relName;

            //Next, creates the set.
            set<pair<string, string>> setEntry = set<pair<string, string>>();

            //Finally, creates the pair with the to and from.
            pair<string, string> edge = pair<string, string>();
            edge.first = toName;
            edge.second = fromName;

            //Inserts the pair in the set
            setEntry.insert(edge);
            relEntry.second = setEntry;
        } else {
            //Creates a to from pair.
            pair<string, string> edge = pair<string, string>();
            edge.first = toName;
            edge.second = fromName;

            //Inserts it.
            relations.at(pos).second.insert(edge);
        }
    }

    //Check if the block comment system in still in place.
    if (blockComment){
        cerr << "No closing symbol found for block comment in TA file!" << endl;
        cerr << "Locate the start of your block comment and close it." << endl;

        return false;
    }

    return true;
}

bool TAProcessor::readAttributes(ifstream& modelStream, int* lineNum){
    string line;
    bool blockComment = false;
    (*lineNum)--;

    //Start iterating through
    auto pos = modelStream.tellg();
    while(getline(modelStream, line)){
        (*lineNum)++;
        if (line.compare("") == 0 || line.find_first_not_of(' ') != std::string::npos) continue;

        //Prepare the line.
        vector<string> entry = prepareAttributeLine(line);

        //Check for comments.
        int commentNum = checkForComment(entry, blockComment);

        //Check comment validity.
        if (blockComment && commentNum == 0){
            continue;
        } else if (blockComment){
            cerr << "Invalid input on line " << *lineNum << "." << endl;
            cerr << "Attributes are invalidly formatted." << endl;
            return false;
        }
        if (commentNum == 0) continue;
        else if (commentNum == 1) {
            cerr << "Invalid input on line " << *lineNum << "." << endl;
            cerr << "Attributes are invalidly formatted." << endl;
            return false;
        }

        if (entry.size() != 1){
            cerr << "Invalid input on line " << *lineNum << "." << endl;
            cerr << "Attribute line is missing attributes or is invalidly formatted!" << endl;
            return false;
        }

        //Next, prepares the attribute list.
        string attrName = entry.at(0);
        bool success = true;
        vector<string> attributes = prepareAttributes(entry.at(1), success);

        //Check whether it succeeded.
        if (!success){
            cerr << "Invalid entry on line " << *lineNum << "." << endl;
            cerr << "Attribute line is missing attributes or is invalidly formatted!" << endl;
            return false;
        }

        //Next, we process the attributes.
        int pos = findAttrEntry(attrName);
        if (pos == -1){
            //Createa a new entry.
            pair<string, vector<pair<string, string>>> attr = pair<string, vector<pair<string, string>>>();
            attr.first = attrName;
            this->attributes.push_back(attr);

            pos = (int) attributes.size() - 1;
        }

        //Now, we add the attributes in.
        for (string curAttribute : attributes){
            string key;
            string value;
            getAttribute(curAttribute, key, value);

            //Adds the key/value pair in.
            pair<string, string> kV = pair<string, string>();
            kV.first = key;
            kV.second = value;

            this->attributes.at(pos).second.push_back(kV);
        }
    }

    return true;
}

bool TAProcessor::writeRelations(TAGraph* graph){
    //First, finds the instance relation.
    int pos = findRelEntry(entityString);
    if (pos == -1){
        cerr << "TA file does not have a relation called " << entityString << "!" << endl;
        cerr << "Cannot continue..." << endl;
        return false;
    }

    //Gets the entity relation.
    auto entity = attributes.at(pos).second;
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

            if (src == NULL || dst == NULL){
                graph->addUnresolvedRef(nodes.first, nodes.second, type);
                continue;
            }

            //Creates a new edge.
            ClangEdge* edge = new ClangEdge(src, dst, type);
        }
    }

    return true;
}

bool TAProcessor::writeAttributes(TAGraph* graph){

}

vector<string> TAProcessor::prepareLine(string line){
    //Split into a vector.
    vector<string> stringList;
    boost::split(stringList, line, boost::is_any_of(" "));

    //Trim the strings.
    vector<string> modified;
    for (string curr : stringList){
        //TODO: Check!
        trim(curr);
        modified.push_back(curr);
    }

    return modified;
}

//TODO: This does a far too simple check for comments.
int TAProcessor::checkForComment(vector<string> line, bool &blockComment){
    int i = 0;

    //Goes through the line.
    for (string curr : line){
        if (curr.find(COMMENT_PREFIX) == 0) return i;
        if (curr.find(COMMENT_BLOCK_START) == 0){
            blockComment = true;
            return i;
        }

        i++;
    }

    return -1;
}

int TAProcessor::checkForBlockEnd(vector<string> line, bool &blockComment){
    int i = 0;

    //Goes through the line.
    for (string curr : line){
        if (curr.find(COMMENT_BLOCK_END) == 0){
            blockComment = false;
            return i;
        }
    }

    return -1;
}

int TAProcessor::findRelEntry(string name){
    int i = 0;

    //Goes through the relation vector.
    for (auto rel : relations){
        if (rel.first.compare(name) == 0) return i;

        i++;
    }

    return -1;
}

vector<string> TAProcessor::prepareAttributeLine(string line){
    //Gets the position of the first space.
    auto pos = line.find(' ');

    //Vectorizes the attribute line.
    vector<string> att;
    att.push_back(line.substr(0, pos));

    if (pos != string::npos)
        att.push_back(line.substr(pos + 1, line.length() - pos));

    //Trims it.
    vector<string> modified;
    for (string curr : att){
        //TODO: Check!
        trim(curr);
        modified.push_back(curr);
    }

    return modified;
}

//TODO This does not handle block comments.
vector<string> TAProcessor::prepareAttributes(string attrList, bool &success){
    //First, checks for the opening bracket.
    if (attrList.find("{") == 0){
        success = false;
        return vector<string>();
    }

    //Removes the first bracket.
    attrList.substr(1, attrList.size() - 1);
    ltrim(attrList);

    vector<string> finalVec;

    //Splits by a space.
    vector<string> stringList;
    boost::split(stringList, attrList, boost::is_any_of(" "));

    //Now builds the next string.
    bool key = false;
    bool eq = false;
    bool end = false;
    string build = "";
    for (int i = 0; i < stringList.size(); i++){
        string current = stringList.at(i);

        //Check for end.
        if (current.find("}") == 0){
            end = true;
            if (current.find("//") == 1){
                success = true;
                break;
            }
        } else if (end){
            success = false;
            return vector<string>();
        }

        //Check for comment.
        if (current.find(COMMENT_PREFIX) == 0) {
            success = false;
            return vector<string>();
        }

        //Check where we're at.
        if (!key){
            build = current + " ";
            key = true;
        } else if (key && !eq){
            build += current + " ";
            eq = true;
        } else if (key && eq){
            build += current;
            key = false;
            eq = false;

            finalVec.push_back(build);
        }
    }

    return finalVec;
}

int TAProcessor::findAttrEntry(string attrName){
    int i = 0;

    //Goes through the relation vector.
    for (auto attr : attributes){
        if (attr.first.compare(attrName) == 0) return i;

        i++;
    }

    return -1;
}

void TAProcessor::getAttribute(string curAttr, string &key, string &value){
    //Split the line based on the equals.
    vector<string> stringList;
    boost::split(stringList, curAttr, boost::is_any_of("="));

    //Next, trims the two sides.
    vector<string> correct;
    for (string curr : stringList){
        trim(curr);
        correct.push_back(curr);
    }

    //Finally, sets the KV.
    key = correct.at(0);
    value = correct.at(1);
}