#include <iostream>
#include <iomanip>
#include <ctime>

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <sys/time.h>
#include <signal.h>

//#include <time.h>

#include "open62541.h" 

#include "datasource.h"
#include "OPCUANodeDataSource.h"

UA_Boolean running = true; 

#define SLEEP_TIME_MILLIS 900

int64_t init_time;



typedef struct NodeSpec {
  std::fstream m_data_in;
  UA_NodeId* m_node_ids;
  int* m_types;
  int m_qty_nodes;
} NodeSpec;



typedef struct LoopArg {
  UA_Server* m_server;
  NodeSpec* m_node_spec;
  int m_start_row;
} LoopArg;




static void stopHandler(int sig) {
  UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Stopping...");
  running = false;
}



uint64_t getTime() {
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return 1000000 * tv.tv_sec + tv.tv_usec;
}



static bool updateNode(UA_Server* p_server, UA_NodeId p_node_id, std::string* p_value, int p_type) {
  UA_Variant t_var; 
  UA_Variant_init(&t_var);

  bool t_ret = false;

  if (p_value == NULL || p_value->c_str() == NULL || strlen(p_value->c_str()) == 0) {
    
    UA_Variant_setScalarCopy(&t_var, NULL, &UA_TYPES[p_type]);
  
  } else {

    switch (p_type) {
    case UA_TYPES_INT64:
      {
        UA_Int64 t_val_i64 = atoll(p_value->c_str());
        UA_Variant_setScalarCopy(&t_var, &t_val_i64, &UA_TYPES[p_type]);
        t_ret = true;
        break;
      }
    case UA_TYPES_DATETIME:
      {
        std::string t_str_in  = *p_value;
        const char* t_cstr_in = t_str_in.c_str();

        int t_fracsecs = 0;
    		int length = strlen(t_cstr_in);

	    	char* x = (char*)strchr(t_cstr_in,'.');

    		if (x != NULL) {

			    x++;
    			int t_decimal_places = length - (x - t_cstr_in);

		    	t_fracsecs = atoi(x);
			    for (int i=0; i<(7-t_decimal_places); i++) t_fracsecs = t_fracsecs * 10;
		    }		
        
        std::string t_str_fullsecs_only = t_str_in.substr(0,19);

        int year;
        int month;
        int day;
        int hour;
        int min;
        int sec;
        //const char * str = "2014-06-10 20:05:57";
        const char* str = t_str_fullsecs_only.c_str();

        time_t epoch;

        if (sscanf(str, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &min, &sec) != 6)
        {
            throw std::runtime_error("Error parsing date '" + t_str_in + "'");
        }

        // sscanf() returns the number of elements scanned, so 6 means the string had all 6 elements.
        // Else it was probably malformed.
        UA_DateTimeStruct t_uadts;
        t_uadts.nanoSec   = t_fracsecs % 10;
        t_uadts.microSec  = (t_fracsecs / 10) % 1000;
        t_uadts.milliSec  = (t_fracsecs / 10000) % 1000;
        t_uadts.sec       = sec;
        t_uadts.min       = min;
        t_uadts.hour      = hour;
        t_uadts.day       = day;
        t_uadts.month     = month;
        t_uadts.year      = year;

        //printf("  result: %d-%d-%d %d:%d:%d . %d %d %d\n", 
        //    year, month, day, hour, min, sec, 
        //    t_uadts.milliSec, t_uadts.microSec, t_uadts.nanoSec);

        UA_DateTime t_val_dt = UA_DateTime_fromStruct(t_uadts);
        UA_Variant_setScalarCopy(&t_var, &t_val_dt, &UA_TYPES[p_type]);
        t_ret = true;
        break;

      }
    default:
      {
        throw std::runtime_error("Undeclared data type");
      }
    }

  }
  
  UA_Server_writeValue(p_server, p_node_id, t_var);

  return t_ret;
}



