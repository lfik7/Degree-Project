import 'package:flutter/material.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:proyecto_receptaculo/globals/variables.dart';

class LoginScreen extends StatefulWidget {
  const LoginScreen({super.key});
  @override
  _LoginScreenState createState() => _LoginScreenState();
}

class _LoginScreenState extends State<LoginScreen> {
  // Clave global para validar el formulario
  final _formKey = GlobalKey<FormState>();

  // Controladores para capturar el texto
  final _emailController = TextEditingController();
  final _passwordController = TextEditingController();

  bool _isLoadingLogin = false;
  bool _isLoadingCreating = false;
  bool _obscurePassword = true;

  void _submit() async {
    if (_formKey.currentState!.validate()) {
      setState(() => _isLoadingLogin = true);

      try {
        // Here implement the login with Firebase
        await FirebaseAuth.instance.signInWithEmailAndPassword(
          email: _emailController.text,
          password: _passwordController.text,
        );
      } catch (e) {
        debugPrint("Error in login: $e");
      }

      setState(() => _isLoadingLogin = false);
      bool isLoggedIn = checkUserSession();
      if (isLoggedIn) {
        debugPrint("Login success: ${_emailController.text}");
        userLogged.value = isLoggedIn;
      } else {
        debugPrint("Login failed");
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Login error. Check your credentials.')),
        );
      }
    }
  }

  void _createAccount() async {
    if (_formKey.currentState!.validate()) {
      setState(() => _isLoadingCreating = true);

      String createAccountMsg = "";
      String userEmail = "";

      try {
        // Here implement the login with Firebase
        UserCredential credentials = await FirebaseAuth.instance
            .createUserWithEmailAndPassword(
              email: _emailController.text,
              password: _passwordController.text,
            );
        userEmail = "${credentials.user?.email}";
        createAccountMsg = "User created: ${credentials.user?.email}";
      } on FirebaseAuthException catch (e) {
        debugPrint("Error creating account: $e");
        if (e.code == 'weak-password') {
          createAccountMsg = "Weak password";
        } else if (e.code == 'email-already-in-use') {
          createAccountMsg = "Email $userEmail is already in use!";
        }
      }

      showDialog(
        context: context,
        builder: (BuildContext context) {
          return AlertDialog(
            title: Text('Account Creation'),
            content: Text(createAccountMsg),
            actions: [
              TextButton(
                onPressed: () => Navigator.of(context).pop(),
                child: Text('OK'),
              ),
            ],
          );
        },
      );

      setState(() => _isLoadingCreating = false);
      bool isLoggedIn = checkUserSession();
      if (isLoggedIn) {
        debugPrint("Creatting acccount success: ${_emailController.text}");
        userLogged.value = isLoggedIn;
      } else {
        debugPrint("Creating account failed");
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
                Image.asset(
                  'assets/images/logo_app.png',
                  height: 200, // Adjust the size as needed
                  fit: BoxFit.contain,
                ),
                SizedBox(height: 40),
                Text('Please enter your email address and password'),
                SizedBox(height: 20),
                Text('Click “LOG IN” or CREATE ACCOUNT”'),
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
                  child: FilledButton(
                    onPressed: _isLoadingLogin ? null : _submit,
                    child: _isLoadingLogin
                        ? CircularProgressIndicator(color: Colors.white)
                        : Text('LOGIN', style: TextStyle(fontSize: 18)),
                  ),
                ),
                SizedBox(height: 20),

                // LOGIN BUTTON
                SizedBox(
                  width: double.infinity,
                  height: 50,
                  child: ElevatedButton(
                    onPressed: _isLoadingCreating ? null : _createAccount,
                    child: _isLoadingCreating
                        ? CircularProgressIndicator(color: Colors.white)
                        : Text(
                            'CREATE ACCOUNT',
                            style: TextStyle(fontSize: 18),
                          ),
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
