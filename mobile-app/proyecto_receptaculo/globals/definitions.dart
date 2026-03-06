class WiFiCredentials {
  String ssid;
  String password;

  WiFiCredentials({required this.ssid, required this.password});
}

enum VariablesType { temperature, humidity, pressure, co2, alcohol, nitrogen }

class TemperatureThresholds {
  double min;
  double max;

  TemperatureThresholds({required this.min, required this.max});
}

class HumidityThresholds {
  double min;
  double max;

  HumidityThresholds({required this.min, required this.max});
}

class PressureThresholds {
  double min;
  double max;

  PressureThresholds({required this.min, required this.max});
}

class CO2Threshold {
  double min;
  double max;

  CO2Threshold({required this.min, required this.max});
}

class AlcoholThreshold {
  double min;
  double max;

  AlcoholThreshold({required this.min, required this.max});
}

class NitrogenThreshold {
  double min;
  double max;

  NitrogenThreshold({required this.min, required this.max});
}

class VariablesAlarmThresholds {
  TemperatureThresholds temperature;
  HumidityThresholds humidity;
  PressureThresholds pressure;
  CO2Threshold co2;
  AlcoholThreshold alcohol;
  NitrogenThreshold nitrogen;

  VariablesAlarmThresholds({
    required this.temperature,
    required this.humidity,
    required this.pressure,
    required this.co2,
    required this.alcohol,
    required this.nitrogen,
  });
}

class CurrentSettings {
  String wifiNetworkSSID;
  int samplingInterval; // in seconds
  bool door;
  bool motorpump;
  double weight;
  double pressureMinTh;
  double pressureMaxTh;
  CurrentSettings({
    required this.wifiNetworkSSID,
    required this.samplingInterval,
    required this.door,
    required this.motorpump,
    required this.weight,
    required this.pressureMinTh,
    required this.pressureMaxTh,
  });
}

class VariablesData {
  final DateTime time;
  final double temperature; // in °C
  final double humidity; // in %
  final double pressure; // in kPa
  final double carbondioxide; // in ppm
  final double alcohol; // in ppm
  final double nitrogen; // in ppm

  VariablesData(
    this.time,
    this.temperature,
    this.humidity,
    this.pressure,
    this.carbondioxide,
    this.alcohol,
    this.nitrogen,
  );
}

// enum EventType { door, motorpump, weight }

class DoorData {
  final DateTime time;
  final bool isOpen; // true = opened, false = closed
  DoorData(this.time, this.isOpen);
}

class WeightData {
  final DateTime time;
  final double weight; // weight in kg
  WeightData(this.time, this.weight);
}

class MotorpumpData {
  final DateTime time;
  final bool isOn; // true = on, false = off
  MotorpumpData(this.time, this.isOn);
}

extension DateOnlyCompare on DateTime {
  bool isSameDate(DateTime other) {
    return year == other.year && month == other.month && day == other.day;
  }
}
