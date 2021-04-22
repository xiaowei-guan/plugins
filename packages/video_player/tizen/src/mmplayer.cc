#include "mmplayer.h"

#include "log.h"
#include "video_player_error.h"

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

static std::string ErrorToString(int code) {
  std::string ret;
  switch (code) {
    case PLAYER_ERROR_NONE:
      ret = "PLAYER_ERROR_NONE";
      break;
    case PLAYER_ERROR_INVALID_PARAMETER:
      ret = "PLAYER_ERROR_INVALID_PARAMETER";
      break;
    case PLAYER_ERROR_OUT_OF_MEMORY:
      ret = "PLAYER_ERROR_OUT_OF_MEMORY";
      break;
    case PLAYER_ERROR_INVALID_OPERATION:
      ret = "PLAYER_ERROR_INVALID_OPERATION";
      break;
    case PLAYER_ERROR_RESOURCE_LIMIT:
      ret = "PLAYER_ERROR_RESOURCE_LIMIT";
      break;
    case PLAYER_ERROR_FILE_NO_SPACE_ON_DEVICE:
      ret = "PLAYER_ERROR_FILE_NO_SPACE_ON_DEVICE";
      break;
    case PLAYER_ERROR_INVALID_STATE:
      ret = "PLAYER_ERROR_INVALID_STATE";
      break;
    case PLAYER_ERROR_INVALID_URI:
      ret = "PLAYER_ERROR_INVALID_URI";
      break;
    case PLAYER_ERROR_NO_SUCH_FILE:
      ret = "PLAYER_ERROR_NO_SUCH_FILE";
      break;
    case PLAYER_ERROR_NOT_SUPPORTED_FILE:
      ret = "PLAYER_ERROR_NOT_SUPPORTED_FILE";
      break;
    case PLAYER_ERROR_CONNECTION_FAILED:
      ret = "PLAYER_ERROR_CONNECTION_FAILED";
      break;
    case PLAYER_ERROR_SEEK_FAILED:
      ret = "PLAYER_ERROR_SEEK_FAILED";
      break;
    case PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE:
      ret = "PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE";
      break;
    case PLAYER_ERROR_DRM_NOT_PERMITTED:
      ret = "PLAYER_ERROR_DRM_NOT_PERMITTED";
      break;
    case PLAYER_ERROR_SERVICE_DISCONNECTED:
      ret = "PLAYER_ERROR_SERVICE_DISCONNECTED";
      break;
    case PLAYER_ERROR_NOT_SUPPORTED_SUBTITLE:
      ret = "PLAYER_ERROR_NOT_SUPPORTED_SUBTITLE";
      break;
    case PLAYER_ERROR_NOT_SUPPORTED_AUDIO_CODEC:
      ret = "PLAYER_ERROR_NOT_SUPPORTED_AUDIO_CODEC";
      break;
    case PLAYER_ERROR_NOT_SUPPORTED_VIDEO_CODEC:
      ret = "PLAYER_ERROR_NOT_SUPPORTED_VIDEO_CODEC";
      break;
    default:
      ret = "PLAYER_ERROR_UNKNOWN";
      break;
  }
  return ret;
}

