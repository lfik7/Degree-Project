// import
import 'dart:async';
import 'package:async/async.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:proyecto_receptaculo/core/database_service.dart';
import 'package:flutter/foundation.dart';
import 'package:proyecto_receptaculo/globals/definitions.dart';
import 'package:proyecto_receptaculo/globals/variables.dart';

class CloudManager {
  // Aquí irían los métodos y propiedades para gestionar la nube.

  CloudManager.internal();
  static final CloudManager _instance = CloudManager.internal();
  factory CloudManager() => _instance;

  static DatabaseService database = DatabaseService();
  static StreamSubscription<DatabaseEvent>? alarmThresholdsStreamEvent;
  static StreamSubscription<DatabaseEvent>? currentSettingsStreamEvent;
  static StreamSubscription<DatabaseEvent>? doorDayStreamEvent;
  static StreamSubscription<DatabaseEvent>? motorpumpDayStreamEvent;
  static StreamSubscription<DatabaseEvent>? weightDayStreamEvent;
  static StreamSubscription<DatabaseEvent>? variablesDayStreamEvent;
  static StreamSubscription<DatabaseEvent>? monitorPresenceEvent;
  static StreamSubscription<DatabaseEvent>? wifiNetsEditsStreamEvent;
  static RestartableTimer? checkMonitorPresenceTimer;

  late DateTime _lastDate;

  void monitorDayChange() {
    _lastDate = DateTime.now();

    // Revisar cada minuto si el día cambió
    Timer.periodic(const Duration(minutes: 1), (timer) {
      final now = DateTime.now();
      if (now.day != _lastDate.day) {
        _lastDate = now;
        debugPrint("¡Es un nuevo día!");
        pauseStreamSubscriptions().then((_) {
          resumeStreamSubscriptions();
        });
      }
    });
  }

  Future<void> getAlarmThresholds() async {
    alarmThresholdsStreamEvent = database.getAlarmThresholds().listen((event) {
      final snapshot = event.snapshot;
      if (snapshot.exists) {
        Map<dynamic, dynamic> thresholds =
            snapshot.value as Map<dynamic, dynamic>;
        thresholds.forEach((key, value) {
          debugPrint(
            "Variable: $key, Min Threshold: ${value['mn']}, Max Threshold: ${value['mx']}",
          );
          if (key == 'Temp') {
            variablesAlarmThresholds.temperature = TemperatureThresholds(
              min: value['mn'].toDouble(),
              max: value['mx'].toDouble(),
            );
          } else if (key == 'Hume') {
            variablesAlarmThresholds.humidity = HumidityThresholds(
              min: value['mn'].toDouble(),
              max: value['mx'].toDouble(),
            );
          } else if (key == 'Pres') {
            variablesAlarmThresholds.pressure = PressureThresholds(
              min: value['mn'].toDouble(),
              max: value['mx'].toDouble(),
            );
          } else if (key == 'CO2') {
            variablesAlarmThresholds.co2 = CO2Threshold(
              min: value['mn'].toDouble(),
              max: value['mx'].toDouble(),
            );
          } else if (key == 'OH') {
            variablesAlarmThresholds.alcohol = AlcoholThreshold(
              min: value['mn'].toDouble(),
              max: value['mx'].toDouble(),
            );
          } else if (key == 'Nx') {
            variablesAlarmThresholds.nitrogen = NitrogenThreshold(
              min: value['mn'].toDouble(),
              max: value['mx'].toDouble(),
            );
          }
        });
        updateAlarmWidgets.value = !updateAlarmWidgets.value;
      } else {
        debugPrint("No alarm thresholds found.");
      }
    });
  }

  Future<void> changeVariableAlarmThresholds(
    VariablesType variable,
    double minTh,
    double maxTh,
  ) async {
    String varNode = '';
    switch (variable) {
      case VariablesType.temperature:
        varNode = 'Temp';
        break;
      case VariablesType.humidity:
        varNode = 'Hume';
        break;
      case VariablesType.pressure:
        varNode = 'Pres';
        break;
      case VariablesType.co2:
        varNode = 'CO2';
        break;
      case VariablesType.alcohol:
        varNode = 'OH';
        break;
      case VariablesType.nitrogen:
        varNode = 'Nx';
        break;
    }

    await database.changeAlarmThresholds(varNode, minTh, maxTh);
  }

