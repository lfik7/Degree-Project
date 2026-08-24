import 'package:firebase_core/firebase_core.dart';
import 'firebase_options.dart';
import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:proyecto_receptaculo/ui/alarms.dart';
import 'package:proyecto_receptaculo/ui/controls.dart';
import 'package:proyecto_receptaculo/ui/stadistics.dart';
import 'package:proyecto_receptaculo/ui/login.dart';
import 'package:proyecto_receptaculo/control/cloud_manager.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:proyecto_receptaculo/globals/variables.dart';

// Título y color primario de la aplicación. Se usan en el tema global.
String appTitle = 'Silo minder';
Color primaryColor = Colors.blue;
CloudManager cloudManager = CloudManager();

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await Firebase.initializeApp(options: DefaultFirebaseOptions.currentPlatform);
  // Forzamos el login antes de cargar la interfaz
  // await FirebaseAuth.instance.signOut();

  userLogged.value = FirebaseAuth.instance.currentUser != null ? true : false;

  runApp(MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  // build devuelve el widget MaterialApp que provee tema y navegación.
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: appTitle,
      theme: ThemeData(
        // ColorScheme generado a partir de un color semilla
        colorScheme: ColorScheme.fromSeed(seedColor: primaryColor),
      ),
      home: AuthWrapper(),
    );
  }
}

class AuthWrapper extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    // Aquí consultas si hay un usuario activo (ej. en Firebase o SharedPreferences)
    return ValueListenableBuilder(
      valueListenable: userLogged,
      builder: (context, value, child) {
        // bool isLoggedIn = checkUserSession();

        if (value) {
          Future.wait([cloudManager.initializingCloudManager()]);
          return MyHomePage(title: appTitle);
        } else {
          return LoginScreen();
        }
      },
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key, required this.title});

  final String title;

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  // Índice de la página seleccionada en la barra de navegación
  int _selectedIndex = 0;

  static const List<Widget> _pages = <Widget>[
    // LoginScreen(),
    StadisticsPage(),
    AlarmsPage(),
    ControlPage(),
  ];

  // Cambia la pestaña seleccionada y provoca una reconstrucción.
  void _onItemTapped(int index) {
    setState(() {
      _selectedIndex = index;
    });
  }

  @override
  Widget build(BuildContext context) {
    final isLandscape =
        MediaQuery.of(context).orientation == Orientation.landscape;

    if (isLandscape) {
      return Scaffold(
        body: Row(
          children: [
            NavigationRail(
              // groupAlignment controla cómo se distribuyen los destinos
              groupAlignment: 0.0,
              selectedIndex: _selectedIndex,
              onDestinationSelected: (index) => setState(() {
                _selectedIndex = index;
              }),
              // Mostrar solo la etiqueta de la pestaña seleccionada
              labelType: NavigationRailLabelType.selected,
              destinations: const [
                NavigationRailDestination(
                  padding: EdgeInsets.symmetric(vertical: 20),
                  icon: Icon(Icons.bar_chart_rounded),
                  label: Text('Stadistics'),
                ),
                NavigationRailDestination(
                  padding: EdgeInsets.symmetric(vertical: 20),
                  icon: Icon(Icons.alarm_rounded),
                  label: Text('Alarms'),
                ),
                NavigationRailDestination(
                  padding: EdgeInsets.symmetric(vertical: 20),
                  icon: Icon(CupertinoIcons.control),
                  label: Text('Controls'),
                ),
              ],
            ),
            const VerticalDivider(thickness: 1, width: 1),
            // Área principal: página seleccionada
            Expanded(child: _pages[_selectedIndex]),
          ],
        ),
      );
    } else {
      return Scaffold(
        body: _pages[_selectedIndex],
        bottomNavigationBar: NavigationBarTheme(
          data: NavigationBarThemeData(
            indicatorColor: Theme.of(context).colorScheme.primary.withAlpha(31),
            labelTextStyle: WidgetStateProperty.resolveWith<TextStyle?>(
              (states) => const TextStyle(color: Colors.black),
            ),
            iconTheme: WidgetStateProperty.resolveWith<IconThemeData?>(
              (states) => IconThemeData(
                color: states.contains(WidgetState.selected)
                    ? Colors.black
                    : Colors.black54,
              ),
            ),
          ),
          child: NavigationBar(
            height: 64,
            selectedIndex: _selectedIndex,
            onDestinationSelected: _onItemTapped,
            // Solo mostrar etiqueta de la opción seleccionada
            labelBehavior: NavigationDestinationLabelBehavior.onlyShowSelected,
            destinations: const [
              NavigationDestination(
                icon: Icon(Icons.bar_chart_rounded),
                label: 'Stadistics',
              ),
              NavigationDestination(
                icon: Icon(Icons.alarm_rounded),
                label: 'Alarms',
              ),
              NavigationDestination(
                icon: Icon(CupertinoIcons.control),
                label: 'Controls',
              ),
            ],
          ),
        ),
      );
    }
  }
}
