# JObject - JavaScript 风格的 C++ 对象库

> 该项目95%由AI生成。

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/yourusername/jobject)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Language](https://img.shields.io/badge/language-C%2B%2B17-red.svg)]()

JObject 是一个现代 C++ 库，提供类似 JavaScript 的动态对象系统。它允许您在 C++ 中使用类似 JavaScript 的语法来操作对象、数组、字符串和函数，让 C++ 开发更加灵活和直观。

## ✨ 特性

- 🎯 **动态属性系统** - 支持运行时添加、删除和修改对象属性
- 📚 **多种数据类型** - 支持字符串、数组、函数、日期等多种数据类型
- 🔧 **JavaScript 风格 API** - 提供熟悉的 JavaScript 方法和语法
- 🚀 **现代 C++** - 基于 C++17 标准，使用智能指针和现代语法
- 🎪 **类型安全** - 使用 std::variant 提供类型安全的值存储
- 📦 **易于集成** - 头文件和源文件简单易用

## 🚀 快速开始

### 构建要求

- C++17 或更高版本的编译器
- CMake 3.10 或更高版本

### 编译

```bash
git clone https://github.com/yourusername/jobject.git
cd jobject
mkdir build
cd build
cmake ..
make
```

### 基本使用

```cpp
#include "JObject.h"
using namespace jobject;

int main() {
    // 创建对象
    auto obj = utils::createObject();
    
    // 设置属性
    obj->setProperty("name", utils::createString("Hello"));
    obj->setProperty("age", static_cast<int32_t>(25));
    
    // 获取属性
    auto name = obj->getProperty("name");
    auto age = obj->getProperty("age");
    
    // 使用操作符访问
    auto value = (*obj)["name"];
    
    return 0;
}
```

## 📖 详细使用指南

### JObject - 基础对象

```cpp
// 创建对象
auto obj = utils::createObject();

// 设置属性
obj->setProperty("title", utils::createString("示例"));
obj->setProperty("count", static_cast<uint32_t>(100));

// 检查属性是否存在
if (obj->hasProperty("title")) {
    std::cout << "标题存在" << std::endl;
}

// 获取所有属性名
auto propertyNames = obj->getPropertyNames();
```

### JString - 字符串操作

```cpp
// 创建字符串
auto str = utils::createString("Hello World");

// 获取长度
auto length = (*str)["length"]; // 返回 11

// 字符串方法
auto concatFunc = (*str)["concat"];
auto result = std::static_pointer_cast<JFunction>(concatFunc)->Call({
    utils::createString("!")
});

// 查找子字符串
auto indexOfFunc = (*str)["indexOf"];
auto index = std::static_pointer_cast<JFunction>(indexOfFunc)->Call({
    utils::createString("World")
});
```

### JArray - 数组操作

```cpp
// 创建数组
auto arr = utils::createArray();

// 添加元素
auto pushFunc = (*arr)["push"];
std::static_pointer_cast<JFunction>(pushFunc)->Call({
    utils::createString("第一个元素"),
    static_cast<int32_t>(42),
    utils::createString("第三个元素")
});

// 获取长度
auto length = (*arr)["length"]; // 返回 3

// 访问元素
auto firstElement = (*arr)[0];

// 数组方法
auto popFunc = (*arr)["pop"];
auto lastElement = std::static_pointer_cast<JFunction>(popFunc)->Call({});

// 切片操作
auto sliceFunc = (*arr)["slice"];
auto sliced = std::static_pointer_cast<JFunction>(sliceFunc)->Call({
    static_cast<int32_t>(0),
    static_cast<int32_t>(2)
});
```

### JFunction - 自定义函数

```cpp
// 创建自定义函数
auto myFunc = utils::createFunction("myFunction", 
    [](const std::vector<ValueVariant>& args) -> ValueVariant {
        if (!args.empty()) {
            return utils::createString("参数: " + utils::valueToString(args[0]));
        }
        return utils::createString("无参数");
    });

// 调用函数
auto result = myFunc->Call({utils::createString("测试")});
```

### JDate - 日期操作

```cpp
// 创建日期对象
auto date = utils::createDate();

// 获取时间戳
auto getTimeFunc = (*date)["getTime"];
auto timestamp = std::static_pointer_cast<JFunction>(getTimeFunc)->Call({});

// 设置时间
auto setTimeFunc = (*date)["setTime"];
std::static_pointer_cast<JFunction>(setTimeFunc)->Call({
    static_cast<uint64_t>(1640995200000) // 2022-01-01 00:00:00
});

// 转换为字符串
std::string dateStr = date->toString();
```

## 🔧 高级功能

### 属性描述符

```cpp
auto obj = utils::createObject();

// 定义只读属性
PropertyDescriptor descriptor;
descriptor.value = utils::createString("只读值");
descriptor.writable = false;
descriptor.enumerable = true;
descriptor.configurable = false;

obj->defineProperty("readOnlyProp", descriptor);
```

### 使用宏简化属性定义

```cpp
auto obj = utils::createObject();

// 定义可读写属性
DEF_PROP_EX(*obj, "customProp",
    []() -> ValueVariant { return utils::createString("getter"); },
    [](const ValueVariant& val) { /* setter 逻辑 */ },
    true, true, true);

// 定义只读属性
DEF_PROP_RO(*obj, "readOnlyProp",
    []() -> ValueVariant { return static_cast<int32_t>(42); });
```

## 🛠️ API 参考

### 工具函数

```cpp
namespace utils {
    // 类型检查
    ValueType getValueType(const ValueVariant& value);
    bool isNumber(const ValueVariant& value);
    
    // 类型转换
    std::string valueToString(const ValueVariant& value);
    double toNumber(const ValueVariant& value);
    bool toBoolean(const ValueVariant& value);
    
    // 对象创建
    std::shared_ptr<JObject> createObject();
    std::shared_ptr<JString> createString(const std::string& str);
    std::shared_ptr<JArray> createArray(size_t size = 0);
    std::shared_ptr<JFunction> createFunction(const std::string& name, 
                                              JFunction::FunctionType func);
    std::shared_ptr<JDate> createDate();
}
```

## 📁 项目结构

```
jobject/
├── src/
│   ├── JObject.h          # 头文件
│   └── JObject.cpp        # 实现文件
├── test/
│   ├── main.cpp           # 基本测试
│   └── macro_test.cpp     # 宏测试
├── CMakeLists.txt         # CMake 构建文件
└── README.md              # 项目说明
```

## 🧪 运行测试

```bash
cd build
./test/main           # 运行基本测试
./test/macro_test     # 运行宏测试
```

## 🤝 贡献

欢迎贡献代码！请遵循以下步骤：

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add some amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 开启 Pull Request

## 📝 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 🌟 致谢

- 感谢所有贡献者的努力
- 灵感来源于 JavaScript 的动态特性
- 基于现代 C++ 标准构建

## 📞 联系方式

如果您有任何问题或建议，请通过以下方式联系：

- 创建 [Issue](https://github.com/yourusername/jobject/issues)
- 发送邮件到 your.email@example.com

---

⭐ 如果这个项目对您有帮助，请给它一个星星！ 