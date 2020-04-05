
#include "json_decoder.h"

/* --------------------------------------------- */
/* ---------- INTERNAL DATA STRUCTURE ---------- */
/* --------------------------------------------- */
static unsigned int json_array_id = 0; // a counter to inspect json_array and set value in the proper src structure , not elegant but functional


// JSON PARSER AND DECODING SECTION
void json_parse_array(void* SSx_data ,json_object *jobj, char *key, unsigned int json_msg_id) {
 
  enum json_type type;
  json_object *jarray = jobj;
  if (key) {
    if (json_object_object_get_ex(jobj, key, &jarray) == false) {
      fprintf(stderr, "Error parsing json object\n");
      return;
    }
  }

  int arraylen = json_object_array_length(jarray);
  int i;
  json_object *jvalue;

  for (i = 0; i < arraylen; i++) {
    jvalue = json_object_array_get_idx(jarray, i);
    type = json_object_get_type(jvalue);

    if (type == json_type_array) {
      json_parse_array(SSx_data, jvalue, NULL, json_msg_id);
    } else if (type != json_type_object) {
    } else {
	  if (json_array_id>=MAX_ODAS_SOURCES) {
		  fprintf(stderr,"ODAS array too big, discarding json object %d\n",json_array_id);
	  } else {
		  debug_print(DEBUG_JSON, "Processing JSON array obj item: %d\n", json_array_id);
		  json_parse(SSx_data, jvalue, json_msg_id);
	  }
	  json_array_id++;
    }
  }
}

void json_parse(void* SSx_data , json_object *jobj, unsigned int json_msg_id) {
  enum json_type type;
  unsigned int count = 0;
  
  SSL_struct * SSL_data;
  SST_struct * SST_data;
  switch(json_msg_id) {
	  case SSL:
		SSL_data = (SSL_struct*)SSx_data;
		debug_print(DEBUG_JSON, "parsing %d message, decoding SSL_data(0x%X)", ODAS_data_source_str[json_msg_id], SSL_data);
		break;
	  case SST:
		SST_data = (SST_struct*)SSx_data;
		debug_print(DEBUG_JSON, "parsing %d message, decoding SST_data(0x%X)",  ODAS_data_source_str[json_msg_id], SST_data);
		break;
	  default:
	    printf("Unknown JSON type: %d, do nothing", json_msg_id);
	    return;
  }
    
  json_object_object_foreach(jobj, key, val) {
    type = json_object_get_type(val);
    switch (type) {
      case json_type_boolean:
        break;
      case json_type_double:
	    if (json_msg_id ==SSL) {
			if (!strcmp(key, "x")) {
			  SSL_data->src[json_array_id].x = json_object_get_double(val);
			  if(DEBUG_JSON) {printf("(0x%X)SSL_data->src[%d].x=%f - ", &SSL_data, json_array_id, SSL_data->src[json_array_id].x);}
			} else if (!strcmp(key, "y")) {
			  SSL_data->src[json_array_id].y = json_object_get_double(val);
			  if(DEBUG_JSON) {printf("(0x%X)SSL_data->src[%d].y=%f - ", &SSL_data, json_array_id, SSL_data->src[json_array_id].y);}
			} else if (!strcmp(key, "z")) {
			  SSL_data->src[json_array_id].z = json_object_get_double(val);
			  if(DEBUG_JSON) {printf("(0x%X)SSL_data->src[%d].z=%f - ", &SSL_data, json_array_id, SSL_data->src[json_array_id].z);}
			} else if (!strcmp(key, "E")) {
			  SSL_data->src[json_array_id].E = json_object_get_double(val);
			  if(DEBUG_JSON) {printf("(0x%X)SSL_data->src[%d].E=%f\n", &SSL_data, json_array_id, SSL_data->src[json_array_id].E);}
			}
		} else if (json_msg_id ==SST) {
			if (!strcmp(key, "x")) {
			  SST_data->src[json_array_id].x = json_object_get_double(val);
			  if(DEBUG_JSON) {printf("(0x%X)SST_data->src[%d].x=%f - ", &SST_data, json_array_id, SST_data->src[json_array_id].x);}
			} else if (!strcmp(key, "y")) {
			  SST_data->src[json_array_id].y = json_object_get_double(val);
			  if(DEBUG_JSON) {printf("(0x%X)SST_data->src[%d].y=%f - ", &SST_data, json_array_id, SST_data->src[json_array_id].y);}
			} else if (!strcmp(key, "z")) {
			  SST_data->src[json_array_id].z = json_object_get_double(val);
			  if(DEBUG_JSON) {printf("(0x%X)SST_data->src[%d].z=%f - ", &SST_data, json_array_id, SST_data->src[json_array_id].z);}
			} else if (!strcmp(key, "activity")) {
			  SST_data->src[json_array_id].activity = json_object_get_double(val);
			  if(DEBUG_JSON) {printf("(0x%X)SST_data->src[%d].activity=%f - ", &SST_data, json_array_id, SST_data->src[json_array_id].activity);}
			} 
		}
        count++;
        break;
      case json_type_int:
		if (json_msg_id ==SSL) {
			if (!strcmp(key, "timeStamp")) {
				SSL_data->timestamp = (unsigned int)json_object_get_int(val);
				if(DEBUG_JSON) {printf("----------------------------(0x%X)SSL_data->timestamp=%d - val is \n", &SSL_data, SSL_data->timestamp);}
			} 
		} else if (json_msg_id ==SST) {
			if (!strcmp(key, "timeStamp")) {
				SST_data->timestamp = (unsigned int)json_object_get_int(val);
				if(DEBUG_JSON) {printf("----------------------------(0x%X)SST_data->timestamp=%d - val is \n", &SST_data, SST_data->timestamp);}
			} else if (!strcmp(key, "id")) {
				SST_data->src[json_array_id].id = (unsigned int)json_object_get_int(val);
			}
		}
        break;
      case json_type_string:
	    if (json_msg_id ==SSL) {
			
		} else if (json_msg_id ==SST) {
			if (!strcmp(key, "tag")) {
				strncpy(SST_data->src[json_array_id].tag, json_object_get_string(val), SST_TAG_LEN);				
				SST_data->src[json_array_id].tag[SST_TAG_LEN] = '\0';
				if(DEBUG_JSON) {printf("(0x%X)SST_data->src[%d].tag=%s - ", &SST_data, json_array_id, SST_data->src[json_array_id].tag);}				
			}
		}
        break;
      case json_type_object:
        if (json_object_object_get_ex(jobj, key, &jobj) == false) {
          fprintf(stderr, "Error parsing json object\n");
          return;
        }
        json_parse(SSx_data, jobj, json_msg_id);
        break;
      case json_type_array:
	    json_array_id = 0;
		switch(json_msg_id) {
		  case SSL:
		    debug_print(DEBUG_JSON, "parsing array SSL data.src %d message, decoding SST_data(0x%X)",  SST_data);
			json_parse_array(SSL_data, jobj, key, json_msg_id);
			break;
		  case SST:
			json_parse_array(SST_data, jobj, key, json_msg_id);
			break;
		  default:
			break;
		}
        
        break;
    }
  }
}

