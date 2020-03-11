#include "OPCUANodeDataSource.h"

OPCUANodeDataSource::OPCUANodeDataSource(string dataSourceFileName)
{
    string nodeNamesLine;
    string nodeTypesLine;
    string nodeName;
    string nodeType;

    this->OPCUANamespace=1;
    this->numberOfNodes = 0;
    this->dataSourceFileStream.open(dataSourceFileName, ios::in); 

    // The first line is a list of node names
    getline(this->dataSourceFileStream, nodeNamesLine);
    stringstream nodeNamesStream(nodeNamesLine);

    // The second line is a list of node types
    getline(this->dataSourceFileStream, nodeTypesLine);
    stringstream nodeTypesStream(nodeTypesLine);

    // Parsing first line for name of nodes
    while (getline(nodeNamesStream, nodeName, ',')) 
    {
        if (getline(nodeTypesStream, nodeType, ','))
        {
            this->nodeNames.push_back(nodeName);
            this->nodeTypes.push_back(nodeType);
            this->numberOfNodes++;

            printf("\nNode %d has name %s with type %s\n", this->numberOfNodes, nodeName.c_str(), nodeType.c_str());
        }
        else
        {
            throw NoTypeDefinedForNodeException(nodeName + " has no type definition.");
        }
    }

    this->dataSourceFileStream.close();
}

OPCUANodeDataSource::~OPCUANodeDataSource()
{
    //this->dataSourceFileStream.close();
}

int OPCUANodeDataSource::getNumberOfNodes()
{
    return this->numberOfNodes;
}
