import 'package:firebase_database/firebase_database.dart';
import 'package:flutter/foundation.dart'; // Importante

class DatabaseService {
  // 1. Instancia de la base de datos
  final FirebaseDatabase _database = FirebaseDatabase.instance;

  // Alarm thresholds
  Stream<DatabaseEvent> getAlarmThresholds() {
    return _database.ref('Alarm_thresholds').onValue;
  }

  Future<void> changeAlarmThresholds(
    String varNode,
    double minTh,
    double maxTh,
  ) async {
    return _database.ref('Alarm_thresholds/$varNode').update({
      'mn': minTh,
      'mx': maxTh,
    });
  }

  // Current settings
  Stream<DatabaseEvent> get getCurrentSettings {
    return _database.ref('Current_Settings').onValue;
  }

  // Data events
  Future<DataSnapshot> getFirtsDateOfData() async {
    // debugPrint("Fetching first date of door events");
    return await _database.ref('Data_first_date').get();
  }

  Future<void> updateFirtsDateOfData(String dateString) async {
    try {
      await _database.ref('').update({'Data_first_date': dateString});
    } catch (e) {
      debugPrint("Error updating first date of data: $e");
    }
  }

  // Door events
  Future<DataSnapshot> getDoorEvents(String node) async {
    // debugPrint("Fetching door events for node: Door/$node");
    return await _database.ref('Data_events/Door/$node').get();
  }

  Stream<DatabaseEvent> get getDoorDayEventsStream {
    return _database
        .ref(
          'Data_events/Door/${DateTime.now().year}/${DateTime.now().month}/${DateTime.now().day}',
        )
        .limitToLast(1)
        .onChildAdded;
  }

  Future<void> deleteDoorDataNode(String node) async {
    String cleanNode = node.trim();
    if (cleanNode.isEmpty) {
      debugPrint("⚠️ INTENTO DE BORRADO MASIVO ABORTADO: El nodo está vacío.");
      return;
    }
    try {
      debugPrint("Deleting data door node: $cleanNode");
      await _database.ref('Data_events/Door/$cleanNode').remove();
    } catch (e) {
      debugPrint("Error deleting door data: $e");
    }
  }

  Future<DataSnapshot> getMotorpumpEvents(String node) async {
    // debugPrint("Fetching motorpump events for node: MoPu/$node");
    return await _database.ref('Data_events/MoPu/$node').get();
  }

  Stream<DatabaseEvent> get getMotorpumpDayEventsStream {
    return _database
        .ref(
          'Data_events/MoPu/${DateTime.now().year}/${DateTime.now().month}/${DateTime.now().day}',
        )
        .limitToLast(1)
        .onChildAdded;
  }

  Future<void> deleteMotorpumpDataNode(String node) async {
    String cleanNode = node.trim();
    if (cleanNode.isEmpty) {
      debugPrint("⚠️ INTENTO DE BORRADO MASIVO ABORTADO: El nodo está vacío.");
      return;
    }
    try {
      debugPrint("Deleting data motorpump node: $cleanNode");
      await _database.ref('Data_events/MoPu/$cleanNode').remove();
    } catch (e) {
      debugPrint("Error deleting motorpump data: $e");
    }
  }

  Future<DataSnapshot> getWeightEvents(String node) async {
    // debugPrint("Fetching weight events for node: Weig/$node");
    return await _database.ref('Data_events/Weig/$node').get();
  }

  Stream<DatabaseEvent> get getWeightDayEventsStream {
    return _database
        .ref(
          'Data_events/Weig/${DateTime.now().year}/${DateTime.now().month}/${DateTime.now().day}',
        )
        .limitToLast(1)
        .onChildAdded;
  }

  Future<void> deleteWeightDataNode(String node) async {
    String cleanNode = node.trim();
    if (cleanNode.isEmpty) {
      debugPrint("⚠️ INTENTO DE BORRADO MASIVO ABORTADO: El nodo está vacío.");
      return;
    }
    try {
      debugPrint("Deleting data weight node: $cleanNode");
      await _database.ref('Data_events/Weig/$cleanNode').remove();
    } catch (e) {
      debugPrint("Error deleting weight data: $e");
    }
  }

  // Data log (get some data)
  Future<DataSnapshot> getDataLog(String node) async {
    // debugPrint("Fetching data log for node: Data_log/$node");
    return await _database.ref('Data_log/$node').get();
  }

  // Data log stream (listen for new data)
  Stream<DatabaseEvent> get getDataLogDayEventsStream {
    // Supongamos que queremos escuchar un nodo específico
    return _database
        .ref(
          'Data_log/${DateTime.now().year}/${DateTime.now().month}/${DateTime.now().day}',
        )
        .limitToLast(1)
        .onChildAdded;
  }

  Future<void> deleteDataLogNode(String node) async {
    String cleanNode = node.trim();
    if (cleanNode.isEmpty) {
      debugPrint("⚠️ INTENTO DE BORRADO MASIVO ABORTADO: El nodo está vacío.");
      return;
    }
    try {
      debugPrint("Deleting data log node: $cleanNode");
      await _database.ref('Data_log/$cleanNode').remove();
    } catch (e) {
      debugPrint("Error deleting data log: $e");
    }
  }

  Stream<DatabaseEvent> get getMonitorPresenceStream {
    return _database.ref('Monitor_presence').onValue;
  }

  // Sample interval
  Future<void> changeSampleInterval(int newInterval) async {
    try {
      await _database.ref('').update({'Sample_Inter': newInterval});
    } catch (e) {
      debugPrint("Error al cambiar el intervalo de muestreo: $e");
    }
  }

  // WiFi Nets Edits stream
  Stream<DatabaseEvent> get getWiFiNetsEditsStream {
    return _database.ref('WiFi_Nets/Edits/ESP').onValue;
  }

  // WiFi Nets Edits change
  Future<void> changeWifiNetsEdits(String node, bool value) async {
    try {
      await _database.ref('WiFi_Nets/Edits').update({node: value});
    } catch (e) {
      debugPrint("Error al cambiar el estado de edición de WiFi_Nets: $e");
    }
  }

  // WiFi Nets get nets
  Future<DataSnapshot> getWiFiNets() async {
    return await _database.ref('WiFi_Nets/Nets').get();
  }

  // WiFi Nets change nets
  Future<void> changeWiFiNets(Map<String, Object> nets) async {
    await _database.ref('WiFi_Nets/Nets').update(nets);
    await changeWifiNetsEdits('APP', true);
  }
}
