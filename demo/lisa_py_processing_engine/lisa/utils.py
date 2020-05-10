import time
import logging

# LOGGING_FORMATTER = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
LOGGING_FORMATTER = logging.Formatter('%(levelname)s-%(asctime)s-%(name)s %(module)s.%(funcName)s():%(lineno)d -[P%(process)d-TH%(thread)d]>%(message)s')

root_logger = logging.getLogger()
handler_logger = logging.StreamHandler()
handler_logger.setFormatter(LOGGING_FORMATTER)
root_logger.addHandler(handler_logger)


class TimeProfiler:

	
	def __init__(self):
		self._start = 0
		self._profiler = {}
		
		
	def start(self):
		self._start = time.time() 
	
	
	def add_time(self, name, restart_timer = True):
		self._profiler[name] = time.time() - self._start
		if restart_timer:
			self._start = time.time() 
		return self._profiler[name]
		
		
	def __str__(self):
		retval = '{}@{}\n'.format(self.__class__.__name__, hex(id(self)))
		retval += '|Item     \t|Time     \n' 
		for _k, _v in self._profiler.items():
			retval += '|{}\t|{}s\t|\n'.format(_k, _v ) 
		
		return retval