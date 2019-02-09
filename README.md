# live_video_10ms_android

Library for live video playback with ultra low latency (below 10ms) for android.
Supports playback of raw h.264 nalus over udp and rtp h264 over udp.
Additionally playback of raw .h264 files for testing.

It has been optimized for low latency and tested on a wide variety of devices, including those running FPV-VR for wifibroadcast.
The example library also contains test cases that can be executed on the 'gooogle firebase test lab'. These tests include feeding
the decoder with faulty NALUs, created by a lossy connection. (e.g. wifibroadcast).

The 2 most important factors for low latency are
a) HW-accelerated decoding via the MediaCodec api
b) Receiving,Parsing and decoding is done in cpp code (multi-threaded). This decouples it from the java runtime, which increses performance
and the latency is more consistent ( garbage collection halts all threads, for example).

However, all native code needed for creating, starting and stopping the decoding process are exposed via the JNI, so you can use the lib
without writing c/c++ code.


Structure:
core: contains the native code and java bindings
example: simple playback of a .h264 file stored in the 'assets folder' of the app. Includes test case(s)


