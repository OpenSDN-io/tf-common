#
#  Copyright (c) 2014 Juniper Networks. All rights reserved.
#
#  sandesh_trace_dump.py
#  
#  gdb macros to dump the sandesh trace buffer

import gdb
import datetime

from libstdcxx.v6.printers import *

def _UTCTimestampUsecToString(utc_usec):
    return datetime.datetime.fromtimestamp(utc_usec/1000000.0).strftime('%Y-%m-%d %H:%M:%S.%f')
#end _UTCTimestampUsecToString

def _get_trace_buffer_map():
    buf_map = {}
    trace_ptr = gdb.parse_and_eval('Trace<SandeshTrace>::trace_')
    trace_buf_map = StdMapPrinter('trace_buf_map_', trace_ptr['trace_buf_map_'])
    it = trace_buf_map.children()
    try:
        while 1:
            val = next(it)[1]
            if it.count % 2 == 1:
                k = val
            else:
                buf_map[k] = val
    except StopIteration:
        pass
    return buf_map
#end _get_trace_buffer_map 

_EXCLUDE_SANDESH_FIELDS = ['SandeshTrace', 'versionsig_', 'ascii_fingerprint', 'binary_fingerprint', '__isset', 'name_', 'more']
_STD_STRING = re.compile('^std::basic_string')
_STD_VECTOR = re.compile('^std::vector')

def _get_trace_string(data, typ):
    s = ''
    for f in typ.fields():
        if f.name not in _EXCLUDE_SANDESH_FIELDS and not f.artificial:
            if f.type.code == gdb.TYPE_CODE_STRUCT:
                try:
                    f.type.template_argument(0)
                except:
                    s += (f.name + ': { ')
                    s += _get_trace_string(data[f.name], f.type)
                    s += ' } '
                else:
                    if not _STD_VECTOR.match(str(f.type)):
                        continue
                    v = StdVectorPrinter(str(f.type), data[f.name])
                    vit = v.children()
                    try:
                        s += (f.name + ': [ ')
                        while 1:
                            val = next(vit)[1]
                            if val.type.code == gdb.TYPE_CODE_STRUCT and not _STD_STRING.match(str(val.type.unqualified())):
                                s += _get_trace_string(val, val.type)
                            else:
                                s += str(val)
                            s += '; '
                    except StopIteration:
                        s += '] '
            else:
                s += (f.name + ' = ' + str(data[f.name]) + ' ')
    return s
#end _get_trace_string

def _print_trace_sandesh(tb, min_index, max_index):
    for i in range(int(min_index), int(max_index)):
        typ = gdb.lookup_type('SandeshTrace').pointer()
        type_name = str(tb['c_']['m_buff'][i].cast(typ).dereference().dynamic_type)
        msg_type = gdb.lookup_type(type_name).pointer()
        trace_sandesh = tb['c_']['m_buff'][i].cast(msg_type).dereference()
        typ = gdb.lookup_type('Sandesh')
        ts = _UTCTimestampUsecToString(int(trace_sandesh.cast(typ)['timestamp_']))
        trace_str = _get_trace_string(trace_sandesh, msg_type.target())
        print(ts + ' ' + msg_type.target().tag + ': ' + trace_str)
#end _print_trace_sandesh

def print_trace_buffer_list():
    """Displays the list of trace buffers created by the daemon."""
    trace_buf_map = _get_trace_buffer_map()
    for key in list(trace_buf_map.keys()):
        print(key)
#end print_trace_buffer_list

def print_trace_buffer(buf_name):
    """Dumps the content of the specified trace buffer."""
    trace_buf_map = _get_trace_buffer_map()
    trace_buffer = None
    for k, v in list(trace_buf_map.items()):
        char_p = k.cast(gdb.lookup_type('char').pointer())
        if char_p.string() == buf_name:
            trace_buffer = v
            break
    if trace_buffer is None:
        print('Invalid trace buffer "%s"' % (buf_name))
    else:
        read_index = trace_buffer['px'].dereference()['read_index_']
        tb = trace_buffer['px'].dereference()['trace_buf_']
        tb_size = tb['c_']['m_size']
        _print_trace_sandesh(tb, read_index, tb_size)
        _print_trace_sandesh(tb, 0, read_index)
#end print_trace_buffer 