static bool updateNodes(UA_Server* p_server, NodeSpec* p_node_spec) {
  bool ret = true;

  std::vector<std::string> t_row;
  std::string t_line, t_word;

  try {

    std::getline(p_node_spec->m_data_in, t_line);
    std::stringstream t_string_stream(t_line);

    while (std::getline(t_string_stream, t_word, ',')) {
      t_row.push_back(t_word);
    }

  } catch (std::exception &e) {
    throw std::runtime_error("Error reading line elements : " + string(e.what()));
  }

  if (t_row.size() < p_node_spec->m_qty_nodes) {
    throw std::runtime_error("Insufficient data to update all nodes");
  }

  int i = 0;

  try {

    for (i=0; i<p_node_spec->m_qty_nodes; i++) {
      UA_NodeId* t_node_id = &p_node_spec->m_node_ids[i];
      int t_type = p_node_spec->m_types[i];
      ret = ret && updateNode(p_server, p_node_spec->m_node_ids[i], &t_row[i], p_node_spec->m_types[i]);
    }

  } catch (std::exception &e) {
    throw std::runtime_error("Error updating node value at column idx " + to_string(i) + " : " + string(e.what()));
  }

  return ret;
}



static void* loop(void* p_ptr) {
  LoopArg* t_loop_arg = (LoopArg*)p_ptr;
 
  int       t_utime;
  int       t_utime_int = SLEEP_TIME_MILLIS * 1000;
  int       t_utime_min = 100;
  uint64_t  t_last_time = getTime();
  uint64_t  t_cur_time;

  int t_row = t_loop_arg->m_start_row + 1;
  while (running == 1) {
    try {
      updateNodes(t_loop_arg->m_server, t_loop_arg->m_node_spec);
    } catch (exception &e) {
      string msg = "Error updating nodes at row " + to_string(t_row) + " : " + string(e.what());
      //throw exception(msg);
      UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, msg.c_str());
    }
    t_row++;

    t_cur_time = getTime();
    t_utime = t_utime_int + t_last_time - t_cur_time;
    t_last_time = t_cur_time;
    if (t_utime < t_utime_min) t_utime = t_utime_min;
    usleep(t_utime);
  }

  return 0;
}



static void NodeSpec_init(NodeSpec* p_node_spec, int p_qty_nodes) {
  p_node_spec->m_qty_nodes = p_qty_nodes;
  p_node_spec->m_types = (int*)malloc(p_qty_nodes*sizeof(int));
  p_node_spec->m_node_ids = (UA_NodeId*)malloc(p_qty_nodes*sizeof(UA_NodeId));
}



static void NodeSpec_deleteMembers(NodeSpec* p_node_spec) {
  for (int i=0; i<p_node_spec->m_qty_nodes; i++) {
    UA_NodeId_deleteMembers(&p_node_spec->m_node_ids[i]);
  }
  free(p_node_spec->m_node_ids);
  free(p_node_spec->m_types);
  //free(p_node_spec);
}



static UA_String* getNodeString(UA_NodeId* p_node_id) {
  return &p_node_id->identifier.string;
}



static bool createNode(UA_Server* p_server, UA_NodeId* p_node_id, int* p_type) {
  char* t_node_string = (char*)getNodeString(p_node_id)->data;
  UA_QualifiedName t_node_name = UA_QUALIFIEDNAME(1, t_node_string);
  UA_VariableAttributes t_attr = UA_VariableAttributes_default;
  t_attr.description = UA_LOCALIZEDTEXT((char*)"en_US", t_node_string);
  t_attr.displayName = UA_LOCALIZEDTEXT((char*)"en_US", t_node_string);
  t_attr.dataType = UA_TYPES[*p_type].typeId;
  //UA_Variant_setScalarCopy(&attr.value, &init_value, &UA_TYPES[p_type]);
  UA_Server_addVariableNode(p_server, *p_node_id,
			    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
			    UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
			    t_node_name, UA_NODEID_NULL, t_attr, NULL, NULL);
  return true;
}



