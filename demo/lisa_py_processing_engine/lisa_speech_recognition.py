try:
    from queue import Queue, Full  # Python 3 import
except ImportError:
    from Queue import Queue, Full  # Python 2 import

import time
import threading
import numpy as np
from signal import signal, SIGINT
from sys import exit
from time import sleep
import logging
from _collections import deque

import speech_recognition as sr
from lisa.lisa_switcher_bindings import callback_SSL_func, callback_SST_func, callback_SSS_S_func, lib_lisa_rcv
from lisa.lisa_switcher_bindings import SSL_struct, SSL_src_struct, SST_struct, SST_src_struct
from lisa.lisa_configuration import SST_TAG_LEN, MAX_ODAS_SOURCES, SAMPLE_RATE_INCOME_STREAM, HOP_SIZE_INCOME_STREAM, \
    N_BITS_INCOME_STREAM

BYTES_PER_SAMPLE = N_BITS_INCOME_STREAM // 8

from lisa.utils import LOGGING_FORMATTER

# call logging after import of lisa.utils, it will set the formatting options
logger = logging.getLogger(name="LISA Speech Recognition")
logger.setLevel(logging.DEBUG)

# Specific Processing Parameters
RECOGNITION_METHOD = 'all'  # 'sphinx', 'all' , 'google'
LANGUAGE = 'de-DE'  # options: 'en-US' 'it-IT' 'de-DE'
# TODO: wake up word
# Params for collecting queue from ODAS callbacks
MAX_QUEUE_SIZE = 10
MEDIAN_WEIGHTS = [1/4, 1/2, 1/4]  # Set to none to skip, apply a median filter
# GLOBAL VARIABLES USED TO SHARE INFORMATION AMONG THREADS
SSL_queue = [deque(maxlen=len(MEDIAN_WEIGHTS)) for _q in range(MAX_ODAS_SOURCES)]
SST_queue = [deque(maxlen=len(MEDIAN_WEIGHTS)) for _q in range(MAX_ODAS_SOURCES)]
SSL_latest = [None for _q in range(MAX_ODAS_SOURCES)]
SST_latest = [None for _q in range(MAX_ODAS_SOURCES)]
SSS_queue = [Queue(maxsize=MAX_QUEUE_SIZE) for _q in range(MAX_ODAS_SOURCES)]

##########################
## callback definitions ##
##########################
from copy import deepcopy

@callback_SSL_func
def callback_SSL(pSSL_struct):
    ssl_str = pSSL_struct[0]
    # msg = ["+++ Python SSL Struct ts={}".format(ssl_str.timestamp)]
    # TODO: use timestamp for checking insertion??
    for i in range(0, MAX_ODAS_SOURCES):
        try:
            if MEDIAN_WEIGHTS is None:
                # Calculate the weighted median filter
                SSL_latest[i] = (ssl_str.timestamp / 100.0, deepcopy(ssl_str.src[i]))
            else:
                # Calculate the weighted median filter
                SSL_queue[i].append((ssl_str.timestamp / 100.0, deepcopy(ssl_str.src[i])))
                x = y = z = E = ts = 0.0
                for ii in range(len(SSL_queue[i])):
                    w = MEDIAN_WEIGHTS[ii]
                    ts = ts + SSL_queue[i][ii][0] * w
                    x = x + SSL_queue[i][ii][1].x * w
                    y = y + SSL_queue[i][ii][1].y * w
                    z = z + SSL_queue[i][ii][1].z * w
                    E = E + SSL_queue[i][ii][1].E * w
                SSL_latest[i] = (ts, SSL_src_struct(x=x,y=y,z=z,E=E))
            # print("SSL_latest[{}]: {}".format(i, SSL_latest[i]))
        except Full:
            logger.warning("SSL queue is Full, this should not happen with deque")
            pass


