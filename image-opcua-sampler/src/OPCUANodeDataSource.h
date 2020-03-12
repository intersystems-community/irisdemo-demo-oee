#ifndef OPCUANodeDataSource_CLASS
#define OPCUANodeDataSource_CLASS

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "open62541.h" 
#include <exception>

using namespace std;

class OPCUANodeDataSource
{
    private:
        ifstream dataSourceFileStream;
        int numberOfNodes;
        int OPCUANamespace;
        vector<string> nodeNames;
        vector<string> nodeTypes;
        // nodeTypes = (int*)malloc(p_qty_nodes*sizeof(int));
        // p_node_spec->m_node_ids = (UA_NodeId*)malloc(p_qty_nodes*sizeof(UA_NodeId));

    public:
        OPCUANodeDataSource(string dataSourceFileName);
        ~OPCUANodeDataSource();

        int getNumberOfNodes();

        string getNodeName(int idx);
        UA_DataTypeKind getNodeType(int idx);

};

class NoTypeDefinedForNodeException : public exception 
{
    private:
        string detail;

    public:
    	NoTypeDefinedForNodeException(char* detail)
        {
            this->detail.assign(detail);
        }

    	NoTypeDefinedForNodeException(const string detail)
        {
            this->detail.assign(detail);
        }

        const char* what() const throw ()
        {
            return this->detail.c_str();
        }
};

#endif