

#This must be the same parameters as in defined in configuration ssl.nPots, the fix number of messages, stream that are transmitted.
MAX_ODAS_SOURCES = 4
SST_TAG_LEN = 20  # as in common.h
# this is connect to the n bits in the output configuration of odas modules (SSS_x) (16 bits)
SAMPLE_RATE_INCOME_STREAM = 16000  # This the value defined as fs  in odas config sss separated or postfiltered
HOP_SIZE_INCOME_STREAM = 128  # This the value defined as hopSize  in odas config sss separated or postfiltered
N_BITS_INCOME_STREAM = 16  # This the value defined as nBits  in odas config sss separated or postfiltered
