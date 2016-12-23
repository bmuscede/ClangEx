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

}

bool TAProcessor::writeTAFile(string fileName){

}

bool TAProcessor::readTAGraph(TAGraph graph){

}

TAGraph* TAProcessor::writeTAGraph(){

}