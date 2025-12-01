import socket
import struct
import json
from typing import Any, Callable, Dict, Optional

class ProtocolHandler: 
    def __init__(self, timeout: float = 5.0): 
        self._timeout = timeout 
        self._socket = Optional[socket.socket]

    # connect socket @ host:port and set connection timeout 
    def connect(self, host: str, port: int):
        self._socket = socket.create_connection((host, port), self._timeout)
        self._socket.settimeout(self._timeout)

    def close(self): 
        if self._socket: 
            try: 
                self._socket.close()
            finally:
                self._socket = None 

    @staticmethod
    def _recv_exact(sock: socket.socket, count: int): 
        buf = bytearray()
        recvd = 0

        while recvd < count: 
            chunk = sock.recv(count)
            recv += 

    
