import 'dart:io';
import 'package:flutter/foundation.dart';
import 'package:proyecto_receptaculo/globals/variables.dart';

class WiFiManager {
  WiFiManager.internal();
  static final WiFiManager _instance = WiFiManager.internal();
  factory WiFiManager() => _instance;
  late Socket socket;

  Future<void> connectToMonitorWifi() async {
    connectingToMonitorNetwork.value = true;
    connectedToMonitorNetwork = false;
    try {
      socket = await Socket.connect(
        '192.168.4.1',
        8080,
        timeout: const Duration(seconds: 5),
      );
      debugPrint(
        'Conectado a: ${socket.remoteAddress.address}:${socket.remotePort}',
      );
      connectedToMonitorNetwork = true;
    } catch (e) {
      debugPrint('Error al conectar al WiFi del monitor: $e');
      connectedToMonitorNetwork = false;
    } finally {
      connectingToMonitorNetwork.value = false;
    }
  }

  Future<void> sendWifiNets() async {
    try {
      int wifiPos = 1;
      for (var cred in wifiCredentialsList) {
        String datos = 'wn$wifiPos${cred.ssid},${cred.password};\n';
        debugPrint('Enviando datos WiFi: $datos');
        socket.write(datos);
        await Future.delayed(
          const Duration(milliseconds: 500),
        ); // Pequeña pausa entre envíos
        wifiPos++;
      }

      await socket.flush(); // Asegura que los datos salieron del buffer
    } catch (e) {
      debugPrint('No se pudo conectar: $e');
    }
  }

  Future<void> closeConnection() async {
    try {
      await socket.close();
      debugPrint('Conexión cerrada.');
    } catch (e) {
      debugPrint('Error al cerrar la conexión: $e');
    }
  }
}
