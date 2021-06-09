from queue import Queue, Full  # Python 3 import
import time
import threading
import numpy as np
from signal import signal, SIGINT
from sys import exit
from time import sleep
import logging
from _collections import deque
import os
import deepspeech
import datetime
import wave
import soundfile as sf
import sounddevice as sd

import speech_recognition as sr
import speech_recognition.batch_recognizer as batch_sr
import speech_recognition.audio_sources as inputs_sources

from lisa.lisa_switcher_bindings import callback_SSL_func, callback_SST_func, callback_SSS_S_func, lib_lisa_rcv
from lisa.lisa_switcher_bindings import SSL_struct, SSL_src_struct, SST_struct, SST_src_struct
from lisa.lisa_configuration import SST_TAG_LEN, MAX_ODAS_SOURCES, SAMPLE_RATE_INCOME_STREAM, HOP_SIZE_INCOME_STREAM, \
    N_BITS_INCOME_STREAM

BYTES_PER_SAMPLE = N_BITS_INCOME_STREAM // 8

from lisa.utils import LOGGING_FORMATTER

# call logging after import of lisa.utils, it will set the formatting options
logger = logging.getLogger(name="LISA Speech Recognition")
logger.setLevel(logging.DEBUG)

# Specific Processing Parameters, should be configurable as parameters
RECOGNITION_METHODS = ['sphinx', 'all' , 'google', 'julius']
AVAILABLE_LANGUAGES = ['en-US', 'it-IT', 'de-DE']

# USED AS GLOBAL VAR
RECOGNITION_METHOD = 'julius'  # 'sphinx', 'all' , 'google'
LANGUAGE = 'en-US'  # options: 'en-US' 'it-IT' 'de-DE'
# TODO: wake up word

VAD_AGGRESSIVENESS = 3 # 0 - 3 , 0 being the least aggressive about filtering out non-speech, 3 the most aggressive.
RT_SPEECH_PROCESSING = False

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
    #print('callback_SSL')
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
    # print('callback_SST')
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
            # print("SST_latest[{}]: {}".format(i, SST_latest[i]))

        except Full:
            logger.warning("SST queue is Full, this should not happen with deque")
            pass


@callback_SSS_S_func
def callback_SSS_S(n_bytes, x):
    """"
    Called by the relative odas switcher stream, save in the proper SSS_queue[] the received data.
    """
    # print('callback_SSS_S')
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
            # logger.warning("SSS_S receiving Queue is Full, skipping frame (TODO: lost for now, change in deque!)")
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
    print("test_recognize_thread working")

    worker_id = 0
    while True:
        worker_id += 1
        actual_id = worker_id
        audio = audio_queue.get()  # retrieve the next audio processing job from the main thread
        if audio is None: break  # stop processing if the main thread is done
        # TODO: add a pause here?? Profiling?
        print("test")
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
        print("hoge")
        if RECOGNITION_METHOD == 'google':  # 'sphinx', 'all'
            print("do google")
            google_thread = threading.Thread(target=_recognize,
                                             args=(audio, recognizer.recognize_google, 'google', actual_id, LANGUAGE))
            google_thread.daemon = True
            google_thread.start()
        if RECOGNITION_METHOD == 'sphinx' or RECOGNITION_METHOD == 'all':
            print("do sphinx")
            raw_data = audio.get_raw_data(convert_rate=16000, convert_width=2)  # the included language models require audio to be 16-bit mono 16 kHz in little-endian format
            sphinx_thread = threading.Thread(target=_recognize,
                                             args=(audio, recognizer.recognize_sphinx, 'sphinx', actual_id))
            sphinx_thread.daemon = True
            sphinx_thread.start()
        if RECOGNITION_METHOD == 'julius' or RECOGNITION_METHOD == 'all':
            print("do julius")
            raw_data = audio.get_raw_data(convert_rate=16000, convert_width=2)  # the included language models require audio to be 16-bit mono 16 kHz in little-endian format
            julius_thread = threading.Thread(target=_recognize,
                                             args=(audio, recognizer.recognize_julius, 'julius', actual_id))
            julius_thread.daemon = True
            julius_thread.start()
            

        audio_queue.task_done()  # mark the audio processing job as completed in the queue

