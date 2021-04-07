#include "plus_player.h"

#include <Ecore.h>
#include <app_manager.h>

#include "log.h"
#include "video_player_error.h"

static std::string ErrorToString(plusplayer::ErrorType code) {
  std::string ret;
  switch (code) {
    case plusplayer::ErrorType::kNone:
      ret = "kNone";
      break;
    case plusplayer::ErrorType::kOutOfMemory:
      ret = "kOutOfMemory";
      break;
    case plusplayer::ErrorType::kInvalidParameter:
      ret = "kInvalidParameter";
      break;
    case plusplayer::ErrorType::kNoSuchFile:
      ret = "kNoSuchFile";
      break;
    case plusplayer::ErrorType::kInvalidOperation:
      ret = "kInvalidOperation";
      break;
    case plusplayer::ErrorType::kFileNoSpaceOnDevice:
      ret = "kFileNoSpaceOnDevice";
      break;
    case plusplayer::ErrorType::kFeatureNotSupportedOnDevice:
      ret = "kFeatureNotSupportedOnDevice";
      break;
    case plusplayer::ErrorType::kSeekFailed:
      ret = "kSeekFailed";
      break;
    case plusplayer::ErrorType::kInvalidState:
      ret = "kInvalidState";
      break;
    case plusplayer::ErrorType::kNotSupportedFile:
      ret = "kNotSupportedFile";
      break;
    case plusplayer::ErrorType::kInvalidUri:
      ret = "kInvalidUri";
      break;
    case plusplayer::ErrorType::kSoundPolicy:
      ret = "kSoundPolicy";
      break;
    case plusplayer::ErrorType::kConnectionFailed:
      ret = "kConnectionFailed";
      break;
    case plusplayer::ErrorType::kVideoCaptureFailed:
      ret = "kVideoCaptureFailed";
      break;
    case plusplayer::ErrorType::kDrmExpired:
      ret = "kDrmExpired";
      break;
    case plusplayer::ErrorType::kDrmNoLicense:
      ret = "kDrmNoLicense";
      break;
    case plusplayer::ErrorType::kDrmFutureUse:
      ret = "kDrmFutureUse";
      break;
    case plusplayer::ErrorType::kDrmNotPermitted:
      ret = "kDrmNotPermitted";
      break;
    case plusplayer::ErrorType::kResourceLimit:
      ret = "kResourceLimit";
      break;
    case plusplayer::ErrorType::kPermissionDenied:
      ret = "kPermissionDenied";
      break;
    case plusplayer::ErrorType::kServiceDisconnected:
      ret = "kServiceDisconnected";
      break;
    case plusplayer::ErrorType::kBufferSpace:
      ret = "kBufferSpace";
      break;
    case plusplayer::ErrorType::kNotSupportedAudioCodec:
      ret = "kNotSupportedAudioCodec";
      break;
    case plusplayer::ErrorType::kNotSupportedVideoCodec:
      ret = "kNotSupportedVideoCodec";
      break;
    case plusplayer::ErrorType::kNotSupportedSubtitle:
      ret = "kNotSupportedSubtitle";
      break;
    default:
      ret = "kUnknown";
      break;
  }
  return ret;
}

static int gPlayerIndex = 1;

