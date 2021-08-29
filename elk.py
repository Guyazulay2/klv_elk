import json
from elasticsearch import Elasticsearch
elastic_client = Elasticsearch(hosts=["http://elastic:changeme@localhost:9200"])
from time import sleep
from datetime import datetime
import os
import argparse


def import_gst():
    import gi
    gi.require_version('Gst', '1.0')
    gi.require_version('GstWebRTC', '1.0')
    gi.require_version('GstSdp', '1.0')
    from gi.repository import Gst, GLib, GstWebRTC, GstSdp
    Gst.init(None)
    return Gst, GLib, GstWebRTC, GstSdp


Gst, GLib, GstWebRTC, GstSdp = import_gst()


def start_pipeline():
    loop: GLib.MainLoop = None
    pipeline = None

    def on_bus_message(bus, message):
        # message: Gst.Message
        t = message.type
        if t == Gst.MessageType.EOS:
            print("EOS")
            if loop:
                loop.quit()
                
        elif t == Gst.MessageType.WARNING:
            err, debug = message.parse_warning()
            print('Warning: %s: %s\n' % (err, debug))
        elif t == Gst.MessageType.ERROR:
            err, debug = message.parse_error()
            print('Error: %s: %s\n' % (err, debug))
        return True

    def on_klv_buffer(data):
        res = elastic_client.index(index='new_index',doc_type="_doc", id="1", body=data)
        print(res['result'],res)


        # print(data)
        


    def _on_klv_buffer(pad: Gst.Pad, info: Gst.PadProbeInfo):
        buffer: Gst.Buffer = info.get_buffer()
        # map_info:Gst.MapInfo = buffer.map(Gst.MapFlags.READ)
        ret, map_info = buffer.map(Gst.MapFlags.READ)    
        if not map_info:
            return Gst.PadProbeReturn.OK
        try:
            # data = json.loads(map_info.data.tobytes())
            # data = json.loads(map_info.data)
            data = map_info.data
            on_klv_buffer(data)
        except Exception as e:
            print("[ERROR]", e)
            pass
        buffer.unmap(map_info)
        return Gst.PadProbeReturn.OK
        

    pipe = ""
    pipe += "filesrc location=\"./stanag4609-239.10.12.2.pcap\" ! "
    pipe += "pcapparse dst-ip=239.10.12.2 ! "
    pipe += "tsdemux ! "
    pipe += "capsfilter caps=\"meta/x-klv\" ! "
    pipe += "klvparse name=klvparse ! "
    pipe += "fakesink"
    pipeline: Gst.Pipeline = Gst.parse_launch(pipe)

    pipeline.get_by_name("klvparse").get_static_pad("src").add_probe(Gst.PadProbeType.BUFFER, _on_klv_buffer)

    bus = pipeline.get_bus()
    bus.add_signal_watch()
    bus.connect('message', on_bus_message)

    pipeline.set_state(Gst.State.PLAYING)

    loop = GLib.MainLoop()
    try:
        loop.run()
    except:
        print("stop")
    pipeline.set_state(Gst.State.NULL)


start_pipeline()