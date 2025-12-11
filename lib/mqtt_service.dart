import 'dart:io';
import 'package:flutter/foundation.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

class MqttService {
  final String server = '10.147.86.132';
  final int port = 1883;
  // Unique ID
  final String clientId = 'FlutterApp_${DateTime.now().millisecondsSinceEpoch}';

  MqttServerClient? client;

  // Callbacks
  Function(String topic, String message)? onMessageReceived;
  Function(bool isConnected)? onConnectionChanged;

  Future<bool> connect() async {
    client = MqttServerClient(server, clientId);
    client!.port = port;
    client!.logging(on: kDebugMode);
    client!.keepAlivePeriod = 20;
    client!.onDisconnected = _onDisconnected;
    client!.onConnected = _onConnected;
    client!.onSubscribed = _onSubscribed;

    // Set the correct protocol parameters
    final connMess = MqttConnectMessage()
        .withClientIdentifier(clientId)
        .withWillTopic('willtopic')
        .withWillMessage('My Will message')
        .startClean() // Non persistent session for testing
        .withWillQos(MqttQos.atLeastOnce);
    client!.connectionMessage = connMess;

    try {
      if (kDebugMode) {
        print('MQTT: Connecting to $server:$port...');
      }
      await client!.connect();
    } on NoConnectionException catch (e) {
      if (kDebugMode) {
        print('MQTT: Client exception - $e');
      }
      client!.disconnect();
      return false;
    } on SocketException catch (e) {
      if (kDebugMode) {
        print('MQTT: Socket exception - $e');
      }
      client!.disconnect();
      return false;
    }

    if (client!.connectionStatus!.state == MqttConnectionState.connected) {
      if (kDebugMode) {
        print('MQTT: Connected');
      }
      _subscribeTopics();

      // Listen for updates
      client!.updates!.listen(_onMessage);

      return true;
    } else {
      if (kDebugMode) {
        print(
          'MQTT: ERROR - Connection failed status is ${client!.connectionStatus!.state}',
        );
      }
      client!.disconnect();
      return false;
    }
  }

  void _subscribeTopics() {
    if (client != null &&
        client!.connectionStatus!.state == MqttConnectionState.connected) {
      // Subscribe to all relevant topics
      client!.subscribe("33424216/suhu", MqttQos.atMostOnce);
      client!.subscribe("33424216/kelembaban", MqttQos.atMostOnce);
      client!.subscribe("33424216/cahaya", MqttQos.atMostOnce);
    }
  }

  void publishLed(bool isOn) {
    if (client != null &&
        client!.connectionStatus!.state == MqttConnectionState.connected) {
      final builder = MqttClientPayloadBuilder();
      builder.addString(isOn ? "ON" : "OFF");
      client!.publishMessage(
        "33424216/led",
        MqttQos.atMostOnce,
        builder.payload!,
      );
    }
  }

  void _onMessage(List<MqttReceivedMessage<MqttMessage?>>? c) {
    final MqttPublishMessage recMess = c![0].payload as MqttPublishMessage;
    final String pt = MqttPublishPayload.bytesToStringAsString(
      recMess.payload.message,
    );

    final topic = c[0].topic;
    if (kDebugMode) {
      print('MQTT: Received message: topic=$topic, payload=$pt');
    }

    if (onMessageReceived != null) {
      onMessageReceived!(topic, pt);
    }
  }

  void _onConnected() {
    if (onConnectionChanged != null) onConnectionChanged!(true);
    if (kDebugMode) {
      print('MQTT: Connected callback');
    }
  }

  void _onDisconnected() {
    if (onConnectionChanged != null) onConnectionChanged!(false);
    if (kDebugMode) {
      print('MQTT: Disconnected callback');
    }
  }

  void _onSubscribed(String topic) {
    if (kDebugMode) {
      print('MQTT: Subscribed to $topic');
    }
  }
}