PlusPlayer::PlusPlayer(const std::string &uri,
                       const VideoPlayerOptions &options) {
  LOG_DEBUG("PlusPlayer costructor");
  isLooping_ = false;
  auto player = plusplayer::PlusPlayer::Create();
  if (player != nullptr) {
    LOG_DEBUG("call Open to set uri (%s)", uri.c_str());
    if (!player->Open(uri)) {
      LOG_ERROR("Open uri(%s) failed", uri.c_str());
      throw VideoPlayerError("PlusPlayer - Open operation failed");
    }

    LOG_DEBUG("call SetAppId");
    char *appId = nullptr;
    long pid = getpid();
    int ret = app_manager_get_app_id(pid, &appId);
    if (ret == APP_MANAGER_ERROR_NONE) {
      LOG_DEBUG("set app id: %s", appId);
      std::string appIdStr(appId);
      player->SetAppId(appIdStr);
    }
    if (appId) {
      free(appId);
    }

    LOG_DEBUG("call RegisterListener");
    player->RegisterListener(listener_.get());

    // TODO, set display

    LOG_DEBUG("call PrepareAsync");
    if (!player->PrepareAsync()) {
      LOG_ERROR("PrepareAsync operation failed");
      throw VideoPlayerError("PlusPlayer - PrepareAsync operation failed");
    }
  } else {
    LOG_ERROR("Create operation failed");
    throw VideoPlayerError("PlusPlayer - Create operation failed");
  }

  textureId_ = gPlayerIndex++;

  player_ = std::move(player);
}

PlusPlayer::~PlusPlayer() {
  LOG_INFO("PlusPlayer destructor");
  dispose();
}

void PlusPlayer::play() {
  LOG_DEBUG("start PlusPlayer");
  if (player_ == nullptr) {
    LOG_ERROR("Plusplayer isn't created");
    throw VideoPlayerError("PlusPlayer - Not created");
  }

  if (player_->GetState() < plusplayer::State::kReady) {
    LOG_ERROR("Invalid state for play operation");
    throw VideoPlayerError("PlusPlayer - Invalid state for play operation");
  }

  if (player_->GetState() == plusplayer::State::kReady) {
    if (!player_->Start()) {
      LOG_ERROR("Start operation failed");
      throw VideoPlayerError("PlusPlayer - Start operation failed");
    }
  } else if (player_->GetState() == plusplayer::State::kPaused) {
    if (!player_->Resume()) {
      LOG_ERROR("Resume operation failed");
      throw VideoPlayerError("PlusPlayer - Resume operation failed");
    }
  }
}

void PlusPlayer::pause() {
  LOG_DEBUG("pause PlusPlayer");
  if (player_ == nullptr) {
    LOG_ERROR("Plusplayer isn't created");
    throw VideoPlayerError("PlusPlayer - Not created");
  }

  if (player_->GetState() < plusplayer::State::kReady) {
    LOG_ERROR("Invalid state for pause operation");
    throw VideoPlayerError("PlusPlayer - Invalid state for pause operation");
  }

  if (player_->GetState() == plusplayer::State::kPlaying) {
    if (!player_->Pause()) {
      LOG_ERROR("Pause operation failed");
      throw VideoPlayerError("PlusPlayer - Pause operation failed");
    }
  }
}

void PlusPlayer::setLooping(bool isLooping) {
  LOG_DEBUG("set looping: %d", isLooping);
  isLooping_ = isLooping;
  throw VideoPlayerError("PlusPlayer - Not support looping");
}

void PlusPlayer::setVolume(double volume) {
  LOG_ERROR("PlusPlayer doesn't support to set volume");
  throw VideoPlayerError("PlusPlayer - Not support to set volume");
}

void PlusPlayer::setPlaybackSpeed(double speed) {
  LOG_DEBUG("set playback speed: %f", speed);
  if (player_ == nullptr) {
    LOG_ERROR("Plusplayer isn't created");
    throw VideoPlayerError("PlusPlayer - Not created");
  }

  plusplayer::ErrorType error = plusplayer::ErrorType::kNone;
  if (!player_->SetPlaybackRate(speed, &error)) {
    LOG_ERROR("SetPlaybackRate failed: %s", ErrorToString(error).c_str());
    throw VideoPlayerError("PlusPlayer - SetPlaybackRate operation failed",
                           ErrorToString(error));
  }
}

void PlusPlayer::seekTo(int position) {
  LOG_DEBUG("seekTo position: %d", position);
  if (player_ == nullptr) {
    LOG_ERROR("Plusplayer isn't created");
    throw VideoPlayerError("PlusPlayer - Not created");
  }

  if (!player_->Seek((unsigned long long)position)) {
    LOG_ERROR("Seek to position %d failed", position);
    throw VideoPlayerError("PlusPlayer - Seek operation failed");
  }
}

