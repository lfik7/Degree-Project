import 'package:flutter/foundation.dart';
import 'package:excel/excel.dart';
import 'dart:io';
import 'package:path_provider/path_provider.dart';
import 'package:share_plus/share_plus.dart';
import 'package:proyecto_receptaculo/globals/variables.dart';
import 'package:proyecto_receptaculo/control/cloud_manager.dart';

class FileManager {
  // Singleton pattern
  static final FileManager _instance = FileManager._internal();

  factory FileManager() {
    return _instance;
  }

  FileManager._internal();

  CloudManager cloudManager = CloudManager();

  Future<void> saveDataToExcel(DateTime startDate, DateTime endDate) async {
    try {
      // Create a new Excel document
      var excel = Excel.createExcel();
      Sheet variablesSheet = excel['VariablesData'];
      Sheet doorSheet = excel['DoorData'];
      Sheet weightSheet = excel['WeightData'];
      Sheet motorpumpSheet = excel['MotorpumpData'];

      // Add headers
      List<CellValue> variablesHeaders = [
        TextCellValue('Timestamp'),
        TextCellValue('Temperature'),
        TextCellValue('Humidity'),
        TextCellValue('Pressure'),
        TextCellValue('CO2'),
        TextCellValue('Alcohol'),
        TextCellValue('Nitrogen'),
      ];
      variablesSheet.appendRow(variablesHeaders);
      List<CellValue> doorHeaders = [
        TextCellValue('Timestamp'),
        TextCellValue('Door Status'),
      ];
      doorSheet.appendRow(doorHeaders);
      List<CellValue> weightHeaders = [
        TextCellValue('Timestamp'),
        TextCellValue('Weight'),
      ];
      weightSheet.appendRow(weightHeaders);
      List<CellValue> motorpumpHeaders = [
        TextCellValue('Timestamp'),
        TextCellValue('Motorpump Status'),
      ];
      motorpumpSheet.appendRow(motorpumpHeaders);

      // Filter data within the date range and add to the sheet
      var lastDate = startDate.subtract(const Duration(days: 1));
      for (
        var firstDate = endDate;
        firstDate.isAfter(lastDate);
        firstDate = firstDate.subtract(const Duration(days: 1))
      ) {
        await cloudManager.getVariablesData(firstDate, firstDate);
        await cloudManager.getDoorData(firstDate, firstDate);
        await cloudManager.getMotorpumpData(firstDate, firstDate);
        await cloudManager.getWeightData(firstDate, firstDate);

        for (var data in variablesDataList) {
          List<CellValue> row = [
            TextCellValue(data.time.toIso8601String()),
            DoubleCellValue(data.temperature),
            DoubleCellValue(data.humidity),
            DoubleCellValue(data.pressure),
            DoubleCellValue(data.carbondioxide),
            DoubleCellValue(data.alcohol),
            DoubleCellValue(data.nitrogen),
          ];
          variablesSheet.appendRow(row);
        }

        for (var data in doorDataList) {
          List<CellValue> row = [
            TextCellValue(data.time.toIso8601String()),
            TextCellValue(data.isOpen ? 'Open' : 'Closed'),
          ];
          doorSheet.appendRow(row);
        }

        for (var data in motorpumpDataList) {
          List<CellValue> row = [
            TextCellValue(data.time.toIso8601String()),
            TextCellValue(data.isOn ? 'Active' : 'Stopped'),
          ];
          motorpumpSheet.appendRow(row);
        }

        for (var data in weightDataList) {
          List<CellValue> row = [
            TextCellValue(data.time.toIso8601String()),
            DoubleCellValue(data.weight),
          ];
          weightSheet.appendRow(row);
        }
      }

      // Delte the Sheet1 from the excel
      excel.delete('Sheet1');

      // Get the directory to save the file
      Directory directory = await getTemporaryDirectory();
      final filePath =
          '${directory.path}/Data_${startDate.toLocal().toString().split(' ').first}_to_${endDate.toLocal().toString().split(' ').first}.xlsx';

      // Save the file
      List<int>? fileBytes = excel.encode();
      if (fileBytes != null) {
        File(filePath)
          ..createSync(recursive: true)
          ..writeAsBytesSync(fileBytes);
      }

      await Share.shareXFiles([
        XFile(filePath),
      ], text: 'Here is the data from $startDate to $endDate');

      debugPrint('Menu to share the file should appear now.');
    } catch (e) {
      if (kDebugMode) {
        print('Error saving data to Excel: $e');
      }
    } finally {
      debugPrint('Excel file creation process completed.');
    }
  }
}
