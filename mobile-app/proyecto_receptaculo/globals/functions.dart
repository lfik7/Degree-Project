import 'package:proyecto_receptaculo/globals/variables.dart';
import 'package:proyecto_receptaculo/globals/definitions.dart';
import 'package:flutter/material.dart';

RangeValues getAlarmThresholds(VariablesType variable) {
  switch (variable) {
    case VariablesType.temperature:
      return RangeValues(
        variablesAlarmThresholds.temperature.min,
        variablesAlarmThresholds.temperature.max,
      );
    case VariablesType.humidity:
      return RangeValues(
        variablesAlarmThresholds.humidity.min,
        variablesAlarmThresholds.humidity.max,
      );
    case VariablesType.pressure:
      return RangeValues(
        variablesAlarmThresholds.pressure.min,
        variablesAlarmThresholds.pressure.max,
      );
    case VariablesType.co2:
      return RangeValues(
        variablesAlarmThresholds.co2.min,
        variablesAlarmThresholds.co2.max,
      );
    case VariablesType.alcohol:
      return RangeValues(
        variablesAlarmThresholds.alcohol.min,
        variablesAlarmThresholds.alcohol.max,
      );
    case VariablesType.nitrogen:
      return RangeValues(
        variablesAlarmThresholds.nitrogen.min,
        variablesAlarmThresholds.nitrogen.max,
      );
  }
}
