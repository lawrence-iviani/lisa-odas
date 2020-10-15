

#include "python_wrapper.h"
#include "matrix_odas_receiver.h"


extern int parse_ini_file(char * ini_name);

/* int start_main_loop() {
	int status = parse_ini_file("/home/pi/dev/rhasspy-lisa-odas-hermes/lisa-odas/config/matrix-lisa/odas-rcv.ini");
	printf("parse_ini_file return: %", status);
	return main_loop();
} */


int start_main_loop(char * ini_name) {
	// TODO: to transform as argument options
	bool dump_pcm = false;
	bool use_overloop_led = false;
	
	printf("use_overloop_led %d - dump_pcm %d - parse_ini_file  %s\n",use_overloop_led, dump_pcm , ini_name);
	
	int status = parse_ini_file(ini_name);
	printf("parse_ini_file return: %d\n", status);
	return main_loop(dump_pcm, use_overloop_led);
} 

/* int start_main_loop() {
	char * ini_name = "/home/pi/dev/rhasspy-lisa-odas-hermes/lisa-odas/config/matrix-lisa/odas-rcv.ini";
	int status = parse_ini_file(ini_name);
	printf("parse_ini_file return: %", status);
	return main_loop();
} */

static callback_SST_t fcn_callback_SST = NULL;
static callback_SSL_t fcn_callback_SSL = NULL;
static callback_SSS_S_t fcn_callback_SSS_S = NULL;
static callback_SSS_S_t fcn_callback_SSS_P = NULL;

void py_callback_message(int c, void * data) {
	if (has_py_callback(c)) {
		switch(c) {
			case SSL:
				callback_SSL((SSL_struct*)data);
				break;
			case SST:
				callback_SST((SST_struct*)data);
				break;
			default:
				printf("py_callback_message: Not found valid callback MESSAGE for type %s", ODAS_data_source_str[c]);
				break;
		}
	} else {
		printf("py_callback_message: Not found callback at all for type %s", ODAS_data_source_str[c]);
	}
}

void py_callback_stream(int c, int n_samples, void * data) {
	if (has_py_callback(c)) {
		switch(c) {
			case SSS_S:
				callback_SSS_S(data, n_samples); 
				break;
			case SSS_P:
				callback_SSS_P(data, n_samples); 
				break;
			default:
				printf("py_callback_stream: Not found valid callback STREAM for type %s", ODAS_data_source_str[c]);
				break;
		}
	} else {
		printf("py_callback_stream: Not found callback at all for type %s", ODAS_data_source_str[c]);
	}
	
}

bool has_py_callback(int c) {
	switch(c) {
		case SSL:
			return has_py_callback_SSL();
		case SST:
			return has_py_callback_SST();
		case SSS_S:
			return has_py_callback_SSS_S();
		case SSS_P:
			return has_py_callback_SSS_P();
		default:
			return false;
	}
}

// SST
void register_callback_SST(callback_SST_t cbk) {
	fcn_callback_SST = cbk;
}

bool has_py_callback_SST() {
	return fcn_callback_SST==NULL ? false : true;
}

void callback_SST(SST_struct* data) {
	debug_print(DEBUG_PYTHON_WRAPPER,"[ts=%d]Calling fcn_callback_SST(&timestamp)@(0x%X)\n", data->timestamp, &data->timestamp);
	fcn_callback_SST(data);
} 

// SSL
void register_callback_SSL(callback_SSL_t cbk) {
	fcn_callback_SSL = cbk;
}

bool has_py_callback_SSL() {
	return fcn_callback_SSL==NULL ? false : true;
}

void callback_SSL(SSL_struct* data) {
	debug_print(DEBUG_PYTHON_WRAPPER,"[ts=%d]Calling fcn_callback_SSL(&timestamp)@(0x%X)\n", data->timestamp, &data->timestamp);
	fcn_callback_SSL(data);
} 

// SSS_S
void register_callback_SSS_S(callback_SSS_S_t cbk) {
	fcn_callback_SSS_S = cbk;
}

bool has_py_callback_SSS_S() {
	return fcn_callback_SSS_S==NULL ? false : true;
}

void callback_SSS_S(void* data, unsigned int n_data) {
	debug_print(DEBUG_PYTHON_WRAPPER,"Calling fcn_callback_SSS_S(&data)@(0x%X) with %d bytes\n", data, n_data);
	fcn_callback_SSS_S(n_data, data);
} 

// SSS_P
void register_callback_SSS_P(callback_SSS_P_t cbk) {
	fcn_callback_SSS_P = cbk;
}

bool has_py_callback_SSS_P() {
	return fcn_callback_SSS_P==NULL ? false : true;
}

void callback_SSS_P(void* data, unsigned int n_data) {
	debug_print(DEBUG_PYTHON_WRAPPER,"Calling fcn_callback_SSS_P(&data)@(0x%X) with %d bytes\n", data, n_data);
	fcn_callback_SSS_P(n_data, data);
} 