#
#
# BEAM_WIDTH = 500 # Beam width used in the CTC decoder when building candidate transcriptions. Default: {BEAM_WIDTH}
# DEFAULT_SAMPLE_RATE = 16000 # Input device sample rate. Default: {DEFAULT_SAMPLE_RATE}. Your device may require 44100.
# LM_ALPHA = 0.75 # The alpha hyperparameter of the CTC decoder. Language Model weight. Default: {LM_ALPHA}
# LM_BETA = 1.85 # The beta hyperparameter of the CTC decoder. Word insertion bonus. Default: {LM_BETA}
# VAD_AGGRESSIVENESS = 3 # "Set aggressiveness of VAD: an integer between 0 and 3, 0 being the least aggressive about
#                        # filtering out non-speech, 3 the most aggressive. Default: 3"
# MODEL_DIR = '/home/pi/dev/deepspeech-0.6.1-models/' # "Path to the model (protocol buffer binary file, or entire directory containing all standard-named files for model)"
# LM_BINARY = 'lm.binary' # "Path to the language model binary file. Default: lm.binary"
# TRIE = 'trie' # Path to the language model trie file created with native_client/generate_trie. Default: trie
#
#
# class rt_params:
#     vad_aggressiveness = VAD_AGGRESSIVENESS
#     model = MODEL_DIR
#     lm = LM_BINARY
#     trie = TRIE
#     rate = DEFAULT_SAMPLE_RATE
#     lm_alpha = LM_ALPHA
#     lm_beta = LM_BETA
#     beam_width = BEAM_WIDTH
#
#
# deepspeech_model = None

def get_rt_model():
    global deepspeech_model
    if deepspeech_model is None:
        params_rt = rt_params()
        # Load DeepSpeech model
        if os.path.isdir(params_rt.model):
            model_dir = params_rt.model
            params_rt.model = os.path.join(model_dir, 'output_graph.tflite')
            params_rt.lm = os.path.join(model_dir, params_rt.lm)
            params_rt.trie = os.path.join(model_dir, params_rt.trie)

        print('Initializing model...')
        logging.info("params_rt.model: %s", params_rt.model)
        deepspeech_model = deepspeech.Model(params_rt.model, params_rt.beam_width)
        if params_rt.lm and params_rt.trie:
            logging.info("params_rt.lm: %s", params_rt.lm)
            logging.info("params_rt.trie: %s", params_rt.trie)
            deepspeech_model.enableDecoderWithLM(params_rt.lm, params_rt.trie, params_rt.lm_alpha, params_rt.lm_beta)
    return deepspeech_model, rt_params()


