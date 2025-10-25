# Simple Plugin Framework — 设计文档

版本与作者
- 仓库: Simple_plugin_framework
- 生成时间: 2025-10-25

概述
-----------------
本文件描述了该工程中实现的轻量级 C++ 插件框架的设计思想与实现细节，目标是为宿主程序提供一种简单、安全、跨平台（Windows / POSIX）地加载共享库插件并通过工厂注册机制创建对象的能力。

设计目标
-----------------
- 简洁：最小化接口复杂度，便于开发与测试示例插件。
- 可移植：支持 Windows (LoadLibrary/GetProcAddress) 与 POSIX (dlopen/dlsym)。
- 可靠：为插件加载、注册与卸载提供明确的错误路径与日志。
- 可扩展：允许把更多接口、生命周期策略和版本管理加入到框架。

约束与假设
-----------------
- 插件以共享库形式提供（Windows: .dll + .lib，Linux/macOS: .so/.dylib）。
- 插件导出一个 C 风格的符号 `registerPlugin(PluginManager&)` 作为入口点，用于在加载时向宿主注册类/工厂。
- 框架当前以单进程、单线程调用为主；若插件在多线程环境使用，需要插件自行负责线程安全，或在未来扩展 PluginManager 的并发保护。

项目结构（与实现对应）
-----------------
- `include/core/` — 公共接口与管理器头文件：`IObject.hpp`, `ISimple.hpp`, `PluginManager.hpp`。
- `src/app/main.cpp` — 示例宿主程序，演示如何加载插件并使用接口。
- `plugins/simple/` — 示例插件源与子 CMake 文件（导出 `registerPlugin` 并实现 `ISimple`）。
- `CMakeLists.txt` — 顶层构建脚本（已配置使 app 与 plugin 输出到统一 `build/bin/<CONFIG>` 目录）。

核心抽象
-----------------
1) core::IObject
- 作用：所有插件对象的基类（最小虚基类）。
- 定义：仅包含虚析构函数，便于通过基类指针管理生命周期。

2) core::ISimple（示例接口）
- 作用：演示如何定义接口（带有唯一标识符宏 `DEFINE_IID`）。
- 特性：继承自 `core::IObject`，并声明虚方法 `int add(int a, int b)`。

3) core::PluginManager
- 单例访问：`static PluginManager& get()`（或 `instance()`）
- 主要职责：
  - 动态加载插件（`loadPlugin(path)`）
  - 在加载后通过查找导出符号执行注册（导出函数 `registerPlugin`）
  - 保存插件句柄，以便在 `unloadAll()` 时正确卸载
  - 管理注册表（map<string, CreateFunc>），通过 `registerClass(clsid, factory)` 完成类注册
  - `create(clsid)` 返回 `std::shared_ptr<IObject>`（由工厂创建）

接口契约（plugin contract）
-----------------
- 插件必须导出一个 C 风格函数：

```
extern "C" void registerPlugin(core::PluginManager& m);
```

- 插件的 `registerPlugin` 函数应调用 `m.registerClass("clsidName", factory)`，其中 `factory` 是一个可调用对象（lambda 或函数指针），返回 `std::shared_ptr<core::IObject>`（或更具体的接口，转换为基类指针保存）。
- 插件可通过 `DEFINE_IID` 提供接口标识字符串，供宿主或注册表做记号/匹配。

运行时流程（简化序列）
-----------------
1. 宿主调用 `PluginManager::loadPlugin(path)`。
2. PluginManager 打开共享库：Windows 使用 `LoadLibraryA(path)`, POSIX 使用 `dlopen(path, RTLD_NOW)`。
3. 查找符号 `registerPlugin`（Windows: `GetProcAddress`, POSIX: `dlsym`）。
4. 若找到则调用 `registerPlugin(manager)`，插件在该调用中向 `manager` 注册一个或多个类工厂。
5. 宿主调用 `manager.create(clsid)` 来从注册表中实例化对象。
6. 程序结束或需要卸载插件时，宿主调用 `manager.unloadAll()`：释放所有实例（shared_ptr 管理）并调用 `FreeLibrary`/`dlclose` 释放库。

错误处理与日志
-----------------
- loadPlugin 返回 bool（或在未来可改为更丰富的错误码/异常）指示加载是否成功，并将错误输出到 stderr 或日志。
- 在查找 `registerPlugin` 失败时，`loadPlugin` 应关闭库句柄并返回失败。
- registerClass 在默认实现将覆盖同名 clsid 的现有工厂；生产代码可改为检测重复并返回错误。

内存与对象生命周期
-----------------
- 工厂返回 `std::shared_ptr<IObject>`，便于宿主和插件之间共享所有权。
- PluginManager 只保存库句柄（void*），不直接持有插件生成的对象指针；在 `unloadAll()` 前应确保没有仍在使用的对象实例（否则 `dlclose` 后对象内代码或虚表可能无效）。
- 建议的使用模式：宿主确保在卸载库之前销毁/释放所有通过该库创建的对象（例如通过 `unloadAll()` 在宿主控制的生命周期点调用）。

