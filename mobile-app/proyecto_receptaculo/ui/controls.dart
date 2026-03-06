import 'package:flutter/material.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:proyecto_receptaculo/globals/definitions.dart';
import 'package:proyecto_receptaculo/globals/variables.dart';
import 'package:proyecto_receptaculo/globals/own_widgets.dart';
import 'package:proyecto_receptaculo/control/cloud_manager.dart';
import 'package:proyecto_receptaculo/core/file_manager.dart';
import 'package:proyecto_receptaculo/core/wifi_manager.dart';

// Página de controles
// Archivo sencillo que muestra cómo construir una página con botones
// y layout básico. Se puede ampliar para enviar comandos al dispositivo.
class ControlPage extends StatefulWidget {
  const ControlPage({super.key});

  @override
  State<ControlPage> createState() => _ControlPageState();
}

class _ControlPageState extends State<ControlPage>
    with SingleTickerProviderStateMixin {
  List<TextEditingController> ssidControllers = [];
  List<TextEditingController> passwordControllers = [];

  final CloudManager cloudManager = CloudManager();
  final FileManager fileManager = FileManager();
  final WiFiManager wifiManager = WiFiManager();
  late DateTimeRange selectedDateRangeDownload;
  late DateTimeRange selectedDateRangeClear;
  // ValueNotifier<bool> isLoadingLocal = ValueNotifier<bool>(false);
  bool directMonitorCommunication = false;

  @override
  void initState() {
    super.initState();
    // Inicializaciones relacionadas con animaciones o controladores
    // se harían aquí. Actualmente no hay inicialización especial.

    for (var cred in wifiCredentialsList) {
      ssidControllers.add(TextEditingController(text: cred.ssid));
      passwordControllers.add(TextEditingController(text: cred.password));
    }
    selectedDateRangeDownload = DateTimeRange(
      start: DateTime.now(),
      end: DateTime.now(),
    );
    selectedDateRangeClear = DateTimeRange(
      start: firstDataTime,
      end: firstDataTime,
    );
  }

  @override
  Widget build(BuildContext context) {
    // Scaffold proporciona la estructura básica (barra, body, etc.).
    return Scaffold(
      body: Stack(
        children: [
          _mainWidget(),
          ValueListenableBuilder(
            valueListenable: isControlsLoading,
            builder: (context, value, child) {
              if (value) {
                return loadingOverlay();
              } else {
                return const SizedBox.shrink();
              }
            },
          ),
        ],
      ),
    );
  }

  @override
  void dispose() {
    // Limpieza de controladores y otros recursos.
    for (var controller in ssidControllers) {
      controller.dispose();
    }
    for (var controller in passwordControllers) {
      controller.dispose();
    }
    super.dispose();
  }

  Widget _monitorPresenceCard() {
    return ValueListenableBuilder(
      valueListenable: updateMonitorPresenceWidget,
      builder: (context, _, __) {
        debugPrint("Updating monitor presence widget.");
        return Card(
          color: null,
          child: ListTile(
            title: const Text('Monitor Presence'),
            leading: const Icon(Icons.monitor),
            subtitle: Text(
              'Last seen: ${monitorLastSeen.toLocal().toString().split('.').first}',
            ),
            trailing: Icon(
              Icons.circle,
              color: DateTime.now().difference(monitorLastSeen).inMinutes < 5
                  ? Colors.green
                  : Colors.red,
            ),
          ),
        );
      },
    );
  }

  Widget _currentSettingsExpansibleCard() {
    return ValueListenableBuilder(
      valueListenable: updateCurrentSettingsWidgets,
      builder: (context, _, __) {
        return Card(
          color: null,
          child: ExpansionTile(
            childrenPadding: const EdgeInsets.symmetric(
              horizontal: 16.0,
              vertical: 8.0,
            ),
            title: const Text('Current Settings'),
            leading: const Icon(Icons.settings),
            children: [
              ListTile(
                title: const Text('WiFi Network SSID'),
                subtitle: Text(currentSettings.wifiNetworkSSID),
              ),
              ListTile(
                title: const Text('Sampling Interval'),
                subtitle: Text(
                  '${(currentSettings.samplingInterval / 60).toInt()} minute(s)',
                ),
              ),
              ListTile(
                title: const Text('Door Status'),
                subtitle: Text(currentSettings.door ? 'Open' : 'Closed'),
              ),
              ListTile(
                title: const Text('Motorpump Status'),
                subtitle: Text(
                  currentSettings.motorpump ? 'Active' : 'Stopped',
                ),
              ),
              ListTile(
                title: const Text('Weight'),
                subtitle: Text('${currentSettings.weight} kg'),
              ),
              ListTile(
                title: const Text('Pressure Min Threshold'),
                subtitle: Text('${currentSettings.pressureMinTh} kPa'),
              ),
              ListTile(
                title: const Text('Pressure Max Threshold'),
                subtitle: Text('${currentSettings.pressureMaxTh} kPa'),
              ),
            ],
          ),
        );
      },
    );
  }

  Widget _wifiNetwroksExpansibleCard() {
    return ValueListenableBuilder(
      valueListenable: isWifiCredentialsListUpdating,
      builder: (context, _, __) {
        for (int i = 0; i < wifiCredentialsList.length; i++) {
          ssidControllers[i].text = wifiCredentialsList[i].ssid;
          passwordControllers[i].text = wifiCredentialsList[i].password;
        }
        return Card(
          color: null,
          child: ExpansionTile(
            childrenPadding: const EdgeInsets.symmetric(
              horizontal: 16.0,
              vertical: 8.0,
            ),
            title: const Text('WiFi Networks'),
            leading: const Icon(Icons.wifi),
            children: [
              for (int iter = 0; iter < wifiCredentialsList.length; iter++)
                ListTile(
                  title: Text('Network ${iter + 1}'),
                  subtitle: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Row(
                        children: [
                          const Text('SSID: '),
                          Expanded(
                            child: TextField(
                              controller: ssidControllers[iter],
                              enabled:
                                  (iter == 0 ||
                                  ssidControllers[iter - 1].text.isNotEmpty),
                              onChanged: (value) {
                                if (value.length > 31) {
                                  ssidControllers[iter].text = value.substring(
                                    0,
                                    32,
                                  );
                                  ssidControllers[iter]
                                      .selection = TextSelection.fromPosition(
                                    TextPosition(
                                      offset: ssidControllers[iter].text.length,
                                    ),
                                  );
                                }
                              },
                            ),
                          ),
                        ],
                      ),
                      Row(
                        children: [
                          const Text('PASS: '),
                          Expanded(
                            child: TextField(
                              controller: passwordControllers[iter],
                              enabled:
                                  (iter == 0 ||
                                  passwordControllers[iter - 1]
                                      .text
                                      .isNotEmpty),
                              onChanged: (value) {
                                if (value.length > 31) {
                                  passwordControllers[iter].text = value
                                      .substring(0, 32);
                                  passwordControllers[iter]
                                      .selection = TextSelection.fromPosition(
                                    TextPosition(
                                      offset:
                                          passwordControllers[iter].text.length,
                                    ),
                                  );
                                }
                              },
                            ),
                          ),
                        ],
                      ),
                      const SizedBox(height: 15),
                    ],
                  ),
                ),
              const SizedBox(height: 15),
              Row(
                children: [
                  Switch(
                    value: directMonitorCommunication,
                    onChanged: (value) {
                      setState(() {
                        directMonitorCommunication = value;
                        monitorWifiConnection();
                      });
                    },
                  ),
                  const SizedBox(width: 10),
                  const Text('Direct Monitor Communication'),
                ],
              ),
              const SizedBox(height: 5),
              Row(
                mainAxisAlignment: MainAxisAlignment.end,
                children: [
                  TextButton(
                    onPressed: () async {
                      for (
                        int iter = 0;
                        iter < wifiCredentialsList.length;
                        iter++
                      ) {
                        ssidControllers[iter].text =
                            wifiCredentialsList[iter].ssid;
                        passwordControllers[iter].text =
                            wifiCredentialsList[iter].password;
                      }
                    },
                    child: const Text('cancel'),
                  ),
                  const SizedBox(width: 20),
                  ElevatedButton(
                    onPressed: () {
                      sendWifiNets();
                    },
                    child: const Text('change'),
                  ),
                ],
              ),
            ],
          ),
        );
      },
    );
  }

  void _selectDateRangeDownload(BuildContext context) async {
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
      // Aquí puedes manejar el rango de fechas seleccionado
      debugPrint('Selected date range: ${picked.start} - ${picked.end}');
      setState(() {
        selectedDateRangeDownload = picked;
      });
    }
  }

  void _selectDateRangeClear(BuildContext context) async {
    final DateTime? picked = await showDatePicker(
      context: context,
      firstDate: firstDataTime,
      lastDate: DateTime.now(),
      initialDate: firstDataTime,
      helpText: 'Select end date to clear database up to',
    );
    if (picked != null) {
      // Aquí puedes manejar el rango de fechas seleccionado
      debugPrint('Selected date range: $picked');
      setState(() {
        selectedDateRangeClear = DateTimeRange(
          start: firstDataTime,
          end: picked,
        );
      });
    }
  }

  Widget _databaseManagerExpansibleCard() {
    return Card(
      color: null,
      child: ExpansionTile(
        childrenPadding: const EdgeInsets.symmetric(
          horizontal: 16.0,
          vertical: 8.0,
        ),
        title: const Text('Database Manager'),
        leading: const Icon(Icons.storage),
        children: [
          Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              ListTile(
                title: const Text('Download data'),
                subtitle: Row(
                  children: [
                    IconButton(
                      icon: const Icon(Icons.date_range),
                      onPressed: () {
                        _selectDateRangeDownload(context);
                      },
                    ),
                    Text(
                      // Here show the selected date range in a short format
                      'from ${selectedDateRangeDownload.start.toLocal().toString().split(' ').first} to ${selectedDateRangeDownload.end.toLocal().toString().split(' ').first}',
                    ),
                    const Spacer(),
                    IconButton.filled(
                      color: Colors.white,
                      icon: const Icon(Icons.download),
                      onPressed: () {
                        // Call AlertDialog to ask for confirmation
                        showDownloadDialog();
                      },
                      // child: const Text('Download'),
                    ),
                  ],
                ),
              ),
              const SizedBox(height: 20),
              ListTile(
                title: const Text('Clear database'),
                subtitle: Row(
                  children: [
                    IconButton(
                      icon: const Icon(Icons.date_range),
                      onPressed: () {
                        _selectDateRangeClear(context);
                      },
                    ),
                    Text(
                      // Here show the selected date range in a short format
                      'from ${selectedDateRangeClear.start.toLocal().toString().split(' ').first} to ${selectedDateRangeClear.end.toLocal().toString().split(' ').first}',
                    ),
                    const Spacer(),
                    IconButton.filled(
                      color: Colors.white,
                      icon: const Icon(Icons.delete_forever),
                      onPressed: () {
                        // Call AlertDialog to ask for confirmation
                        showClearDialog();
                      },
                      // child: const Text('Download'),
                    ),
                  ],
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  void showDownloadDialog() {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: const Text('Confirm Download'),
          content: Text(
            'Are you sure you want to download data from ${selectedDateRangeDownload.start.toLocal().toString().split(' ').first} to ${selectedDateRangeDownload.end.toLocal().toString().split(' ').first}?',
          ),
          actions: [
            TextButton(
              onPressed: () {
                Navigator.of(context).pop(); // Cierra el diálogo
              },
              child: const Text('Cancel'),
            ),
            ElevatedButton(
              onPressed: () async {
                await cloudManager.pauseStreamSubscriptions();
                debugPrint('Downloading data from database...');
                // Aquí iría la lógica para descargar los datos
                isControlsLoading.value = true;
                // Simulate some delay
                Navigator.of(context).pop(); // Cierra el diálogo
                await fileManager
                    .saveDataToExcel(
                      selectedDateRangeDownload.start,
                      selectedDateRangeDownload.end,
                    )
                    .then((_) {
                      debugPrint('Data saved to Excel file');
                      ScaffoldMessenger.of(this.context).showSnackBar(
                        SnackBar(
                          content: Text(
                            'Data from ${selectedDateRangeDownload.start.toLocal().toString().split(' ').first} to ${selectedDateRangeDownload.end.toLocal().toString().split(' ').first} downloaded and shared.',
                          ),
                          duration: const Duration(seconds: 3),
                        ),
                      );
                    });
                var endDate = firstDataTime;
                var startDate = DateTime.now();
                await Future.wait([
                  cloudManager.getVariablesData(startDate, endDate),
                  cloudManager.getMotorpumpData(startDate, endDate),
                  cloudManager.getWeightData(startDate, endDate),
                  cloudManager.getDoorData(startDate, endDate),
                ]);
                await cloudManager.resumeStreamSubscriptions();
                isControlsLoading.value = false;
              },
              child: const Text('Confirm'),
            ),
          ],
        );
      },
    );
  }

  void showClearDialog() {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: const Text('Confirm Clear'),
          content: Text(
            'Are you sure you want to clear data from ${selectedDateRangeClear.start.toLocal().toString().split(' ').first} to ${selectedDateRangeClear.end.toLocal().toString().split(' ').first}?',
          ),
          actions: [
            TextButton(
              onPressed: () {
                Navigator.of(context).pop(); // Cierra el diálogo
              },
              child: const Text('Cancel'),
            ),
            ElevatedButton(
              onPressed: () async {
                Navigator.of(context).pop(); // Cierra el diálogo
                debugPrint('Clearing data from database...');
                isControlsLoading.value = true;

                await cloudManager.clearDataBaseNodes(
                  selectedDateRangeClear.start,
                  selectedDateRangeClear.end,
                );
                ScaffoldMessenger.of(this.context).showSnackBar(
                  SnackBar(
                    content: Text(
                      'Database from ${selectedDateRangeClear.start.toLocal().toString().split(' ').first} to ${selectedDateRangeClear.end.toLocal().toString().split(' ').first} cleared successfully.',
                    ),
                    duration: const Duration(seconds: 3),
                  ),
                );
                isControlsLoading.value = false;
              },
              child: const Text('Confirm'),
            ),
          ],
        );
      },
    );
  }

  Future<void> monitorWifiConnection() async {
    isControlsLoading.value = true;
    if (directMonitorCommunication) {
      await wifiManager.connectToMonitorWifi();
    } else {
      await wifiManager.closeConnection();
    }
    isControlsLoading.value = false;
    if (connectedToMonitorNetwork) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Connected to monitor WiFi network successfully.'),
        ),
      );
    } else {
      showDialog(
        context: context,
        builder: (BuildContext context) {
          return AlertDialog(
            title: const Text('Connection Failed'),
            content: const Text(
              'Could not connect to the WiFi network of the monitor. Make sure your device has mobile data turned off and is connected to the monitor\'s WiFi network (SSID: Monitor_silo) and try again.',
            ),
            actions: [
              TextButton(
                onPressed: () {
                  Navigator.of(context).pop(); // Close the dialog
                  setState(() {
                    directMonitorCommunication = false;
                  });
                },
                child: const Text('OK'),
              ),
            ],
          );
        },
      );
    }
  }

  Future<void> sendWifiNets() async {
    isControlsLoading.value = true;
    for (int iter = 0; iter < wifiCredentialsList.length; iter++) {
      wifiCredentialsList[iter] = WiFiCredentials(
        ssid: ssidControllers[iter].text,
        password: passwordControllers[iter].text,
      );
    }
    if (directMonitorCommunication) {
      if (connectedToMonitorNetwork) {
        await wifiManager.sendWifiNets();
      } else {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text(
              "⚠️ Cannot send WiFi networks: Not connected to monitor WiFi.",
            ),
          ),
        );
      }
    } else {
      await cloudManager.changeWiFiNets();
    }
    // await cloudManager.changeWiFiNets();
    isWifiCredentialsListUpdating.value = !isWifiCredentialsListUpdating.value;
    isControlsLoading.value = false;
    if (directMonitorCommunication && !connectedToMonitorNetwork) {
      return;
    }
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text("WiFi networks changed successfully.")),
    );
  }

  Widget logoutButton() {
    return ElevatedButton.icon(
      onPressed: () async {
        // here goes an alert dialog to confirm logout
        showDialog(
          context: context,
          builder: (BuildContext context) {
            return AlertDialog(
              title: const Text('Confirm Logout'),
              content: const Text('Are you sure you want to logout?'),
              actions: [
                TextButton(
                  onPressed: () {
                    Navigator.of(context).pop(); // Cierra el diálogo
                  },
                  child: const Text('Cancel'),
                ),
                ElevatedButton(
                  onPressed: () async {
                    Navigator.of(context).pop(); // Cierra el diálogo
                    isControlsLoading.value = true;
                    await cloudManager.disposeCloudManager();
                    await FirebaseAuth.instance.signOut();
                    userLogged.value = false;
                    isControlsLoading.value = false;
                  },
                  child: const Text('Confirm'),
                ),
              ],
            );
          },
        );
      },
      icon: const Icon(Icons.logout),
      label: const Text('Logout'),
    );
  }

  // Contenido principal de la página de control.
  // Usa Column para apilar widgets verticalmente; en este ejemplo
  // hay un título y un botón de muestra.
  Widget _mainWidget() {
    return Center(
      child: ListView(
        // padding: const EdgeInsets.all(16.0),
        children: [
          _monitorPresenceCard(),
          const SizedBox(height: 10),
          _currentSettingsExpansibleCard(),
          const SizedBox(height: 10),
          _wifiNetwroksExpansibleCard(),
          const SizedBox(height: 10),
          _databaseManagerExpansibleCard(),
          // Add a logout button at the end
          const SizedBox(height: 20),
          logoutButton(),
        ],
      ),
    );
  }
}
