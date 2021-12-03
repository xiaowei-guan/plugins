#include "video_player.h"

#include <dlfcn.h>
#include <flutter/event_stream_handler_functions.h>
#include <flutter/standard_method_codec.h>
#include <system_info.h>

#include <cstdarg>
#include <functional>

#include "log.h"
#include "video_player_error.h"

static int gPlayerIndex = 1;

static std::string RotationToString(player_display_rotation_e rotation) {
  std::string ret;
  switch (rotation) {
    case PLAYER_DISPLAY_ROTATION_NONE:
      ret = "PLAYER_DISPLAY_ROTATION_NONE";
      break;
    case PLAYER_DISPLAY_ROTATION_90:
      ret = "PLAYER_DISPLAY_ROTATION_90";
      break;
    case PLAYER_DISPLAY_ROTATION_180:
      ret = "PLAYER_DISPLAY_ROTATION_180";
      break;
    case PLAYER_DISPLAY_ROTATION_270:
      ret = "PLAYER_DISPLAY_ROTATION_270";
      break;
  }
  return ret;
}

static std::string StateToString(player_state_e state) {
  std::string ret;
  switch (state) {
    case PLAYER_STATE_NONE:
      ret = "PLAYER_STATE_NONE";
      break;
    case PLAYER_STATE_IDLE:
      ret = "PLAYER_STATE_IDLE";
      break;
    case PLAYER_STATE_READY:
      ret = "PLAYER_STATE_READY";
      break;
    case PLAYER_STATE_PLAYING:
      ret = "PLAYER_STATE_PLAYING";
      break;
    case PLAYER_STATE_PAUSED:
      ret = "PLAYER_STATE_PAUSED";
      break;
  }
  return ret;
}

enum DeviceProfile { kUnknown, kMobile, kWearable, kTV, kCommon };

static DeviceProfile GetDeviceProfile() {
  char *feature_profile = nullptr;
  system_info_get_platform_string("http://tizen.org/feature/profile",
                                  &feature_profile);
  if (feature_profile == nullptr) {
    return DeviceProfile::kUnknown;
  }
  std::string profile(feature_profile);
  free(feature_profile);

  if (profile == "mobile") {
    return DeviceProfile::kMobile;
  } else if (profile == "wearable") {
    return DeviceProfile::kWearable;
  } else if (profile == "tv") {
    return DeviceProfile::kTV;
  } else if (profile == "common") {
    return DeviceProfile::kCommon;
  }
  return DeviceProfile::kUnknown;
}

