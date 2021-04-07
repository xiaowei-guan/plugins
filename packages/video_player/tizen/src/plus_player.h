#ifndef PLUS_PLAYER_H_
#define PLUS_PLAYER_H_

#include "plusplayer/plusplayer.h"
#include "video_player.h"

class PlusPlayer : public VideoPlayer {
 public:
  PlusPlayer(const std::string& uri, const VideoPlayerOptions& options);
  ~PlusPlayer();

  void play() override;
  void pause() override;
  void setLooping(bool isLooping) override;
  void setVolume(double volume) override;
  void setPlaybackSpeed(double speed) override;
  void seekTo(int position) override;  // milliseconds
  int getPosition() override;          // milliseconds
  void dispose() override;

 private:
  void initialize() override;

  class PlusPlayerEventListener : public plusplayer::EventListener {
   public:
    PlusPlayerEventListener(PlusPlayer* handler) : handler_(handler) {}
    void OnErrorMsg(const plusplayer::ErrorType& error_code,
                    const char* error_msg, UserData userdata) override;
    void OnResourceConflicted(UserData userdata) override;
    virtual void OnEos(UserData userdata) override;
    void OnPrepareDone(bool ret, UserData userdata) override;

   private:
    PlusPlayer* handler_ = nullptr;
  };

  std::unique_ptr<plusplayer::PlusPlayer> player_;
  std::unique_ptr<PlusPlayerEventListener> listener_{
      new PlusPlayerEventListener(this)};
  bool isLooping_;
};

#endif  // PLUS_PLAYER_H_