def source_listening(source_id):
    print("source_listening Part 1 ************",source_id)
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

    print("source_listening Part 2 ************",source_id)
    # The processing queue between a recognizer thread and received data identified as speech
    # This process is a sequetial  job with a long execution time , TODO: a possible check or maxlen could be added, for safety
    
    ## [COMMENT]複数のソースから音声認識する際は2行コメントアウトする
    # if source_id > 0:
    #     return  # do nothignproperty
    
    print("source_listening Part 3 ************",source_id)
    if RT_SPEECH_PROCESSING:
        if source_id>0: return # do nothign
        sl_rt = batch_sr.SpeechListener_RT(aggressiveness=VAD_AGGRESSIVENESS)
        model, params_rt = get_rt_model()
        print("source_listening Part 4 ************",source_id)
    else:
        print("source_listening Part 5 ************",source_id)
        recognizer_queue = Queue()
        r = batch_sr.Recognizer(sphinx_language=LANGUAGE)
        sl = batch_sr.SpeechListener()
        print("test_recognize_thread1")
        recognize_thread = threading.Thread(target=recognize_worker, args=(recognizer_queue, r,))
        recognize_thread.daemon = True
        recognize_thread.start()
        print("test_recognize_thread2")
        print("source_listening Part 6 ************",source_id)

    print("Start odas")
    with inputs_sources.OdasRaw(audio_queue=SSS_queue[source_id], sample_rate=SAMPLE_RATE_INCOME_STREAM,
                        chunk_size=HOP_SIZE_INCOME_STREAM, nbits=N_BITS_INCOME_STREAM) as source:
        if RT_SPEECH_PROCESSING:
            # TODO, propertymodify accordinglyproperty
            print('1. sl_rt.my_listen()')
            frames = sl_rt.my_listen(source, timeout=None, phrase_time_limit=None, padding_ms=300, ratio=0.75)
            #   frames = vad_audio.vad_collector()
            print('2. model.createStream()')
            stream_context = model.createStream()
            # wav_data = bytearray()property
            for frame in frames:
                if frame is not None:
                    #if spinner: spinner.start()
                    logging.debug("streaming frame")
                    model.feedAudioContent(stream_context, np.frombuffer(frame, np.int16))
                    # if ARGS.savewav: wav_data.extend(frame)
                else:
                    #if spinner: spinner.stop()
                    logging.debug("end utterence")
                    # if ARGS.savewav:
                    #     vad_audio.write_wav(
                    #         os.path.join(ARGS.savewav, datetime.now().strftime("savewav_%Y-%m-%d_%H-%M-%S_%f.wav")),
                    #         wav_data)
                    #     wav_data = bytearray()
                    text = model.finishStream(stream_context)
                    print("Recognized: %s" % text)
                    stream_context = model.createStream()
        else:
            while True:  # repeatedly listen for phrases and put the resulting audio on the audio processing job queue
                # TODO SIGNAL THREADING FOR EXIT
                print("please talk something in English!")
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


# PARSER OPTION
import argparse
parser = argparse.ArgumentParser(description='Running option for Lisa receiver')

parser.add_argument('-l', '--language', dest='language', action='store', default=LANGUAGE,
                    choices=AVAILABLE_LANGUAGES, help='Default language: ' + str(LANGUAGE))
parser.add_argument('-r', '--recognition', dest='recognition_method', action='store', default=RECOGNITION_METHOD,
                    choices=RECOGNITION_METHODS, help='Default recognition method: ' + str(RECOGNITION_METHOD))


if __name__ == '__main__':
    args = parser.parse_args()
    print("selected language: {}".format(args.language))
    print("selected recognition_method: {}".format(args.recognition_method))
    LANGUAGE = args.language
    RECOGNITION_METHODS = args.recognition_method

    signal(SIGINT, handler)

    # SST, SSL, SSSのコールバック(常に動いているやつ)
    lib_lisa_rcv.register_callback_SST(callback_SST)
    lib_lisa_rcv.register_callback_SSL(callback_SSL)
    lib_lisa_rcv.register_callback_SSS_S(callback_SSS_S)

    # 4 sourceで待つ
    listener_threads = []
    for _n in range(MAX_ODAS_SOURCES):
        _th = threading.Thread(target=source_listening, args=(_n,))
        _th.daemon = True
        _th.start()
        listener_threads.append(_th)
        sleep(2)
    try:
        odas_th = threading.Thread(target=thread_start_odas_switcher, )
        odas_th.start()
        odas_th.join()
        print("Exit odas thread")
        # starting acquisition and wait
        #retval = lib_lisa_rcv.main_loop()
        # print("Exit odas loop with {}".format(retval))
    except KeyboardInterrupt:  # allow Ctrl + C to shut down the program
        print("Exit odas loop with KeyboardInterrupt")

    print("Exit main")
    exit(0)
