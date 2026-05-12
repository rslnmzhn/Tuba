import 'dart:ffi';
import 'dart:io';

final class NativeLib {
  NativeLib._();

  static final DynamicLibrary instance = _open();

  static DynamicLibrary _open() {
    if (Platform.isAndroid) {
      return DynamicLibrary.open('librc_native.so');
    }

    if (Platform.isLinux) {
      return DynamicLibrary.open(_linuxLibraryPath('librc_native.so'));
    }

    if (Platform.isWindows) {
      return DynamicLibrary.open(_desktopLibraryPath('rc_native.dll'));
    }

    throw UnsupportedError('Unsupported platform: ${Platform.operatingSystem}');
  }

  static String _desktopLibraryPath(String libraryName) {
    final executableDirectory = File(Platform.resolvedExecutable).parent;
    final bundledLibrary = File.fromUri(
      executableDirectory.uri.resolve('lib/$libraryName'),
    );

    if (bundledLibrary.existsSync()) {
      return bundledLibrary.path;
    }

    return File.fromUri(executableDirectory.uri.resolve(libraryName)).path;
  }

  static String _linuxLibraryPath(String libraryName) {
    final executableDirectory = File(Platform.resolvedExecutable).parent;
    final candidates = <File>[
      File.fromUri(executableDirectory.uri.resolve('lib/$libraryName')),
      File.fromUri(executableDirectory.uri.resolve(libraryName)),
      File.fromUri(Directory.current.uri.resolve('lib/$libraryName')),
      File.fromUri(Directory.current.uri.resolve(libraryName)),
    ];

    for (final candidate in candidates) {
      if (candidate.existsSync()) {
        return candidate.path;
      }
    }

    return candidates.first.path;
  }
}