VideoPlayer::VideoPlayer(FlutterDesktopPluginRegistrarRef registrar_ref,
                         flutter::PluginRegistrar *plugin_registrar,
                         const std::string &uri, VideoPlayerOptions &options) {
  is_initialized_ = false;
  is_interrupted_ = false;

  LOG_INFO("[VideoPlayer] register texture");
  // texture_id_ = texture_registrar->RegisterTexture(texture_variant_.get());
  texture_id_ = gPlayerIndex++;
  LOG_DEBUG("[VideoPlayer] call player_create to create player");
  int ret = player_create(&player_);
  if (ret != PLAYER_ERROR_NONE) {
    LOG_ERROR("[VideoPlayer] player_create failed: %s", get_error_message(ret));
    throw VideoPlayerError("player_create failed", get_error_message(ret));
  }
  int width = 0;
  int height = 0;
  if (system_info_get_platform_int("http://tizen.org/feature/screen.width",
                                   &width) != SYSTEM_INFO_ERROR_NONE &&
      system_info_get_platform_int("http://tizen.org/feature/screen.height",
                                   &height) != SYSTEM_INFO_ERROR_NONE) {
    LOG_ERROR("Could not obtain the screen size.");
  }
  if (GetDeviceProfile() == kWearable) {
    ret = player_set_display(player_, PLAYER_DISPLAY_TYPE_OVERLAY,
                             FlutterDesktopGetWindow(registrar_ref));
  } else {
    ret = PLAYER_ERROR_NOT_AVAILABLE;
    void *libHandle = dlopen("libcapi-media-player.so.0", RTLD_LAZY);
    int (*player_set_ecore_wl_display)(
        player_h player, player_display_type_e type, void *ecore_wl_window,
        int x, int y, int width, int height);
    if (libHandle) {
      *(void **)(&player_set_ecore_wl_display) =
          dlsym(libHandle, "player_set_ecore_wl_display");
      if (player_set_ecore_wl_display) {
        ret = player_set_ecore_wl_display(
            player_, PLAYER_DISPLAY_TYPE_OVERLAY,
            FlutterDesktopGetWindow(registrar_ref), 0, 0, width, height);
      } else {
        LOG_ERROR("[VideoPlayer] Symbol not found %s: ", dlerror());
      }
      dlclose(libHandle);
    } else {
      LOG_DEBUG("[VideoPlayer] dlopen failed %s: ", dlerror());
    }
  }

  if (ret != PLAYER_ERROR_NONE) {
    player_destroy(player_);
    LOG_ERROR("[VideoPlayer] player_set_ecore_wl_display failed: %s",
              get_error_message(ret));
    throw VideoPlayerError("player_set_ecore_wl_display failed",
                           get_error_message(ret));
  }

  ret = player_set_display_mode(player_, PLAYER_DISPLAY_MODE_DST_ROI);
  if (ret != PLAYER_ERROR_NONE) {
    player_destroy(player_);
    LOG_ERROR("[VideoPlayer] player_get_display_mode failed: %s",
              get_error_message(ret));
    throw VideoPlayerError("player_get_display_mode failed",
                           get_error_message(ret));
  }

  LOG_DEBUG("[VideoPlayer] call player_set_uri to set video path (%s)",
            uri.c_str());
  ret = player_set_uri(player_, uri.c_str());
  if (ret != PLAYER_ERROR_NONE) {
    player_destroy(player_);
    LOG_ERROR("[VideoPlayer] player_set_uri failed: %s",
              get_error_message(ret));
    throw VideoPlayerError("player_set_uri failed", get_error_message(ret));
  }

  LOG_DEBUG("[VideoPlayer] call player_set_buffering_cb");
  ret = player_set_buffering_cb(player_, onBuffering, (void *)this);
  if (ret != PLAYER_ERROR_NONE) {
    player_destroy(player_);
    LOG_ERROR("[VideoPlayer] player_set_buffering_cb failed: %s",
              get_error_message(ret));
    throw VideoPlayerError("player_set_buffering_cb failed",
                           get_error_message(ret));
  }

  LOG_DEBUG("[VideoPlayer] call player_set_completed_cb");
  ret = player_set_completed_cb(player_, onPlayCompleted, (void *)this);
  if (ret != PLAYER_ERROR_NONE) {
    player_destroy(player_);
    LOG_ERROR("[VideoPlayer] player_set_completed_cb failed: %s",
              get_error_message(ret));
    throw VideoPlayerError("player_set_completed_cb failed",
                           get_error_message(ret));
  }

  LOG_DEBUG("[VideoPlayer] call player_set_interrupted_cb");
  ret = player_set_interrupted_cb(player_, onInterrupted, (void *)this);
  if (ret != PLAYER_ERROR_NONE) {
    player_destroy(player_);
    LOG_ERROR("[VideoPlayer] player_set_interrupted_cb failed: %s",
              get_error_message(ret));
    throw VideoPlayerError("player_set_interrupted_cb failed",
                           get_error_message(ret));
  }

  LOG_DEBUG("[VideoPlayer] call player_set_display_visible");
  ret = player_set_display_visible(player_, true);
  if (ret != PLAYER_ERROR_NONE) {
    player_destroy(player_);
    LOG_ERROR("[VideoPlayer] player_set_display_visible failed: %s",
              get_error_message(ret));
    throw VideoPlayerError("player_set_display_visible failed",
                           get_error_message(ret));
  }

  LOG_DEBUG("[VideoPlayer] call player_set_error_cb");
  ret = player_set_error_cb(player_, onErrorOccurred, (void *)this);
  if (ret != PLAYER_ERROR_NONE) {
    player_destroy(player_);
    LOG_ERROR("[VideoPlayer] player_set_error_cb failed: %s",
              get_error_message(ret));
    throw VideoPlayerError("player_set_error_cb failed",
                           get_error_message(ret));
  }

  LOG_DEBUG("[VideoPlayer] call player_prepare_async");
  ret = player_prepare_async(player_, onPrepared, (void *)this);
  if (ret != PLAYER_ERROR_NONE) {
    player_destroy(player_);
    LOG_ERROR("[VideoPlayer] player_prepare_async failed: %s",
              get_error_message(ret));
    throw VideoPlayerError("player_prepare_async failed",
                           get_error_message(ret));
  }
  setupEventChannel(plugin_registrar->messenger());
}

