#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#
# Copyright 2006-2017 The Apache Software Foundation.
# https://github.com/apache/thrift
#

class TType(object):
    STOP = 0
    VOID = 1
    BOOL = 2
    BYTE = 3
    I08 = 3
    DOUBLE = 4
    I16 = 6
    I32 = 8
    U64 = 9
    I64 = 10
    STRING = 11
    UTF7 = 11
    STRUCT = 12
    MAP = 13
    SET = 14
    LIST = 15
    UTF8 = 16
    UTF16 = 17
    SANDESH = 18
    U16 = 19
    U32 = 20
    XML = 21
    IPV4 = 22
    UUID = 23
    IPADDR = 24

    _VALUES_TO_NAMES = ('STOP',
                        'VOID',
                        'BOOL',
                        'BYTE',
                        'DOUBLE',
                        None,
                        'I16',
                        None,
                        'I32',
                        'U64',
                        'I64',
                        'STRING',
                        'STRUCT',
                        'MAP',
                        'SET',
                        'LIST',
                        'UTF8',
                        'UTF16',
                        'SANDESH',
                        'U16',
                        'U32',
                        'XML'
                        'IPV4',
                        'UUID',
                        'IPADDR')


class TMessageType(object):
    CALL = 1
    REPLY = 2
    EXCEPTION = 3
    ONEWAY = 4


class TProcessor(object):
    """Base class for procsessor, which works on two streams."""

    def process(iprot, oprot):
        pass


class TException(Exception):
    """Base class for all thrift exceptions."""

    def __init__(self, message=None):
        Exception.__init__(self, message)
        self.message = message


class TApplicationException(TException):
    """Application level thrift exceptions."""

    UNKNOWN = 0
    UNKNOWN_METHOD = 1
    INVALID_MESSAGE_TYPE = 2
    WRONG_METHOD_NAME = 3
    BAD_SEQUENCE_ID = 4
    MISSING_RESULT = 5
    INTERNAL_ERROR = 6
    PROTOCOL_ERROR = 7

    def __init__(self, type=UNKNOWN, message=None):
        TException.__init__(self, message)
        self.type = type

    def __str__(self):
        if self.message:
            return self.message
        elif self.type == self.UNKNOWN_METHOD:
            return 'Unknown method'
        elif self.type == self.INVALID_MESSAGE_TYPE:
            return 'Invalid message type'
        elif self.type == self.WRONG_METHOD_NAME:
            return 'Wrong method name'
        elif self.type == self.BAD_SEQUENCE_ID:
            return 'Bad sequence ID'
        elif self.type == self.MISSING_RESULT:
            return 'Missing result'
        else:
            return 'Default (unknown) TApplicationException'

    def read(self, iprot):
        iprot.readStructBegin()
        while True:
            (fname, ftype, fid) = iprot.readFieldBegin()
            if ftype == TType.STOP:
                break
            if fid == 1:
                if ftype == TType.STRING:
                    self.message = iprot.readString()
                else:
                    iprot.skip(ftype)
            elif fid == 2:
                if ftype == TType.I32:
                    self.type = iprot.readI32()
                else:
                    iprot.skip(ftype)
            else:
                iprot.skip(ftype)
            iprot.readFieldEnd()
        iprot.readStructEnd()

    def write(self, oprot):
        oprot.writeStructBegin('TApplicationException')
        if self.message is not None:
            oprot.writeFieldBegin('message', TType.STRING, 1)
            oprot.writeString(self.message)
            oprot.writeFieldEnd()
        if self.type is not None:
            oprot.writeFieldBegin('type', TType.I32, 2)
            oprot.writeI32(self.type)
            oprot.writeFieldEnd()
        oprot.writeFieldStop()
        oprot.writeStructEnd()