  Future<void> getCurrentSettings() async {
    // Get current settings
    currentSettingsStreamEvent = database.getCurrentSettings.listen((event) {
      var data = event.snapshot.value as Map<dynamic, dynamic>;
      debugPrint("Current Settings: $data");
      currentSettings = CurrentSettings(
        wifiNetworkSSID: data['WID'] ?? '',
        samplingInterval: data['SI'] ?? 0,
        door: data['Do'] ?? true,
        motorpump: data['Mo'] ?? false,
        weight: (data['We'] ?? 0.0).toDouble(),
        pressureMinTh: (data['PTmn'] ?? 0.0).toDouble(),
        pressureMaxTh: (data['PTmx'] ?? 0.0).toDouble(),
      );
      updateCurrentSettingsWidgets.value = !updateCurrentSettingsWidgets.value;
      selectedInterval =
          '${(currentSettings.samplingInterval / 60).toInt().toString()} min';
    });
  }

  Future<void> getFirstDateOfData() async {
    DataSnapshot snapshot = await database.getFirtsDateOfData();
    debugPrint("First date of Data: ${snapshot.value}");
    String firstDateString = snapshot.value.toString();
    List<String> dateParts = firstDateString.split('/');
    int year = int.parse(dateParts[0]);
    int month = int.parse(dateParts[1]);
    int day = int.parse(dateParts[2]);
    firstDataTime = DateTime(year, month, day);
  }

  Future<void> getDoorData(DateTime startDate, DateTime endDate) async {
    // Get door events data
    if (doorDataList.isNotEmpty) {
      doorDataList.clear();
    }
    bool frontToBack = startDate.isAfter(endDate); // front is the most recent
    int startYear = startDate.year;
    int startMonth = startDate.month;
    int startDay = startDate.day;
    DataSnapshot doorEventsSnapshot;
    int doorDataListIndex = 0;
    int endYear = endDate.year;
    int endMonth = endDate.month;
    int endDay = endDate.day;
    debugPrint(
      "Range - Start: $startYear/$startMonth/$startDay, End: $endYear/$endMonth/$endDay",
    );
    endDay += frontToBack ? -1 : 1; // To include the end date
    while (doorDataListIndex < doorDataListDesiredLength) {
      if (frontToBack) {
        if (DateTime(
          startYear,
          startMonth,
          startDay,
        ).isBefore(DateTime(endYear, endMonth, endDay))) {
          debugPrint("No more data available. Stopping.");
          break;
        }
      } else {
        if (DateTime(
          startYear,
          startMonth,
          startDay,
        ).isAfter(DateTime(endYear, endMonth, endDay))) {
          debugPrint("No more data available. Stopping.");
          break;
        }
      }

      String node = "$startYear/$startMonth/$startDay";
      debugPrint("Fetching door events for node: Door/$node");
      doorEventsSnapshot = await database.getDoorEvents(node);
      debugPrint(
        "Door Events Snapshot length: ${doorEventsSnapshot.children.length}",
      );
      startDay += frontToBack ? -1 : 1;
      if (!frontToBack) {
        if (startDay > DateTime(startYear, startMonth + 1, 0).day) {
          startDay = 1;
          startMonth += 1;
          if (startMonth > 12) {
            startMonth = 1;
            startYear += 1;
          }
        }
      } else {
        if (startDay == 0) {
          startMonth -= 1;
          if (startMonth == 0) {
            startMonth = 12;
            startYear -= 1;
          }
          startDay = DateTime(startYear, startMonth + 1, 0).day;
        }
      }

      for (var child in doorEventsSnapshot.children) {
        var data = child.value as Map<dynamic, dynamic>;
        var doorData = DoorData(
          DateTime.parse(
            DateTime.fromMillisecondsSinceEpoch(
              (data['t'] * 1000),
            ).toIso8601String(),
          ),
          data['st'],
        );
        doorDataList.add(doorData);
        doorDataListIndex++;
        if (doorDataListIndex >= doorDataListDesiredLength) {
          break;
        }
      }
    }

    doorDataList.sort((a, b) => b.time.compareTo(a.time));
    isDoorDataListUpdating.value = !isDoorDataListUpdating.value;

    debugPrint("Door Events Entries: $doorDataListIndex");
    if (doorDataList.isEmpty) {
      debugPrint("No door data found in the specified range.");
      return;
    }
    debugPrint("First Entry Time: ${doorDataList.last.time}");
    debugPrint("Last  Entry Time: ${doorDataList.first.time}");
  }

