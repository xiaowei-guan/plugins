#include "video_player_tizen_plugin.h"

#include <app_common.h>
#include <flutter/event_channel.h>
#include <flutter/event_stream_handler_functions.h>
#include <flutter/plugin_registrar.h>
#include <flutter/standard_method_codec.h>

#include "flutter_texture_registrar.h"
#include "log.h"
#include "message.h"
#include "video_player.h"
#include "video_player_error.h"
#include "video_player_options.h"

class VideoPlayerTizenPlugin : public flutter::Plugin, public VideoPlayerApi {
 public:
  static void RegisterWithRegistrar(
      FlutterDesktopPluginRegistrarRef registrar_ref,
      flutter::PluginRegistrar *pluginRegistrar,
      FlutterTextureRegistrar *textureRegistrar);
  // Creates a plugin that communicates on the given channel.
  VideoPlayerTizenPlugin(FlutterDesktopPluginRegistrarRef registrar_ref,
                         flutter::PluginRegistrar *pluginRegistrar,
                         FlutterTextureRegistrar *textureRegistrar);
  virtual ~VideoPlayerTizenPlugin();

  virtual void initialize() override;
  virtual TextureMessage create(const CreateMessage &createMsg) override;
  virtual void dispose(const TextureMessage &textureMsg) override;
  virtual void setLooping(const LoopingMessage &loopingMsg) override;
  virtual void setVolume(const VolumeMessage &volumeMsg) override;
  virtual void setPlaybackSpeed(const PlaybackSpeedMessage &speedMsg) override;
  virtual void play(const TextureMessage &textureMsg) override;
  virtual void pause(const TextureMessage &textureMsg) override;
  virtual PositionMessage position(const TextureMessage &textureMsg) override;
  virtual void seekTo(const PositionMessage &positionMsg) override;
  virtual void setMixWithOthers(
      const MixWithOthersMessage &mixWithOthersMsg) override;
  virtual void setDisplayRoi(const GeometryMessage &geometryMsg) override;

 private:
  void disposeAllPlayers();

  flutter::PluginRegistrar *pluginRegistrar_;
  FlutterTextureRegistrar *textureRegistrar_;
  FlutterDesktopPluginRegistrarRef registrar_ref_;
  VideoPlayerOptions options_;
  std::map<long, std::unique_ptr<VideoPlayer>> videoPlayers_;
};

// static
void VideoPlayerTizenPlugin::RegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar_ref,
    flutter::PluginRegistrar *pluginRegistrar,
    FlutterTextureRegistrar *textureRegistrar) {
  auto plugin = std::make_unique<VideoPlayerTizenPlugin>(
      registrar_ref, pluginRegistrar, textureRegistrar);
  pluginRegistrar->AddPlugin(std::move(plugin));
}

VideoPlayerTizenPlugin::VideoPlayerTizenPlugin(
    FlutterDesktopPluginRegistrarRef registrar_ref,
    flutter::PluginRegistrar *pluginRegistrar,
    FlutterTextureRegistrar *textureRegistrar)
    : pluginRegistrar_(pluginRegistrar),
      textureRegistrar_(textureRegistrar),
      registrar_ref_(registrar_ref) {
  VideoPlayerApi::setup(pluginRegistrar->messenger(), this);
}

VideoPlayerTizenPlugin::~VideoPlayerTizenPlugin() { disposeAllPlayers(); }

void VideoPlayerTizenPlugin::disposeAllPlayers() {
  LOG_DEBUG("[VideoPlayerTizenPlugin.disposeAllPlayers] player count: %d",
            videoPlayers_.size());
  auto iter = videoPlayers_.begin();
  while (iter != videoPlayers_.end()) {
    iter->second->dispose();
    iter++;
  }
  videoPlayers_.clear();
}

void VideoPlayerTizenPlugin::initialize() {
  LOG_DEBUG("[VideoPlayerTizenPlugin.initialize] init ");
  disposeAllPlayers();
}

TextureMessage VideoPlayerTizenPlugin::create(const CreateMessage &createMsg) {
  LOG_DEBUG("[VideoPlayerTizenPlugin.create] asset: %s",
            createMsg.getAsset().c_str());
  LOG_DEBUG("[VideoPlayerTizenPlugin.create] uri: %s",
            createMsg.getUri().c_str());
  LOG_DEBUG("[VideoPlayerTizenPlugin.create] packageName: %s",
            createMsg.getPackageName().c_str());
  LOG_DEBUG("[VideoPlayerTizenPlugin.create] formatHint: %s",
            createMsg.getFormatHint().c_str());

  std::string uri;
  VideoPlayerType type = VideoPlayerType::kMMPlayer;
  if (createMsg.getAsset().empty()) {
    uri = createMsg.getUri();
    type = VideoPlayerType::kPlusPlayer;
  } else {
    char *resPath = app_get_resource_path();
    if (resPath) {
      uri = uri + resPath + "flutter_assets/" + createMsg.getAsset();
      free(resPath);
    } else {
      LOG_DEBUG(
          "[VideoPlayerTizenPlugin.create] failed to get resource path "
          "of package");
      throw VideoPlayerError("failed to get resource path",
                             "PLAYER_ERROR_INVALID_RESOURCE_PATH");
    }
  }
  LOG_DEBUG("[VideoPlayerTizenPlugin.create] uri of video player: %s",
            uri.c_str());

  auto player = VideoPlayer::create(registrar_ref_, type, pluginRegistrar_,
                                    textureRegistrar_, uri, options_);
  long textureId = player->getTextureId();
  videoPlayers_[textureId] = std::move(player);

  TextureMessage result;
  result.setTextureId(textureId);
  return result;
}