MMPlayer::MMPlayer(FlutterTextureRegistrar *textureRegistrar,
                   const std::string &uri, const VideoPlayerOptions &options) {
  textureRegistrar_ = textureRegistrar;

  LOG_INFO("register texture");
  textureId_ = FlutterRegisterExternalTexture(textureRegistrar_);

  LOG_DEBUG("call player_create to create mmplayer");
  int ret = player_create(&player_);
  if (ret != PLAYER_ERROR_NONE) {
    LOG_ERROR("player_create failed: %s", ErrorToString(ret).c_str());
    throw VideoPlayerError("player_create failed", ErrorToString(ret));
  }

  LOG_DEBUG("call player_set_uri to set video path (%s)", uri.c_str());
  ret = player_set_uri(player_, uri.c_str());
  if (ret != PLAYER_ERROR_NONE) {
    player_destroy(player_);
    LOG_ERROR("player_set_uri failed: %s", ErrorToString(ret).c_str());
    throw VideoPlayerError("player_set_uri failed", ErrorToString(ret));
  }

  LOG_DEBUG("call player_set_media_packet_video_frame_decoded_cb");
  ret = player_set_media_packet_video_frame_decoded_cb(
      player_, onVideoFrameDecoded, (void *)this);
  if (ret != PLAYER_ERROR_NONE) {
    player_destroy(player_);
    LOG_ERROR("player_set_media_packet_video_frame_decoded_cb failed: %s",
              ErrorToString(ret).c_str());
    throw VideoPlayerError(
        "player_set_media_packet_video_frame_decoded_cb failed",
        ErrorToString(ret));
  }

  LOG_DEBUG("call player_set_completed_cb");
  ret = player_set_completed_cb(player_, onPlayCompleted, (void *)this);
  if (ret != PLAYER_ERROR_NONE) {
    player_destroy(player_);
    LOG_ERROR("player_set_completed_cb failed: %s", ErrorToString(ret).c_str());
    throw VideoPlayerError("player_set_completed_cb failed",
                           ErrorToString(ret));
  }

  LOG_DEBUG("call player_set_interrupted_cb");
  ret = player_set_interrupted_cb(player_, onInterrupted, (void *)this);
  if (ret != PLAYER_ERROR_NONE) {
    player_destroy(player_);
    LOG_ERROR("player_set_interrupted_cb failed: %s",
              ErrorToString(ret).c_str());
    throw VideoPlayerError("player_set_interrupted_cb failed",
                           ErrorToString(ret));
  }

  LOG_DEBUG("call player_set_error_cb");
  ret = player_set_error_cb(player_, onErrorOccurred, (void *)this);
  if (ret != PLAYER_ERROR_NONE) {
    player_destroy(player_);
    LOG_ERROR("player_set_error_cb failed: %s", ErrorToString(ret).c_str());
    throw VideoPlayerError("player_set_error_cb failed", ErrorToString(ret));
  }

  LOG_DEBUG("call player_prepare_async");
  ret = player_prepare_async(player_, onPrepared, (void *)this);
  if (ret != PLAYER_ERROR_NONE) {
    player_destroy(player_);
    LOG_ERROR("player_prepare_async failed: %s", ErrorToString(ret).c_str());
    throw VideoPlayerError("player_prepare_async failed", ErrorToString(ret));
  }
}

MMPlayer::~MMPlayer() {
  LOG_INFO("mmplayer destructor");
  dispose();
}

void MMPlayer::play() {
  LOG_DEBUG("start player");
  player_state_e state;
  int ret = player_get_state(player_, &state);
  if (ret == PLAYER_ERROR_NONE) {
    LOG_INFO(" player state: %s", StateToString(state).c_str());
    if (state != PLAYER_STATE_PAUSED && state != PLAYER_STATE_READY) {
      return;
    }
  }

  ret = player_start(player_);
  if (ret != PLAYER_ERROR_NONE) {
    LOG_ERROR("player_start failed: %s", ErrorToString(ret).c_str());
    throw VideoPlayerError("player_start failed", ErrorToString(ret));
  }
}

void MMPlayer::pause() {
  LOG_DEBUG("pause player");
  player_state_e state;
  int ret = player_get_state(player_, &state);
  if (ret == PLAYER_ERROR_NONE) {
    LOG_INFO("player state: %s", StateToString(state).c_str());
    if (state != PLAYER_STATE_PLAYING) {
      return;
    }
  }

  ret = player_pause(player_);
  if (ret != PLAYER_ERROR_NONE) {
    LOG_ERROR("player_pause failed: %s", ErrorToString(ret).c_str());
    throw VideoPlayerError("player_pause failed", ErrorToString(ret));
  }
}

void MMPlayer::setDisplayRoi(int x, int y, int w, int h){
  LOG_DEBUG("MM player not support set roi");
}

void MMPlayer::setLooping(bool isLooping) {
  LOG_DEBUG("set video loop: %d", isLooping);
  int ret = player_set_looping(player_, isLooping);
  if (ret != PLAYER_ERROR_NONE) {
    LOG_ERROR("player_set_looping failed: %s", ErrorToString(ret).c_str());
    throw VideoPlayerError("player_set_looping failed", ErrorToString(ret));
  }
}

void MMPlayer::setVolume(double volume) {
  LOG_DEBUG("set volume: %f", volume);
  int ret = player_set_volume(player_, volume, volume);
  if (ret != PLAYER_ERROR_NONE) {
    LOG_ERROR("player_set_volume failed: %s", ErrorToString(ret).c_str());
    throw VideoPlayerError("player_set_volume failed", ErrorToString(ret));
  }
}

void MMPlayer::setPlaybackSpeed(double speed) {
  LOG_DEBUG("set playback speed: %f", speed);
  int ret = player_set_playback_rate(player_, speed);
  if (ret != PLAYER_ERROR_NONE) {
    LOG_ERROR("player_set_playback_rate failed: %s",
              ErrorToString(ret).c_str());
    throw VideoPlayerError("player_set_playback_rate failed",
                           ErrorToString(ret));
  }
}

