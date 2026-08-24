import 'package:firebase_auth/firebase_auth.dart';

import 'package:flutter/foundation.dart'; // Importante

class AuthService {
  // Obtenemos la instancia única de FirebaseAuth
  final FirebaseAuth _auth = FirebaseAuth.instance;

  // Stream: Escuchar cambios en el estado de autenticación
  // Esto nos dirá en tiempo real si el usuario entró o salió
  Stream<User?> get userStatus => _auth.authStateChanges();

  // Registro con correo y contraseña
  Future<UserCredential?> registerWithEmail(
    String email,
    String password,
  ) async {
    try {
      return await _auth.createUserWithEmailAndPassword(
        email: email,
        password: password,
      );
    } on FirebaseAuthException catch (e) {
      // Aquí puedes manejar errores específicos (ej. contraseña débil)
      debugPrint("Error en registro: ${e.code}");
      rethrow;
    }
  }

  // Inicio de sesión
  Future<UserCredential?> loginWithEmail(String email, String password) async {
    try {
      return await _auth.signInWithEmailAndPassword(
        email: email,
        password: password,
      );
    } on FirebaseAuthException catch (e) {
      debugPrint("Error en login: ${e.code}");
      rethrow;
    }
  }

  // Cerrar sesión
  Future<void> signOut() async {
    await _auth.signOut();
  }

  // Obtener el usuario actual (si existe)
  User? get currentUser => _auth.currentUser;
}
