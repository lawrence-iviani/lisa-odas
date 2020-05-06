#!/usr/bin/env python3

# NOTE: this example requires PyAudio because it uses the Microphone class
import time
import threading
try:
    from queue import Queue  # Python 3 import
except ImportError:
    from Queue import Queue  # Python 2 import

import speech_recognition as sr
from lisa.utils import TimeProfiler, LOGGING_FORMATTER
import logging

DEVICE_INDEX = 7


logger = logging.getLogger(name="Threaded Workers")
logger.setLevel(logging.INFO)

r = sr.Recognizer()
audio_queue = Queue()


def recognize_worker():
    # this runs in a background thread
    worker_id = 0
    while True:
        worker_id += 1
        actual_id = worker_id
        audio = audio_queue.get()  # retrieve the next audio processing job from the main thread
        if audio is None: break  # stop processing if the main thread is done 
		# TODO: add a pause here!!!

        # received audio data, now we'll recognize it using Google Speech Recognition
            # for testing purposes, we're just using the default API key
            # to use another API key, use `r.recognize_google(audio, key="GOOGLE_SPEECH_RECOGNITION_API_KEY")`
            # instead of `r.recognize_google(audio)`
            # print("Google Speech Recognition thinks you said " + r.recognize_google(audio))
            # def _google_recognize(audio):
                # logger.info("Google Speech Recognition thinks you said ")
                # logger.info("{}Google: |{}|".format("+-" *10,r.recognize_google(audio)))	
            # def _sphinx_recognize(audio):	
                # logger.info("Sphinx thinks you said ")
                # logger.info("{}Sphinx: |{}|".format("*=" *10, r.recognize_sphinx(audio)))
        def _recognize(audio, func, tag, actual_id):	
            try:
                logger.info("{}-{} thinks you said ".format(actual_id, tag))
                _start_time = time.time() 
                _response = func(audio)
                _end_time = time.time() - _start_time
                logger.info("{}-{}-{} response in {}s: |{}|".format("+-" *2, actual_id, tag, _end_time, _response))
            except sr.UnknownValueError:
                logger.error("{}-Speech Recognition {} could not understand audio".format(actual_id, tag))
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


# start a new thread to recognize audio, while this thread focuses on listening
recognize_thread = threading.Thread(target=recognize_worker)
recognize_thread.daemon = True
recognize_thread.start()
with sr.Microphone(device_index=DEVICE_INDEX) as source:
    try:
        while True:  # repeatedly listen for phrases and put the resulting audio on the audio processing job queue
            logger.info("Main: audio queue")
            audio_queue.put(r.listen(source))
            logger.info("List of threads: {}".format(threading.enumerate()))
    except KeyboardInterrupt:  # allow Ctrl + C to shut down the program
        pass

audio_queue.join()  # block until all current audio processing jobs are done
audio_queue.put(None)  # tell the recognize_thread to stop
recognize_thread.join()  # wait for the recognize_thread to actually stop
print("EXIT")