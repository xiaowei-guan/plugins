#ifndef PLUS_PLAYER_WRAPPER_H_
#define PLUS_PLAYER_WRAPPER_H_

#include <stddef.h>
#include <stdint.h>

enum DisplayType { kDisplayNone, kOverlay, kEvas, kMixer, kOverlaySyncUI };

enum DisplayMode {
  kLetterBox,
  kOriginSize,
  kFullScreen,
  kCroppedFull,
  kOriginOrLetter,
  kDstRoi,
  kAutoAspectRatio,
  kMax
};

enum DisplayRotation { kRotateNone, kRotate90, kRotate180, kRotate270 };

enum State {
  kNone, /**< Player is created, but not opened */
  kIdle, /**< Player is opened, but not prepared or player is stopped */
  kTypeFinderReady,  /**< TypeFinder prepared */
  kTrackSourceReady, /**< TrackSource prepared */
  kReady,            /**< Player is ready to play(start) */
  kPlaying,          /**< Player is playing media */
  kPaused            /**< Player is paused while playing media */
};

struct Geometry {
  int x = 0;
  int y = 0;
  int w = 1920;
  int h = 1080;
};

struct PlusPlayer;
typedef struct PlusPlayer* PlusPlayerRef;

typedef void (*plusplayer_prepared_cb)(bool ret, void* user_data);
typedef void (*plusplayer_seek_completed_cb)(void* user_data);
typedef void (*plusplayer_resource_conflicted_cb)(void* user_data);
typedef void (*plusplayer_buffering_cb)(int percent, void* user_data);
typedef void (*plusplayer_completed_cb)(void* user_data);
typedef void (*plusplayer_playing_cb)(void* user_data);

class PlusPlayerWrapperProxy {
 public:
  static PlusPlayerWrapperProxy& GetInstance() {
    static PlusPlayerWrapperProxy instance;
    return instance;
  }

  ~PlusPlayerWrapperProxy();
  PlusPlayerWrapperProxy(const PlusPlayerWrapperProxy&) = delete;
  PlusPlayerWrapperProxy& operator=(const PlusPlayerWrapperProxy&) = delete;

  PlusPlayerRef CreatePlayer();

  bool Open(PlusPlayerRef player, const char* uri);
  
  bool Close(PlusPlayerRef player);

  void SetAppId(PlusPlayerRef player, const char* app_id);

  void SetPrebufferMode(PlusPlayerRef player, bool is_prebuffer_mode);

  bool StopSource(PlusPlayerRef player);

  bool SetDisplay(PlusPlayerRef player, const DisplayType& type,
                  const uint32_t serface_id, const int x, const int y,
                  const int w, const int h);

  bool SetDisplayMode(PlusPlayerRef player, const DisplayMode& mode);
  bool SetDisplayRoi(PlusPlayerRef player, const Geometry& roi);

  bool SetDisplayRotate(PlusPlayerRef player, const DisplayRotation& rotate);

  bool GetDisplayRotate(PlusPlayerRef player, DisplayRotation* rotate);

  bool SetDisplayVisible(PlusPlayerRef player, bool is_visible);

  bool SetAudioMute(PlusPlayerRef player, bool is_mute);

  State GetState(PlusPlayerRef player);

  bool GetDuration(PlusPlayerRef player, int64_t* duration_in_milliseconds);

  bool GetPlayingTime(PlusPlayerRef player, uint64_t* time_in_milliseconds);
  bool SetPlaybackRate(PlusPlayerRef player, const double speed);

  bool Prepare(PlusPlayerRef player);

  bool PrepareAsync(PlusPlayerRef player);
  bool Start(PlusPlayerRef player);

  bool Stop(PlusPlayerRef player);

  bool Pause(PlusPlayerRef player);

  bool Resume(PlusPlayerRef player);

  bool Seek(PlusPlayerRef player, const uint64_t time_millisecond);

  void SetStopPosition(PlusPlayerRef player, const uint64_t time_millisecond);

  bool Suspend(PlusPlayerRef player);

  bool Restore(PlusPlayerRef player, State state);

  bool GetVideoSize(PlusPlayerRef player, int* width, int* height);

  void DestoryPlayer(PlusPlayerRef player);

  void SetCompletedCallback(PlusPlayerRef player,
                            plusplayer_completed_cb callback, void* user_data);

  void UnsetCompletedCallback(PlusPlayerRef player);

  void SetBufferingCallback(PlusPlayerRef player,
                            plusplayer_buffering_cb callback, void* user_data);

  void UnsetBufferingCallback(PlusPlayerRef player);

  void SetPreparedCallback(PlusPlayerRef player,
                           plusplayer_prepared_cb callback, void* user_data);

  void UnsetPreparedCallback(PlusPlayerRef player);

  void SetResourceConflictedCallback(PlusPlayerRef player,
                                     plusplayer_resource_conflicted_cb callback,
                                     void* user_data);

  void UnsetResourceConflictedCallback(PlusPlayerRef player);

  void SetPlayingCallback(PlusPlayerRef player, plusplayer_playing_cb callback,
                          void* user_data);

  void UnsetPlayingCallback(PlusPlayerRef player);

  int GetSurfaceId(PlusPlayerRef player, void* window);

 private:
  PlusPlayerWrapperProxy();
  void* plus_player_hander_{nullptr};
};
#endif