# Live Video 10ms Android
[![Release](https://jitpack.io/v/Consti10/LiveVideo10ms.svg)](https://jitpack.io/#Consti10/LiveVideo10ms)
[![](https://jitci.com/gh/Consti10/LiveVideo10ms/svg)](https://jitci.com/gh/Consti10/LiveVideo10ms)

**Description** \
Library for live video playback with ultra low latency (below 10ms) on android devices.
Supports playback of .h264 encoded live video data transmitted via UDP encapsulated in RAW or RTP and simple file playback. \
Latency data (see example for more information)

| Device   | Encoder   | avgTotalDecodingTime
| --- | --- | --- |
| Galaxy s9+ | rpi cam | 8.0ms |
| Galaxy s9+ | x264 | 11.5ms |
| Pixel 3 | rpi cam | 11.2ms |
| Pixel 3 | x264 | 10.5ms |
| HTC U11 | rpi cam | 8.6ms |
| HTC U11 | x264 | 10.1ms |


**Example App** \
<img src="https://github.com/Consti10/LiveVideo10ms/blob/master/Screenshots/device1.png" alt="ExampleMain" width="240"> <img src="https://github.com/Consti10/LiveVideo10ms/blob/master/Screenshots/device2.png" height="240">

This library has been optimized for low latency and tested on a wide variety of devices, including those running FPV-VR for wifibroadcast.
The example library also contains test cases that can be executed on the 'gooogle firebase test lab'. These tests include feeding
the decoder with faulty NALUs, created by a lossy connection. (e.g. wifibroadcast).

The 2 most important factors for low latency are
1. HW-accelerated decoding via the MediaCodec api
2. Receiving,Parsing and decoding is done in cpp code (multi-threaded). This decouples it from the java runtime, which increases performance and makes latency more consistent ( garbage collection halts all java threads, for example).

However, all native code needed for creating, starting and stopping the decoding process are exposed via the JNI, so you can use the lib
without writing c/c++ code.
The VideoPlayer class purposely has no pause/resume functions,since this library is for live video playback.
Playback works on a 'best effort' principle, e.g. as soon as a receiver is created nalus are forwarded to the LowLagDecoder,
but no frames can be decoded until enough I-frame data was received. Make sure to use a low enough I-frame interval with your h264 encoder.
When receiving corrupted data (e.g from a lossy connection) the decoder will still generate frames if possible.

**Structure:**
- VideoCore: contains the native code and java bindings
- Example: simple example app. Playback of different .h264 files stored in the 'assets folder' of the app. Includes test case(s) \

**Setup Dependencies**\
There are 2 ways to use VideoCore in your Project \
**1 Declaring Dependency via Jitpack: [jitpack.io](https://jitpack.io)** \
:heavy_plus_sign: Easy \
:heavy_minus_sign: cannot browse native libraries \
Gradle example:
```gradle
    allprojects {
        repositories {
            jcenter()
            maven { url "https://jitpack.io" }
        }
   }
   dependencies {
        implementation 'com.github.Consti10:LiveVideo10ms:v1.1'
   }
```
**2 Forking the repo and including sources manually:** \
:heavy_plus_sign: browse native libraries \
:heavy_plus_sign: modify code
* To your top level settings.gradle file, add
```
include ':VideoCore'
project(':VideoCore').projectDir=new File('..\\LiveVideo10ms\\VideoCore')
```
and modify the path according to your download file
* To your app level gradle file add
```
implementation project(':VideoCore')
```
See [FPV-VR](https://github.com/Consti10/FPV_VR_2018) as an example how to add dependencies.
