#pragma once

#include "JObject.h"

#include <cmath>
#include <functional>
#include <memory>
#include <string>

namespace jobject {

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
jvalue evalValue(jvalue value, const std::string &expr);

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