  void suscribeToDoorEventsStream() {
    doorDayStreamEvent?.cancel();
    doorDayStreamEvent = database.getDoorDayEventsStream.listen((event) {
      var data = event.snapshot.value as Map<dynamic, dynamic>;
      final doorTime = DateTime.fromMillisecondsSinceEpoch(data['t'] * 1000);
      var doorData = DoorData(doorTime, data['st']);
      if (doorData.time == doorDataList.first.time) {
        // Evitar duplicados
        return;
      }
      debugPrint("New door event added: ${event.snapshot.value}");
      doorDataList.insert(0, doorData);
      if (doorDataList.length > doorDataListDesiredLength) {
        doorDataList.removeLast();
      }
      isDoorDataListUpdating.value = !isDoorDataListUpdating.value;
    });
  }

  Future<void> getMotorpumpData(DateTime startDate, DateTime endDate) async {
    // Get door events data
    if (motorpumpDataList.isNotEmpty) {
      motorpumpDataList.clear();
    }
    bool frontToBack = startDate.isAfter(endDate); // front is the most recent
    int startYear = startDate.year;
    int startMonth = startDate.month;
    int startDay = startDate.day;
    DataSnapshot motorpumpEventsSnapshot;
    int motorpumpDataListIndex = 0;
    int endYear = endDate.year;
    int endMonth = endDate.month;
    int endDay = endDate.day;
    debugPrint(
      "Range - Start: $startYear/$startMonth/$startDay, End: $endYear/$endMonth/$endDay",
    );
    endDay += frontToBack ? -1 : 1; // To include the end date
    while (motorpumpDataListIndex < motorpumpDataListDesiredLength) {
      if (frontToBack) {
        if (DateTime(
          startYear,
          startMonth,
          startDay,
        ).isBefore(DateTime(endYear, endMonth, endDay))) {
          debugPrint("No more data available. Stopping.");
          break;
        }
      } else {
        if (DateTime(
          startYear,
          startMonth,
          startDay,
        ).isAfter(DateTime(endYear, endMonth, endDay))) {
          debugPrint("No more data available. Stopping.");
          break;
        }
      }

      String node = "$startYear/$startMonth/$startDay";
      motorpumpEventsSnapshot = await database.getMotorpumpEvents(node);
      debugPrint(
        "Motorpump Events Snapshot length: ${motorpumpEventsSnapshot.children.length}",
      );
      startDay += frontToBack ? -1 : 1;
      if (!frontToBack) {
        if (startDay > DateTime(startYear, startMonth + 1, 0).day) {
          startDay = 1;
          startMonth += 1;
          if (startMonth > 12) {
            startMonth = 1;
            startYear += 1;
          }
          startDay = DateTime(startYear, startMonth + 1, 0).day;
        }
      } else {
        if (startDay == 0) {
          startMonth -= 1;
          if (startMonth == 0) {
            startMonth = 12;
            startYear -= 1;
          }
          startDay = DateTime(startYear, startMonth + 1, 0).day;
        }
      }

      for (var child in motorpumpEventsSnapshot.children) {
        var data = child.value as Map<dynamic, dynamic>;
        var motorpumpData = MotorpumpData(
          DateTime.parse(
            DateTime.fromMillisecondsSinceEpoch(
              (data['t'] * 1000),
            ).toIso8601String(),
          ),
          data['st'],
        );
        motorpumpDataList.add(motorpumpData);
        motorpumpDataListIndex++;
        if (motorpumpDataListIndex >= motorpumpDataListDesiredLength) {
          break;
        }
      }
    }

    motorpumpDataList.sort((a, b) => b.time.compareTo(a.time));
    isMotorpumpDataListUpdating.value = !isMotorpumpDataListUpdating.value;

    debugPrint("Motorpump Events Entries: $motorpumpDataListIndex");
    if (motorpumpDataList.isEmpty) {
      debugPrint("No motorpump data found in the specified range.");
      return;
    }
    debugPrint("First Entry Time: ${motorpumpDataList.last.time}");
    debugPrint("Last  Entry Time: ${motorpumpDataList.first.time}");
  }

