import numpy as np
import numpy.ctypeslib as npct
import ctypes
import os.path
from time import sleep
from signal import signal, SIGINT
from sys import exit

##################



SST_TAG_LEN = 20  # as in common.h
#This must be the same parameters as in defined in configuration ssl.nPots, the fix number of messages, stream that are transmitted.
MAX_ODAS_SOURCES = 4

pTag = ctypes.create_string_buffer(SST_TAG_LEN)
print(pTag)

	

libtest = npct.load_library('../../lib/liblisarcv', os.path.dirname(__file__))
print(libtest)
libtest.main_loop.restype = ctypes.c_int
libtest.main_loop.argtypes = None


##################
## callback_SSL ##
##################
# 1.
# struct SSL_src_struct {
	# double x;
	# double y;
	# double z;
	# double E;
# }; // SSL src 
class SSL_src_struct(ctypes.Structure):
    _fields_=[("x",ctypes.c_double),("y",ctypes.c_double),("z",ctypes.c_double), ("E",ctypes.c_double) ]

# 2.
# struct SSL_struct {
	# unsigned int timestamp;
	# SSL_src_struct src[MAX_ODAS_SOURCES]; // TODO, Max value or variable?
# }; // SSL struct message
class SSL_struct(ctypes.Structure):
    _fields_=[("timestamp",ctypes.c_uint), ("src", SSL_src_struct*MAX_ODAS_SOURCES)]

# 3.
# void callback_SSL(SSL_struct* data);	
callback_SSL_func = ctypes.CFUNCTYPE(
    None,            # return
	ctypes.POINTER(SSL_struct) # x
)

# 4.
# define SSL register and callback args and output
libtest.callback_SSL.restype = None  # with  C++ compiler be sure it is declared as extern "C"
libtest.callback_SSL.argtypes = [ctypes.POINTER(SSL_struct)]#[array_1d_double, .c_int]
libtest.register_callback_SSL.restype = None
libtest.register_callback_SSL.argtypes = [callback_SSL_func]

# 5.
@callback_SSL_func
def callback_SSL(pSSL_struct):
	ssl_str = pSSL_struct[0]
	msg = ["+++ Python SSL Struct ts={}".format(ssl_str.timestamp)]
	
	for i in range(0, MAX_ODAS_SOURCES):
		# Do something with the variable... (e.g calc DOA or should arrive from C program?)
		# Probably i should also consider other structure with pre processed data..
		src = ssl_str.src[i]
		x = src.x
		y = src.y
		z = src.z
		E = src.E
		msg.append("\n\tsrc[{}] E={} (x={},y={},z={})".format(i, src.E, src.x, src.y, src.z ))
		
	msg = ''.join(msg)
	print(msg)

##################
## callback_SST ##
##################
# IT IS FUNDAMENTAL DECLARE THE FIELDS IN THE SAME ORDER OF THE C STRUCTURE!
# 1.
# struct SST_src_struct {
	# unsigned int id;
	# char tag[SST_TAG_LEN]; // TODO, VERIFY THIS IS NOT BUGGY, NO IDEA WHAT IS THE MAX LEN!!!
	# double x;
	# double y;
	# double z;
	# double activity;
# }; // SST src 
class SST_src_struct(ctypes.Structure):
    #_fields_=[("id",ctypes.c_uint), ("tag", ctypes.c_char_p*(SST_TAG_LEN+1)), ("x",ctypes.c_double),("y",ctypes.c_double),("z",ctypes.c_double), ("activity",ctypes.c_double)]
	_fields_=[("id",ctypes.c_uint), ("tag", ctypes.c_char * SST_TAG_LEN), ("x",ctypes.c_double),("y",ctypes.c_double),("z",ctypes.c_double), ("activity",ctypes.c_double)]

# 2.
# struct SSL_struct {
	# unsigned int timestamp;
	# SSL_src_struct src[MAX_ODAS_SOURCES]; // TODO, Max value or variable?
# }; // SSL struct message
class SST_struct(ctypes.Structure):
    _fields_=[("timestamp",ctypes.c_uint), ("src", SST_src_struct*MAX_ODAS_SOURCES)] 

# 3.
# callback_SST(SST_struct* data);
callback_SST_func = ctypes.CFUNCTYPE(
    None,            # return
	ctypes.POINTER(SST_struct) # x
)

# 4.
# define SST register and callback args and output
libtest.callback_SST.restype = None  # with  C++ compiler be sure it is declared as extern "C"
libtest.callback_SST.argtypes = [ctypes.POINTER(SST_struct)]#[array_1d_double, .c_int]
libtest.register_callback_SST.restype = None
libtest.register_callback_SST.argtypes = [callback_SST_func]

# 5.
@callback_SST_func
def callback_SST(pSST_struct):
	sst_str = pSST_struct[0]
	msg = ["+++ Python SST Struct ts={}".format(sst_str.timestamp)]
	
	for i in range(0, MAX_ODAS_SOURCES):
		# Do something with the variable... (e.g calc DOA or should arrive from C program?)
		# Probably i should also consider other structure with pre processed data..
		src = sst_str.src[i]
		id = src.id
		x = src.x
		y = src.y
		z = src.z
		activity = src.activity
		msg.append("\n\tsrc[{}] id({}) activity={} (x={},y={},z={})".format(i, id, src.activity, src.x, src.y, src.z ))
		
	msg = ''.join(msg)
	print(msg)

####################
## callback_SSS_S ##
####################
array_1d_int16 = npct.ndpointer(dtype=np.int16, ndim=1 , flags='CONTIGUOUS')
# MAX_ODAS_SOURCES
bytes_per_sample = 2 # Short (16 bits) is used. to change the format must be in sync with

# 3.
# callback_SSS_S...
callback_SSS_S_func = ctypes.CFUNCTYPE(
    None,            # return
	ctypes.c_int, 
	ctypes.POINTER(ctypes.c_short)
)

# 4.
# define SSS register and callback args and output
libtest.callback_SSS_S.restype = None  # with  C++ compiler be sure it is declared as extern "C"
libtest.callback_SSS_S.argtypes = [ctypes.c_int,  array_1d_int16] #ctypes.c_void_p*65536]#* 65536]#[array_1d_double, .c_int]
libtest.register_callback_SSS_S.restype = None
libtest.register_callback_SSS_S.argtypes = [callback_SSS_S_func]

# 5.
@callback_SSS_S_func
def callback_SSS_S(n_bytes, x):
	print("+++ Python SSS_S {} bytes in x={}".format(n_bytes, x))
	shp = (n_bytes//bytes_per_sample//MAX_ODAS_SOURCES, MAX_ODAS_SOURCES)
	buf = np.ctypeslib.as_array(x, shape=shp)
	print("extract buffer shape {} - first sample content {}".format(shp, buf[0:3]))
	
	
def handler(signal_received, frame):
    # Handle any cleanup here
    print('SIGINT or CTRL-C detected. Exiting..')
    exit(0)


if __name__ == '__main__':
	# x = np.array([20, 13, 8, 100, 1, 3], dtype=np.double)
	#libtest.callback_SST(x, x.shape[0])
	signal(SIGINT, handler)
	libtest.register_callback_SST(callback_SST)
	libtest.register_callback_SSL(callback_SSL)
	libtest.register_callback_SSS_S(callback_SSS_S)
	print('Running. Press CTRL-C to exit.')
	retval = libtest.main_loop()
	
	print("Exit main loop {}".format(retval))