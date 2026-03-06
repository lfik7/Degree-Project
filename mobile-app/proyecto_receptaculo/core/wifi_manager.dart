import 'dart:io';
// import 'dart:convert'; // Para convertir Strings a Bytes
import 'package:flutter/foundation.dart';
// import 'package:app_settings/app_settings.dart';
// import 'package:proyecto_receptaculo/globals/definitions.dart';
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
        // var monitorResponse;
        // String responseDecoder;
        // socket.listen(
        //   (monitorResponse) {
        //     responseDecoder = utf8.decode(monitorResponse);
        //     debugPrint('Respuesta del servidor: $responseDecoder');
        //   },
        //   onError: (error) => debugPrint('Error en el stream: $error'),
        //   onDone: () => debugPrint('Servidor cerró la conexión.'),
        // );
        // if (responseDecoder != "Ok") {
        //   debugPrint('Error al enviar datos WiFi: $responseDecoder');
        // }
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
