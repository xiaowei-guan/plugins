#ifndef VIDEO_PLAYER_H_
#define VIDEO_PLAYER_H_

#include <flutter/encodable_value.h>
#include <flutter/event_channel.h>
#include <flutter/plugin_registrar.h>
#include <flutter_tizen.h>
#include <plusplayer/plusplayer.h>

#include <mutex>
#include <string>

#include "video_player_options.h"

using SeekCompletedCb = std::function<void()>;

class VideoPlayer {
 public:
  VideoPlayer(FlutterDesktopPluginRegistrarRef registrar_ref,
              flutter::PluginRegistrar *plugin_registrar,
              const std::string &uri, VideoPlayerOptions &options);
  ~VideoPlayer();

  long getTextureId();
  void play();
  void pause();
  void setLooping(bool is_looping);
  void setVolume(double volume);
  void setPlaybackSpeed(double speed);
  void seekTo(int position,
              const SeekCompletedCb &seek_completed_cb);  // milliseconds
  int getPosition();                                      // milliseconds
  void dispose();
  void setDisplayRoi(int x, int y, int w, int h);

 private:
  class PlusPlayerEventListener : public plusplayer::EventListener {
   public:
    PlusPlayerEventListener(VideoPlayer *handler) : handler_(handler) {}
    void OnErrorMsg(const plusplayer::ErrorType &error_code,
                    const char *error_msg, UserData userdata) override;
    void OnResourceConflicted(UserData userdata) override;
    void OnPrepareDone(bool ret, UserData userdata) override;
    void OnError(const plusplayer::ErrorType &error_code,
                 UserData userdata) override;
    void OnBufferStatus(const int percent, UserData userdata) override;
    void OnEos(UserData userdata) override;
    void OnClosedCaptionData(std::unique_ptr<char[]> data, const int size,
                             UserData userdata) override;
    void OnAdaptiveStreamingControlEvent(
        const plusplayer::StreamingMessageType &type,
        const plusplayer::MessageParam &msg, UserData userdata) override;
    void OnCueEvent(const char *msgType, const uint64_t timestamp,
                    unsigned int duration, UserData userdata) override;
    void OnDateRangeEvent(const char *DateRangeData,
                          UserData userdata) override;
    void OnStopReachEvent(bool StopReach, UserData userdata) override;
    void OnCueOutContEvent(const char *CueOutContData,
                           UserData userdata) override;
    void OnSeekDone(UserData userdata) override;
    void OnChangeSourceDone(bool ret, UserData userdata) override;
    void OnStateChangedToPlaying(UserData userdata) override;

   private:
    VideoPlayer *handler_ = nullptr;
  };
  void initialize();
  void setupEventChannel(flutter::BinaryMessenger *messenger);
  void sendInitialized();
  void sendBufferingStart();
  void sendBufferingUpdate(int position);  // milliseconds
  void sendBufferingEnd();

  bool is_initialized_;
  std::unique_ptr<plusplayer::PlusPlayer> player_;
  std::unique_ptr<PlusPlayerEventListener> listener_;
  std::unique_ptr<flutter::EventChannel<flutter::EncodableValue>>
      event_channel_;
  std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink_;
  long texture_id_;
  SeekCompletedCb on_seek_completed_;
  bool is_interrupted_;
  bool isLooping_;
};

#endif  // VIDEO_PLAYER_H_