@callback_SST_func
def callback_SST(pSST_struct):
    sst_str = pSST_struct[0]
    for i in range(0, MAX_ODAS_SOURCES):
        try:
            if MEDIAN_WEIGHTS is None:
                # Calculate the weighted median filter
                SST_latest[i] = (sst_str.timestamp / 100.0, deepcopy(sst_str.src[i]))
            else:
                # Calculate the weighted median filter
                SST_queue[i].append((sst_str.timestamp / 100.0, deepcopy(sst_str.src[i])))
                x = y = z = activity = ts = 0.0
                id = []
                for ii in range(len(SST_queue[i])):
                    w = MEDIAN_WEIGHTS[ii]
                    ts = ts + SST_queue[i][ii][0] * w
                    x = x + SST_queue[i][ii][1].x * w
                    y = y + SST_queue[i][ii][1].y * w
                    z = z + SST_queue[i][ii][1].z * w
                    activity = activity + SST_queue[i][ii][1].activity * w
                    id.append(SST_queue[i][ii][1].z)
                id = np.argmax(np.bincount(id))  # The more probable
                SST_latest[i] = (ts, SST_src_struct(x=x,y=y,z=z,activity=activity, id=id, tag=SST_queue[i][-1][1].tag)) # For tag take the latest inserted
            #print("SSL_latest[{}]: {}".format(i, SST_latest[i]))

        except Full:
            logger.warning("SST queue is Full, this should not happen with deque")
            pass