void VideoPlayer::setDisplayRoi(int x, int y, int w, int h) {
  if (player_ == nullptr) {
    LOG_ERROR("VideoPlayer isn't created");
  }
  int ret = player_set_display_roi_area(player_, x, y, w, h);

  if (ret != PLAYER_ERROR_NONE) {
    LOG_ERROR("Plusplayer SetDisplayRoi failed");
  }
}

VideoPlayer::~VideoPlayer() {
  LOG_INFO("[VideoPlayer] destructor");
  dispose();
}

long VideoPlayer::getTextureId() { return texture_id_; }

void VideoPlayer::play() {
  LOG_DEBUG("[VideoPlayer.play] start player");
  player_state_e state;
  int ret = player_get_state(player_, &state);
  if (ret == PLAYER_ERROR_NONE) {
    LOG_INFO("[VideoPlayer.play] player state: %s",
             StateToString(state).c_str());
    if (state != PLAYER_STATE_PAUSED && state != PLAYER_STATE_READY) {
      return;
    }
  }

  ret = player_start(player_);
  if (ret != PLAYER_ERROR_NONE) {
    LOG_ERROR("[VideoPlayer.play] player_start failed: %s",
              get_error_message(ret));
    throw VideoPlayerError("player_start failed", get_error_message(ret));
  }
}

void VideoPlayer::pause() {
  LOG_DEBUG("[VideoPlayer.pause] pause player");
  player_state_e state;
  int ret = player_get_state(player_, &state);
  if (ret == PLAYER_ERROR_NONE) {
    LOG_INFO("[VideoPlayer.pause] player state: %s",
             StateToString(state).c_str());
    if (state != PLAYER_STATE_PLAYING) {
      return;
    }
  }

  ret = player_pause(player_);
  if (ret != PLAYER_ERROR_NONE) {
    LOG_ERROR("[VideoPlayer.pause] player_pause failed: %s",
              get_error_message(ret));
    throw VideoPlayerError("player_pause failed", get_error_message(ret));
  }
}

void VideoPlayer::setLooping(bool is_looping) {
  LOG_DEBUG("[VideoPlayer.setLooping] isLooping: %d", is_looping);
  int ret = player_set_looping(player_, is_looping);
  if (ret != PLAYER_ERROR_NONE) {
    LOG_ERROR("[VideoPlayer.setLooping] player_set_looping failed: %s",
              get_error_message(ret));
    throw VideoPlayerError("player_set_looping failed", get_error_message(ret));
  }
}

void VideoPlayer::setVolume(double volume) {
  LOG_DEBUG("[VideoPlayer.setVolume] volume: %f", volume);
  int ret = player_set_volume(player_, volume, volume);
  if (ret != PLAYER_ERROR_NONE) {
    LOG_ERROR("[VideoPlayer.setVolume] player_set_volume failed: %s",
              get_error_message(ret));
    throw VideoPlayerError("player_set_volume failed", get_error_message(ret));
  }
}

void VideoPlayer::setPlaybackSpeed(double speed) {
  LOG_DEBUG("[VideoPlayer.setPlaybackSpeed] speed: %f", speed);
  int ret = player_set_playback_rate(player_, speed);
  if (ret != PLAYER_ERROR_NONE) {
    LOG_ERROR(
        "[VideoPlayer.setPlaybackSpeed] player_set_playback_rate failed: %s",
        get_error_message(ret));
    throw VideoPlayerError("player_set_playback_rate failed",
                           get_error_message(ret));
  }
}

void VideoPlayer::seekTo(int position,
                         const SeekCompletedCb &seek_completed_cb) {
  LOG_DEBUG("[VideoPlayer.seekTo] position: %d", position);
  on_seek_completed_ = seek_completed_cb;
  int ret =
      player_set_play_position(player_, position, true, onSeekCompleted, this);
  if (ret != PLAYER_ERROR_NONE) {
    on_seek_completed_ = nullptr;
    LOG_ERROR("[VideoPlayer.seekTo] player_set_play_position failed: %s",
              get_error_message(ret));
    throw VideoPlayerError("player_set_play_position failed",
                           get_error_message(ret));
  }
}

int VideoPlayer::getPosition() {
  int position;
  int ret = player_get_play_position(player_, &position);
  if (ret != PLAYER_ERROR_NONE) {
    LOG_ERROR("[VideoPlayer.getPosition] player_get_play_position failed: %s",
              get_error_message(ret));
    throw VideoPlayerError("player_get_play_position failed",
                           get_error_message(ret));
  }
  return position;
}