跨平台注意事项
-----------------
- Windows
  - Load: `HMODULE h = LoadLibraryA(path.c_str());`
  - Resolve symbol: `FARPROC p = GetProcAddress(h, "registerPlugin");`
  - Unload: `FreeLibrary(h);`
  - 编译：在 MSVC/Visual Studio generator 下，CMake 使用多配置输出；在本项目 CMake 已配置 target output 到 `build/bin/$<CONFIG>`。

- POSIX (Linux/macOS)
  - Load: `void* h = dlopen(path.c_str(), RTLD_NOW);`
  - Resolve: `void* p = dlsym(h, "registerPlugin");`
  - Unload: `dlclose(h);`
  - 链接：在某些系统下，可能需要链接 `-ldl`（CMake 中按平台条件链接）。

线程安全与并发
-----------------
- 当前实现假定单线程注册/调用场景。若宿主在多个线程并发加载/创建插件，则 `PluginManager` 需要内部同步（mutex）来保护 registry_ 和 handles_。
- 插件本身若需要并发访问，须在插件内自行保证线程安全或暴露线程安全的接口。

版本/兼容性管理
-----------------
- 二进制 ABI 兼容性是插件系统中最复杂的议题之一。当前框架将接口设计为纯虚类并通过 `extern "C"` 入口暴露版本稳定点，减少 C++ name mangling 带来的问题。
- 更严谨的做法：为每个接口定义严格的 ABI 版本号，并在 `registerPlugin` 注册时提交兼容的接口版本；PluginManager 在注册/创建时验证版本兼容性。

安全性考量
-----------------
- 动态加载任意共享库存在风险：恶意库可能执行不安全代码。生产环境应限制插件来源、校验签名或在受限进程/容器中运行插件。
- 避免在插件提供的代码中执行未受信任的数据（输入校验、权限控制）。

测试策略
-----------------
- 单元测试：对 `PluginManager` 的注册表行为（register/create/unload）和错误路径进行单元测试（使用模拟工厂）。
- 集成测试：构建示例插件并验证加载/注册/创建/卸载流程在目标平台上工作。
- 自动化：在 CI 中分别在 Windows 和 Linux runner 上构建并运行基本集成测试。

构建系统与部署
-----------------
- 项目使用 CMake，示例 CMakeLists 已将 app 与 plugin 输出目录统一到 `build/bin/<CONFIG>`，便于运行时查找插件（可根据需要改为单一 `build/bin`）。
- 建议在部署时把宿主二进制和相应插件放在同一目录，或配置插件搜索路径（配置文件或环境变量）。

如何添加新插件（步骤）
-----------------
1. 新建子目录 `plugins/yourplugin`，编写实现文件 `YourPlugin.cpp`，实现一个或多个 `core::IObject` 派生类并在 `registerPlugin` 中调用 `m.registerClass("clsidYour", factory)`。
2. 添加 `plugins/yourplugin/CMakeLists.txt` 并在顶层 `CMakeLists.txt` 中 `add_subdirectory(plugins/yourplugin)`。
3. 构建，然后在宿主中调用 `loadPlugin("../bin/Debug/yourplugin.dll")`（或按平台/路径调整）。

示例：registerPlugin 实现
```
extern "C" void registerPlugin(core::PluginManager& m) {
    m.registerClass("clsidSimple", []() -> std::shared_ptr<core::IObject> {
        return std::make_shared<CSimple>();
    });
}
```

扩展点与未来改进
-----------------
- 并发安全：为 `PluginManager` 添加互斥保护与原子操作。
- 版本/ABI：扩展注册签名以携带接口版本，保证运行时做兼容检查；或使用 C 接口/flat C structs 以避免 C++ ABI 问题。
- 热重载：实现插件热重载策略（保存实例快照、迁移状态、或使用代理/适配器）。
- 沙箱：将插件进程隔离到子进程或容器中以提升安全性。
- 插件签名：部署时校验插件签名以确保来源可信。

附录：现有项目对照
-----------------
- `include/core/IObject.hpp` — 定义 `core::IObject`。
- `include/core/ISimple.hpp` — 示例接口 `core::ISimple`。
- `include/core/PluginManager.hpp` — 主要实现（含跨平台 dlopen/LoadLibrary 分支）。
- `src/app/main.cpp` — 加载示例插件并调用 `create("clsidSimple")`。
- `plugins/simple/SimplePlugin.cpp` — 示例插件，导出 `registerPlugin` 并注册 `clsidSimple`。

结束语
-----------------
此设计以最小可行性为目标，便于快速上手并作为进一步开发（线程安全、版本管理、热重载、安全隔离等）的基础。如果你希望我把其中某一部分（比如 ABI 版本方案或多线程保护）实现为代码样例或测试，我可以接着实现与验证。
