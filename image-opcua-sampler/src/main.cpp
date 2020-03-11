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
        UA_Int64 t_val_i64 = atoi(p_value->c_str());
        UA_Variant_setScalarCopy(&t_var, &t_val_i64, &UA_TYPES[p_type]);
        t_ret = true;
        break;
      }
    case UA_TYPES_DATETIME:
      {
        std::string t_str_in  = *p_value;
        const char* t_cstr_in = t_str_in.c_str();

        //printf("Date: %s\n", t_cstr_in);
         
        int t_fracsecs = 0;
    		int length = strlen(t_cstr_in);

        //printf("  Len: %d\n", length);

	    	char* x = (char*)strchr(t_cstr_in,'.');

        //printf("  x: %s\n", x);

    		if (x != NULL) {

			    x++;
    			int t_decimal_places = length - (x - t_cstr_in);

		    	t_fracsecs = atoi(x);
			    for (int i=0; i<(7-t_decimal_places); i++) t_fracsecs = t_fracsecs * 10;
		    }		
        
        //printf("  fracsecs: %d\n", t_fracsecs);

        std::string t_str_fullsecs_only = t_str_in.substr(0,19);

        //printf("  rest: %s\n", t_str_fullsecs_only.c_str());

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
            // ERROR
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
        break;
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

  std::getline(p_node_spec->m_data_in, t_line);
  std::stringstream t_string_stream(t_line);

  while (std::getline(t_string_stream, t_word, ',')) {
    t_row.push_back(t_word);
  }

  for (int i=0; i<p_node_spec->m_qty_nodes; i++) {
    UA_NodeId* t_node_id = &p_node_spec->m_node_ids[i];
    int t_type = p_node_spec->m_types[i];
    ret = ret && updateNode(p_server, p_node_spec->m_node_ids[i], &t_row[i], p_node_spec->m_types[i]);
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

  int i = 0;
  while (running == 1) {
    updateNodes(t_loop_arg->m_server, t_loop_arg->m_node_spec);

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
  t_attr.description = UA_LOCALIZEDTEXT("en_US", t_node_string);
  t_attr.displayName = UA_LOCALIZEDTEXT("en_US", t_node_string);
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
  t_node_ids[0]   = UA_NODEID_STRING_ALLOC(t_namespace, "TimeStamp");
  t_node_ids[1]   = UA_NODEID_STRING_ALLOC(t_namespace, "MT1_1");
  t_node_ids[2]   = UA_NODEID_STRING_ALLOC(t_namespace, "MT1_2");
  t_node_ids[3]   = UA_NODEID_STRING_ALLOC(t_namespace, "MT1_3");
  t_node_ids[4]   = UA_NODEID_STRING_ALLOC(t_namespace, "MT1_4");
  t_node_ids[5]   = UA_NODEID_STRING_ALLOC(t_namespace, "MT1_5");
  t_node_ids[6]   = UA_NODEID_STRING_ALLOC(t_namespace, "MT1_6");
  t_node_ids[7]   = UA_NODEID_STRING_ALLOC(t_namespace, "MT1_7");
  t_node_ids[8]   = UA_NODEID_STRING_ALLOC(t_namespace, "MT4_1");
  t_node_ids[9]   = UA_NODEID_STRING_ALLOC(t_namespace, "MT4_2");
  t_node_ids[10]  = UA_NODEID_STRING_ALLOC(t_namespace, "MT4_3");
  t_node_ids[11]  = UA_NODEID_STRING_ALLOC(t_namespace, "MT4_4");
  t_node_ids[12]  = UA_NODEID_STRING_ALLOC(t_namespace, "MT4_5");
  t_node_ids[13]  = UA_NODEID_STRING_ALLOC(t_namespace, "MT4_6");
  t_node_ids[14]  = UA_NODEID_STRING_ALLOC(t_namespace, "MT4_7");
  t_node_ids[15]  = UA_NODEID_STRING_ALLOC(t_namespace, "MachineStatus");
  t_node_ids[16]  = UA_NODEID_STRING_ALLOC(t_namespace, "RECEPTNAAM1");
  t_node_ids[17]  = UA_NODEID_STRING_ALLOC(t_namespace, "RECEPTNAAM2");
  t_node_ids[18]  = UA_NODEID_STRING_ALLOC(t_namespace, "RECEPTNAAM3");
  t_node_ids[19]  = UA_NODEID_STRING_ALLOC(t_namespace, "RECEPTNAAM4");
  t_node_ids[20]  = UA_NODEID_STRING_ALLOC(t_namespace, "RECEPTNAAM5");
  t_node_ids[21]  = UA_NODEID_STRING_ALLOC(t_namespace, "VDoek");
  t_node_ids[22]  = UA_NODEID_STRING_ALLOC(t_namespace, "VRecept");
  t_node_ids[23]  = UA_NODEID_STRING_ALLOC(t_namespace, "WerkOrder");
  t_node_ids[24]  = UA_NODEID_STRING_ALLOC(t_namespace, "Yards");
  
  p_node_spec->m_types[0] = UA_TYPES_DATETIME;
  for (int i=1; i<nodeDataSource->getNumberOfNodes(); i++) {
    p_node_spec->m_types[i] = UA_TYPES_INT64;
  }

  for (int i=0; i<nodeDataSource->getNumberOfNodes(); i++) {
    t_ret = t_ret && createNode(p_server, &p_node_spec->m_node_ids[i], &p_node_spec->m_types[i]);
  }

  return t_ret;
}



static bool openDataFile(NodeSpec* p_node_spec, const char* p_filename, int p_throw_away_lines = 3) {
  p_node_spec->m_data_in.open(p_filename, std::ios::in); 
  std::string t_line;

  for (int i=0; i<p_throw_away_lines; i++) {
    std::getline(p_node_spec->m_data_in, t_line); // throw away
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

  setupDataModel(server, nodeDataSource, &t_node_spec);

  delete nodeDataSource;

  LoopArg t_loop_arg;
  t_loop_arg.m_node_spec = &t_node_spec;
  t_loop_arg.m_server = server;

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
