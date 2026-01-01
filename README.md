# AmethystXRenderEngineModule
 Render Module
## Building

This repository produces a shared module named `AmethystXRenderEngineModule`.

On Linux the module is produced as `AmethystXRenderEngineModule.so` (the CMake option
`REMOVE_LIB_PREFIX_ON_UNIX` controls whether the `lib` prefix is removed).
On Windows the module is produced as `AmethystXRenderEngineModule.dll`.

Recommended Linux build (from project root):
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel "$(nproc)"
```

There is a convenience script for Unix/Linux:
```bash
./scripts/build.sh
```

Cross-building a Windows DLL from Linux (requires MinGW-w64):
```bash
./scripts/build-windows-mingw.sh
```

If you build on Windows using Visual Studio or MSVC, configure and build
via the usual CMake workflow (select the MSVC generator in CMake or use
the Visual Studio IDE).
