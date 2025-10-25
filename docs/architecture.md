Project architecture — Simple Plugin Framework

Overview

This small C++ plugin framework contains three main areas:

- core: shared interfaces and `PluginManager` that handles dynamic plugin loading and a central registry.
- app: the host executable (`src/app/main.cpp`) that uses `PluginManager` to load plugins and create instances.
- plugins: one or more plugin libraries (example: `plugins/simple`) that implement interfaces and register themselves with the manager.

Directory layout (relevant files)

- `CMakeLists.txt`            — top-level build file
- `include/core/IObject.hpp` — base interface for plugin objects (`core::IObject`)
- `include/core/ISimple.hpp` — example interface (`core::ISimple`)
- `include/core/PluginManager.hpp` — plugin registry and dynamic loader
- `src/app/main.cpp`         — sample host app (loads plugin, creates instance)
- `plugins/simple/SimplePlugin.cpp` — sample plugin, exports `registerPlugin(PluginManager&)`
- `plugins/simple/CMakeLists.txt`   — plugin build rules

ASCII component diagram

plugin_framework/
├─ include/core/
│  ├─ IObject.hpp     (core::IObject)
│  ├─ ISimple.hpp     (core::ISimple)
│  └─ PluginManager.hpp (core::PluginManager)
├─ src/app/
│  └─ main.cpp        (host app)
└─ plugins/simple/
   ├─ SimplePlugin.cpp (implements ISimple, exports registerPlugin)
   └─ CMakeLists.txt

Key runtime flow (high level)

1. Host (main) calls PluginManager::loadPlugin(path).
   - On Windows this is LoadLibrary/GetProcAddress, on Unix dlopen/dlsym.
2. PluginManager looks up the exported symbol `registerPlugin`.
3. Plugin library's `registerPlugin(PluginManager&)` calls `PluginManager::registerClass("clsidSimple", factory)`.
   - The factory is a callable returning `std::shared_ptr<IObject>` (or to the specific interface type).
4. Host calls `PluginManager::create("clsidSimple")` to obtain an object instance.
5. Host dynamic_casts/shared_ptr->ISimple and calls `add(a,b)`.
6. On shutdown the host calls `PluginManager::unloadAll()` which dlclose/FreeLibrary the loaded libraries.

Important symbols / signatures

- core::IObject
  - virtual ~IObject()

- core::ISimple : public core::IObject
  - static const char* IID(); // via DEFINE_IID
  - virtual int add(int a, int b) = 0;

- core::PluginManager
  - static PluginManager& get() or instance()
  - void registerClass(const std::string& clsid, CreateFunc func)
  - std::shared_ptr<IObject> create(const std::string& clsid)
  - bool loadPlugin(const std::string& path)
  - void unloadAll()

- plugin export
  - extern "C" void registerPlugin(PluginManager& m);

Build and outputs

- CMake builds the host and the plugins. The repository is configured so both app and plugin outputs land in `build/bin/<CONFIG>`.
- Example outputs after building Debug configuration:
  - `build/bin/Debug/app.exe`
  - `build/bin/Debug/simple.dll` (or libsimple.so on Unix)

Render the PlantUML diagram

I added `docs/architecture.puml` with the PlantUML source. I also generated an SVG diagram and added it as `docs/architecture.svg` so you can view it directly in the repo.

View the diagram:

![Architecture](architecture.svg)

If you still want to render PlantUML locally, here are optional commands (PowerShell examples):


# Using plantuml.jar (requires Java)

```powershell
# java -jar plantuml.jar docs/architecture.puml
```


# Or with docker-based plantuml

```powershell
# docker run --rm -v ${PWD}:/workspace plantuml/plantuml -tpng docs/architecture.puml
```

Notes and next steps

- I can render the PlantUML to an image and add it to the repo if you want (I can create PNG/SVG). Tell me if you prefer PNG or SVG.
- I can also embed the diagram directly into `README.md` and/or update `docs/README.md` with build/run instructions.
- If you want, I can expand the architecture doc to include sequence diagrams for edge cases (failed load, registration error, lifetime rules) or show a class diagram with method signatures.


Generated files

- `docs/architecture.puml` — PlantUML source
- `docs/architecture.md`  — this explanation + ASCII diagram

If you'd like the rendered image added to the repo, say which format you prefer (PNG or SVG) and I'll generate and commit it.