  void suscribeToMotorpumpEventsStream() {
    motorpumpDayStreamEvent?.cancel();
    motorpumpDayStreamEvent = database.getMotorpumpDayEventsStream.listen((
      event,
    ) {
      var data = event.snapshot.value as Map<dynamic, dynamic>;
      var motorpumpTime = DateTime.fromMillisecondsSinceEpoch(data['t'] * 1000);
      var motorpumpData = MotorpumpData(motorpumpTime, data['st']);
      if (motorpumpData.time == motorpumpDataList.first.time) {
        // Evitar duplicados
        return;
      }
      debugPrint("New motorpump event added: ${event.snapshot.value}");
      motorpumpDataList.insert(0, motorpumpData);
      if (motorpumpDataList.length > motorpumpDataListDesiredLength) {
        motorpumpDataList.removeLast();
      }
      isMotorpumpDataListUpdating.value = !isMotorpumpDataListUpdating.value;
    });
  }

  Future<void> getWeightData(DateTime startDate, DateTime endDate) async {
    // Get weight events data
    if (weightDataList.isNotEmpty) {
      weightDataList.clear();
    }
    bool frontToBack = startDate.isAfter(endDate); // front is the most recent
    int startYear = startDate.year;
    int startMonth = startDate.month;
    int startDay = startDate.day;
    DataSnapshot weightEventsSnapshot;
    int weightDataListIndex = 0;
    int endYear = endDate.year;
    int endMonth = endDate.month;
    int endDay = endDate.day;
    debugPrint(
      "Range - Start: $startYear/$startMonth/$startDay, End: $endYear/$endMonth/$endDay",
    );
    endDay += frontToBack ? -1 : 1; // To include the end date
    while (weightDataListIndex < weightDataListDesiredLength) {
      if (frontToBack) {
        if (DateTime(
          startYear,
          startMonth,
          startDay,
        ).isBefore(DateTime(endYear, endMonth, endDay))) {
          debugPrint("No more data available. Stopping.");
          break;
        }
      } else {
        if (DateTime(
          startYear,
          startMonth,
          startDay,
        ).isAfter(DateTime(endYear, endMonth, endDay))) {
          debugPrint("No more data available. Stopping.");
          break;
        }
      }
      String node = "$startYear/$startMonth/$startDay";
      weightEventsSnapshot = await database.getWeightEvents(node);
      debugPrint(
        "Weight Events Snapshot length: ${weightEventsSnapshot.children.length}",
      );
      startDay += frontToBack ? -1 : 1;
      if (!frontToBack) {
        if (startDay > DateTime(startYear, startMonth + 1, 0).day) {
          startDay = 1;
          startMonth += 1;
          if (startMonth > 12) {
            startMonth = 1;
            startYear += 1;
          }
        }
      } else {
        if (startDay == 0) {
          startMonth -= 1;
          if (startMonth == 0) {
            startMonth = 12;
            startYear -= 1;
          }
          startDay = DateTime(startYear, startMonth + 1, 0).day;
        }
      }

      for (var child in weightEventsSnapshot.children) {
        var data = child.value as Map<dynamic, dynamic>;
        var weightData = WeightData(
          DateTime.parse(
            DateTime.fromMillisecondsSinceEpoch(
              (data['t'] * 1000),
            ).toIso8601String(),
          ),
          data['vl'].toDouble(),
        );
        weightDataList.add(weightData);
        weightDataListIndex++;
        if (weightDataListIndex >= weightDataListDesiredLength) {
          break;
        }
      }
    }

    weightDataList.sort((a, b) => b.time.compareTo(a.time));
    isWeightDataListUpdating.value = !isWeightDataListUpdating.value;

    debugPrint("Weight Events Entries: $weightDataListIndex");
    if (weightDataList.isEmpty) {
      debugPrint("No weight data found in the specified range.");
      return;
    }
    debugPrint("First Entry Time: ${weightDataList.last.time}");
    debugPrint("Last  Entry Time: ${weightDataList.first.time}");
  }

