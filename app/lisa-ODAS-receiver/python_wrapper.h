
#ifndef PYTHON_WRAPPER
#define PYTHON_WRAPPER

#include "common.h"


// Must reflect the arguments defined in python callback
typedef void (*callback_SST_t)(
	SST_struct *pSST_struct
);
typedef void (*callback_SSL_t)(
	SSL_struct *pSSL_struct
);
typedef void (*callback_SSS_S_t)(
	unsigned int n_samples,
	void *raw_pcm_data // todo: add channels? SR? BITS?
);
typedef void (*callback_SSS_P_t)(
	unsigned int n_samples,
	void *raw_pcm_data // todo: add channels? SR? BITS?
);

// Functions callable from pyhton
//with  C++ compiler be sure it is declared as extern "C"
// Called by python to register the call back
EXTERN_C

// Start the main loop, should be called once
// ini name is the path to a ini file (see common.h)
int start_main_loop(char * ini_name);

// Check if there is a registered callback for item type c, see in common.h the enumeration ODAS_data_source
bool has_py_callback(int c);

// Based on the type of SSx different callbacks are available with different payload
void py_callback_message(int c, void * data); 
void py_callback_stream(int c, int n_samples, void * data); 

// Use internally and externally to check if a callback is registered
bool has_py_callback_SST();
bool has_py_callback_SSL();
bool has_py_callback_SSS_S();
bool has_py_callback_SSS_P();

// External py codes must register the callback
void register_callback_SST(callback_SST_t cbk);
void register_callback_SSL(callback_SSL_t cbk);
void register_callback_SSS_S(callback_SSS_S_t cbk);
void register_callback_SSS_S(callback_SSS_P_t cbk);

// Call back, called by C main loop (which call the callback registered)
void callback_SST(SST_struct* data);
void callback_SSL(SSL_struct* data);
void callback_SSS_S(void* data, unsigned int n_data );
void callback_SSS_P(void* data, unsigned int n_data );

EXTERN_C_END

#endif