import 'package:flutter/foundation.dart';
import 'package:proyecto_receptaculo/globals/definitions.dart';

ValueNotifier<bool> userLogged = ValueNotifier<bool>(false);

ValueNotifier<bool> isStatisticsLoading = ValueNotifier<bool>(false);
ValueNotifier<bool> isAlarmasLoading = ValueNotifier<bool>(false);
ValueNotifier<bool> isControlsLoading = ValueNotifier<bool>(false);
// bool updateGraphs = true;

VariablesAlarmThresholds variablesAlarmThresholds = VariablesAlarmThresholds(
  temperature: TemperatureThresholds(min: 5.0, max: 25.0),
  humidity: HumidityThresholds(min: 20.0, max: 80.0),
  pressure: PressureThresholds(min: 10.0, max: 110.0),
  co2: CO2Threshold(min: 0.0, max: 5000.0),
  alcohol: AlcoholThreshold(min: 0.0, max: 1000.0),
  nitrogen: NitrogenThreshold(min: 0.0, max: 1000.0),
);
ValueNotifier<bool> updateAlarmWidgets = ValueNotifier<bool>(false);

ValueNotifier<bool> connectingToMonitorNetwork = ValueNotifier<bool>(false);
bool connectedToMonitorNetwork = false;

CurrentSettings currentSettings = CurrentSettings(
  wifiNetworkSSID: '',
  samplingInterval: 0,
  door: true,
  motorpump: false,
  weight: 0.0,
  pressureMinTh: 0.0,
  pressureMaxTh: 0.0,
);
ValueNotifier<bool> updateCurrentSettingsWidgets = ValueNotifier<bool>(false);

int variablesDataListDesiredLength = 1500;
List<VariablesData> variablesDataList = [];
ValueNotifier<bool> isVariablesDataListUpdating = ValueNotifier<bool>(false);
DateTime firstDataTime = DateTime(2000, 1, 1);

int doorDataListDesiredLength = 50;
List<DoorData> doorDataList = [];
ValueNotifier<bool> isDoorDataListUpdating = ValueNotifier<bool>(false);

int weightDataListDesiredLength = 50;
List<WeightData> weightDataList = [];
ValueNotifier<bool> isWeightDataListUpdating = ValueNotifier<bool>(false);

int motorpumpDataListDesiredLength = 50;
List<MotorpumpData> motorpumpDataList = [];
ValueNotifier<bool> isMotorpumpDataListUpdating = ValueNotifier<bool>(false);

// Here create and initialize monitor presence in the past to avoid null issues
DateTime monitorLastSeen = DateTime.now().subtract(const Duration(days: 365));
ValueNotifier<bool> updateMonitorPresenceWidget = ValueNotifier<bool>(false);

String selectedInterval = '5 min';
// String currentInterval = '5 min';

List<WiFiCredentials> wifiCredentialsList = List.filled(
  5,
  WiFiCredentials(ssid: '', password: ''),
);
ValueNotifier<bool> isWifiCredentialsListUpdating = ValueNotifier<bool>(false);