  void suscribeToWeightEventsStream() {
    weightDayStreamEvent?.cancel();
    weightDayStreamEvent = database.getWeightDayEventsStream.listen((event) {
      var data = event.snapshot.value as Map<dynamic, dynamic>;
      var weightTime = DateTime.fromMillisecondsSinceEpoch(data['t'] * 1000);
      var weightData = WeightData(weightTime, data['vl'].toDouble());
      if (weightData.time == weightDataList.first.time) {
        // Evitar duplicados
        return;
      }
      debugPrint("New weight event added: ${event.snapshot.value}");
      weightDataList.insert(0, weightData);
      if (weightDataList.length > weightDataListDesiredLength) {
        weightDataList.removeLast();
      }
      isWeightDataListUpdating.value = !isWeightDataListUpdating.value;
    });
  }

  Future<void> getVariablesData(DateTime startDate, DateTime endDate) async {
    // Get data log data
    if (variablesDataList.isNotEmpty) {
      variablesDataList.clear();
    }
    bool frontToBack = startDate.isAfter(endDate); // front is the most recent
    int startYear = startDate.year;
    int startMonth = startDate.month;
    int startDay = startDate.day;
    DataSnapshot dataLogSnapshot;
    int variablesDataListIndex = 0;
    int endYear = endDate.year;
    int endMonth = endDate.month;
    int endDay = endDate.day;
    debugPrint(
      "Range - Start: $startYear/$startMonth/$startDay, End: $endYear/$endMonth/$endDay",
    );
    endDay += frontToBack ? -1 : 1; // To include the end date
    while (variablesDataListIndex < variablesDataListDesiredLength) {
      if (frontToBack) {
        if (DateTime(
          startYear,
          startMonth,
          startDay,
        ).isBefore(DateTime(endYear, endMonth, endDay))) {
          debugPrint("No more data available. Stopping.");
          break;
        }
      } else {
        if (DateTime(
          startYear,
          startMonth,
          startDay,
        ).isAfter(DateTime(endYear, endMonth, endDay))) {
          debugPrint("No more data available. Stopping.");
          break;
        }
      }
      String node = "$startYear/$startMonth/$startDay";
      debugPrint("Fetching variables data for node: $node");
      dataLogSnapshot = await database.getDataLog(node);
      debugPrint(
        "Data Log Snapshot length: ${dataLogSnapshot.children.length}",
      );
      startDay += frontToBack ? -1 : 1;
      if (!frontToBack) {
        if (startDay > DateTime(startYear, startMonth + 1, 0).day) {
          startDay = 1;
          startMonth += 1;
          if (startMonth > 12) {
            startMonth = 1;
            startYear += 1;
          }
        }
      } else {
        if (startDay == 0) {
          startMonth -= 1;
          if (startMonth == 0) {
            startMonth = 12;
            startYear -= 1;
          }
          startDay = DateTime(startYear, startMonth + 1, 0).day;
        }
      }

      for (var child in dataLogSnapshot.children) {
        var data = child.value as Map<dynamic, dynamic>;
        var variablesData = VariablesData(
          DateTime.parse(
            DateTime.fromMillisecondsSinceEpoch(
              (data['t'] * 1000),
            ).toIso8601String(),
          ),
          data['T'].toDouble(),
          data['H'].toDouble(),
          data['P'].toDouble(),
          data['C'].toDouble(),
          data['O'].toDouble(),
          data['N'].toDouble(),
        );
        variablesDataList.add(variablesData);
        variablesDataListIndex++;
        if (variablesDataListIndex >= variablesDataListDesiredLength) {
          break;
        }
      }
    }

    variablesDataList.sort((a, b) => b.time.compareTo(a.time));
    isVariablesDataListUpdating.value = !isVariablesDataListUpdating.value;

    debugPrint("Data Log  Entries: $variablesDataListIndex");
    if (variablesDataList.isEmpty) {
      debugPrint("No variables data found in the specified range.");
      return;
    }
    debugPrint("First Entry Time: ${variablesDataList.last.time}");
    debugPrint("Last  Entry Time: ${variablesDataList.first.time}");
  }

