#include "video_player.h"

#include <app_manager.h>
#include <dlfcn.h>
#include <flutter/event_stream_handler_functions.h>
#include <flutter/standard_method_codec.h>
#include <system_info.h>

#include <cstdarg>
#include <functional>

#include "log.h"
#include "video_player_error.h"
//#define EFL_BETA_API_SUPPORT
#include <Ecore_Wl2.h>

static int gPlayerIndex = 1;

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

VideoPlayer::VideoPlayer(FlutterDesktopPluginRegistrarRef registrar_ref,
                         flutter::PluginRegistrar *plugin_registrar,
                         const std::string &uri, VideoPlayerOptions &options) {
  is_initialized_ = false;
  is_interrupted_ = false;
  listener_ = std::make_unique<PlusPlayerEventListener>(this);
  LOG_DEBUG("[VideoPlayer] call player_create to create player");
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
    int w = 0;
    int h = 0;
    if (system_info_get_platform_int("http://tizen.org/feature/screen.width",
                                     &w) != SYSTEM_INFO_ERROR_NONE ||
        system_info_get_platform_int("http://tizen.org/feature/screen.height",
                                     &h) != SYSTEM_INFO_ERROR_NONE) {
      LOG_ERROR("Could not obtain the screen size.");
      throw VideoPlayerError("PlusPlayer - Could not obtain the screen size");
    }
    Ecore_Wl2_Window *window_handle =
        (Ecore_Wl2_Window *)FlutterDesktopGetWindow(registrar_ref);
    if (!player->SetDisplay(plusplayer::DisplayType::kOverlay,
                            ecore_wl2_window_surface_id_get(window_handle), 0,
                            0, w, h)) {
      LOG_ERROR("set display failed");
      throw VideoPlayerError("PlusPlayer - set display failed");
    }

    if (!player->SetDisplayMode(plusplayer::DisplayMode::kDstRoi)) {
      LOG_ERROR("set display mode failed");
      throw VideoPlayerError("PlusPlayer - set display mode failed");
    }

    if (!player->PrepareAsync()) {
      LOG_ERROR("parepare async failed");
      throw VideoPlayerError("PlusPlayer - prepare async failed");
    }
  } else {
    LOG_ERROR("Create operation failed");
    throw VideoPlayerError("PlusPlayer - Create operation failed");
  }
  texture_id_ = gPlayerIndex++;
  player_ = std::move(player);
  setupEventChannel(plugin_registrar->messenger());
}

void VideoPlayer::setDisplayRoi(int x, int y, int w, int h) {
  LOG_DEBUG("setDisplayRoi PlusPlayer x == %d, y == %d, w == %d, h == %d", x, y,
            w, h);
  if (player_ == nullptr) {
    LOG_ERROR("Plusplayer isn't created");
    throw VideoPlayerError("PlusPlayer - Not created");
  }
  plusplayer::Geometry roi;
  roi.x = x;
  roi.y = y;
  roi.w = w;
  roi.h = 1080 - y;
  bool ret = player_->SetDisplayRoi(roi);

  if (!ret) {
    LOG_ERROR("Plusplayer SetDisplayRoi failed");
  }
}

VideoPlayer::~VideoPlayer() {
  LOG_INFO("[VideoPlayer] destructor");
  dispose();
}

long VideoPlayer::getTextureId() { return texture_id_; }

