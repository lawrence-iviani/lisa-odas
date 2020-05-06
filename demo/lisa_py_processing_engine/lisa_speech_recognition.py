try:
    from queue import Queue, Full  # Python 3 import
except ImportError:
    from Queue import Queue, Full  # Python 2 import

import threading
import numpy as np
from signal import signal, SIGINT
from sys import exit
from time import sleep
import logging



import speech_recognition as sr
from lisa.lisa_switcher_bindings import callback_SSL_func, callback_SST_func, callback_SSS_S_func, lib_lisa_rcv
from lisa.lisa_configuration import SST_TAG_LEN, MAX_ODAS_SOURCES, SAMPLE_RATE_INCOME_STREAM, HOP_SIZE_INCOME_STREAM, N_BITS_INCOME_STREAM 
BYTES_PER_SAMPLE = N_BITS_INCOME_STREAM // 8

from lisa.utils import LOGGING_FORMATTER

# call logging after import of lisa.utils, it will set the formatting options 
logger = logging.getLogger(name="LISA Speech Recognition")
logger.setLevel(logging.DEBUG)

# Params for collecting queue from ODAS callbacks
MAX_QUEUE_SIZE = 10
SSL_queue = [Queue(maxsize=MAX_QUEUE_SIZE) for _q in range(MAX_ODAS_SOURCES)]
SST_queue = [Queue(maxsize=MAX_QUEUE_SIZE) for _q in range(MAX_ODAS_SOURCES)]
SSS_queue = [Queue(maxsize=MAX_QUEUE_SIZE) for _q in range(MAX_ODAS_SOURCES)]

# The processing queue
audio_queue = Queue()


##########################
## callback definitions ##
##########################
@callback_SSL_func
def callback_SSL(pSSL_struct):
	ssl_str = pSSL_struct[0]
	#msg = ["+++ Python SSL Struct ts={}".format(ssl_str.timestamp)]
	
	for i in range(0, MAX_ODAS_SOURCES):
		try:
			SSL_queue[i].put_nowait(ssl_str.src[i])
		except Full:
			# do nothing for now, perhaps extract the first to make space? Kind of circular queue behavior.....
			pass
			
		# Do something with the variable... (e.g calc DOA or should arrive from C program?)
		# Probably i should also consider other structure with pre processed data..
		# src = ssl_str.src[i]
		# x = src.x
		# y = src.y
		# z = src.z
		# E = src.E
		# msg.append("\n\tsrc[{}] E={} (x={},y={},z={})".format(i, src.E, src.x, src.y, src.z ))
		
	# msg = ''.join(msg)
	# print(msg)

@callback_SST_func
def callback_SST(pSST_struct):
	sst_str = pSST_struct[0]
	#msg = ["+++ Python SST Struct ts={}".format(sst_str.timestamp)]
	
	for i in range(0, MAX_ODAS_SOURCES):
		try:
			SST_queue[i].put_nowait(sst_str.src[i])
		except Full:
			# do nothing for now, perhaps extract the first to make space? Kind of circular queue behavior.....
			pass
			
		# Do something with the variable... (e.g calc DOA or should arrive from C program?)
		# Probably i should also consider other structure with pre processed data..
		# src = sst_str.src[i]
		# id = src.id
		# x = src.x
		# y = src.y
		# z = src.z
		# activity = src.activity
		# msg.append("\n\tsrc[{}] id({}) activity={} (x={},y={},z={})".format(i, id, src.activity, src.x, src.y, src.z ))
		
	# msg = ''.join(msg)
	# print(msg)


