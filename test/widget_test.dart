import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tuba/discovered_devices.dart';

import 'package:tuba/screens/home_screen.dart';

void main() {
  testWidgets('Home screen renders connection form', (
    WidgetTester tester,
  ) async {
    await tester.pumpWidget(
      MaterialApp(
        home: HomeScreen(
          enableNativeServices: false,
          discoveredDevices: DiscoveredDevices.empty(),
        ),
      ),
    );

    expect(find.text('Tuba'), findsOneWidget);
    expect(find.text('IP адрес хоста'), findsOneWidget);
    expect(find.text('Подключиться'), findsOneWidget);
  });
}
