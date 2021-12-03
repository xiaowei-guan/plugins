import 'package:flutter/services.dart';

class GeometryMessage {
  int? textureId;
  int? x;
  int? y;
  int? w;
  int? h;

  Object encode() {
    final Map<Object?, Object?> pigeonMap = <Object?, Object?>{};
    pigeonMap['textureId'] = textureId;
    pigeonMap['x'] = x;
    pigeonMap['y'] = y;
    pigeonMap['w'] = w;
    pigeonMap['h'] = h;
    return pigeonMap;
  }

  static GeometryMessage decode(Object message) {
    final Map<Object?, Object?> pigeonMap = message as Map<Object?, Object?>;
    return GeometryMessage()
      ..textureId = pigeonMap['textureId'] as int?
      ..x = pigeonMap['x'] as int?
      ..y = pigeonMap['y'] as int?
      ..w = pigeonMap['w'] as int?
      ..h = pigeonMap['h'] as int?;
  }
}

Future<void> setDisplayGeometry(int texureId, int x, int y, int w, int h) {
  return setDisplayRoi(GeometryMessage()
    ..textureId = texureId
    ..x = x
    ..y = y
    ..w = w
    ..h = h);
}

Future<void> setDisplayRoi(GeometryMessage arg) async {
  final Object encoded = arg.encode();
  const BasicMessageChannel<Object?> channel = BasicMessageChannel<Object?>(
      'dev.flutter.pigeon.VideoPlayerApi.setDisplayRoi',
      StandardMessageCodec());
  final Map<Object?, Object?>? replyMap =
      await channel.send(encoded) as Map<Object?, Object?>?;
  if (replyMap == null) {
    throw PlatformException(
      code: 'channel-error',
      message: 'Unable to establish connection on channel.',
      details: null,
    );
  } else if (replyMap['error'] != null) {
    final Map<Object?, Object?> error =
        replyMap['error'] as Map<Object?, Object?>;
    throw PlatformException(
      code: error['code'] as String,
      message: error['message'] as String?,
      details: error['details'],
    );
  } else {
    // noop
  }
}
