# Demo：JSON 插件与对比插件

本文档给出一个最小示例，展示如何运行 `json_process` 与 `json_compare` 两个插件，并查看输出结果。

## 运行环境

1. 已经通过 CMake/MSVC 构建完成主程序 `app.exe`。
2. 最新编译的 `plugins/json_process.dll` 与 `plugins/json_compare.dll` 位于 `plugins/` 目录。
3. 工程根目录包含示例输入文件 `input.json`。

## json_process 插件示例

执行 `app.exe` 会自动：

1. 加载 `json_process.dll`。
2. 读取当前目录下的 `input.json`。
3. 调用插件 `find` 功能查找 `key1/key2`，结果保存到 `output.json`。示例输出：

   ```json
   {
       "processMethod": "find",
       "keywords": "key1/key2",
       "results": [
           "VALUE-A"
       ]
   }
   ```

4. 再调用 `add` 功能，将 `key1/key2` 包装到 `key11/key12` 下方，结果保存到 `output_add.json`：

   ```json
   {
       "processMethod": "add",
       "keywords": "key11/key12|key1/key2",
       "newPath": "key11/key12/key1/key2",
       "updatedJson": {
           "key11": {
               "key12": {
                   "key1": {
                       "key2": "VALUE-A",
                       "x": 1
                   }
               }
           },
           "...": "(其余字段保持不变)"
       }
   }
   ```

## json_compare 插件示例

1. 准备两份 JSON 文件，例如：

   ```json
   // golden.json
   {"value":2,"arr":[1,3]}

   // ours.json
   {"value":1,"arr":[1,2]}
   ```

2. 调用插件（可以写一个简单的 driver，如）：  

   ```cpp
   auto comparator = core::PluginManager::instance()
       .createTyped<core::IComparator>("default_json_compare");
   comparator->compareFiles("ours.json", "golden.json", "report.html");
   ```

3. 打开生成的 `report.html`，即可看到带样式的对比报告，包含 √ / × / E 的说明及转义后的原始差异。

> 你也可以运行 `src/test.cpp` 中的 `JsonProcessTests` 与 `JsonCompareTests`，验证插件接口是否工作正常。