@callback_SSS_S_func
def callback_SSS_S(n_bytes, x):
	#print("+++ Python SSS_S {} bytes in x={}".format(n_bytes, x))
	shp = (n_bytes//BYTES_PER_SAMPLE//MAX_ODAS_SOURCES, MAX_ODAS_SOURCES)
	n_frames = shp[0]//HOP_SIZE_INCOME_STREAM  # I assume shp[0] is always a multiple integer, which in my understanding seems to be the case with odas
	buf = np.ctypeslib.as_array(x, shape=shp)
	for i in range(0, MAX_ODAS_SOURCES):
		try:
			for _fr in range(n_frames): # there could be more than one frame so put in queue with the expected length
				_idl = _fr*HOP_SIZE_INCOME_STREAM
				_idh = _fr*HOP_SIZE_INCOME_STREAM + HOP_SIZE_INCOME_STREAM
				_ch_buf = buf[_idl:_idh,i]
				# print("extract buffer source {} - frame {}. 3 samples  {} ...".format(i, _fr, _ch_buf[0:3]))
				SSS_queue[i].put_nowait(_ch_buf) # eventually_ch_buf this has to be transformed in bytes or store as a byte IO?
												 # manage fuell queue is requried?
		except Full:
			# do nothing for now, perhaps extract the first to make space? Kind of circular queue behavior.....
			pass
	
def handler(signal_received, frame):
    # Handle any cleanup here
    print('\nSIGINT or CTRL-C detected. Exiting..')
    exit(0)


def thread_start_odas_switcher():
	
	retval = lib_lisa_rcv.main_loop()
	print("Exit thread odas loop with {}".format(retval))
	

import time
def recognize_worker():
	# this runs in a background thread
	worker_id = 0
	while True:
		worker_id += 1
		actual_id = worker_id
		audio = audio_queue.get()  # retrieve the next audio processing job from the main thread
		if audio is None: break  # stop processing if the main thread is done 
		# TODO: add a pause here?? Profiling?
		logger.debug("{}- start recognize response in audio data, len is {}sec".format(actual_id, len(audio.frame_data)/(audio.sample_rate * audio.sample_width)))
		def _recognize(audio, func, tag, actual_id):	
			try:
				logger.debug("{}-{} thinks you said ".format(actual_id, tag))
				_start_time = time.time() 
				_response = func(audio)
				_end_time = time.time() - _start_time	
				logger.debug("{}-{}-{} response in {}s: |{}|".format("+-" *2, actual_id, tag, _end_time, _response))
			except sr.UnknownValueError:
				logger.warning("{}-Speech Recognition {} could not understand audio".format(actual_id, tag))
			except sr.RequestError as e:
				logger.error("{}-Could not request results {} recognition service {}".format(actual_id, tag, e))
			except Exception as e:
				logger.warning("Exception in _recognize{}-{}: {}".format(actual_id, tag, e))
		
		google_thread = threading.Thread(target=_recognize, args = (audio, r.recognize_google, 'google', actual_id))
		google_thread.daemon = True
		google_thread.start()

		sphinx_thread = threading.Thread(target=_recognize, args = (audio, r.recognize_sphinx, 'sphinx' , actual_id))
		sphinx_thread.daemon = True
		sphinx_thread.start()
			
		audio_queue.task_done()  # mark the audio processing job as completed in the queue


if __name__ == '__main__':

	signal(SIGINT, handler)
	lib_lisa_rcv.register_callback_SST(callback_SST)
	lib_lisa_rcv.register_callback_SSL(callback_SSL)
	lib_lisa_rcv.register_callback_SSS_S(callback_SSS_S)
	
	odas_thread = threading.Thread(target=thread_start_odas_switcher)
	odas_thread.daemon = True
	print('Running Odas thread receiver. Press CTRL-C to exit.')
	odas_thread.start()
	# should i start odas as well?

	recognize_thread = threading.Thread(target=recognize_worker)
	recognize_thread.daemon = True
	recognize_thread.start()

	r = sr.Recognizer()
	# I need to do the same with all the queues... or here i should have already only one voice as recognizer
	with sr.OdasRaw(callback_queue=SSS_queue[0], sample_rate=SAMPLE_RATE_INCOME_STREAM, chunk_size=HOP_SIZE_INCOME_STREAM, nbits=N_BITS_INCOME_STREAM) as source:
		while True:  # repeatedly listen for phrases and put the resulting audio on the audio processing job queue
			print("Main: audio queue, source: {}".format(source)) 
			listen_audio_data = r.listen(source)
			print("Main: audio queue, listen : {}".format(listen_audio_data)) 
			audio_queue.put(listen_audio_data)
			print("List of threads: {}".format(threading.enumerate()))

	
	print("Exit main loop")
	exit(0)