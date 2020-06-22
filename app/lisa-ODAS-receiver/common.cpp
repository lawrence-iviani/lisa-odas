
#include <iniparser.h>
#include "common.h"

int parse_ini_file(char * ini_name)
{
    dictionary  *   ini ;

    ini = iniparser_load(ini_name);
    if (ini==NULL) {
        fprintf(stderr, "cannot parse file: %s\n", ini_name);
		printf("cannot parse file: %s\n", ini_name);
        return -1 ;
    }
    
	// ENERGY_COUNT = iniparser_getint(ini, "led:energy_count",36);
	MAX_VALUE = iniparser_getint(ini, "led:max_value",150);
	INCREMENT = iniparser_getint(ini, "led:increment",20);
	DECREMENT = iniparser_getint(ini, "led:decrement",2);
	MIN_THRESHOLD = iniparser_getint(ini, "led:min_threshold",5);
	MAX_BRIGHTNESS = iniparser_getint(ini, "led:max_brightness",220	);
	
	SLEEP_ACCEPT_LOOP = iniparser_getdouble(ini, "internal:sleep_accept_loop",0.5);
	MAX_EMPTY_MESSAGE = iniparser_getint(ini, "internal:max_empty_message",200);
	// RECV_PCM_BUFFERS = iniparser_getint(ini, "internal:recv_pcm_buffers",4);
	// MAX_ODAS_SOURCES = iniparser_getint(ini, "internal:dump_pcm",4);
	DUMP_PCM = iniparser_getboolean(ini, "internal:dump_pcm", 0);
	// NUM_OF_ODAS_DATA_SOURCES = iniparser_getint(ini, "internal:num_of_odas_data_sources",4 );
	MAX_RECV_BACKLOG = iniparser_getint(ini, "internal:max_recv_backlog",1);
	
	// Raw wave data stream from ODAS
	// as defined in SSS module in configuration sss.separated|postfiltered
	SSS_SAMPLERATE =  iniparser_getint(ini, "odas:sss_samplerate",16000);
	// SSS_HOPSIZE = iniparser_getint(ini, "odas:sss_hopsize",128);
	// SSS_BITS = iniparser_getint(ini, "odas:sss_bits",16);
	
	DEBUG_CONNECTION = iniparser_getint(ini, "debug:debug_connection", 0);
	DEBUG_DOA = iniparser_getint(ini, "debug:debug_doa", 0);
	DEBUG_JSON = iniparser_getint(ini, "debug:debug_json", 0);
	DEBUG_INCOME_MSG = iniparser_getint(ini, "debug:debug_income_msg", 0);
	DEBUG_DECODE = iniparser_getint(ini, "debug:debug_decode", 0);
	DEBUG_DUMP_FILES = iniparser_getint(ini, "debug:debug_dump_files", 0);
	DEBUG_PYTHON_WRAPPER = iniparser_getint(ini, "debug:debug_python_wrapper", 0);
	DEBUG_DUMP_FILES = iniparser_getboolean(ini, "debug:debug_dump_files", 0);
	DEBUG_PYTHON_WRAPPER = iniparser_getint(ini, "debug:debug_python_wrapper", 0);
	PRINT_DETECTION = iniparser_getboolean(ini, "debug:print_detection", 0);
	PRINT_MIN_DETECTION_SSL_E = iniparser_getdouble(ini, "debug:print_min_detection_ssl_e", 0.2);
		
	//iniparser_dump(ini, stdout);

    return 0 ;
}