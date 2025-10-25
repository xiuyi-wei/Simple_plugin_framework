This folder contains a sample plugin (SimplePlugin).

Build the top-level project with CMake. The sample plugin is built as a shared library named SimplePlugin.

The host application can look for a factory function `create_plugin` to instantiate the plugin.
