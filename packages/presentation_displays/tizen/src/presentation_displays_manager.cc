#include "presentation_displays_manager.h"

#include <Elementary.h>

#include "log.h"

Display::Display(eom_output_id output_id) : output_id_(output_id) {
  int ret = eom_get_output_resolution(output_id, &output_resolution_width_,
                                      &output_resolution_height_);
  if (ret != EOM_ERROR_NONE) {
    LOG_ERROR("eom_get_output_resolution fail");
  }
}

eom_output_id Display::GetOutputId() { return output_id_; }

eom_output_attribute_e Display::GetOutputAttribute() {
  eom_output_attribute_e attribute = EOM_OUTPUT_ATTRIBUTE_NONE;
  eom_get_output_attribute(output_id_, &attribute);
  return attribute;
}

eom_output_attribute_state_e Display::GetOutputAttributeState() {
  eom_output_attribute_state_e state = EOM_OUTPUT_ATTRIBUTE_STATE_NONE;
  eom_get_output_attribute_state(output_id_, &state);
  return state;
}
eom_output_mode_e Display::GetOutputMode() {
  eom_output_mode_e mode = EOM_OUTPUT_MODE_NONE;
  eom_get_output_mode(output_id_, &mode);
  return mode;
}

int Display::GetWidth() { return output_resolution_width_; }
int Display::GetHeight() { return output_resolution_height_; }

void PresentationDispalysManager::NotifyOutputAdd(eom_output_id output_id,
                                                  void* user_data) {
  auto* presentationDispalysManager =
      reinterpret_cast<PresentationDispalysManager*>(user_data);
  presentationDispalysManager->OnOutputAdd(output_id);
}
void PresentationDispalysManager::NotifyOutputRemove(eom_output_id output_id,
                                                     void* user_data) {
  auto* presentationDispalysManager =
      reinterpret_cast<PresentationDispalysManager*>(user_data);
  presentationDispalysManager->OnOutputRemove(output_id);
}
void PresentationDispalysManager::NotifyModeChanged(eom_output_id output_id,
                                                    void* user_data) {
  auto* presentationDispalysManager =
      reinterpret_cast<PresentationDispalysManager*>(user_data);
  presentationDispalysManager->OnModeChanged(output_id);
}
void PresentationDispalysManager::NotifyAtributeChanged(eom_output_id output_id,
                                                        void* user_data) {
  auto* presentatonDispalysManager =
      reinterpret_cast<PresentationDispalysManager*>(user_data);
  presentatonDispalysManager->OnAtributeChanged(output_id);
}

PresentationDispalysManager::PresentationDispalysManager() { InitEom(); }
PresentationDispalysManager::~PresentationDispalysManager() { DeinitEom(); }

bool PresentationDispalysManager::InitEom() {
  if (eom_init()) {
    LOG_ERROR("eom init fail");
    return false;
  }
  eom_output_id* output_ids = NULL;
  eom_output_type_e output_type = EOM_OUTPUT_TYPE_UNKNOWN;
  int id_cnt = 0;
  output_ids = eom_get_eom_output_ids(&id_cnt);
  for (int i = 0; i < id_cnt; i++) {
    eom_get_output_type(output_ids[i], &output_type);
    if (output_type == EOM_OUTPUT_TYPE_HDMIA ||
        output_type == EOM_OUTPUT_TYPE_HDMIB) {
      output_displays_.push_back(Display(output_ids[i]));
    }
  }
  if (output_ids) {
    free(output_ids);
  }
  eom_set_output_added_cb(NotifyOutputAdd, this);
  eom_set_output_removed_cb(NotifyOutputRemove, this);
  eom_set_mode_changed_cb(NotifyModeChanged, this);
  eom_set_attribute_changed_cb(NotifyAtributeChanged, this);
  return true;
}

std::vector<Display> PresentationDispalysManager::GetOutputDisplays() {
  return output_displays_;
}

void PresentationDispalysManager::DeinitEom() {
  eom_unset_output_added_cb(NotifyOutputAdd);
  eom_unset_output_removed_cb(NotifyOutputRemove);
  eom_unset_mode_changed_cb(NotifyModeChanged);
  eom_unset_attribute_changed_cb(NotifyAtributeChanged);
  eom_deinit();
}

void PresentationDispalysManager::OnOutputRemove(eom_output_id output_id) {
  auto iter = output_displays_.begin();
  for (; iter != output_displays_.end(); iter++) {
    if (iter->GetOutputId() == output_id) {
      output_displays_.erase(iter);
      break;
    }
  }
}

