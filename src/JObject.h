#pragma once

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
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

// ValueVariant访问器
class jvalue {
public:
  explicit jvalue(const ValueVariant &value = JUndefined{});
  explicit jvalue(bool v) : jvalue(ValueVariant(v)) {}
  explicit jvalue(int32_t v) : jvalue(ValueVariant(v)) {}
  explicit jvalue(uint32_t v) : jvalue(ValueVariant(v)) {}
  explicit jvalue(uint64_t v) : jvalue(ValueVariant(v)) {}
  explicit jvalue(double v) : jvalue(ValueVariant(v)) {}
  explicit jvalue(const std::string &v)
      : jvalue(ValueVariant(std::make_shared<JString>(v))) {}
  explicit jvalue(const char *v)
      : jvalue(ValueVariant(std::make_shared<JString>(v))) {}

  jvalue operator[](const std::string &name) const;
  jvalue operator[](size_t index) const;

  jvalue &operator=(const ValueVariant &value);

  const ValueVariant &getValue() const;
  ValueVariant &getValue();

  operator ValueVariant() const;

  // ========== 类型转换 ==========
  template <typename T> T to() const;

  // ========== 迭代器支持 ==========

  // 属性迭代器 - 迭代JObject的属性，产出 pair<string, jvalue>
  class property_iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<std::string, jvalue>;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type *;
    using reference = value_type;

    property_iterator();
    property_iterator(const std::shared_ptr<JObject> &obj,
                      const std::shared_ptr<const std::vector<std::string>> &names,
                      size_t index);

    reference operator*() const;
    property_iterator &operator++();
    property_iterator operator++(int);
    bool operator==(const property_iterator &other) const;
    bool operator!=(const property_iterator &other) const;

  private:
    std::shared_ptr<JObject> object_;
    std::shared_ptr<const std::vector<std::string>> names_;
    size_t index_ = 0;
  };

  // 属性迭代范围 - 用于 for(auto& [k,v] : val.properties())
  class property_range {
  public:
    property_range(property_iterator b, property_iterator e);
    property_iterator begin() const;
    property_iterator end() const;

  private:
    property_iterator begin_;
    property_iterator end_;
  };

  // 获取属性迭代范围
  property_range properties() const;

  // 默认迭代器 - 使用property_iterator
  using iterator = property_iterator;

  iterator begin() const;
  iterator end() const;

  // 获取迭代元素数量
  size_t size() const;
  bool empty() const;

  // ========== 类型判断 ==========
  bool isUndefined() const;
  bool isNull() const;
  bool isNullish() const; // null 或 undefined（对应 JS 的 ?? 判断）

private:
  enum class AccessType { None, ObjectProperty, ArrayIndex };

  ValueVariant value_;
  AccessType accessType_ = AccessType::None;
  std::weak_ptr<JObject> objectTarget_;
  std::weak_ptr<JArray> arrayTarget_;
  std::string propertyName_;
  size_t arrayIndex_ = 0;

  jvalue(const ValueVariant &value, const std::shared_ptr<JObject> &object,
         const std::string &propertyName);
  jvalue(const ValueVariant &value, const std::shared_ptr<JArray> &array,
         size_t index);

protected:
  std::shared_ptr<JObject> getObjectLike() const;
  std::shared_ptr<JArray> getArray() const;
};

// 数组访问器 - 派生自jvalue，提供数组操作方法
class jarray : public jvalue {
public:
  using jvalue::jvalue;
  jarray(const jvalue &val) : jvalue(val.getValue()) {}

  void push(const ValueVariant &value);
  void push(const jvalue &val);
  ValueVariant pop();
  size_t length() const;

  // ========== 迭代器支持 ==========

  // 元素迭代器 - 迭代JArray的元素，产出 jvalue
  class element_iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = jvalue;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type *;
    using reference = value_type;

    element_iterator();
    element_iterator(const std::shared_ptr<JArray> &arr, size_t index);

    reference operator*() const;
    element_iterator &operator++();
    element_iterator operator++(int);
    bool operator==(const element_iterator &other) const;
    bool operator!=(const element_iterator &other) const;

  private:
    std::shared_ptr<JArray> array_;
    size_t index_ = 0;
  };

  // 元素迭代范围 - 用于 for(auto& v : arr.elements())
  class element_range {
  public:
    element_range(element_iterator b, element_iterator e);
    element_iterator begin() const;
    element_iterator end() const;

  private:
    element_iterator begin_;
    element_iterator end_;
  };

  // 获取元素迭代范围
  element_range elements() const;

  // 默认迭代器 - 使用element_iterator
  using iterator = element_iterator;

  iterator begin() const;
  iterator end() const;

  size_t size() const;
  bool empty() const;
};

// jvalue::to<T> 特化声明
template <> bool jvalue::to<bool>() const;
template <> int32_t jvalue::to<int32_t>() const;
template <> uint32_t jvalue::to<uint32_t>() const;
template <> int64_t jvalue::to<int64_t>() const;
template <> uint64_t jvalue::to<uint64_t>() const;
template <> std::string jvalue::to<std::string>() const;

// 字符串访问器 - 派生自jvalue，提供字符串操作方法
class jstring : public jvalue {
public:
  using jvalue::jvalue;
  jstring(const jvalue &val) : jvalue(val.getValue()) {}
  explicit jstring(const std::string &str) : jvalue(str) {}

  std::string str() const { return to<std::string>(); }
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
        if constexpr (std::is_same_v<T, JUndefined>) {
          return ValueType::Undefined;
        } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
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
        if constexpr (std::is_same_v<T, JUndefined>) {
          return "undefined";
        } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
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
        if constexpr (std::is_same_v<T, JUndefined>) {
          return std::nan(""); // JS 规范：undefined → NaN
        } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
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
        if constexpr (std::is_same_v<T, JUndefined>) {
          return false; // JS 规范：undefined → false
        } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
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

// Conversion utilities
inline std::shared_ptr<JArray> toJArray(const ValueVariant &value) {
  if (auto arrayPtr = std::get_if<std::shared_ptr<JArray>>(&value)) {
    return *arrayPtr;
  }
  return nullptr;
}

inline std::shared_ptr<JString> toJString(const ValueVariant &value) {
  if (auto stringPtr = std::get_if<std::shared_ptr<JString>>(&value)) {
    return *stringPtr;
  }
  return nullptr;
}

inline std::shared_ptr<JObject> toJObject(const ValueVariant &value) {
  if (auto objectPtr = std::get_if<std::shared_ptr<JObject>>(&value)) {
    return *objectPtr;
  }
  return nullptr;
}

inline std::shared_ptr<JFunction> toJFunction(const ValueVariant &value) {
  if (auto functionPtr = std::get_if<std::shared_ptr<JFunction>>(&value)) {
    return *functionPtr;
  }
  return nullptr;
}

inline std::shared_ptr<JDate> toJDate(const ValueVariant &value) {
  if (auto datePtr = std::get_if<std::shared_ptr<JDate>>(&value)) {
    return *datePtr;
  }
  return nullptr;
}
} // namespace utils

} // namespace jobject
