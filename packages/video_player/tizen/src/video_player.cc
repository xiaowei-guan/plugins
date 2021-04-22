#include "video_player.h"

#include <flutter/event_stream_handler_functions.h>
#include <flutter/standard_method_codec.h>

#include <functional>

#include "log.h"
#include "mmplayer.h"
#include "plus_player.h"
#include "video_player_error.h"


std::unique_ptr<VideoPlayer> VideoPlayer::create(
    FlutterDesktopPluginRegistrarRef registrar_ref,
    VideoPlayerType type, flutter::PluginRegistrar *pluginRegistrar,
    FlutterTextureRegistrar *textureRegistrar, const std::string &uri,
    VideoPlayerOptions &options) {
  if (type == VideoPlayerType::kMMPlayer) {
    auto mmplayer = std::make_unique<MMPlayer>(textureRegistrar, uri, options);
    mmplayer->isInitialized_ = false;
    mmplayer->setupEventChannel(pluginRegistrar->messenger());
    return std::move(mmplayer);
  } else {
    auto plusPlayer = std::make_unique<PlusPlayer>(FlutterPluginRegistrarGetWindow(registrar_ref), uri, options);
    plusPlayer->isInitialized_ = false;
    plusPlayer->setupEventChannel(pluginRegistrar->messenger());
    return std::move(plusPlayer);
  }
}

VideoPlayer::~VideoPlayer() {
  eventSink_ = nullptr;
  eventChannel_->SetStreamHandler(nullptr);
}

long VideoPlayer::getTextureId() { return textureId_; }

void VideoPlayer::setupEventChannel(flutter::BinaryMessenger *messenger) {
  LOG_DEBUG("setup event channel");
  std::string name =
      "flutter.io/videoPlayer/videoEvents" + std::to_string(textureId_);
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
        LOG_DEBUG("call listen of StreamHandler");
        eventSink_ = std::move(events);
        initialize();
        return nullptr;
      },
      [&](const flutter::EncodableValue *arguments)
          -> std::unique_ptr<
              flutter::StreamHandlerError<flutter::EncodableValue>> {
        LOG_DEBUG("call cancel of StreamHandler");
        eventSink_ = nullptr;
        return nullptr;
      });
  channel->SetStreamHandler(std::move(handler));
  eventChannel_ = std::move(channel);
}

void VideoPlayer::sendInitialized(int duration, int videoWidth,
                                  int videoHeight) {
  if (!isInitialized_ && eventSink_ != nullptr) {
    isInitialized_ = true;
    flutter::EncodableMap encodables = {
        {flutter::EncodableValue("event"),
         flutter::EncodableValue("initialized")},
        {flutter::EncodableValue("duration"),
         flutter::EncodableValue(duration)},
        {flutter::EncodableValue("width"), flutter::EncodableValue(videoWidth)},
        {flutter::EncodableValue("height"),
         flutter::EncodableValue(videoHeight)}};
    flutter::EncodableValue eventValue(encodables);
    LOG_INFO("send initialized event");
    eventSink_->Success(eventValue);
  }
}

void VideoPlayer::sendBufferingStart() {
  if (eventSink_) {
    flutter::EncodableMap encodables = {
        {flutter::EncodableValue("event"),
         flutter::EncodableValue("bufferingStart")}};
    flutter::EncodableValue eventValue(encodables);
    LOG_INFO("send bufferingStart event");
    eventSink_->Success(eventValue);
  }
}

void VideoPlayer::sendBufferingUpdate(int position) {
  if (eventSink_) {
    flutter::EncodableList range = {flutter::EncodableValue(0),
                                    flutter::EncodableValue(position)};
    flutter::EncodableList rangeList = {flutter::EncodableValue(range)};
    flutter::EncodableMap encodables = {
        {flutter::EncodableValue("event"),
         flutter::EncodableValue("bufferingUpdate")},
        {flutter::EncodableValue("values"),
         flutter::EncodableValue(rangeList)}};
    flutter::EncodableValue eventValue(encodables);
    LOG_INFO("send bufferingUpdate event");
    eventSink_->Success(eventValue);
  }
}

void VideoPlayer::sendBufferingEnd() {
  if (eventSink_) {
    flutter::EncodableMap encodables = {
        {flutter::EncodableValue("event"),
         flutter::EncodableValue("bufferingEnd")}};
    flutter::EncodableValue eventValue(encodables);
    LOG_INFO("send bufferingEnd event");
    eventSink_->Success(eventValue);
  }
}

void VideoPlayer::sendCompleted() {
  if (eventSink_) {
    flutter::EncodableMap encodables = {
        {flutter::EncodableValue("event"),
         flutter::EncodableValue("completed")}};
    flutter::EncodableValue eventValue(encodables);
    LOG_INFO("send completed event");
    eventSink_->Success(eventValue);
  }
}

void VideoPlayer::sendError(const std::string &code,
                            const std::string &message) {
  if (eventSink_) {
    LOG_INFO("send error event");
    eventSink_->Error(code, message);
  }
}
