import 'dart:async';
import 'package:flutter/services.dart';
import 'package:presentation_displays/display.dart';

class DisplayManager {
  static const MethodChannel _channel =
      const MethodChannel('presentation.displays');
  Future<List<Display>> getDisplays() async {
    List<dynamic> displayList = await _channel.invokeMethod('GetDisplays');
    List<Display> result = [];
    for (int i = 0; i < displayList.length; i++) {
      Display display = Display();
      display.displayId = displayList[i]['display_id'];
      result.add(display);
    }
    return result;
  }

  Future<bool> showSecondaryDisplay(int? displayId) async {
    return await _channel.invokeMethod(
        'SetPresentationDisplay', <String, dynamic>{'output_id': displayId});
  }

  Future<void> closeSecondaryDisplay(int? displayId) async {
    await _channel.invokeMethod(
        'ClosePresentationDisplay', <String, dynamic>{'output_id': displayId});
  }
}
