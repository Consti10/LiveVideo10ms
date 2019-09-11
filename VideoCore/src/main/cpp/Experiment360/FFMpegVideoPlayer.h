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

class FFMpegVideoPlayer {
public:
  FFMpegVideoPlayer(std::string url, int cpu_priority,
		    NALU_DATA_CALLBACK callback, uint32_t bufsize = 1000000);
  void start_playing();
  void stop_playing();
  int shutdown_callback();

private:
  void run();

  std::string m_url;
  int m_cpu_priority;
  uint32_t m_bufsize;
  int m_video_stream;
  bool m_shutdown;
  std::shared_ptr<std::thread> m_thread;

  AVCodec *m_codec;
  AVCodecContext  *m_codec_ctx;
  AVCodecParserContext *m_parser;
  AVFormatContext *m_format_ctx;
  AVPacket m_packet;
  AVFrame *m_frame;

  const NALU_DATA_CALLBACK m_callback;
};


#endif //FFMPEG_VIDEO_PLAYER_H