void MMPlayer::seekTo(int position) {
  LOG_DEBUG("seek to position: %d", position);
  int ret = player_set_play_position(player_, position, true, nullptr, nullptr);
  if (ret != PLAYER_ERROR_NONE) {
    LOG_ERROR("player_set_play_position failed: %s",
              ErrorToString(ret).c_str());
    throw VideoPlayerError("player_set_play_position failed",
                           ErrorToString(ret));
  }
}

int MMPlayer::getPosition() {
  LOG_DEBUG("get video player position");
  int position;
  int ret = player_get_play_position(player_, &position);
  if (ret != PLAYER_ERROR_NONE) {
    LOG_ERROR("player_get_play_position failed: %s",
              ErrorToString(ret).c_str());
    throw VideoPlayerError("player_get_play_position failed",
                           ErrorToString(ret));
  }

  LOG_DEBUG("currently position: %d", position);
  return position;
}

void MMPlayer::dispose() {
  LOG_DEBUG("dispose video player");

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

  if (textureRegistrar_) {
    FlutterUnregisterExternalTexture(textureRegistrar_, textureId_);
    textureRegistrar_ = nullptr;
  }
}

void MMPlayer::initialize() {
  player_state_e state;
  int ret = player_get_state(player_, &state);
  if (ret == PLAYER_ERROR_NONE) {
    LOG_INFO("player state: %s", StateToString(state).c_str());
    if (state == PLAYER_STATE_READY) {
      int duration;
      int ret = player_get_duration(player_, &duration);
      if (ret != PLAYER_ERROR_NONE) {
        LOG_ERROR("player_get_duration failed: %s", ErrorToString(ret).c_str());
        sendError(ErrorToString(ret), "player_get_duration failed");
        return;
      }
      LOG_DEBUG("video duration: %d", duration);

      int width, height;
      ret = player_get_video_size(player_, &width, &height);
      if (ret != PLAYER_ERROR_NONE) {
        LOG_ERROR("player_get_video_size failed: %s",
                  ErrorToString(ret).c_str());
        sendError(ErrorToString(ret), "player_get_video_size failed");
        return;
      }
      LOG_DEBUG("video width: %d, height: %d", width, height);

      player_display_rotation_e rotation;
      ret = player_get_display_rotation(player_, &rotation);
      if (ret != PLAYER_ERROR_NONE) {
        LOG_ERROR(
            "[VideoPlayer.sendInitialized] player_get_display_rotation "
            "failed: %s",
            ErrorToString(ret).c_str());
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

      sendInitialized(duration, width, height);
    }
  } else {
    LOG_ERROR("player_get_state failed: %s", ErrorToString(ret).c_str());
  }
}

void MMPlayer::onPrepared(void *data) {
  LOG_DEBUG("video player is prepared");
  MMPlayer *player = (MMPlayer *)data;
  if (!player->isInitialized_) {
    player->initialize();
  }
}

void MMPlayer::onPlayCompleted(void *data) {
  LOG_DEBUG("completed to playe video");
  MMPlayer *player = (MMPlayer *)data;
  player->sendCompleted();
}

void MMPlayer::onInterrupted(player_interrupted_code_e code, void *data) {
  LOG_DEBUG("interrupted code: %d", code);
  MMPlayer *player = (MMPlayer *)data;
  player->sendError("VideoInterrupted", "Video player is interrupted");
}

void MMPlayer::onErrorOccurred(int code, void *data) {
  LOG_DEBUG("error code: %s", ErrorToString(code).c_str());
  MMPlayer *player = (MMPlayer *)data;
  player->sendError(ErrorToString(code), "Video player had error");
}

void MMPlayer::onVideoFrameDecoded(media_packet_h packet, void *data) {
  MMPlayer *player = (MMPlayer *)data;
  tbm_surface_h surface;
  int ret = media_packet_get_tbm_surface(packet, &surface);
  if (ret != MEDIA_PACKET_ERROR_NONE) {
    LOG_ERROR(
        "[VideoPlayer.onVideoFrameDecoded] media_packet_get_tbm_surface "
        "failed, error: %d",
        ret);
    media_packet_destroy(packet);
    return;
  }
  FlutterMarkExternalTextureFrameAvailable(player->textureRegistrar_,
                                           player->textureId_, surface);
  media_packet_destroy(packet);
}
