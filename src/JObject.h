#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
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

// 值类型枚举
enum class ValueType {
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
using ValueVariant = std::variant<std::nullptr_t,           // Null
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

  // 操作符重载
  ValueVariant operator[](const std::string &name) const;
  ValueVariant operator[](const char *name) const;
  ValueVariant operator[](int index) const;
  ValueVariant operator[](size_t index) const;

  // 设置属性的辅助方法
  void setPropertyValue(const std::string &name, const ValueVariant &value);
  void setPropertyValue(int index, const ValueVariant &value);
  void setPropertyValue(size_t index, const ValueVariant &value);

  // 数据上下文
  void *data = nullptr;

  // 类型相关
  virtual ValueType getType() const { return ValueType::Object; }
  virtual std::string toString() const;

protected:
  std::unordered_map<std::string, PropertyDescriptor> properties_;
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
  ValueVariant At(size_t index) const;
  ValueVariant Front() const;
  ValueVariant Back() const;

  // 重写基类方法
  ValueType getType() const override { return ValueType::Array; }
  std::string toString() const override;

  // 数组特有的属性访问
  ValueVariant operator[](size_t index) const;
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
  void updateLength();
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

// 辅助命名空间
namespace utils {
// 基础属性定义函数
template <typename Obj>
void def_prop_rw(Obj &obj, const std::string &prop_name,
                 const std::function<ValueVariant()> &read_fn,
                 const std::function<void(const ValueVariant &)> &write_fn) {
  PropertyDescriptor desc;
  desc.getter = read_fn;
  desc.setter = write_fn;
  desc.writable = true;
  desc.enumerable = true;
  desc.configurable = true;
  obj.defineProperty(prop_name, desc);
}

// 只读属性定义函数
template <typename Obj>
void def_prop_ro(Obj &obj, const std::string &prop_name,
                 const std::function<ValueVariant()> &read_fn) {
  PropertyDescriptor desc;
  desc.getter = read_fn;
  desc.setter = nullptr;
  desc.writable = false;
  desc.enumerable = true;
  desc.configurable = true;
  obj.defineProperty(prop_name, desc);
}

// 自定义属性定义函数（可指定writable, enumerable, configurable）
template <typename Obj>
void def_prop_ex(Obj &obj, const std::string &prop_name,
                 const std::function<ValueVariant()> &read_fn,
                 const std::function<void(const ValueVariant &)> &write_fn,
                 bool writable, bool enumerable, bool configurable) {
  PropertyDescriptor desc;
  desc.getter = read_fn;
  desc.setter = write_fn;
  desc.writable = writable;
  desc.enumerable = enumerable;
  desc.configurable = configurable;
  obj.defineProperty(prop_name, desc);
}

// 值属性定义函数（直接设置值，不使用getter/setter）
template <typename Obj>
void def_prop_val(Obj &obj, const std::string &prop_name,
                  const ValueVariant &val, bool writable, bool enumerable,
                  bool configurable) {
  PropertyDescriptor desc;
  desc.value = val;
  desc.getter = nullptr;
  desc.setter = nullptr;
  desc.writable = writable;
  desc.enumerable = enumerable;
  desc.configurable = configurable;
  obj.defineProperty(prop_name, desc);
}

// 工具函数
ValueType getValueType(const ValueVariant &value);
std::string valueToString(const ValueVariant &value);
bool isNumber(const ValueVariant &value);
double toNumber(const ValueVariant &value);
bool toBoolean(const ValueVariant &value);

// 创建不同类型的值
std::shared_ptr<JObject> createObject();
std::shared_ptr<JString> createString(const std::string &str = "");
std::shared_ptr<JArray> createArray(size_t size = 0);
std::shared_ptr<JFunction>
createFunction(const std::string &name = "",
               JFunction::FunctionType func = nullptr);
std::shared_ptr<JDate> createDate();

// 实现部分
inline ValueType getValueType(const ValueVariant &value) {
  return std::visit(
      [](const auto &v) -> ValueType {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
          return ValueType::Null;
        } else if constexpr (std::is_same_v<T, bool>) {
          return ValueType::Boolean;
        } else if constexpr (std::is_same_v<T, int32_t>) {
          return ValueType::Int32;
        } else if constexpr (std::is_same_v<T, uint32_t>) {
          return ValueType::UInt32;
        } else if constexpr (std::is_same_v<T, uint64_t>) {
          return ValueType::UInt64;
        } else if constexpr (std::is_same_v<T, double>) {
          return ValueType::Double;
        } else if constexpr (std::is_same_v<T, std::shared_ptr<JString>>) {
          return ValueType::String;
        } else if constexpr (std::is_same_v<T, std::shared_ptr<JArray>>) {
          return ValueType::Array;
        } else if constexpr (std::is_same_v<T, std::shared_ptr<JObject>>) {
          return ValueType::Object;
        } else if constexpr (std::is_same_v<T, std::shared_ptr<JFunction>>) {
          return ValueType::Function;
        } else if constexpr (std::is_same_v<T, std::shared_ptr<JDate>>) {
          return ValueType::Date;
        }
        return ValueType::Null;
      },
      value);
}

inline std::string valueToString(const ValueVariant &value) {
  return std::visit(
      [](const auto &v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
          return "null";
        } else if constexpr (std::is_same_v<T, bool>) {
          return v ? "true" : "false";
        } else if constexpr (std::is_same_v<T, int32_t>) {
          return std::to_string(v);
        } else if constexpr (std::is_same_v<T, uint32_t>) {
          return std::to_string(v);
        } else if constexpr (std::is_same_v<T, uint64_t>) {
          return std::to_string(v);
        } else if constexpr (std::is_same_v<T, double>) {
          return std::to_string(v);
        } else if constexpr (std::is_same_v<T, std::shared_ptr<JString>>) {
          return v ? v->toString() : "null";
        } else if constexpr (std::is_same_v<T, std::shared_ptr<JArray>>) {
          return v ? v->toString() : "null";
        } else if constexpr (std::is_same_v<T, std::shared_ptr<JObject>>) {
          return v ? v->toString() : "null";
        } else if constexpr (std::is_same_v<T, std::shared_ptr<JFunction>>) {
          return v ? v->toString() : "null";
        } else if constexpr (std::is_same_v<T, std::shared_ptr<JDate>>) {
          return v ? v->toString() : "null";
        }
        return "undefined";
      },
      value);
}