void PresentationDispalysManager::OnOutputAdd(eom_output_id output_id) {
  eom_output_type_e output_type = EOM_OUTPUT_TYPE_UNKNOWN;
  eom_get_output_type(output_id, &output_type);
  if (output_type == EOM_OUTPUT_TYPE_HDMIA ||
      output_type == EOM_OUTPUT_TYPE_HDMIB) {
    output_displays_.push_back(Display(output_id));
  }
}

int PresentationDispalysManager::GetOutputWidth(eom_output_id output_id) {
  auto iter = output_displays_.begin();
  for (; iter != output_displays_.end(); iter++) {
    if (iter->GetOutputId() == output_id) {
      return iter->GetWidth();
    }
  }
  return 0;
}

int PresentationDispalysManager::GetOutputHeight(eom_output_id output_id) {
  auto iter = output_displays_.begin();
  for (; iter != output_displays_.end(); iter++) {
    if (iter->GetOutputId() == output_id) {
      return iter->GetHeight();
    }
  }
  return 0;
}

void PresentationDispalysManager::OnModeChanged(eom_output_id output_id) {}
void PresentationDispalysManager::OnAtributeChanged(eom_output_id output_id) {}

void PresentationDispalysManager::CloseDisplay(unsigned int output_id) {
  if (output_id != select_display_id_) {
    LOG_ERROR("output_id not valid");
  }

  select_display_id_ = 0;

  if (handle_) {
    DestoryFlutterEngine();
  }

  if (elm_window_) {
    DestroyWindow();
  }
}

bool PresentationDispalysManager::ShowDisplay(unsigned int output_id) {
  auto iter = output_displays_.begin();
  for (; iter != output_displays_.end(); iter++) {
    if (iter->GetOutputId() == output_id) {
      select_display_id_ = output_id;
      break;
    }
  }
  if (iter == output_displays_.end()) {
    LOG_ERROR("output_id not exist");
    return false;
  }

  int ret =
      eom_set_output_attribute(select_display_id_, EOM_OUTPUT_ATTRIBUTE_NORMAL);

  if (ret != EOM_ERROR_NONE) {
    LOG_ERROR("eom set output attribute fail");
    return false;
  }

  eom_output_mode_e output_mode = EOM_OUTPUT_MODE_NONE;
  eom_get_output_mode(output_id, &output_mode);

  if (output_mode == EOM_OUTPUT_MODE_NONE) {
    LOG_ERROR("output mode not set");
    return false;
  }
  
  if (elm_window_ == nullptr) {
    CreateWindow();
  } else {
    LOG_ERROR("elm window not null");
  }

  if (eom_set_output_window(select_display_id_, elm_window_) !=
      EOM_ERROR_NONE) {
    LOG_ERROR("eom set output window fail");
    return false;
  }

  if (handle_ == nullptr) {
    RunFlutterEngine();
  } else {
    LOG_ERROR("Flutter engine already running");
  }
  return true;
}

void PresentationDispalysManager::CreateWindow() {
  elm_window_ = elm_win_add(NULL, "external_window", ELM_WIN_BASIC);
  int x = 0, y = 0, w = GetOutputWidth(select_display_id_),
      h = GetOutputHeight(select_display_id_);
  elm_win_screen_size_get(elm_window_, &x, &y, &w, &h);
  elm_win_alpha_set(elm_window_, EINA_FALSE);
  evas_object_move(elm_window_, x, y);
  evas_object_resize(elm_window_, w, h);
  evas_object_show(elm_window_);
}

void PresentationDispalysManager::DestroyWindow() {
  evas_object_del(elm_window_);
}

bool PresentationDispalysManager::RunFlutterEngine() {
  FlutterDesktopWindowProperties window_prop = {};
  window_prop.custom_win = elm_window_;
  window_prop.headed = true;
  window_prop.x = 0;
  window_prop.y = 0;
  window_prop.width = GetOutputWidth(select_display_id_);
  window_prop.height = GetOutputHeight(select_display_id_);
  window_prop.transparent = false;
  window_prop.focusable = true;

  FlutterDesktopEngineProperties engine_prop = {};
  engine_prop.assets_path = "../res/flutter_assets";
  engine_prop.icu_data_path = "../res/icudtl.dat";
  engine_prop.aot_library_path = "../lib/libapp.so";
  engine_prop.entrypoint = "showPresentationDisplay";

  handle_ = FlutterDesktopRunEngine(window_prop, engine_prop);
  if (!handle_) {
    LOG_ERROR("Could not launch a Flutter application.");
    return false;
  }
  return true;
}

void PresentationDispalysManager::DestoryFlutterEngine() {
  FlutterDesktopShutdownEngine(handle_);
  handle_ = nullptr;
}
