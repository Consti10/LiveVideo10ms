# live_video_10ms_android

Library for live video playback with ultra low latency (below 10ms) on android devices.
Supports playback of 
a) raw h264 nalus over udp and 
b) rtp h264 data over udp.
Additionally playback of raw .h264 files for testing.

It has been optimized for low latency and tested on a wide variety of devices, including those running FPV-VR for wifibroadcast.
The example library also contains test cases that can be executed on the 'gooogle firebase test lab'. These tests include feeding
the decoder with faulty NALUs, created by a lossy connection. (e.g. wifibroadcast).

The 2 most important factors for low latency are
a) HW-accelerated decoding via the MediaCodec api
b) Receiving,Parsing and decoding is done in cpp code (multi-threaded). This decouples it from the java runtime, which increases performance and makes latency more consistent ( garbage collection halts all java threads, for example).

However, all native code needed for creating, starting and stopping the decoding process are exposed via the JNI, so you can use the lib
without writing c/c++ code.
The VideoPlayer class purposely has no pause/resume functions,since this library is for live video playback.
Playback works on a 'best effort' principle, e.g. as soon as a receiver is created nalus are forwarded to the LowLagDecoder,
but no frames can be decoded until enough I-frame data was received. Make sure to use a low enough I-frame interval with your h264 encoder.
When receiving corrupted data (e.g from a lossy connection) the decoder will still generate frames if possible.

Structure:
core: contains the native code and java bindings, one test file
example: simple playback of different .h264 files stored in the 'assets folder' of the app. Includes test case(s)


