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


string OPCUANodeDataSource::getNodeName(int idx)
{
    return this->nodeNames.at(idx);
}


UA_DataTypeKind OPCUANodeDataSource::getNodeType(int idx)
{
    string s = nodeTypes.at(idx);

    if      (s.compare("INT64")     == 0)   { return UA_DATATYPEKIND_INT64;     }
    
    else if (s.compare("DATETIME")  == 0)   { return UA_DATATYPEKIND_DATETIME;  }
    
    else throw NoTypeDefinedForNodeException("'" + s + "' is not a valid/supported data type.");
}