void VideoPlayer::dispose() {
  LOG_DEBUG("[VideoPlayer.dispose] dispose video player");
  is_initialized_ = false;
  event_sink_ = nullptr;
  event_channel_->SetStreamHandler(nullptr);

  if (player_) {
    player_unprepare(player_);
    player_unset_media_packet_video_frame_decoded_cb(player_);
    player_unset_buffering_cb(player_);
    player_unset_completed_cb(player_);
    player_unset_interrupted_cb(player_);
    player_unset_error_cb(player_);
    player_destroy(player_);
    player_ = 0;
  }
}

void VideoPlayer::setupEventChannel(flutter::BinaryMessenger *messenger) {
  LOG_DEBUG("[VideoPlayer.setupEventChannel] setup event channel");
  std::string name =
      "flutter.io/videoPlayer/videoEvents" + std::to_string(texture_id_);
  auto channel =
      std::make_unique<flutter::EventChannel<flutter::EncodableValue>>(
          messenger, name, &flutter::StandardMethodCodec::GetInstance());
  // SetStreamHandler be called after player_prepare,
  // because initialized event will be send in listen function of event channel
  auto handler = std::make_unique<
      flutter::StreamHandlerFunctions<flutter::EncodableValue>>(
      [&](const flutter::EncodableValue *arguments,
          std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> &&events)
          -> std::unique_ptr<
              flutter::StreamHandlerError<flutter::EncodableValue>> {
        LOG_DEBUG(
            "[VideoPlayer.setupEventChannel] call listen of StreamHandler");
        event_sink_ = std::move(events);
        initialize();
        return nullptr;
      },
      [&](const flutter::EncodableValue *arguments)
          -> std::unique_ptr<
              flutter::StreamHandlerError<flutter::EncodableValue>> {
        LOG_DEBUG(
            "[VideoPlayer.setupEventChannel] call cancel of StreamHandler");
        event_sink_ = nullptr;
        return nullptr;
      });
  channel->SetStreamHandler(std::move(handler));
  event_channel_ = std::move(channel);
}

void VideoPlayer::initialize() {
  player_state_e state;
  int ret = player_get_state(player_, &state);
  if (ret == PLAYER_ERROR_NONE) {
    LOG_INFO("[VideoPlayer.initialize] player state: %s",
             StateToString(state).c_str());
    if (state == PLAYER_STATE_READY && !is_initialized_) {
      sendInitialized();
    }
  } else {
    LOG_ERROR("[VideoPlayer.initialize] player_get_state failed: %s",
              get_error_message(ret));
  }
}

void VideoPlayer::sendInitialized() {
  if (!is_initialized_ && !is_interrupted_ && event_sink_ != nullptr) {
    int duration;
    int ret = player_get_duration(player_, &duration);
    if (ret != PLAYER_ERROR_NONE) {
      LOG_ERROR("[VideoPlayer.sendInitialized] player_get_duration failed: %s",
                get_error_message(ret));
      event_sink_->Error("player_get_duration failed", get_error_message(ret));
      return;
    }
    LOG_DEBUG("[VideoPlayer.sendInitialized] video duration: %d", duration);

    int width, height;
    ret = player_get_video_size(player_, &width, &height);
    if (ret != PLAYER_ERROR_NONE) {
      LOG_ERROR(
          "[VideoPlayer.sendInitialized] player_get_video_size failed: %s",
          get_error_message(ret));
      event_sink_->Error("player_get_video_size failed",
                         get_error_message(ret));
      return;
    }
    LOG_DEBUG("[VideoPlayer.sendInitialized] video width: %d, height: %d",
              width, height);

    player_display_rotation_e rotation;
    ret = player_get_display_rotation(player_, &rotation);
    if (ret != PLAYER_ERROR_NONE) {
      LOG_ERROR(
          "[VideoPlayer.sendInitialized] player_get_display_rotation "
          "failed: %s",
          get_error_message(ret));
    } else {
      LOG_DEBUG("[VideoPlayer.sendInitialized] rotation: %s",
                RotationToString(rotation).c_str());
      if (rotation == PLAYER_DISPLAY_ROTATION_90 ||
          rotation == PLAYER_DISPLAY_ROTATION_270) {
        int tmp = width;
        width = height;
        height = tmp;
      }
    }

    is_initialized_ = true;
    flutter::EncodableMap encodables = {
        {flutter::EncodableValue("event"),
         flutter::EncodableValue("initialized")},
        {flutter::EncodableValue("duration"),
         flutter::EncodableValue(duration)},
        {flutter::EncodableValue("width"), flutter::EncodableValue(width)},
        {flutter::EncodableValue("height"), flutter::EncodableValue(height)}};
    flutter::EncodableValue eventValue(encodables);
    LOG_INFO("[VideoPlayer.sendInitialized] send initialized event");
    event_sink_->Success(eventValue);
  }
}

