import numpy as np
from signal import signal, SIGINT
from sys import exit
from time import sleep

from lisa.lisa_switcher_bindings import callback_SSL_func, callback_SST_func, callback_SSS_S_func, lib_lisa_rcv
from lisa.lisa_configuration import SST_TAG_LEN, MAX_ODAS_SOURCES, N_BITS_INCOME_STREAM
BYTES_PER_SAMPLE = N_BITS_INCOME_STREAM//8


##########################
## callback definitions ##
##########################

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


@callback_SSS_S_func
def callback_SSS_S(n_bytes, x):
	shp = (n_bytes//BYTES_PER_SAMPLE//MAX_ODAS_SOURCES, MAX_ODAS_SOURCES)
	print("+++ Python SSS_S {} bytes shape {} in x={}".format(n_bytes, shp, x))
	buf = np.ctypeslib.as_array(x, shape=shp)
	print("extract buffer shape {} - first sample content {}".format(shp, [buf[0:3,ch] for ch in range(MAX_ODAS_SOURCES)]))
	
	
def handler(signal_received, frame):
    # Handle any cleanup here
    print('SIGINT or CTRL-C detected. Exiting..')
    exit(0)


if __name__ == '__main__':
	# x = np.array([20, 13, 8, 100, 1, 3], dtype=np.double)
	#lib_lisa_rcv.callback_SST(x, x.shape[0])
	signal(SIGINT, handler)
	lib_lisa_rcv.register_callback_SST(callback_SST)
	lib_lisa_rcv.register_callback_SSL(callback_SSL)
	lib_lisa_rcv.register_callback_SSS_S(callback_SSS_S)
	print('Running. Press CTRL-C to exit.')
	retval = lib_lisa_rcv.main_loop()
	
	print("Exit main loop {}".format(retval))