int PlusPlayer::getPosition() {
  LOG_DEBUG("get video player position");
  if (player_ == nullptr) {
    LOG_ERROR("Plusplayer isn't created");
    throw VideoPlayerError("PlusPlayer - Not created");
  }

  plusplayer::State state = player_->GetState();
  if (state == plusplayer::State::kPlaying ||
      state == plusplayer::State::kPaused) {
    uint64_t position;
    player_->GetPlayingTime(&position);
    LOG_DEBUG("playing time: %lld", position);
    return (int)position;
  } else {
    LOG_ERROR("Invalid state for GetPlayingTime operation");
    throw VideoPlayerError(
        "PlusPlayer - Invalid state for GetPlayingTime operation");
  }
}

void PlusPlayer::dispose() {
  LOG_DEBUG("dispose PlusPlayer");

  if (player_) {
    player_->Stop();
    player_->Close();
    player_ = nullptr;
  }
}

void PlusPlayer::initialize() {
  if (player_ == nullptr) {
    LOG_ERROR("Plusplayer isn't created");
    throw VideoPlayerError("PlusPlayer - Not created");
  }

  if (player_->GetState() == plusplayer::State::kReady) {
    int64_t duration;
    if (!player_->GetDuration(&duration)) {
      LOG_ERROR("PlusPlayer - GetDuration operation failed");
      sendError("", "PlusPlayer - GetDuration operation failed");
      return;
    }
    LOG_DEBUG("video duration: %lld", duration);

    int width = 0, height = 0;
    std::vector<plusplayer::Track> tracks = player_->GetActiveTrackInfo();
    for (auto track : tracks) {
      if (track.type == plusplayer::TrackType::kTrackTypeVideo) {
        width = track.width;
        height = track.height;
      }
    }
    LOG_DEBUG("video widht: %d, video height: %d", width, height);

    plusplayer::DisplayRotation rotate;
    if (!player_->GetDisplayRotate(&rotate)) {
      LOG_ERROR("PlusPlayer - GetDisplayRotate operation failed");
      sendError("", "PlusPlayer - GetDisplayRotate operation failed");
    } else {
      if (rotate == plusplayer::DisplayRotation::kRotate90 ||
          rotate == plusplayer::DisplayRotation::kRotate270) {
        int tmp = width;
        width = height;
        height = tmp;
      }
    }

    sendInitialized((int)duration, width, height);
  } else {
    LOG_DEBUG("PlusPlayer isn't prepared, wait for ready");
  }
}

void PlusPlayer::PlusPlayerEventListener::OnErrorMsg(
    const plusplayer::ErrorType &error_code, const char *error_msg,
    UserData userdata) {
  LOG_DEBUG("Error occurred: %s", error_msg);
  if (handler_ != nullptr) {
    handler_->sendError(ErrorToString(error_code), error_msg);
  }
}

void PlusPlayer::PlusPlayerEventListener::OnResourceConflicted(
    UserData userdata) {
  LOG_DEBUG("Resource conflicted");
  if (handler_ != nullptr) {
    handler_->sendError("", "PlusPlayer - Resource conflicted");
  }
}

void PlusPlayer::PlusPlayerEventListener::OnEos(UserData userdata) {
  LOG_DEBUG("completed to playe video");
  if (handler_ != nullptr) {
    handler_->sendCompleted();
  }
}

void PlusPlayer::PlusPlayerEventListener::OnPrepareDone(bool ret,
                                                        UserData userdata) {
  if (!ret) {
    LOG_ERROR("Failed to prepare");
    if (handler_ != nullptr) {
      handler_->sendError("", "PlusPlayer - Prepare operation failed");
    }
    return;
  }

  LOG_DEBUG("PlusPlayer is prepared");
  if (handler_ != nullptr) {
    handler_->initialize();
  }
}