void VideoPlayer::sendBufferingStart() {
  if (event_sink_) {
    flutter::EncodableMap encodables = {
        {flutter::EncodableValue("event"),
         flutter::EncodableValue("bufferingStart")}};
    flutter::EncodableValue eventValue(encodables);
    LOG_INFO("[VideoPlayer.onBuffering] send bufferingStart event");
    event_sink_->Success(eventValue);
  }
}

void VideoPlayer::sendBufferingUpdate(int position) {
  if (event_sink_) {
    flutter::EncodableList range = {flutter::EncodableValue(0),
                                    flutter::EncodableValue(position)};
    flutter::EncodableList rangeList = {flutter::EncodableValue(range)};
    flutter::EncodableMap encodables = {
        {flutter::EncodableValue("event"),
         flutter::EncodableValue("bufferingUpdate")},
        {flutter::EncodableValue("values"),
         flutter::EncodableValue(rangeList)}};
    flutter::EncodableValue eventValue(encodables);
    LOG_INFO("[VideoPlayer.onBuffering] send bufferingUpdate event");
    event_sink_->Success(eventValue);
  }
}

void VideoPlayer::sendBufferingEnd() {
  if (event_sink_) {
    flutter::EncodableMap encodables = {
        {flutter::EncodableValue("event"),
         flutter::EncodableValue("bufferingEnd")}};
    flutter::EncodableValue eventValue(encodables);
    LOG_INFO("[VideoPlayer.onBuffering] send bufferingEnd event");
    event_sink_->Success(eventValue);
  }
}

void VideoPlayer::onPrepared(void *data) {
  VideoPlayer *player = (VideoPlayer *)data;
  LOG_DEBUG("[VideoPlayer.onPrepared] video player is prepared");

  if (!player->is_initialized_) {
    player->sendInitialized();
  }
}

void VideoPlayer::onBuffering(int percent, void *data) {
  // percent isn't used for video size, it's the used storage of buffer
  LOG_DEBUG("[VideoPlayer.onBuffering] percent: %d", percent);
}

void VideoPlayer::onSeekCompleted(void *data) {
  VideoPlayer *player = (VideoPlayer *)data;
  LOG_DEBUG("[VideoPlayer.onSeekCompleted] completed to seek");

  if (player->on_seek_completed_) {
    player->on_seek_completed_();
    player->on_seek_completed_ = nullptr;
  }
}

void VideoPlayer::onPlayCompleted(void *data) {
  VideoPlayer *player = (VideoPlayer *)data;
  LOG_DEBUG("[VideoPlayer.onPlayCompleted] completed to play video");

  if (player->event_sink_) {
    flutter::EncodableMap encodables = {{flutter::EncodableValue("event"),
                                         flutter::EncodableValue("completed")}};
    flutter::EncodableValue eventValue(encodables);
    LOG_INFO("[VideoPlayer.onPlayCompleted] send completed event");
    player->event_sink_->Success(eventValue);

    LOG_DEBUG("[VideoPlayer.onPlayCompleted] change player state to pause");
    player->pause();
  }
}

void VideoPlayer::onInterrupted(player_interrupted_code_e code, void *data) {
  VideoPlayer *player = (VideoPlayer *)data;
  LOG_DEBUG("[VideoPlayer.onInterrupted] interrupted code: %d", code);

  player->is_interrupted_ = true;
  if (player->event_sink_) {
    LOG_INFO("[VideoPlayer.onInterrupted] send error event");
    player->event_sink_->Error("Video player had error",
                               "Video player is interrupted");
  }
}

void VideoPlayer::onErrorOccurred(int code, void *data) {
  VideoPlayer *player = (VideoPlayer *)data;
  LOG_DEBUG("[VideoPlayer.onErrorOccurred] error code: %s",
            get_error_message(code));

  if (player->event_sink_) {
    LOG_INFO("[VideoPlayer.onErrorOccurred] send error event");
    player->event_sink_->Error("Video player had error",
                               get_error_message(code));
  }
}
