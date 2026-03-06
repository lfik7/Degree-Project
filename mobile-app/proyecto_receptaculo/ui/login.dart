import 'package:flutter/material.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:proyecto_receptaculo/globals/variables.dart';
import 'package:proyecto_receptaculo/control/cloud_manager.dart';

class LoginScreen extends StatefulWidget {
  const LoginScreen({super.key});
  @override
  _LoginScreenState createState() => _LoginScreenState();
}

class _LoginScreenState extends State<LoginScreen> {
  // 1. Clave global para validar el formulario
  final _formKey = GlobalKey<FormState>();

  // 2. Controladores para capturar el texto
  final _emailController = TextEditingController();
  final _passwordController = TextEditingController();
  CloudManager cloudManager = CloudManager();

  bool _isLoading = false;
  bool _obscurePassword = true;

  void _submit() async {
    if (_formKey.currentState!.validate()) {
      setState(() => _isLoading = true);

      try {
        // Here implement the login with Firebase
        await FirebaseAuth.instance.signInWithEmailAndPassword(
          email: _emailController.text,
          password: _passwordController.text,
        );
      } catch (e) {
        debugPrint("Error in login: $e");
      }

      // Simulation of server request (Firebase/API)
      // await Future.delayed(Duration(seconds: 2));

      setState(() => _isLoading = false);
      bool isLoggedIn = checkUserSession();
      if (isLoggedIn) {
        debugPrint("Login success: ${_emailController.text}");
        // await cloudManager.initializingCloudManager();
        userLogged.value = isLoggedIn;
      } else {
        debugPrint("Login failed");
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Login error. Check your credentials.')),
        );
      }
    }
  }

  bool checkUserSession() {
    User? user = FirebaseAuth.instance.currentUser;
    return user != null;
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Center(
        child: SingleChildScrollView(
          // Avoid overflow errors with the keyboard
          padding: EdgeInsets.all(24.0),
          child: Form(
            key: _formKey,
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                // FlutterLogo(size: 100),
                Image.asset(
                  'assets/images/logo_app.png',
                  height: 200, // Adjust the size as needed
                  fit: BoxFit.contain,
                ),
                SizedBox(height: 40),

                // EMAIL FIELD
                TextFormField(
                  controller: _emailController,
                  keyboardType: TextInputType.emailAddress,
                  decoration: InputDecoration(
                    labelText: 'Email',
                    prefixIcon: Icon(Icons.email),
                    border: OutlineInputBorder(),
                  ),
                  validator: (value) {
                    if (value == null || !value.contains('@')) {
                      return 'Please enter a valid email';
                    }
                    return null;
                  },
                ),
                SizedBox(height: 20),

                // PASSWORD FIELD
                TextFormField(
                  controller: _passwordController,
                  obscureText: _obscurePassword,
                  decoration: InputDecoration(
                    labelText: 'Password',
                    prefixIcon: Icon(Icons.lock),
                    suffixIcon: IconButton(
                      icon: Icon(
                        _obscurePassword
                            ? Icons.visibility
                            : Icons.visibility_off,
                      ),
                      onPressed: () =>
                          setState(() => _obscurePassword = !_obscurePassword),
                    ),
                    border: OutlineInputBorder(),
                  ),
                  validator: (value) =>
                      (value!.length < 6) ? 'Minimum 6 characters' : null,
                ),
                SizedBox(height: 30),

                // LOGIN BUTTON
                SizedBox(
                  width: double.infinity,
                  height: 50,
                  child: ElevatedButton(
                    onPressed: _isLoading ? null : _submit,
                    child: _isLoading
                        ? CircularProgressIndicator(color: Colors.white)
                        : Text('LOGIN', style: TextStyle(fontSize: 18)),
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
