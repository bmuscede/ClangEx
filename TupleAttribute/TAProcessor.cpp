//
// Created by bmuscede on 22/12/16.
//

#include <fstream>
#include "TAProcessor.h"

using namespace std;

TAProcessor::TAProcessor(string entityRelName){
    this->entityString = entityRelName;
}

TAProcessor::~TAProcessor(){ }

//TODO
bool TAProcessor::readTAFile(string fileName){
    //Starts by creating the file stream.
    ifstream modelStream(fileName);

    //Check if the file opens.
    if (!modelStream.is_open()){
        cerr << "The TA file " << fileName << " does not exist!";
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
    for (auto entry : relations) writeRelation(graph, entry);
    for (auto entry : attributes) writeAttributes(graph, entry);

    return graph;
}

bool TAProcessor::readGeneric(ifstream modelStream, string fileName){
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

bool TAProcessor::readScheme(ifstream modelStream, int* lineNum){
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

bool TAProcessor::readRelations(ifstream modelStream, int* lineNum){
    string line;

    //Start iterating through
    auto pos = modelStream.tellg();
    while(getline(modelStream, line)){
        //Check
    }
}

bool TAProcessor::readAttributes(ifstream modelStream, int* lineNum){

}

void TAProcessor::writeRelations(TAGraph* graph, pair<string, set<pair<string, string>>> relation){
    //Gets the relation name.
    string name = relation.first;

    //Check if we're dealing with new items.
    if (!name.compare(entryString)){
        //Iterate through the relation.
        for (auto curRel : relation.second){
            ClangNode* curNode = new ClangNode(curRel.first, );
            graph->addNode(curNode);
        }
    } else {

    }
}