#ifndef FLUTTER_PLUGIN_PRESENTATION_DISPLAYS_MANAGER_H_
#define FLUTTER_PLUGIN_PRESENTATION_DISPLAYS_MANAGER_H_

#include <eom.h>
#include <flutter_tizen.h>

#include <memory>
#include <vector>

class Display {
 public:
  Display(eom_output_id output_id);
  Display();
  eom_output_id GetOutputId();
  int GetWidth();
  int GetHeight();
  eom_output_attribute_e GetOutputAttribute();
  eom_output_attribute_state_e GetOutputAttributeState();
  eom_output_mode_e GetOutputMode();

 private:
  eom_output_id output_id_;
  int output_resolution_width_ = 0;
  int output_resolution_height_ = 0;
};

class PresentationDispalysManager {
 public:
  PresentationDispalysManager();
  virtual ~PresentationDispalysManager();
  std::vector<Display> GetOutputDisplays();
  bool ShowDisplay(unsigned int output_id);
  void CloseDisplay(unsigned int output_id);

 private:
  static void NotifyOutputAdd(eom_output_id output_id, void* user_data);
  static void NotifyOutputRemove(eom_output_id output_id, void* user_data);
  static void NotifyModeChanged(eom_output_id output_id, void* user_data);
  static void NotifyAtributeChanged(eom_output_id output_id, void* user_data);
  void OnOutputAdd(eom_output_id output_id);
  void OnOutputRemove(eom_output_id output_id);
  void OnModeChanged(eom_output_id output_id);
  void OnAtributeChanged(eom_output_id output_id);
  bool InitEom();
  void DeinitEom();
  void CreateWindow();
  void DestroyWindow();
  bool RunFlutterEngine();
  void DestoryFlutterEngine();
  int GetOutputWidth(eom_output_id output_id);
  int GetOutputHeight(eom_output_id output_id);
  std::vector<Display> output_displays_;
  eom_output_id select_display_id_;
  Evas_Object* elm_window_{nullptr};
  FlutterDesktopEngineRef handle_{nullptr};
};

#endif  // FLUTTER_PLUGIN_PRESENTATION_DISPLAYS_MANAGER_H_