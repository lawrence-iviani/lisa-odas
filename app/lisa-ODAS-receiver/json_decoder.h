
#ifndef JSON_PARSER 
#define JSON_PARSER

#include <json.h>
#include "common.h"


void json_parse_array(json_object *jobj, char *key, unsigned int msg_id);
void json_parse(void* SSx_data , json_object *jobj, unsigned int msg_id); 

#endif