inline bool isNumber(const ValueVariant &value) {
  return std::visit(
      [](const auto &v) -> bool {
        using T = std::decay_t<decltype(v)>;
        return std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t> ||
               std::is_same_v<T, uint64_t> || std::is_same_v<T, double>;
      },
      value);
}

inline double toNumber(const ValueVariant &value) {
  return std::visit(
      [](const auto &v) -> double {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
          return 0.0;
        } else if constexpr (std::is_same_v<T, bool>) {
          return v ? 1.0 : 0.0;
        } else if constexpr (std::is_same_v<T, int32_t>) {
          return static_cast<double>(v);
        } else if constexpr (std::is_same_v<T, uint32_t>) {
          return static_cast<double>(v);
        } else if constexpr (std::is_same_v<T, uint64_t>) {
          return static_cast<double>(v);
        } else if constexpr (std::is_same_v<T, double>) {
          return v;
        } else {
          return std::nan("");
        }
      },
      value);
}

inline bool toBoolean(const ValueVariant &value) {
  return std::visit(
      [](const auto &v) -> bool {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
          return false;
        } else if constexpr (std::is_same_v<T, bool>) {
          return v;
        } else if constexpr (std::is_same_v<T, int32_t>) {
          return v != 0;
        } else if constexpr (std::is_same_v<T, uint32_t>) {
          return v != 0;
        } else if constexpr (std::is_same_v<T, uint64_t>) {
          return v != 0;
        } else if constexpr (std::is_same_v<T, double>) {
          return v != 0.0 && !std::isnan(v);
        } else if constexpr (std::is_same_v<T, std::shared_ptr<JString>>) {
          return v && !v->Empty();
        } else {
          return v != nullptr;
        }
      },
      value);
}

inline std::shared_ptr<JObject> createObject() {
  return std::make_shared<JObject>();
}

inline std::shared_ptr<JString> createString(const std::string &str) {
  return std::make_shared<JString>(str);
}

inline std::shared_ptr<JArray> createArray(size_t size) {
  return std::make_shared<JArray>(size);
}

inline std::shared_ptr<JFunction> createFunction(const std::string &name,
                                                 JFunction::FunctionType func) {
  return std::make_shared<JFunction>(name, func);
}

inline std::shared_ptr<JDate> createDate() { return std::make_shared<JDate>(); }
} // namespace utils

} // namespace jobject