static bool setupDataModel(UA_Server* p_server, OPCUANodeDataSource* nodeDataSource, NodeSpec* p_node_spec) {
  bool t_ret = true;

  NodeSpec_init(p_node_spec, nodeDataSource->getNumberOfNodes());

  UA_NodeId* t_node_ids = p_node_spec->m_node_ids;

  int t_namespace = 1;

  int i = 0;

  try {

    //t_node_ids[0]   = UA_NODEID_STRING_ALLOC(t_namespace, "TimeStamp");  
    //p_node_spec->m_types[0] = UA_TYPES_DATETIME;

    for (i=0; i<p_node_spec->m_qty_nodes; i++) {

      t_node_ids[i] = UA_NODEID_STRING_ALLOC(t_namespace, nodeDataSource->getNodeName(i).c_str());
      p_node_spec->m_types[i] = nodeDataSource->getNodeType(i);

      t_ret = t_ret && createNode(p_server, &p_node_spec->m_node_ids[i], &p_node_spec->m_types[i]);

    }

  } catch (std::exception& e) {
    throw std::runtime_error("Error encountered setting up data model at node " + to_string(i) + " : " + string(e.what()));
  }

  return t_ret;
}



static bool openDataFile(NodeSpec* p_node_spec, const char* p_filename, int p_throw_away_lines = 3) {
  try {

    p_node_spec->m_data_in.open(p_filename, std::ios::in); 
    std::string t_line;

    for (int i=0; i<p_throw_away_lines; i++) {
      std::getline(p_node_spec->m_data_in, t_line); // throw away
    }

    return true;

  } catch (...) {

    return false;

  }
}



int main(int argc, char** argv) {
  
  OPCUANodeDataSource* nodeDataSource;

  char* t_filename;

  if (!findNextFile()) 
  {
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Could not find files on folder %s.", "/app/data/");
    return(1);
  }

  t_filename = getCurrentFile();
  UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Using file %s", t_filename);

  try
  {
    nodeDataSource = new OPCUANodeDataSource(t_filename);
  }
  catch(NoTypeDefinedForNodeException& e)
  {
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Error reading node type in file %s: %s", t_filename, e.what());

    return(1);
  }
  catch(exception& e) 
  {
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Error when parsing file %s: %s", t_filename, e.what());

    return(1);
  }

  // For now
  int t_start_row = 3;
  
  signal(SIGINT, stopHandler);
  signal(SIGTERM, stopHandler); 

  UA_Server *server = UA_Server_new(); 
  UA_ServerConfig *config = UA_Server_getConfig(server);

  config->verifyRequestTimestamp = UA_RULEHANDLING_WARN;
  UA_ServerConfig_setMinimal(config, 4840, NULL); // certificate=NULL
  //UA_ServerConfig_setMinimal(config, 4899, 0);

  // EDIT
  //UA_DurationRange interval_limits = { 0.0, 5.0 };
  //config->publishingIntervalLimits = interval_limits;
  //config->samplingIntervalLimits = interval_limits;

  NodeSpec t_node_spec;

  try
  {
    setupDataModel(server, nodeDataSource, &t_node_spec);
  }
  catch(NoTypeDefinedForNodeException& e)
  {
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Error reading node type in file %s: %s", t_filename, e.what());

    return(1);
  }
  catch(exception& e) 
  {
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Error setting up data model for file %s: %s", t_filename, e.what());

    return(1);
  }

  delete nodeDataSource;

  if (t_node_spec.m_qty_nodes == 0) {
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Zero nodes configured. Terminating.");

    return(1); 
  }

  UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Proceeding with %d node(s) configured.", t_node_spec.m_qty_nodes);

  LoopArg t_loop_arg;
  t_loop_arg.m_node_spec = &t_node_spec;
  t_loop_arg.m_server = server;
  t_loop_arg.m_start_row = t_start_row;

  openDataFile(&t_node_spec, t_filename, t_start_row);

  pthread_t thread;
  int ret = 0;
  if (pthread_create(&thread, NULL, loop, &t_loop_arg)) {
    fprintf(stderr, "Thread error: %d\n", ret);
    exit(EXIT_FAILURE);
  }

  UA_StatusCode retval = UA_Server_run(server, &running); 

  UA_Server_delete(server);

  NodeSpec_deleteMembers(&t_node_spec);

  return (int)retval;
}
