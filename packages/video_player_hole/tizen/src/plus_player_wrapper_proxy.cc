#include "plus_player_wrapper_proxy.h"

#include <dlfcn.h>

#include "log.h"

PlusPlayerWrapperProxy::PlusPlayerWrapperProxy() {
  plus_player_hander_ = dlopen("libplus_player_wrapper.so", RTLD_LAZY);
}
PlusPlayerWrapperProxy::~PlusPlayerWrapperProxy() {
  if (plus_player_hander_) {
    dlclose(plus_player_hander_);
    plus_player_hander_ = nullptr;
  }
}

PlusPlayerRef PlusPlayerWrapperProxy::CreatePlayer() {}

bool PlusPlayerWrapperProxy::Open(PlusPlayerRef player, const char* uri) {}

void PlusPlayerWrapperProxy::SetAppId(PlusPlayerRef player,
                                      const char* app_id) {}

void PlusPlayerWrapperProxy::SetPrebufferMode(PlusPlayerRef player,
                                              bool is_prebuffer_mode) {}

bool PlusPlayerWrapperProxy::StopSource(PlusPlayerRef player) {}

bool PlusPlayerWrapperProxy::SetDisplay(PlusPlayerRef player,
                                        const DisplayType& type,
                                        const uint32_t serface_id, const int x,
                                        const int y, const int w, const int h) {
}

bool PlusPlayerWrapperProxy::SetDisplayMode(PlusPlayerRef player,
                                            const DisplayMode& mode) {}
bool SetDisplayRoi(PlusPlayerRef player, const Geometry& roi);

bool PlusPlayerWrapperProxy::SetDisplayRotate(PlusPlayerRef player,
                                              const DisplayRotation& rotate) {}

bool PlusPlayerWrapperProxy::GetDisplayRotate(PlusPlayerRef player,
                                              DisplayRotation* rotate) {}

bool PlusPlayerWrapperProxy::SetDisplayVisible(PlusPlayerRef player,
                                               bool is_visible) {}

bool PlusPlayerWrapperProxy::SetAudioMute(PlusPlayerRef player, bool is_mute) {}

State PlusPlayerWrapperProxy::GetState(PlusPlayerRef player) {}

bool PlusPlayerWrapperProxy::GetDuration(PlusPlayerRef player,
                                         int64_t* duration_in_milliseconds) {}

bool PlusPlayerWrapperProxy::GetPlayingTime(PlusPlayerRef player,
                                            uint64_t* time_in_milliseconds) {}
bool PlusPlayerWrapperProxy::SetPlaybackRate(PlusPlayerRef player,
                                             const double speed) {}

bool PlusPlayerWrapperProxy::Prepare(PlusPlayerRef player) {}

bool PlusPlayerWrapperProxy::PrepareAsync(PlusPlayerRef player) {}
bool PlusPlayerWrapperProxy::Start(PlusPlayerRef player) {}

bool PlusPlayerWrapperProxy::Stop(PlusPlayerRef player) {}

bool PlusPlayerWrapperProxy::Pause(PlusPlayerRef player) {}

bool PlusPlayerWrapperProxy::Resume(PlusPlayerRef player) {}

bool PlusPlayerWrapperProxy::Seek(PlusPlayerRef player,
                                  const uint64_t time_millisecond) {}

void PlusPlayerWrapperProxy::SetStopPosition(PlusPlayerRef player,
                                             const uint64_t time_millisecond) {}

bool PlusPlayerWrapperProxy::Suspend(PlusPlayerRef player) {}

bool PlusPlayerWrapperProxy::Restore(PlusPlayerRef player, State state) {}

void PlusPlayerWrapperProxy::DestoryPlayer(PlusPlayerRef player) {}

void PlusPlayerWrapperProxy::SetCompletedCallback(
    PlusPlayerRef player, plusplayer_completed_cb callback, void* user_data) {}

void PlusPlayerWrapperProxy::UnsetCompletedCallback(PlusPlayerRef player) {}

void PlusPlayerWrapperProxy::SetBufferingCallback(
    PlusPlayerRef player, plusplayer_buffering_cb callback, void* user_data) {}

void PlusPlayerWrapperProxy::UnsetBufferingCallback(PlusPlayerRef player) {}

void PlusPlayerWrapperProxy::SetPreparedCallback(
    PlusPlayerRef player, plusplayer_prepared_cb callback, void* user_data) {}

void PlusPlayerWrapperProxy::UnsetPreparedCallback(PlusPlayerRef player) {}

void PlusPlayerWrapperProxy::SetResourceConflictedCallback(
    PlusPlayerRef player, plusplayer_resource_conflicted_cb callback,
    void* user_data) {}

void PlusPlayerWrapperProxy::UnsetResourceConflictedCallback(
    PlusPlayerRef player) {}

void PlusPlayerWrapperProxy::SetPlayingCallback(PlusPlayerRef player,
                                                plusplayer_playing_cb callback,
                                                void* user_data) {}

void PlusPlayerWrapperProxy::UnsetPlayingCallback(PlusPlayerRef player) {}
