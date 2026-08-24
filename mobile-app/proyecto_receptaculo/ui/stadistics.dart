import 'package:flutter/material.dart';
import 'package:intl/intl.dart';
import 'package:syncfusion_flutter_charts/charts.dart';
import 'package:proyecto_receptaculo/globals/definitions.dart';
import 'package:proyecto_receptaculo/globals/variables.dart';
import 'package:proyecto_receptaculo/globals/functions.dart';
import 'package:proyecto_receptaculo/globals/own_widgets.dart';
import 'package:proyecto_receptaculo/control/cloud_manager.dart';

// Página de estadísticas y gráficas

class StadisticsPage extends StatefulWidget {
  const StadisticsPage({super.key});

  @override
  State<StadisticsPage> createState() => _StadisticsPageState();
}

// Pequeña clase para representar un punto del gráfico (eje X = DateTime)
class _ChartPoint {
  final DateTime x;
  final double y;
  _ChartPoint(this.x, this.y);
}

class _StadisticsPageState extends State<StadisticsPage>
    with SingleTickerProviderStateMixin {
  // Estado simple para la puerta

  final CloudManager cloudManager = CloudManager();

  @override
  void initState() {
    super.initState();
    // Inits opcionales aquí
  }

  // Formatea DateTime como HH:MM
  String _formatTime(DateTime dt) {
    final h = dt.hour.toString().padLeft(2, '0');
    final m = dt.minute.toString().padLeft(2, '0');
    return '$h:$m';
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Stack(
        children: [
          _mainWidget(),
          ValueListenableBuilder(
            valueListenable: isStatisticsLoading,
            builder: (context, isLoadingValue, _) {
              if (!isLoadingValue) {
                return const SizedBox.shrink();
              }
              return loadingOverlay();
            },
          ),
        ],
      ),
    );
  }

  // Diálogo modal que muestra el historial de eventos de la puerta.
  // Se utiliza por simplicidad un AlertDialog con una ListView.
  Widget _doorHistory() {
    return ValueListenableBuilder(
      valueListenable: isDoorDataListUpdating,
      builder: (context, value, child) {
        return AlertDialog(
          title: const Text(
            'Door History',
            style: TextStyle(fontWeight: FontWeight.bold),
          ),
          content: SizedBox(
            width: 350,
            height: 300,
            child: ListView.separated(
              itemCount: doorDataList.length,
              separatorBuilder: (_, __) => const Divider(height: 1),
              itemBuilder: (context, index) {
                final e = doorDataList[index];
                return ListTile(
                  leading: Icon(
                    e.isOpen ? Icons.door_sliding : Icons.door_front_door,
                    color: e.isOpen ? Colors.orange : Colors.green,
                  ),
                  title: Text(
                    e.isOpen ? 'Opened' : 'Closed',
                    style: const TextStyle(fontWeight: FontWeight.bold),
                  ),
                  subtitle: Text(_formatTime(e.time)),
                  trailing: Text(
                    // fecha corta
                    '${e.time.day}/${e.time.month}',
                    style: const TextStyle(fontSize: 12, color: Colors.grey),
                  ),
                );
              },
            ),
          ),
          actions: [
            TextButton(
              onPressed: () {
                Navigator.of(context).pop();
              },
              child: const Text('OK'),
            ),
          ],
        );
      },
    );
  }

  // Tarjeta que muestra el estado de la puerta (color y texto según estado)
  Widget _doorStatus() {
    bool closedDoor = false;

    return ValueListenableBuilder<bool>(
      valueListenable: isDoorDataListUpdating,
      builder: (context, value, child) {
        if (doorDataList.isEmpty) {
          closedDoor = false;
        } else {
          closedDoor = doorDataList[0].isOpen ? false : true;
        }
        return Card(
          color: closedDoor
              ? const Color.fromARGB(255, 52, 104, 53)
              : const Color.fromARGB(255, 139, 58, 58),
          child: ListTile(
            leading: Icon(
              closedDoor ? Icons.door_front_door : Icons.door_sliding,
              color: Colors.white,
            ),
            title: Text(
              'Door Status',
              style: const TextStyle(color: Colors.white),
            ),
            subtitle: Text(
              closedDoor ? 'Closed' : 'Opened',
              style: const TextStyle(color: Colors.white),
            ),
            onTap: () {
              // Al tocar mostramos el historial en un diálogo
              showDialog(
                context: context,
                builder: (BuildContext context) {
                  return _doorHistory();
                },
              );
            },
          ),
        );
      },
    );
  }

  Widget _motorpumpHistory() {
    return ValueListenableBuilder(
      valueListenable: isMotorpumpDataListUpdating,
      builder: (context, value, child) {
        return AlertDialog(
          title: const Text(
            'Motorpump History',
            style: TextStyle(fontWeight: FontWeight.bold),
          ),
          content: SizedBox(
            width: 350,
            height: 300,
            child: ListView.separated(
              itemCount: motorpumpDataList.length,
              separatorBuilder: (_, __) => const Divider(height: 1),
              itemBuilder: (context, index) {
                final e = motorpumpDataList[index];
                return ListTile(
                  leading: Icon(
                    e.isOn ? Icons.play_arrow : Icons.stop,
                    color: e.isOn ? Colors.green : Colors.orange,
                  ),
                  title: Text(
                    e.isOn ? 'Active' : 'Stopped',
                    style: const TextStyle(fontWeight: FontWeight.bold),
                  ),
                  subtitle: Text(_formatTime(e.time)),
                  trailing: Text(
                    // fecha corta
                    '${e.time.day}/${e.time.month}',
                    style: const TextStyle(fontSize: 12, color: Colors.grey),
                  ),
                );
              },
            ),
          ),
          actions: [
            TextButton(
              onPressed: () {
                Navigator.of(context).pop();
              },
              child: const Text('OK'),
            ),
          ],
        );
      },
    );
  }

  // Tarjeta que muestra el estado de la puerta (color y texto según estado)
  Widget _motorpumpStatus() {
    bool activeMotorpump = false;
    return ValueListenableBuilder(
      valueListenable: isMotorpumpDataListUpdating,
      builder: (context, value, child) {
        if (motorpumpDataList.isEmpty) {
          activeMotorpump = false;
        } else {
          activeMotorpump = motorpumpDataList[0].isOn;
        }
        return Card(
          color: activeMotorpump
              ? const Color.fromARGB(255, 52, 104, 53)
              : const Color.fromARGB(255, 139, 58, 58),
          child: ListTile(
            leading: Icon(
              activeMotorpump ? Icons.play_arrow : Icons.stop,
              color: Colors.white,
            ),
            title: Text(
              'Motorpump Status',
              style: const TextStyle(color: Colors.white),
            ),
            subtitle: Text(
              activeMotorpump ? 'Active' : 'Stopped',
              style: const TextStyle(color: Colors.white),
            ),
            onTap: () {
              // Al tocar mostramos el historial en un diálogo
              showDialog(
                context: context,
                builder: (BuildContext context) {
                  return _motorpumpHistory();
                },
              );
            },
          ),
        );
      },
    );
  }

  Widget _weightHistory() {
    return ValueListenableBuilder(
      valueListenable: isWeightDataListUpdating,
      builder: (context, value, child) {
        return AlertDialog(
          title: const Text(
            'Weight History',
            style: TextStyle(fontWeight: FontWeight.bold),
          ),
          content: SizedBox(
            width: 350,
            height: 300,
            child: ListView.separated(
              itemCount: weightDataList.length,
              separatorBuilder: (_, __) => const Divider(height: 1),
              itemBuilder: (context, index) {
                final e = weightDataList[index];
                return ListTile(
                  leading: Icon(
                    e.weight > 1 ? Icons.check : Icons.close,
                    color: e.weight > 500 ? Colors.green : Colors.orange,
                  ),
                  title: Text(
                    '${e.weight} g',
                    style: const TextStyle(fontWeight: FontWeight.bold),
                  ),
                  subtitle: Text(_formatTime(e.time)),
                  trailing: Text(
                    // fecha corta
                    '${e.time.day}/${e.time.month}',
                    style: const TextStyle(fontSize: 12, color: Colors.grey),
                  ),
                );
              },
            ),
          ),
          actions: [
            TextButton(
              onPressed: () {
                Navigator.of(context).pop();
              },
              child: const Text('OK'),
            ),
          ],
        );
      },
    );
  }

  // Tarjeta que muestra el estado de la puerta (color y texto según estado)
  Widget _weightFood() {
    double weightFood = 0.0;
    return ValueListenableBuilder(
      valueListenable: isWeightDataListUpdating,
      builder: (context, value, child) {
        if (weightDataList.isEmpty) {
          weightFood = 0.0;
        } else {
          weightFood = weightDataList[0].weight;
        }
        return Card(
          color: weightFood > 500
              ? const Color.fromARGB(255, 52, 104, 53)
              : const Color.fromARGB(255, 139, 58, 58),
          child: ListTile(
            leading: Icon(
              weightFood > 1 ? Icons.check : Icons.close,
              color: Colors.white,
            ),
            title: Text(
              'Weight food',
              style: const TextStyle(color: Colors.white),
            ),
            subtitle: Text(
              '$weightFood g',
              style: const TextStyle(color: Colors.white),
            ),
            onTap: () {
              // Al tocar mostramos el historial en un diálogo
              showDialog(
                context: context,
                builder: (BuildContext context) {
                  return _weightHistory();
                },
              );
            },
          ),
        );
      },
    );
  }

  List<_ChartPoint> _buildSampleData(VariablesType variable) {
    switch (variable) {
      case VariablesType.temperature:
        List<_ChartPoint> data = variablesDataList
            .map((e) => _ChartPoint(e.time, e.temperature))
            .toList();
        return data;
      case VariablesType.humidity:
        List<_ChartPoint> data = variablesDataList
            .map((e) => _ChartPoint(e.time, e.humidity))
            .toList();
        return data;
      case VariablesType.pressure:
        List<_ChartPoint> data = variablesDataList
            .map((e) => _ChartPoint(e.time, e.pressure))
            .toList();
        return data;
      case VariablesType.co2:
        List<_ChartPoint> data = variablesDataList
            .map((e) => _ChartPoint(e.time, e.carbondioxide))
            .toList();
        return data;
      case VariablesType.alcohol:
        List<_ChartPoint> data = variablesDataList
            .map((e) => _ChartPoint(e.time, e.alcohol))
            .toList();
        return data;
      case VariablesType.nitrogen:
        List<_ChartPoint> data = variablesDataList
            .map((e) => _ChartPoint(e.time, e.nitrogen))
            .toList();
        return data;
    }
  }

  List<_ChartPoint> _buildThresholdMaxLines(VariablesType variable) {
    switch (variable) {
      case VariablesType.temperature:
        return variablesDataList
            .map(
              (e) =>
                  _ChartPoint(e.time, variablesAlarmThresholds.temperature.max),
            )
            .toList();
      case VariablesType.humidity:
        return variablesDataList
            .map(
              (e) => _ChartPoint(e.time, variablesAlarmThresholds.humidity.max),
            )
            .toList();
      case VariablesType.pressure:
        return variablesDataList
            .map(
              (e) => _ChartPoint(e.time, variablesAlarmThresholds.pressure.max),
            )
            .toList();
      case VariablesType.co2:
        return variablesDataList
            .map((e) => _ChartPoint(e.time, variablesAlarmThresholds.co2.max))
            .toList();
      case VariablesType.alcohol:
        return variablesDataList
            .map(
              (e) => _ChartPoint(e.time, variablesAlarmThresholds.alcohol.max),
            )
            .toList();
      case VariablesType.nitrogen:
        return variablesDataList
            .map(
              (e) => _ChartPoint(e.time, variablesAlarmThresholds.nitrogen.max),
            )
            .toList();
    }
  }

  List<_ChartPoint> _buildThresholdMinLines(VariablesType variable) {
    switch (variable) {
      case VariablesType.temperature:
        return variablesDataList
            .map(
              (e) =>
                  _ChartPoint(e.time, variablesAlarmThresholds.temperature.min),
            )
            .toList();
      case VariablesType.humidity:
        return variablesDataList
            .map(
              (e) => _ChartPoint(e.time, variablesAlarmThresholds.humidity.min),
            )
            .toList();
      case VariablesType.pressure:
        return variablesDataList
            .map(
              (e) => _ChartPoint(e.time, variablesAlarmThresholds.pressure.min),
            )
            .toList();
      case VariablesType.co2:
        return variablesDataList
            .map((e) => _ChartPoint(e.time, variablesAlarmThresholds.co2.min))
            .toList();
      case VariablesType.alcohol:
        return variablesDataList
            .map(
              (e) => _ChartPoint(e.time, variablesAlarmThresholds.alcohol.min),
            )
            .toList();
      case VariablesType.nitrogen:
        return variablesDataList
            .map(
              (e) => _ChartPoint(e.time, variablesAlarmThresholds.nitrogen.min),
            )
            .toList();
    }
  }

  // Construye una tarjeta con un gráfico de línea.
  // El gráfico se coloca dentro de un SingleChildScrollView horizontal
  // para permitir desplazamiento si la anchura total excede el espacio.
  Widget _graph(VariablesType variable, IconData? icon) {
    return ValueListenableBuilder(
      valueListenable: isVariablesDataListUpdating,
      builder: (context, value, child) {
        String title = '';
        var data = _buildSampleData(variable);
        var thresholdMax = _buildThresholdMaxLines(variable);
        var thresholdMin = _buildThresholdMinLines(variable);
        if (data.isEmpty) {
          data = [_ChartPoint(DateTime.now(), 0)];
        }
        RangeValues alarmThresholds = getAlarmThresholds(variable);
        double currentValue = data[0].y;
        switch (variable) {
          case VariablesType.temperature:
            title = "Temperature: ${data[0].y.toStringAsFixed(1)} °C";
            break;
          case VariablesType.humidity:
            title = "Humidity: ${data[0].y.toStringAsFixed(1)} %";
            break;
          case VariablesType.pressure:
            title = "Pressure: ${data[0].y.toStringAsFixed(1)} kPa";
            break;
          case VariablesType.co2:
            title = "CO₂ (Carbon Dioxide): ${data[0].y.toStringAsFixed(1)} ppm";
            break;
          case VariablesType.alcohol:
            title = "OH (Alcohol): ${data[0].y.toStringAsFixed(1)} ppm";
            break;
          case VariablesType.nitrogen:
            title = "Nitrogen: ${data[0].y.toStringAsFixed(1)} ppm";
            break;
        }

        final chartWidth = (data.length * 28).clamp(600, 1400).toDouble();

        return Card(
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(12.0),
            side: BorderSide(
              color:
                  (currentValue < alarmThresholds.start ||
                      currentValue > alarmThresholds.end)
                  ? const Color.fromARGB(255, 153, 26, 17)
                  : Colors.transparent,
              width: 3.0,
            ),
          ),
          child: ListTile(
            title: Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Row(
                  children: [
                    Icon(
                      icon,
                      color:
                          (currentValue < alarmThresholds.start ||
                              currentValue > alarmThresholds.end)
                          ? const Color.fromARGB(255, 153, 26, 17)
                          : Colors.black,
                    ),
                    const SizedBox(width: 8),
                    Text(
                      title,
                      style: const TextStyle(
                        color: Colors.black,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                  ],
                ),
                Icon(
                  (currentValue < alarmThresholds.start ||
                          currentValue > alarmThresholds.end)
                      ? Icons.warning_amber_rounded
                      : null,
                  color: const Color.fromARGB(255, 153, 26, 17),
                ),
              ],
            ),
            subtitle: SizedBox(
              height: 200,
              child: SingleChildScrollView(
                scrollDirection: Axis.horizontal,
                child: SizedBox(
                  width: chartWidth,
                  child: SfCartesianChart(
                    // Habilita interacción (pan/zoom)
                    zoomPanBehavior: ZoomPanBehavior(
                      enablePanning: true,
                      enablePinching: true,
                      zoomMode: ZoomMode.x,
                    ),
                    primaryXAxis: DateTimeAxis(
                      edgeLabelPlacement: EdgeLabelPlacement.shift,
                      intervalType: DateTimeIntervalType.minutes,
                      majorGridLines: const MajorGridLines(width: 0.2),
                      dateFormat: DateFormat('MM/dd HH:mm'),
                    ),
                    primaryYAxis: NumericAxis(
                      numberFormat: null,
                      axisLine: const AxisLine(width: 0),
                      majorGridLines: const MajorGridLines(
                        dashArray: <double>[5, 5],
                      ),
                    ),
                    series: <LineSeries<_ChartPoint, DateTime>>[
                      LineSeries<_ChartPoint, DateTime>(
                        dataSource: thresholdMax,
                        xValueMapper: (_ChartPoint p, _) => p.x,
                        yValueMapper: (_ChartPoint p, _) => p.y,
                        color: Colors.grey[500],
                        markerSettings: const MarkerSettings(isVisible: false),
                        width: 1,
                      ),
                      LineSeries<_ChartPoint, DateTime>(
                        dataSource: thresholdMin,
                        xValueMapper: (_ChartPoint p, _) => p.x,
                        yValueMapper: (_ChartPoint p, _) => p.y,
                        color: Colors.grey[500],
                        markerSettings: const MarkerSettings(isVisible: false),
                        width: 1,
                      ),
                      LineSeries<_ChartPoint, DateTime>(
                        // dataSource es la lista de puntos
                        dataSource: data,
                        xValueMapper: (_ChartPoint p, _) => p.x,
                        yValueMapper: (_ChartPoint p, _) => p.y,
                        color: Theme.of(context).colorScheme.primary,
                        markerSettings: const MarkerSettings(isVisible: false),
                        width: 2,
                      ),
                    ],
                    // Tooltip personalizado: muestra nombre de la serie, hora y valor
                    tooltipBehavior: TooltipBehavior(
                      enable: true,
                      builder:
                          (
                            dynamic data,
                            dynamic point,
                            dynamic series,
                            int pointIndex,
                            int seriesIndex,
                          ) {
                            final _ChartPoint p = data as _ChartPoint;
                            final seriesName =
                                title; // usamos el título como nombre
                            final dt = p.x;
                            final MM = dt.month.toString().padLeft(2, '0');
                            final dd = dt.day.toString().padLeft(2, '0');
                            final hh = dt.hour.toString().padLeft(2, '0');
                            final mm = dt.minute.toString().padLeft(2, '0');
                            return Container(
                              padding: const EdgeInsets.all(8),
                              decoration: BoxDecoration(
                                color: Colors.white,
                                borderRadius: BorderRadius.circular(6),
                                boxShadow: const [
                                  BoxShadow(
                                    color: Colors.black26,
                                    blurRadius: 4,
                                    offset: Offset(0, 2),
                                  ),
                                ],
                              ),
                              child: Text(
                                '$seriesName\n$MM/$dd $hh:$mm — ${p.y.toStringAsFixed(2)}',
                              ),
                            );
                          },
                    ),
                  ),
                ),
              ),
            ),
          ),
        );
      },
    );
  }

  void _selectDateRange(BuildContext context) async {
    final DateTimeRange? picked = await showDateRangePicker(
      context: context,
      firstDate: firstDataTime,
      lastDate: DateTime.now(),
      initialDateRange: DateTimeRange(
        start: DateTime.now(),
        end: DateTime.now(),
      ),
    );
    if (picked != null) {
      isStatisticsLoading.value = true;
      // Aquí puedes manejar el rango de fechas seleccionado
      debugPrint('Selected date range: ${picked.start} - ${picked.end}');
      DateTime startDate = picked.start;
      DateTime endDate = picked.end;
      bool resumeStreams = false;
      if (picked.end.isSameDate(DateTime.now()) &&
          picked.start.isSameDate(DateTime.now())) {
        // Si la fecha final es hoy, ajustamos la hora al momento actual
        endDate = firstDataTime;
        startDate = DateTime.now();
        resumeStreams = true;
      } else {
        await cloudManager.pauseStreamSubscriptions();
      }
      await Future.wait([
        cloudManager.getVariablesData(startDate, endDate),
        cloudManager.getMotorpumpData(startDate, endDate),
        cloudManager.getWeightData(startDate, endDate),
        cloudManager.getDoorData(startDate, endDate),
      ]);
      if (resumeStreams) {
        await cloudManager.resumeStreamSubscriptions();
      }
      isStatisticsLoading.value = false;
    }
  }

  // Encabezado de la sección de gráficas
  Widget _graphSectionGraph() {
    return Stack(
      alignment: Alignment.center, // Centra los hijos por defecto
      children: [
        // El texto estará en el centro absoluto
        Positioned(
          child: Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              const Text(
                "Stadistics",
                style: TextStyle(fontWeight: FontWeight.bold, fontSize: 16),
              ),
              const SizedBox(width: 10),
              Icon(
                Icons.area_chart,
                color: Theme.of(context).colorScheme.primary,
              ),
            ],
          ),
        ),
        // El botón lo posicionamos manualmente a la derecha
        Positioned(
          right: 5,
          child: IconButton(
            icon: const Icon(Icons.calendar_today),
            onPressed: () {
              _selectDateRange(context);
            },
            color: Theme.of(context).colorScheme.primary,
          ),
        ),
      ],
    );
  }

  // Contenido principal: estado puerta + múltiples gráficas
  Widget _mainWidget() {
    return Center(
      child: ListView(
        children: [
          const SizedBox(height: 10),
          Row(
            children: [
              Expanded(child: _doorStatus()),
              Expanded(child: _weightFood()),
            ],
          ),
          _motorpumpStatus(),
          const SizedBox(height: 10),
          _graphSectionGraph(),
          const SizedBox(height: 10),
          _graph(VariablesType.temperature, Icons.thermostat_rounded),
          _graph(VariablesType.humidity, Icons.water_drop),
          _graph(VariablesType.pressure, Icons.speed),
          // _graph(VariablesType.co2, Icons.cloud),
          _graph(VariablesType.alcohol, Icons.cloud),
          // _graph(VariablesType.nitrogen, Icons.cloud),
        ],
      ),
    );
  }
}
