import 'package:flutter/material.dart';

Widget loadingOverlay() {
  return Container(
    color: Colors.black.withValues(alpha: 0.3),
    child: const Center(child: CircularProgressIndicator()),
  );
}
