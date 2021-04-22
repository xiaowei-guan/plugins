#ifndef MMPLAYER_H_
#define MMPLAYER_H_

#include <player.h>

#include "video_player.h"

class MMPlayer : public VideoPlayer {
 public:
  MMPlayer(FlutterTextureRegistrar *textureRegistrar, const std::string &uri,
           const VideoPlayerOptions &options);
  ~MMPlayer() override;

  void play() override;
  void pause() override;
  void setLooping(bool isLooping) override;
  void setVolume(double volume) override;
  void setPlaybackSpeed(double speed) override;
  void seekTo(int position) override;  // milliseconds
  int getPosition() override;          // milliseconds
  void dispose() override;
  void setDisplayRoi(int x, int y, int w, int h) override;

 private:
  void initialize() override;

  static void onPrepared(void *data);
  static void onPlayCompleted(void *data);
  static void onInterrupted(player_interrupted_code_e code, void *data);
  static void onErrorOccurred(int code, void *data);
  static void onVideoFrameDecoded(media_packet_h packet, void *data);

  player_h player_;
  FlutterTextureRegistrar *textureRegistrar_;
};

#endif  // MMPLAYER_H_
