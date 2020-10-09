#ifndef FFMPEG_VIDEO_PLAYER_H
#define FFMPEG_VIDEO_PLAYER_H

#include <jni.h>

#include <string>
#include <thread>
#include <memory>

#include <media/NdkMediaCodec.h>
#include "../NALU/NALU.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
}

class FFMpegVideoReceiver {
public:
	//For debugging takes a raw h264 data (not parsed) and another callback for already parsed data (not working yet)
  FFMpegVideoReceiver(std::string url, int cpu_priority,
					  std::function<void(uint8_t[],int)> callback,
					  NALU_DATA_CALLBACK callback2,uint32_t bufsize = 1000000);
  void start_playing();
  void stop_playing();
  int shutdown_callback();
  const std::string m_url;
  size_t currentlyReceivedVideoData=0;
  std::string currentErrorMessage="";
private:
  void run();
  int m_cpu_priority;
  uint32_t m_bufsize;
  int m_video_stream;
  bool m_shutdown;
  std::unique_ptr<std::thread> m_thread;

  AVCodec *m_codec;
  AVCodecContext  *m_codec_ctx;
  AVCodecParserContext *m_pCodecPaser;
  AVFormatContext *m_format_ctx;
  AVPacket m_packet;
  AVFrame *m_frame;

  const std::function<void(uint8_t[],int)> raw_h264_data_callback;
  const NALU_DATA_CALLBACK onNewNALUCallback;
  //
  void LOG_TERMINATING_ERROR(const std::string message);
};


#endif //FFMPEG_VIDEO_PLAYER_H