  void suscribeToDataLogStream() {
    variablesDayStreamEvent?.cancel();
    variablesDayStreamEvent = database.getDataLogDayEventsStream.listen((
      event,
    ) {
      var data = event.snapshot.value as Map<dynamic, dynamic>;
      var variablesTime = DateTime.fromMillisecondsSinceEpoch(data['t'] * 1000);
      var variablesData = VariablesData(
        variablesTime,
        data['T'].toDouble(),
        data['H'].toDouble(),
        data['P'].toDouble(),
        data['C'].toDouble(),
        data['O'].toDouble(),
        data['N'].toDouble(),
      );
      if (variablesData.time == variablesDataList.first.time) {
        // Evitar duplicados
        return;
      }
      debugPrint("New data log entry added: ${event.snapshot.value}");
      variablesDataList.insert(0, variablesData);
      if (variablesDataList.length > variablesDataListDesiredLength) {
        variablesDataList.removeLast();
      }
      isVariablesDataListUpdating.value = !isVariablesDataListUpdating.value;
      updateAlarmWidgets.value = !updateAlarmWidgets.value;
    });
  }

  Future<void> clearDataBaseNodes(DateTime startDate, DateTime endDate) async {
    debugPrint("Clearing data from $startDate to $endDate");

    var firstDate = startDate;
    var lastDate = endDate;
    var nodeDate = '${firstDate.year}/${firstDate.month}/${firstDate.day}';

    for (
      firstDate;
      firstDate.isBefore(lastDate) || firstDate.isAtSameMomentAs(lastDate);
      firstDate = firstDate.add(const Duration(days: 1))
    ) {
      nodeDate = '${firstDate.year}/${firstDate.month}/${firstDate.day}';
      debugPrint("Deleting data for date node: $nodeDate");
      await Future.wait([
        database.deleteDoorDataNode(nodeDate),
        database.deleteMotorpumpDataNode(nodeDate),
        database.deleteWeightDataNode(nodeDate),
        database.deleteDataLogNode(nodeDate),
      ]);
      await Future.delayed(const Duration(milliseconds: 5));
      debugPrint("Data deleted for date node: $nodeDate");
    }

    if (firstDate.isAtSameMomentAs(firstDate)) {
      lastDate = lastDate.add(const Duration(days: 1));
      String newFirstDataDate =
          '${lastDate.year}/${lastDate.month}/${lastDate.day}';
      await database.updateFirtsDateOfData(newFirstDataDate);
      debugPrint("First date of data updated to: $newFirstDataDate");
      firstDataTime = lastDate;
    }

    debugPrint("Data cleared from $startDate to $endDate");
  }

  Future<void> pauseStreamSubscriptions() async {
    await doorDayStreamEvent?.cancel();
    await motorpumpDayStreamEvent?.cancel();
    await weightDayStreamEvent?.cancel();
    await variablesDayStreamEvent?.cancel();
  }

  Future<void> resumeStreamSubscriptions() async {
    suscribeToDoorEventsStream();
    suscribeToMotorpumpEventsStream();
    suscribeToWeightEventsStream();
    suscribeToDataLogStream();
  }

