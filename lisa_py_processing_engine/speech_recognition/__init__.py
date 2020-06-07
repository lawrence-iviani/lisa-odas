#!/usr/bin/env python3

"""Library for performing speech recognition, with support for several engines and APIs, online and offline."""

from queue import Queue  # Python 3 import

import io
import os
import sys
import subprocess
import wave
import aifc
import math
import audioop
import collections
import json
import base64
import threading
import platform
import stat
import hashlib
import hmac
import time
import uuid
import logging

__author__ = "Lawrence Iviani"
# __author__ = "Anthony Zhang (Uberi)"
__version__ = "0.0.1"
__license__ = "BSD"




class WaitTimeoutError(Exception):
    pass


class RequestError(Exception):
    pass


class UnknownValueError(Exception):
    pass






# ===============================
#  backwards compatibility shims
# ===============================
#
# WavFile = AudioFile  # WavFile was renamed to AudioFile in 3.4.1
#
#
# def recognize_api(self, audio_data, client_access_token, language="en", session_id=None, show_all=False):
#     wav_data = audio_data.get_wav_data(convert_rate=16000, convert_width=2)
#     url = "https://api.api.ai/v1/query"
#     while True:
#         boundary = uuid.uuid4().hex
#         if boundary.encode("utf-8") not in wav_data: break
#     if session_id is None: session_id = uuid.uuid4().hex
#     data = b"--" + boundary.encode(
#         "utf-8") + b"\r\n" + b"Content-Disposition: form-data; name=\"request\"\r\n" + b"Content-Type: application/json\r\n" + b"\r\n" + b"{\"v\": \"20150910\", \"sessionId\": \"" + session_id.encode(
#         "utf-8") + b"\", \"lang\": \"" + language.encode("utf-8") + b"\"}\r\n" + b"--" + boundary.encode(
#         "utf-8") + b"\r\n" + b"Content-Disposition: form-data; name=\"voiceData\"; filename=\"audio.wav\"\r\n" + b"Content-Type: audio/wav\r\n" + b"\r\n" + wav_data + b"\r\n" + b"--" + boundary.encode(
#         "utf-8") + b"--\r\n"
#     request = Request(url, data=data, headers={"Authorization": "Bearer {}".format(client_access_token),
#                                                "Content-Length": str(len(data)), "Expect": "100-continue",
#                                                "Content-Type": "multipart/form-data; boundary={}".format(boundary)})
#     try:
#         response = urlopen(request, timeout=10)
#     except HTTPError as e:
#         raise RequestError("recognition request failed: {}".format(e.reason))
#     except URLError as e:
#         raise RequestError("recognition connection failed: {}".format(e.reason))
#     response_text = response.read().decode("utf-8")
#     result = json.loads(response_text)
#     if show_all: return result
#     if "status" not in result or "errorType" not in result["status"] or result["status"]["errorType"] != "success":
#         raise UnknownValueError()
#     return result["result"]["resolvedQuery"]
#
#
# Recognizer.recognize_api = classmethod(
#     recognize_api)  # API.AI Speech Recognition is deprecated/not recommended as of 3.5.0, and currently is only optionally available for paid plans
