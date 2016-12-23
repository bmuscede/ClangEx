//
// Created by bmuscede on 22/12/16.
//

#include "TAProcessor.h"

using namespace std;

TAProcessor::TAProcessor(string entityRelName){
    this->entityString = entityRelName;
}

TAProcessor::~TAProcessor(){ }

bool TAProcessor::readTAFile(string fileName){
    //Looks up the file to read in.
    return true;
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

    return graph;
}