void VideoPlayer::play() {
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

void VideoPlayer::pause() {
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

void VideoPlayer::setLooping(bool is_looping) {
  LOG_DEBUG("set looping: %d", is_looping);
  isLooping_ = is_looping;
  // throw VideoPlayerError("PlusPlayer - Not support looping");
}

void VideoPlayer::setVolume(double volume) {
  LOG_ERROR("PlusPlayer doesn't support to set volume");
  // throw VideoPlayerError("PlusPlayer - Not support to set volume");
}

void VideoPlayer::setPlaybackSpeed(double speed) {
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

void VideoPlayer::seekTo(int position,
                         const SeekCompletedCb &seek_completed_cb) {
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

int VideoPlayer::getPosition() {
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

void VideoPlayer::dispose() {
  LOG_DEBUG("[VideoPlayer.dispose] dispose video player");
  is_initialized_ = false;
  event_sink_ = nullptr;
  event_channel_->SetStreamHandler(nullptr);

  LOG_DEBUG("dispose PlusPlayer");

  if (player_) {
    player_->Stop();
    player_->Close();
    player_ = nullptr;
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
  if (player_ == nullptr) {
    LOG_ERROR("Plusplayer isn't created");
    throw VideoPlayerError("PlusPlayer - Not created");
  }

  if (player_->GetState() == plusplayer::State::kReady) {
    int64_t duration;
    if (!player_->GetDuration(&duration)) {
      LOG_ERROR("PlusPlayer - GetDuration operation failed");
      // sendError("", "PlusPlayer - GetDuration operation failed");
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
      // sendError("", "PlusPlayer - GetDisplayRotate operation failed");
    } else {
      if (rotate == plusplayer::DisplayRotation::kRotate90 ||
          rotate == plusplayer::DisplayRotation::kRotate270) {
        int tmp = width;
        width = height;
        height = tmp;
      }
    }

    sendInitialized();
  } else {
    LOG_DEBUG("PlusPlayer isn't prepared, wait for ready");
  }
}

void VideoPlayer::sendInitialized() {
  if (!is_initialized_ && !is_interrupted_ && event_sink_ != nullptr) {
    int64_t duration;
    if (!player_->GetDuration(&duration)) {
      LOG_ERROR("PlusPlayer - GetDuration operation failed");
      event_sink_->Error("PlusPlayer", "GetDuration operation failed");
      return;
    }
    LOG_DEBUG("[VideoPlayer.sendInitialized] video duration: %lld", duration);

    int width, height;
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
      event_sink_->Error("", "PlusPlayer - GetDisplayRotate operation failed");
    } else {
      if (rotate == plusplayer::DisplayRotation::kRotate90 ||
          rotate == plusplayer::DisplayRotation::kRotate270) {
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

void VideoPlayer::PlusPlayerEventListener::OnErrorMsg(
    const plusplayer::ErrorType &error_code, const char *error_msg,
    UserData userdata) {
  LOG_DEBUG("Error occurred: %s", error_msg);
  if (handler_ != nullptr) {
    handler_->event_sink_->Error(ErrorToString(error_code), error_msg);
  }
}

void VideoPlayer::PlusPlayerEventListener::OnResourceConflicted(
    UserData userdata) {
  LOG_DEBUG("Resource conflicted");
}

void VideoPlayer::PlusPlayerEventListener::OnPrepareDone(bool ret,
                                                         UserData userdata) {
  if (!ret) {
    LOG_ERROR("Failed to prepare");
    if (handler_ != nullptr) {
      handler_->event_sink_->Error("PlusPlayerEventListener",
                                   "Failed to prepare");
    }
    return;
  }

  LOG_DEBUG("PlusPlayer is prepared");
  if (handler_ != nullptr) {
    handler_->initialize();
  }
}

void VideoPlayer::PlusPlayerEventListener::OnError(
    const plusplayer::ErrorType &error_code, UserData userdata) {}

void VideoPlayer::PlusPlayerEventListener::OnBufferStatus(const int percent,
                                                          UserData userdata) {
  LOG_DEBUG("OnBufferStatus percent == %d", percent);
  if (handler_ != nullptr) {
    handler_->sendBufferingUpdate(percent);
  }
}

void VideoPlayer::PlusPlayerEventListener::OnEos(UserData userdata) {
  LOG_DEBUG("OnEos");
}

void VideoPlayer::PlusPlayerEventListener::OnClosedCaptionData(
    std::unique_ptr<char[]> data, const int size, UserData userdata) {}

void VideoPlayer::PlusPlayerEventListener::OnAdaptiveStreamingControlEvent(
    const plusplayer::StreamingMessageType &type,
    const plusplayer::MessageParam &msg, UserData userdata) {}

void VideoPlayer::PlusPlayerEventListener::OnCueEvent(const char *msgType,
                                                      const uint64_t timestamp,
                                                      unsigned int duration,
                                                      UserData userdata) {}
void VideoPlayer::PlusPlayerEventListener::OnDateRangeEvent(
    const char *DateRangeData, UserData userdata) {}

void VideoPlayer::PlusPlayerEventListener::OnStopReachEvent(bool StopReach,
                                                            UserData userdata) {
  LOG_DEBUG("OnStopReachEvent");
}

void VideoPlayer::PlusPlayerEventListener::OnCueOutContEvent(
    const char *CueOutContData, UserData userdata) {}

void VideoPlayer::PlusPlayerEventListener::OnSeekDone(UserData userdata) {
  LOG_DEBUG("OnSeekDone");
}

void VideoPlayer::PlusPlayerEventListener::OnChangeSourceDone(
    bool ret, UserData userdata) {
  LOG_DEBUG("OnChangeSourceDone");
}

void VideoPlayer::PlusPlayerEventListener::OnStateChangedToPlaying(
    UserData userdata) {
  LOG_DEBUG("OnStateChangedToPlaying");
}