void VideoPlayerTizenPlugin::dispose(const TextureMessage &textureMsg) {
  LOG_DEBUG("[VideoPlayerTizenPlugin.dispose] textureId: %ld",
            textureMsg.getTextureId());

  auto iter = videoPlayers_.find(textureMsg.getTextureId());
  if (iter != videoPlayers_.end()) {
    iter->second->dispose();
    videoPlayers_.erase(iter);
  }
}

void VideoPlayerTizenPlugin::setLooping(const LoopingMessage &loopingMsg) {
  LOG_DEBUG("[VideoPlayerTizenPlugin.setLooping] textureId: %ld",
            loopingMsg.getTextureId());
  LOG_DEBUG("[VideoPlayerTizenPlugin.setLooping] isLooping: %d",
            loopingMsg.getIsLooping());

  auto iter = videoPlayers_.find(loopingMsg.getTextureId());
  if (iter != videoPlayers_.end()) {
    iter->second->setLooping(loopingMsg.getIsLooping());
  }
}

void VideoPlayerTizenPlugin::setVolume(const VolumeMessage &volumeMsg) {
  LOG_DEBUG("[VideoPlayerTizenPlugin.setVolume] textureId: %ld",
            volumeMsg.getTextureId());
  LOG_DEBUG("[VideoPlayerTizenPlugin.setVolume] volume: %f",
            volumeMsg.getVolume());

  auto iter = videoPlayers_.find(volumeMsg.getTextureId());
  if (iter != videoPlayers_.end()) {
    iter->second->setVolume(volumeMsg.getVolume());
  }
}

void VideoPlayerTizenPlugin::setPlaybackSpeed(
    const PlaybackSpeedMessage &speedMsg) {
  LOG_DEBUG("[VideoPlayerTizenPlugin.setPlaybackSpeed] textureId: %ld",
            speedMsg.getTextureId());
  LOG_DEBUG("[VideoPlayerTizenPlugin.setPlaybackSpeed] speed: %f",
            speedMsg.getSpeed());

  auto iter = videoPlayers_.find(speedMsg.getTextureId());
  if (iter != videoPlayers_.end()) {
    iter->second->setPlaybackSpeed(speedMsg.getSpeed());
  }
}

void VideoPlayerTizenPlugin::play(const TextureMessage &textureMsg) {
  LOG_DEBUG("[VideoPlayerTizenPlugin.play] textureId: %ld",
            textureMsg.getTextureId());

  auto iter = videoPlayers_.find(textureMsg.getTextureId());
  if (iter != videoPlayers_.end()) {
    iter->second->play();
  }
}

void VideoPlayerTizenPlugin::pause(const TextureMessage &textureMsg) {
  LOG_DEBUG("[VideoPlayerTizenPlugin.pause] textureId: %ld",
            textureMsg.getTextureId());

  auto iter = videoPlayers_.find(textureMsg.getTextureId());
  if (iter != videoPlayers_.end()) {
    iter->second->pause();
  }
}

void VideoPlayerTizenPlugin::setDisplayRoi(const GeometryMessage &geometryMsg) {
  LOG_DEBUG("[VideoPlayerTizenPlugin.setDisplayRoi] textureId: %ld",
            geometryMsg.getTextureId());
  auto iter = videoPlayers_.find(geometryMsg.getTextureId());
  if (iter != videoPlayers_.end()) {
    iter->second->setDisplayRoi(geometryMsg.getX(), geometryMsg.getY(),
                                geometryMsg.getW(), geometryMsg.getH());
  }
}

PositionMessage VideoPlayerTizenPlugin::position(
    const TextureMessage &textureMsg) {
  LOG_DEBUG("[VideoPlayerTizenPlugin.position] textureId: %ld",
            textureMsg.getTextureId());

  PositionMessage result;
  auto iter = videoPlayers_.find(textureMsg.getTextureId());
  if (iter != videoPlayers_.end()) {
    result.setTextureId(textureMsg.getTextureId());
    result.setPosition(iter->second->getPosition());
  }
  return result;
}

void VideoPlayerTizenPlugin::seekTo(const PositionMessage &positionMsg) {
  LOG_DEBUG("[VideoPlayerTizenPlugin.seekTo] textureId: %ld",
            positionMsg.getTextureId());
  LOG_DEBUG("[VideoPlayerTizenPlugin.seekTo] position: %ld",
            positionMsg.getPosition());

  auto iter = videoPlayers_.find(positionMsg.getTextureId());
  if (iter != videoPlayers_.end()) {
    iter->second->seekTo(positionMsg.getPosition());
  }
}

void VideoPlayerTizenPlugin::setMixWithOthers(
    const MixWithOthersMessage &mixWithOthersMsg) {
  LOG_DEBUG("[VideoPlayerTizenPlugin.setMixWithOthers] mixWithOthers: %d",
            mixWithOthersMsg.getMixWithOthers());
  options_.setMixWithOthers(mixWithOthersMsg.getMixWithOthers());
}

void VideoPlayerTizenPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  VideoPlayerTizenPlugin::RegisterWithRegistrar(
      registrar,
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrar>(registrar),
      FlutterPluginRegistrarGetTexture(registrar));
}