  Future<void> getMonitorPresence() async {
    monitorPresenceEvent = database.getMonitorPresenceStream.listen((event) {
      var data = event.snapshot.value as Map<dynamic, dynamic>;
      debugPrint("Monitor Presence Data: $data");
      monitorLastSeen = DateTime.parse(
        DateTime.fromMillisecondsSinceEpoch(
          (data['t'] * 1000),
        ).toIso8601String(),
      );
      checkMonitorPresenceTimer!.reset();
      updateMonitorPresenceWidget.value = !updateMonitorPresenceWidget.value;
    });
  }

  Future<void> changeSampleInterval(String sampInt) async {
    debugPrint("Changing sample interval to: $sampInt");
    int newInterval = sampInt.contains('min')
        ? int.parse(sampInt.split(' ')[0]) * 60
        : int.parse(sampInt.split(' ')[0]);
    await database.changeSampleInterval(newInterval);
  }

  Future<void> getWiFiNets() async {
    DataSnapshot snapshot = await database.getWiFiNets();
    debugPrint("Datos obtenidos: ${snapshot.value}");
    if (snapshot.exists) {
      Map<dynamic, dynamic> nets = snapshot.value as Map<dynamic, dynamic>;
      int index = 0;
      nets.forEach((key, value) {
        if (index < wifiCredentialsList.length) {
          wifiCredentialsList[index] = WiFiCredentials(
            ssid: value['ID'] ?? '',
            password: value['PS'] ?? '',
          );
          index++;
        }
      });
      isWifiCredentialsListUpdating.value =
          !isWifiCredentialsListUpdating.value;
    } else {
      debugPrint("No WiFi networks found.");
    }
  }

  Future<void> changeWiFiNets() async {
    Map<String, Object> nets = {};
    int wifiListPointer = 0;
    for (var cred in wifiCredentialsList) {
      debugPrint(
        "Before changing - SSID: ${cred.ssid}, Password: ${cred.password}",
      );
      String node = "N${wifiListPointer + 1}";
      wifiListPointer++;
      nets[node] = {'ID': cred.ssid, 'PS': cred.password};
    }
    await database.changeWiFiNets(nets);
  }

  Future<void> suscribeToWiFiNetsEditsStream() async {
    wifiNetsEditsStreamEvent = database.getWiFiNetsEditsStream.listen((
      event,
    ) async {
      debugPrint("WiFi Networks Edited: ${event.snapshot.value}");
      if (event.snapshot.value == true) {
        debugPrint("Descargando nuevas credenciales WiFi...");
        await getWiFiNets();
        await database.changeWifiNetsEdits('ESP', false);
        debugPrint("Nuevas credenciales WiFi descargadas.");
      }
    });
  }

  Future<void> initializingCloudManager() async {
    isStatisticsLoading.value = true;
    isAlarmasLoading.value = true;
    isControlsLoading.value = true;

    await getFirstDateOfData();

    await getAlarmThresholds();

    await getCurrentSettings();

    await getVariablesData(DateTime.now(), firstDataTime);

    await getDoorData(DateTime.now(), firstDataTime);

    await getMotorpumpData(DateTime.now(), firstDataTime);

    await getWeightData(DateTime.now(), firstDataTime);

    await resumeStreamSubscriptions();

    await getMonitorPresence();

    await getWiFiNets();
    await suscribeToWiFiNetsEditsStream();

    monitorDayChange();

    checkMonitorPresenceTimer = RestartableTimer(
      const Duration(minutes: 6),
      () {
        updateMonitorPresenceWidget.value = !updateMonitorPresenceWidget.value;
      },
    );

    isStatisticsLoading.value = false;
    isAlarmasLoading.value = false;
    isControlsLoading.value = false;
  }

  Future<void> disposeCloudManager() async {
    await pauseStreamSubscriptions();
    await alarmThresholdsStreamEvent?.cancel();
    await currentSettingsStreamEvent?.cancel();
    await monitorPresenceEvent?.cancel();
    await wifiNetsEditsStreamEvent?.cancel();
    checkMonitorPresenceTimer?.cancel();
  }
}
