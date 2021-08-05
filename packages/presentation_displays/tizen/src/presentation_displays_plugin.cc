#include "presentation_displays_plugin.h"

#include "presentation_displays_manager.h"

#include <flutter/basic_message_channel.h>
#include <flutter/binary_messenger.h>
#include <flutter/encodable_value.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar.h>
#include <flutter/standard_message_codec.h>
#include <flutter/standard_method_codec.h>
#include <flutter_tizen.h>
#include <system_info.h>

#include <map>
#include <memory>
#include <sstream>
#include <string>

#include "log.h"

template <typename T>
void ExtractValueFromMap(const flutter::EncodableValue &arguments,
                         const char *key, T &outValue) {
  if (std::holds_alternative<flutter::EncodableMap>(arguments)) {
    flutter::EncodableMap values = std::get<flutter::EncodableMap>(arguments);
    flutter::EncodableValue value = values[flutter::EncodableValue(key)];
    if (std::holds_alternative<T>(value)) outValue = std::get<T>(value);
  }
}

class PresentationDisplaysPlugin : public flutter::Plugin {
 public:
  using MethodResultPtr =
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>>;
  PresentationDisplaysPlugin();
  virtual ~PresentationDisplaysPlugin(){};
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      MethodResultPtr result);

 private:
  std::map<std::string, void (PresentationDisplaysPlugin::*)(
                            const flutter::EncodableValue &arguments,
                            MethodResultPtr result)>
      m_mapStr2Func;
  std::unique_ptr<PresentationDispalysManager> presentaton_dispalys_manager_;
  void GetDisplays(const flutter::EncodableValue &arguments,
                   MethodResultPtr result);
  void SetPresentationDisplay(const flutter::EncodableValue &arguments,
                              MethodResultPtr result);
  void ClosePresentationDisplay(const flutter::EncodableValue &arguments,
                                MethodResultPtr result);
};

static void RegisterWithRegistrar(flutter::PluginRegistrar *registrar) {
  auto channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), "presentation.displays",
          &flutter::StandardMethodCodec::GetInstance());
  auto plugin = std::make_unique<PresentationDisplaysPlugin>();
  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto &call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });
  registrar->AddPlugin(std::move(plugin));
}

PresentationDisplaysPlugin::PresentationDisplaysPlugin() {
  presentaton_dispalys_manager_ =
      std::make_unique<PresentationDispalysManager>();
  m_mapStr2Func["GetDisplays"] = &PresentationDisplaysPlugin::GetDisplays;
  m_mapStr2Func["SetPresentationDisplay"] =
      &PresentationDisplaysPlugin::SetPresentationDisplay;
  m_mapStr2Func["ClosePresentationDisplay"] =
      &PresentationDisplaysPlugin::ClosePresentationDisplay;
}

void PresentationDisplaysPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    MethodResultPtr result) {
  auto string2FunMapIter = m_mapStr2Func.find(method_call.method_name());
  if (string2FunMapIter != m_mapStr2Func.end()) {
    auto func = string2FunMapIter->second;
    (this->*func)(*method_call.arguments(), std::move(result));
  } else {
    result->NotImplemented();
  }
}

void PresentationDisplaysPlugin::GetDisplays(
    const flutter::EncodableValue &arguments, MethodResultPtr result) {
  std::vector<Display> dispalys =
      presentaton_dispalys_manager_->GetOutputDisplays();
  flutter::EncodableList list;
  for (auto iter = dispalys.begin(); iter != dispalys.end(); iter++) {
    flutter::EncodableMap display_map = {
        {flutter::EncodableValue("display_id"),
         flutter::EncodableValue((int)iter->GetOutputId())},
    };
    list.push_back(flutter::EncodableValue(display_map));
  }
  result->Success(flutter::EncodableValue(list));
}

void PresentationDisplaysPlugin::SetPresentationDisplay(
    const flutter::EncodableValue &arguments, MethodResultPtr result) {
  int output_id = 0;
  ExtractValueFromMap(arguments, "output_id", output_id);
  bool ret = presentaton_dispalys_manager_->ShowDisplay(output_id);
  result->Success(flutter::EncodableValue(ret));
}

void PresentationDisplaysPlugin::ClosePresentationDisplay(
    const flutter::EncodableValue &arguments, MethodResultPtr result) {
  int output_id = 0;
  ExtractValueFromMap(arguments, "output_id", output_id);
  presentaton_dispalys_manager_->CloseDisplay(output_id);
  result->Success();
}

void PresentationDisplaysPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrar>(registrar));
}