@callback_SSS_S_func
def callback_SSS_S(n_bytes, x):
    """"
    Called by the relative odas switcher stream, save in the proper SSS_queue[] the received data.
    """
    # print("+++ Python SSS_S {} bytes in x={}".format(n_bytes, x))
    shp = (n_bytes // BYTES_PER_SAMPLE // MAX_ODAS_SOURCES, MAX_ODAS_SOURCES)
    n_frames = shp[0] // HOP_SIZE_INCOME_STREAM  # I assume shp[0] is always a multiple integer, which in my understanding seems to be the case with odas
    buf = np.ctypeslib.as_array(x, shape=shp)
    for i in range(0, MAX_ODAS_SOURCES):
        try:
            for _fr in range(n_frames):  # there could be more than one frame so put in queue with the expected length
                _idl = _fr * HOP_SIZE_INCOME_STREAM
                _idh = _fr * HOP_SIZE_INCOME_STREAM + HOP_SIZE_INCOME_STREAM
                _ch_buf = buf[_idl:_idh, i]
                # print("extract buffer source {} - frame {}. 3 samples  {} ...".format(i, _fr, _ch_buf[0:3]))
                SSS_queue[i].put_nowait(
                    _ch_buf)  # eventually_ch_buf this has to be transformed in bytes or store as a byte IO?
            # manage fuell queue is requried?
        except Full:
            logger.warning("SSS_S receiving Queue is Full, skipping frame (TODO: lost for now, change in deque!)")
            # do nothing for now, perhaps extract the first to make space? Kind of circular queue behavior.....
            pass


def handler(signal_received, frame):
    # Handle any cleanup here
    print('\nSIGINT or CTRL-C detected. Exiting with 0 ...')
    exit(0)


def thread_start_odas_switcher():
    import os
    retval = lib_lisa_rcv.main_loop()
    print("Exit thread odas loop with {}".format(retval))
    threading.main_thread()  # _thread.interrupt_main()
    os._exit(retval)  # exit the main thread as well, brutal but it works if the odas dies (the entire app is switched off)


def recognize_worker(audio_queue, recognizer):
    # this runs in a background thread
    worker_id = 0
    while True:
        worker_id += 1
        actual_id = worker_id
        audio = audio_queue.get()  # retrieve the next audio processing job from the main thread
        if audio is None: break  # stop processing if the main thread is done
        # TODO: add a pause here?? Profiling?
        logger.debug("{}- start recognize response in audio data, len is {}sec".format(actual_id,
                                                                                       len(audio.frame_data) / (
                                                                                               audio.sample_rate * audio.sample_width)))

        def _recognize(audio, func, tag, actual_id, language=None):
            try:
                logger.debug("{}-{} start processing ".format(actual_id, tag))
                _start_time = time.time()
                if language is None:
                    _response = func(audio,)
                else:
                    _response = func(audio, language=language)
                _end_time = time.time() - _start_time
                logger.info("{}-{}-{} speech len {}s recognized in {}s: |{}|".format("+-" * 2, actual_id, tag,
                                                                                     audio.length, _end_time,
                                                                                     _response))
            except sr.UnknownValueError:
                logger.warning("{}-Speech Recognition {} could not understand audio".format(actual_id, tag))
            except sr.RequestError as e:
                logger.error("{}-Could not request results {} recognition service {}".format(actual_id, tag, e))
            except Exception as e:
                logger.warning("Exception in _recognize{}-{}: {}".format(actual_id, tag, e))

        if RECOGNITION_METHOD == 'google' or RECOGNITION_METHOD == 'all':  # 'sphinx', 'all'
            google_thread = threading.Thread(target=_recognize,
                                             args=(audio, recognizer.recognize_google, 'google', actual_id, LANGUAGE))
            google_thread.daemon = True
            google_thread.start()
        if RECOGNITION_METHOD == 'sphinx' or RECOGNITION_METHOD == 'all':
            sphinx_thread = threading.Thread(target=_recognize,
                                             args=(audio, recognizer.recognize_sphinx, 'sphinx', actual_id))
            sphinx_thread.daemon = True
            sphinx_thread.start()

        audio_queue.task_done()  # mark the audio processing job as completed in the queue


def source_listening(source_id):
    """"
    Listen to data from the callback queue (a queue populated by a callback in another thread), identify activity
    (this might be redundant because activity is identified by odas), and then recognize in a different thread
    (speech recognition takes time, so in order to continue to listen i have split this two. the idea is to go for
    a pipeline processing via enriching the actual AudioData class)

    TODO or ideas:
    - split activity detection and listening phase in recognizer -> to add other processing in between e.g. speaker identification
    - lookahead for speech activity detection?
    """

    def _get_postion_message():
        return {"SST": SST_latest[source_id], "SSL":  SSL_latest[source_id]}

    # The processing queue between a recognizer thread and received data identified as speech
    # This process is a sequetial  job with a long execution time , TODO: a possible check or maxlen could be added, for safety
    recognizer_queue = Queue()
    r = sr.Recognizer(sphinx_language=LANGUAGE)
    sl = sr.SpeechListener()
    recognize_thread = threading.Thread(target=recognize_worker, args=(recognizer_queue, r,))
    recognize_thread.daemon = True
    recognize_thread.start()
    with sr.OdasRaw(audio_queue=SSS_queue[source_id], sample_rate=SAMPLE_RATE_INCOME_STREAM,
                    chunk_size=HOP_SIZE_INCOME_STREAM, nbits=N_BITS_INCOME_STREAM) as source:
        while True:  # repeatedly listen for phrases and put the resulting audio on the audio processing job queue
            # TODO SIGNAL THREADING FOR EXIT
            logger.info("\n*-*(source id {})*-* Start Listening ".format(source_id))
            detected_audio_data = sl.detect_speech_activity(source, detection_callback=_get_postion_message)
            detected_audio_data.add_metadata({"ODAS_source_ID": source_id})
            logger.info("\n*-*(source id {})*-* Speech DETECTED {}".format(source_id, detected_audio_data))
            if detected_audio_data:
                # TODO: idea, here the speaker should be also identified
                processed_audio_data = sl.process_speech(source, detected_audio_data, detection_callback=_get_postion_message)
                logger.info("\n*-*(source id {})*-* Speech COMPLETED: {}".format(source_id, processed_audio_data))
                # add to the recognition process
                recognizer_queue.put(processed_audio_data)


if __name__ == '__main__':
    signal(SIGINT, handler)
    lib_lisa_rcv.register_callback_SST(callback_SST)
    lib_lisa_rcv.register_callback_SSL(callback_SSL)
    lib_lisa_rcv.register_callback_SSS_S(callback_SSS_S)

    listener_threads = []
    for _n in range(MAX_ODAS_SOURCES):
        _th = threading.Thread(target=source_listening, args=(_n,))
        _th.daemon = True
        _th.start()
        listener_threads.append(_th)
        sleep(.25)
    try:
        odas_th = threading.Thread(target=thread_start_odas_switcher, )
        odas_th.start()
        odas_th.join()
        # starting acquisition and wait
        #retval = lib_lisa_rcv.main_loop()
        # print("Exit odas loop with {}".format(retval))
    except KeyboardInterrupt:  # allow Ctrl + C to shut down the program
        print("Exit odas loop with KeyboardInterrupt")

    print("Exit main")
    exit(0)
