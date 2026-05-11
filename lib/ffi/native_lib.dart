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
      return DynamicLibrary.open(_desktopLibraryPath('librc_native.so'));
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
}
