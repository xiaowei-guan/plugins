#ifndef VIDEO_PLAYER_H_
#define VIDEO_PLAYER_H_

#include <flutter/encodable_value.h>
#include <flutter/event_channel.h>
#include <flutter/plugin_registrar.h>

#include <string>

#include "flutter_tizen_texture_registrar.h"
#include "video_player_options.h"
#include "flutter_tizen.h"
#include "message.h"

enum VideoPlayerType { kMMPlayer, kPlusPlayer };

class VideoPlayer {
 public:
  static std::unique_ptr<VideoPlayer> create(
      FlutterDesktopPluginRegistrarRef registrar_ref,
      VideoPlayerType type, flutter::PluginRegistrar *pluginRegistrar,
      FlutterTextureRegistrar *textureRegistrar, const std::string &uri,
      VideoPlayerOptions &options);
  virtual ~VideoPlayer();

  long getTextureId();

  virtual void play() = 0;
  virtual void pause() = 0;
  virtual void setLooping(bool isLooping) = 0;
  virtual void setVolume(double volume) = 0;
  virtual void setPlaybackSpeed(double speed) = 0;
  virtual void seekTo(int position) = 0;  // milliseconds
  virtual int getPosition() = 0;          // milliseconds
  virtual void dispose() = 0;
  virtual void setDisplayRoi(int x, int y, int w, int h) = 0;

 private:
  virtual void initialize() = 0;
  void setupEventChannel(flutter::BinaryMessenger *messenger);

  std::unique_ptr<flutter::EventChannel<flutter::EncodableValue>> eventChannel_;
  std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> eventSink_;

 protected:
  void sendInitialized(int duration, int videoWidth, int videoHeight);
  void sendBufferingStart();
  void sendBufferingUpdate(int position);  // milliseconds
  void sendBufferingEnd();
  void sendCompleted();
  void sendError(const std::string &code, const std::string &message = "");

  bool isInitialized_;
  long textureId_;
};

#endif  // VIDEO_PLAYER_H_
