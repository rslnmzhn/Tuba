import 'dart:io';

import 'package:flutter/material.dart';

class LocalIpCard extends StatelessWidget {
  const LocalIpCard({
    super.key,
    required this.addresses,
    required this.onRefresh,
    required this.onSelected,
  });

  final List<InternetAddress> addresses;
  final VoidCallback onRefresh;
  final ValueChanged<String> onSelected;

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Expanded(
                  child: Text(
                    'IP этого устройства',
                    style: Theme.of(context).textTheme.titleSmall,
                  ),
                ),
                IconButton(
                  tooltip: 'Обновить IP',
                  onPressed: onRefresh,
                  icon: const Icon(Icons.refresh),
                ),
              ],
            ),
            const SizedBox(height: 4),
            if (addresses.isEmpty)
              const Text('IPv4 адреса не найдены')
            else
              Wrap(
                spacing: 8,
                runSpacing: 8,
                children: [
                  for (final address in addresses)
                    InputChip(
                      avatar: const Icon(Icons.content_copy),
                      label: Text(address.address),
                      onPressed: () => onSelected(address.address),
                    ),
                ],
              ),
          ],
        ),
      ),
    );
  }
}
