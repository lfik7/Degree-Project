import 'package:flutter/material.dart';
import 'package:proyecto_receptaculo/globals/definitions.dart';
import 'package:proyecto_receptaculo/globals/variables.dart';
import 'package:proyecto_receptaculo/globals/functions.dart';
import 'package:proyecto_receptaculo/control/cloud_manager.dart';

// Página de alarmas

class AlarmsPage extends StatefulWidget {
  const AlarmsPage({super.key});

  @override
  State<AlarmsPage> createState() => _AlarmsPageState();
}

class _AlarmsPageState extends State<AlarmsPage>
    with SingleTickerProviderStateMixin {
  // Lista de opciones para el Dropdown de ejemplo (intervalo de envío)
  final CloudManager cloudManager = CloudManager();
  final List<String> _intervalOptions = [
    '1 min',
    '5 min',
    '10 min',
    '30 min',
    '60 min',
  ];

  final Map<VariablesType, RangeValues> _thresholdsLimits = {
    VariablesType.temperature: const RangeValues(5.0, 35.0),
    VariablesType.humidity: const RangeValues(20.0, 90.0),
    VariablesType.pressure: const RangeValues(10.0, 115.0),
    VariablesType.co2: const RangeValues(0.0, 5000.0),
    VariablesType.alcohol: const RangeValues(0.0, 500000.0),
    VariablesType.nitrogen: const RangeValues(0.0, 5000.0),
  };

  @override
  void initState() {
    super.initState();
    // Inicialización adicional si fuera necesaria
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Stack(
        children: [
          _mainWidget(),
          ValueListenableBuilder(
            valueListenable: isAlarmasLoading,
            builder: (context, isLoadingValue, _) {
              if (!isLoadingValue) {
                return const SizedBox.shrink();
              }
              return Container(
                color: Colors.black45,
                child: const Center(child: CircularProgressIndicator()),
              );
            },
          ),
        ],
      ),
    );
  }

  Widget _confSample() {
    return Card(
      child: ListTile(
        leading: Icon(
          Icons.alarm,
          color: Theme.of(context).colorScheme.primary,
        ),
        title: Text(
          'Sample interval',
          style: TextStyle(
            color: Theme.of(context).colorScheme.primary,
            fontWeight: FontWeight.bold,
          ),
        ),
        subtitle: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const SizedBox(height: 6),
            const SizedBox(height: 8),
            Row(
              children: [
                // Dropdown para elegir un intervalo
                Expanded(
                  child: DropdownButtonFormField<String>(
                    initialValue: selectedInterval,
                    items: _intervalOptions.map((opt) {
                      return DropdownMenuItem<String>(
                        value: opt,
                        child: Text(opt),
                      );
                    }).toList(),
                    onChanged: (val) {
                      if (val == null) return;
                      setState(() => selectedInterval = val);
                      debugPrint("Changed interval to: $val");
                      debugPrint("Selected interval: $selectedInterval");
                    },
                    decoration: const InputDecoration(
                      isDense: true,
                      contentPadding: EdgeInsets.symmetric(
                        horizontal: 12,
                        vertical: 8,
                      ),
                      border: OutlineInputBorder(),
                    ),
                  ),
                ),
                const SizedBox(width: 8),
                // Botón de ejemplo que muestra un SnackBar
                FilledButton(
                  onPressed: () {
                    ScaffoldMessenger.of(context).showSnackBar(
                      SnackBar(
                        behavior: SnackBarBehavior.floating,
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(20),
                        ),
                        duration: const Duration(milliseconds: 500),
                        content: Text('Selected Interval: $selectedInterval'),
                      ),
                    );
                    cloudManager.changeSampleInterval(selectedInterval);
                  },
                  child: const Text('OK'),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  static void _setAlarmThresholds(
    VariablesType variable,
    double minTh,
    double maxTh,
  ) {
    switch (variable) {
      case VariablesType.temperature:
        variablesAlarmThresholds.temperature.min = minTh;
        variablesAlarmThresholds.temperature.max = maxTh;
        break;
      case VariablesType.humidity:
        variablesAlarmThresholds.humidity.min = minTh;
        variablesAlarmThresholds.humidity.max = maxTh;
        break;
      case VariablesType.pressure:
        variablesAlarmThresholds.pressure.min = minTh;
        variablesAlarmThresholds.pressure.max = maxTh;
        break;
      case VariablesType.co2:
        variablesAlarmThresholds.co2.min = minTh;
        variablesAlarmThresholds.co2.max = maxTh;
        break;
      case VariablesType.alcohol:
        variablesAlarmThresholds.alcohol.min = minTh;
        variablesAlarmThresholds.alcohol.max = maxTh;
        break;
      case VariablesType.nitrogen:
        variablesAlarmThresholds.nitrogen.min = minTh;
        variablesAlarmThresholds.nitrogen.max = maxTh;
        break;
    }
  }

  double _getCurrentValues(VariablesType variable) {
    switch (variable) {
      case VariablesType.temperature:
        return variablesDataList.isNotEmpty
            ? variablesDataList.first.temperature
            : 0.0;
      case VariablesType.humidity:
        return variablesDataList.isNotEmpty
            ? variablesDataList.first.humidity
            : 0.0;
      case VariablesType.pressure:
        return variablesDataList.isNotEmpty
            ? variablesDataList.first.pressure
            : 0.0;
      case VariablesType.co2:
        return variablesDataList.isNotEmpty
            ? variablesDataList.first.carbondioxide
            : 0.0;
      case VariablesType.alcohol:
        return variablesDataList.isNotEmpty
            ? variablesDataList.first.alcohol
            : 0.0;
      case VariablesType.nitrogen:
        return variablesDataList.isNotEmpty
            ? variablesDataList.first.nitrogen
            : 0.0;
    }
  }

  static String _getVariableName(VariablesType variable) {
    switch (variable) {
      case VariablesType.temperature:
        return "Temperature";
      case VariablesType.humidity:
        return "Humidity";
      case VariablesType.pressure:
        return "Pressure";
      case VariablesType.co2:
        return "CO₂ (Carbon Dioxide)";
      case VariablesType.alcohol:
        return "OH (Alcohol)";
      case VariablesType.nitrogen:
        return "Nitrogen";
    }
  }

  Widget _alarmCard(VariablesType variable, IconData icon) {
    return ValueListenableBuilder(
      valueListenable: updateAlarmWidgets,
      builder: (context, value, child) {
        RangeValues rv = getAlarmThresholds(variable);
        final subtitleText =
            'Threshold: ${rv.start.toStringAsFixed(2)}  —  ${rv.end.toStringAsFixed(2)}';

        final name = _getVariableName(variable);

        double currentValues = _getCurrentValues(variable);
        return Card(
          color: currentValues > (rv.end) || currentValues < (rv.start)
              ? const Color.fromARGB(255, 153, 26, 17).withValues(alpha: 0.8)
              : null,
          child: ListTile(
            leading: Icon(
              icon,
              color: currentValues > (rv.end) || currentValues < (rv.start)
                  ? Colors.white
                  : Theme.of(context).colorScheme.primary,
            ),
            title: Text(
              name,
              style: TextStyle(
                color: currentValues > (rv.end) || currentValues < (rv.start)
                    ? Colors.white
                    : Theme.of(context).colorScheme.primary,
                fontWeight: FontWeight.bold,
              ),
            ),
            subtitle: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const SizedBox(height: 6),
                Text(
                  subtitleText,
                  style: TextStyle(
                    color:
                        currentValues > (rv.end) || currentValues < (rv.start)
                        ? Colors.white
                        : null,
                  ),
                ),
                const SizedBox(height: 4),
                Text(
                  "Current Value: ${currentValues.toStringAsFixed(2)}",
                  style: TextStyle(
                    color:
                        currentValues > (rv.end) || currentValues < (rv.start)
                        ? Colors.white
                        : null,
                  ),
                ),
              ],
            ),
            // Al tocar la tarjeta se abre el diálogo para editar los umbrales
            onTap: () => _showThresholdDialog(variable, icon),
          ),
        );
      },
    );
  }

  // Diálogo para editar min/max de un umbral.
  // Devuelve RangeValues (min, max) si el usuario pulsa "Apply",
  // o null si cancela.
  Future<void> _showThresholdDialog(
    VariablesType variable,
    IconData icon,
  ) async {
    final currentThresholds = getAlarmThresholds(variable);
    double tempMin = currentThresholds.start;
    double tempMax = currentThresholds.end;
    final thresholdsLimits = _thresholdsLimits[variable]!;

    final minController = TextEditingController(
      text: tempMin.toStringAsFixed(2),
    );
    final maxController = TextEditingController(
      text: tempMax.toStringAsFixed(2),
    );

    final name = _getVariableName(variable);

    // showDialog devuelve RangeValues cuando se pulsa "Guardar"
    final result = await showDialog<RangeValues>(
      context: context,
      builder: (dialogContext) {
        // Obtener dimensiones para limitar el alto/ancho en landscape
        final mq = MediaQuery.of(dialogContext);
        final maxHeight = mq.size.height * 0.8;
        final maxWidth = mq.orientation == Orientation.landscape
            ? mq.size.width * 0.6
            : mq.size.width * 0.95;

        return AlertDialog(
          insetPadding: const EdgeInsets.symmetric(
            horizontal: 20,
            vertical: 24,
          ),
          title: Row(
            children: [
              Icon(icon, color: Theme.of(context).colorScheme.primary),
              const SizedBox(width: 8),
              Expanded(
                // evita overflow, permite salto de línea corto y ellipsis
                child: Text(
                  'Threshold — $name',
                  style: TextStyle(
                    color: Theme.of(context).colorScheme.primary,
                    fontWeight: FontWeight.bold,
                  ),
                  softWrap: true,
                  maxLines: 2,
                  overflow: TextOverflow.ellipsis,
                ),
              ),
            ],
          ),
          content: ConstrainedBox(
            constraints: BoxConstraints(
              maxHeight: maxHeight,
              maxWidth: maxWidth,
            ),
            child: SingleChildScrollView(
              child: StatefulBuilder(
                builder: (context, setState) {
                  void syncFromTextFields() {
                    final parsedMin = double.tryParse(
                      minController.text.replaceAll(',', '.'),
                    );
                    final parsedMax = double.tryParse(
                      maxController.text.replaceAll(',', '.'),
                    );
                    if (parsedMin != null) tempMin = parsedMin;
                    if (parsedMax != null) tempMax = parsedMax;
                    if (tempMin > tempMax) {
                      final t = tempMin;
                      tempMin = tempMax;
                      tempMax = t;
                      minController.text = tempMin.toStringAsFixed(2);
                      maxController.text = tempMax.toStringAsFixed(2);
                    }
                    setState(() {});
                  }

                  return Column(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Row(
                        children: [
                          Expanded(
                            child: TextField(
                              controller: minController,
                              keyboardType:
                                  const TextInputType.numberWithOptions(
                                    decimal: true,
                                  ),
                              decoration: const InputDecoration(
                                labelText: 'Min',
                                isDense: true,
                              ),
                              onChanged: (_) => syncFromTextFields(),
                            ),
                          ),
                          const SizedBox(width: 12),
                          Expanded(
                            child: TextField(
                              controller: maxController,
                              keyboardType:
                                  const TextInputType.numberWithOptions(
                                    decimal: true,
                                  ),
                              decoration: const InputDecoration(
                                labelText: 'Max',
                                isDense: true,
                              ),
                              onChanged: (_) => syncFromTextFields(),
                            ),
                          ),
                        ],
                      ),
                      const SizedBox(height: 12),
                      Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(
                            'Adjust Range (${thresholdsLimits.start.toStringAsFixed(0)} — ${thresholdsLimits.end.toStringAsFixed(0)})',
                          ),
                          RangeSlider(
                            values: RangeValues(
                              tempMin.clamp(
                                thresholdsLimits.start,
                                thresholdsLimits.end,
                              ),
                              tempMax.clamp(
                                thresholdsLimits.start,
                                thresholdsLimits.end,
                              ),
                            ),
                            min: thresholdsLimits.start,
                            max: thresholdsLimits.end,
                            divisions: 100,
                            labels: RangeLabels(
                              tempMin.toStringAsFixed(2),
                              tempMax.toStringAsFixed(2),
                            ),
                            onChanged: (rv) {
                              setState(() {
                                tempMin = rv.start;
                                tempMax = rv.end;
                                minController.text = tempMin.toStringAsFixed(2);
                                maxController.text = tempMax.toStringAsFixed(2);
                              });
                            },
                          ),
                        ],
                      ),
                    ],
                  );
                },
              ),
            ),
          ),
          actions: [
            ElevatedButton(
              onPressed: () => Navigator.of(dialogContext).pop(null),
              child: const Text('Cancel'),
            ),
            FilledButton(
              onPressed: () {
                final parsedMin = double.tryParse(
                  minController.text.replaceAll(',', '.'),
                );
                final parsedMax = double.tryParse(
                  maxController.text.replaceAll(',', '.'),
                );
                final newMin = parsedMin ?? tempMin;
                final newMax = parsedMax ?? tempMax;
                if (newMin > newMax) {
                  Navigator.of(dialogContext).pop(RangeValues(newMax, newMin));
                } else {
                  Navigator.of(dialogContext).pop(RangeValues(newMin, newMax));
                }
                cloudManager.changeVariableAlarmThresholds(
                  variable,
                  parsedMin!,
                  parsedMax!,
                );
              },
              child: const Text('Apply'),
            ),
          ],
        );
      },
    );

    // Actualizar estado exterior después de cerrar el dialog
    if (result != null) {
      _setAlarmThresholds(variable, result.start, result.end);
      // _thresholds[variable] = result;
      setState(() {}); // refrescar UI
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          behavior: SnackBarBehavior.floating,
          duration: const Duration(milliseconds: 1000),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(20),
          ),
          content: Text(
            'Threshold saved for "$name": ${result.start.toStringAsFixed(2)} — ${result.end.toStringAsFixed(2)}',
          ),
        ),
      );
    }

    minController.dispose();
    maxController.dispose();
  }

  Widget _mainWidget() {
    return Center(
      child: ListView(
        children: [
          _confSample(),
          const Divider(thickness: 2),
          _alarmCard(VariablesType.temperature, Icons.thermostat_rounded),
          _alarmCard(VariablesType.humidity, Icons.water_drop_rounded),
          _alarmCard(VariablesType.pressure, Icons.speed_rounded),
          // _alarmCard(VariablesType.co2, Icons.cloud_rounded),
          _alarmCard(VariablesType.alcohol, Icons.cloud),
          // _alarmCard(VariablesType.nitrogen, Icons.cloud),
        ],
      ),
    );
  }
}
