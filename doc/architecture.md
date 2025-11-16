Architecture Overview

This document captures the current architecture of the Simple Plugin Framework, reflecting the plugin core, runtime services, workflow engine, UI helpers, the host app, and example plugins.

- core: Base interfaces (`IObject`, `ISimple`, `IComparator`) and `PluginManager` for dynamic plugin registration/loading.
- runtime: Cross-cutting services (`ILogger`, `IClock`, `IFileSystem`) with concrete implementations (`StdLogger`, `SteadyClock`, `LocalFS`), plus `EventBus` and `ThreadPool` for execution.
- workflow: DAG model and execution (`WorkflowSpec`, `ITask`, `ITaskContext`, `Executor`), JSON parser (`WorkflowParser`), and built-in `ShellTask`.
- ui: Console helpers for lightweight progress/task printing.
- app: Host executable wiring services, parsing workflow.json, and executing the workflow.
- plugins: Example `simple` (implements `ISimple`) and `json_compare` (implements `IComparator`).

Diagram

See the rendered diagram:

![Architecture](architecture.svg)

PlantUML Source

The diagram source is maintained in `doc/architecture.puml`.

Notes

- `Executor` publishes task lifecycle events via `EventBus` and schedules work on `ThreadPool`.
- `ITaskContext` provides tasks access to `ILogger`, `IClock`, and `IFileSystem`.
- `WorkflowParser` builds a `WorkflowSpec` from `workflow.json` (supports `ShellTask`).
- Plugins register factories with `PluginManager` by exporting `registerPlugin(PluginManager&)`.

