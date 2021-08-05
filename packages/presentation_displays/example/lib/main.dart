import 'package:flutter/material.dart';
import 'package:flutter/widgets.dart';
import 'package:presentation_displays/display.dart';
import 'package:presentation_displays/displays_manager.dart';

void main() {
  runApp(MyApp());
}

class MyApp extends StatefulWidget {
  @override
  _MyAppState createState() => _MyAppState();
}

@pragma("vm:entry-point")
void showPresentationDisplay() {
  runApp(PresendtationDisplay());
}

class PresendtationDisplay extends StatefulWidget {
  @override
  PresendtationDisplayState createState() => PresendtationDisplayState();
}

class PresendtationDisplayState extends State<PresendtationDisplay> {
  @override
  void initState() {
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
        home: Scaffold(
      appBar: AppBar(
        title: const Text('Second display'),
      ),
    ));
  }
}

class _MyAppState extends State<MyApp> {
  DisplayManager displayManager = DisplayManager();
  List<Display> displays = [];
  int? currentDisplayId = 0;
  @override
  void initState() {
    super.initState();
  }

  Future<void> getDisplays() async {
    displays = await displayManager.getDisplays();
    print('displays size is ${displays.length}');
    for (int i = 0; i < displays.length; i++) {
      print('displays[$i] == ${displays[i].displayId}');
    }
  }

  Future<void> showSecondaryDisplay() async {
    if (displays.length > 0) {
      bool ret =
          await displayManager.showSecondaryDisplay(displays[0].displayId);
      if (ret) {
        currentDisplayId = displays[0].displayId;
      } else {
        print('showSecondaryDisplay fail');
      }
    } else {
      print('displays empty');
    }
  }

  Future<void> closeSecondaryDisplay() async {
    if (currentDisplayId != 0) {
      await displayManager.closeSecondaryDisplay(currentDisplayId);
      currentDisplayId = 0;
    } else {
      print('currentDisplayId == 0');
    }
  }

  @override
  Widget build(BuildContext context) {
    Container buttonSelection = Container(
      padding: const EdgeInsets.all(20),
      child: Column(
        children: [
          Row(
            children: [
              ElevatedButton(
                onPressed: () {
                  getDisplays();
                },
                child: Text('GetDisplays'),
              ),
              ElevatedButton(
                onPressed: () {
                  showSecondaryDisplay();
                },
                child: Text('showSecondaryDisplay'),
              ),
              ElevatedButton(
                onPressed: () {
                  closeSecondaryDisplay();
                },
                child: Text('closeSecondaryDisplay'),
              ),
            ],
          ),
        ],
      ),
    );
    return MaterialApp(
      home: Scaffold(
          appBar: AppBar(
            title: const Text('PresendtationDisplay'),
          ),
          body: buttonSelection),
    );
  }
}
