#pragma once

#include "JObject.h"

#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace jobject {

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

} // namespace jobject
