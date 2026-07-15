#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace jobject {

// 前向声明
class JObject;
class JString;
class JArray;
class JFunction;
class JDate;
class jvalue;

// undefined 标签类型（零开销空结构体，区分 JS 的 undefined 与 null）
struct JUndefined {
  bool operator==(const JUndefined &) const { return true; }
};

// 全局 undefined 常量（可写 jobject::undefined）
inline constexpr JUndefined undefined{};

// 值类型枚举
enum class ValueType {
  Undefined,
  Null,
  Boolean,
  Int32,
  UInt32,
  UInt64,
  Double,
  String,
  Array,
  Object,
  Function,
  Date
};

// 值的变体类型
using ValueVariant = std::variant<JUndefined,               // Undefined
                                  std::nullptr_t,           // Null
                                  bool,                     // Boolean
                                  int32_t,                  // Int32
                                  uint32_t,                 // UInt32
                                  uint64_t,                 // UInt64
                                  double,                   // Double
                                  std::shared_ptr<JString>, // String
                                  std::shared_ptr<JArray>,  // Array

                                  std::shared_ptr<JObject>,   // Object
                                  std::shared_ptr<JFunction>, // Function
                                  std::shared_ptr<JDate>      // Date
                                  >;

// 属性描述符
struct PropertyDescriptor {
  ValueVariant value;
  bool writable = true;
  bool enumerable = true;
  bool configurable = true;
  std::function<ValueVariant()> getter = nullptr;
  std::function<void(const ValueVariant &)> setter = nullptr;
};

// 基础对象类
class JObject {
public:
  JObject();
  virtual ~JObject() = default;

  // 属性管理
  virtual bool defineProperty(const std::string &name,
                              const PropertyDescriptor &descriptor);
  virtual bool deleteProperty(const std::string &name);
  virtual bool hasProperty(const std::string &name) const;
  virtual std::vector<std::string> getPropertyNames() const;

  // 属性访问
  virtual ValueVariant getProperty(const std::string &name) const;
  virtual bool setProperty(const std::string &name, const ValueVariant &value);

  // 设置属性的辅助方法
  void setPropertyValue(const std::string &name, const ValueVariant &value);
  void setPropertyValue(size_t index, const ValueVariant &value);

  // 属性枚举顺序
  void keepOrder(bool enable);

  // 数据上下文
  void *data = nullptr;

  // 类型相关
  virtual ValueType getType() const { return ValueType::Object; }
  virtual std::string toString() const;

protected:
  std::unordered_map<std::string, PropertyDescriptor> properties_;
  std::vector<std::string> insertionOrder_;
  bool keepOrder_ = false;
  void initializeCommonProperties();

protected:
  // 内部属性访问辅助方法，子类可以重写来处理特有的属性
  virtual ValueVariant getPropertyInternal(const std::string &name) const;
};

// 字符串类
class JString : public JObject {
public:
  JString(const std::string &str = "");
  JString(const char *str);

  // C++方法
  size_t Size() const;
  bool Empty() const;
  void Clear();
  char At(size_t index) const;
  char Front() const;
  char Back() const;

  // 重写基类方法
  ValueType getType() const override { return ValueType::String; }
  std::string toString() const override;

  // 获取底层字符串
  const std::string &getValue() const { return value_; }
  void setValue(const std::string &value);

protected:
  // 重写属性访问方法以处理字符串特有的方法
  ValueVariant getPropertyInternal(const std::string &name) const override;

private:
  std::string value_;
  void initializeStringProperties();
};

// 数组类
class JArray : public JObject {
public:
  JArray(size_t size = 0);
  JArray(const std::vector<ValueVariant> &values);

  // C++方法
  size_t Size() const;
  bool Empty() const;
  void Clear();
  void Push(const ValueVariant &value);
  ValueVariant Pop();
  ValueVariant At(size_t index) const;
  ValueVariant Front() const;
  ValueVariant Back() const;

  // 重写基类方法
  ValueType getType() const override { return ValueType::Array; }
  std::string toString() const override;

  // 重写属性管理方法，数字索引按需动态解析，避免维护索引属性描述符
  bool hasProperty(const std::string &name) const override;
  std::vector<std::string> getPropertyNames() const override;
  bool setProperty(const std::string &name,
                   const ValueVariant &value) override;

  // 数组特有的属性访问
  void setElement(size_t index, const ValueVariant &value);

  // 获取底层数组
  const std::vector<ValueVariant> &getValue() const { return value_; }
  std::vector<ValueVariant> &getValue() { return value_; }

protected:
  // 重写属性访问方法以处理数组特有的方法
  ValueVariant getPropertyInternal(const std::string &name) const override;

private:
  std::vector<ValueVariant> value_;
  void initializeArrayProperties();
};

// 函数类
class JFunction : public JObject {
public:
  using FunctionType =
      std::function<ValueVariant(const std::vector<ValueVariant> &)>;

  JFunction(const std::string &name = "", FunctionType func = nullptr);

  // C++方法
  ValueVariant Call(const std::vector<ValueVariant> &args);

  // 重写基类方法
  ValueType getType() const override { return ValueType::Function; }
  std::string toString() const override;

  // 获取函数信息
  const std::string &getName() const { return name_; }
  void setName(const std::string &name);

protected:
  // 重写属性访问方法以处理函数特有的方法
  ValueVariant getPropertyInternal(const std::string &name) const override;

private:
  std::string name_;
  FunctionType function_;
  void initializeFunctionProperties();
};

// 日期类
class JDate : public JObject {
public:
  JDate();
  JDate(const std::chrono::system_clock::time_point &time);
  JDate(int64_t timestamp); // 毫秒时间戳

  // 重写基类方法
  ValueType getType() const override { return ValueType::Date; }
  std::string toString() const override;

  // 日期相关方法
  int64_t getTime() const; // 获取毫秒时间戳
  void setTime(int64_t timestamp);

protected:
  // 重写属性访问方法以处理日期特有的方法
  ValueVariant getPropertyInternal(const std::string &name) const override;

private:
  std::chrono::system_clock::time_point time_;
  void initializeDateProperties();
};

} // namespace jobject

// 访问器（jvalue/jarray/jstring）与工具函数（utils）从独立头文件提供，
// 在此包含以保持对 "JObject.h" 的向后兼容。
#include "Accessor.h"
#include "Utils.h"
