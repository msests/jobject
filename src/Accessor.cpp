#include "Accessor.h"

#include "Utils.h"

namespace jobject {

// =======================
// jvalue 实现
// =======================

jvalue::jvalue(const ValueVariant &value) : value_(value) {}

jvalue::jvalue(const ValueVariant &value,
               const std::shared_ptr<JObject> &object,
               const std::string &propertyName)
    : value_(value), accessType_(AccessType::ObjectProperty),
      objectTarget_(object), propertyName_(propertyName) {}

jvalue::jvalue(const ValueVariant &value, const std::shared_ptr<JArray> &array,
               size_t index)
    : value_(value), accessType_(AccessType::ArrayIndex), arrayTarget_(array),
      arrayIndex_(index) {}

jvalue jvalue::operator[](const std::string &name) const {
  auto objectLike = getObjectLike();
  if (!objectLike) {
    return jvalue(JUndefined{});
  }
  return jvalue(objectLike->getProperty(name), objectLike, name);
}

jvalue jvalue::operator[](size_t index) const {
  auto array = getArray();
  if (array) {
    // 模拟JavaScript行为：先将index转为字符串，检查是否有对应属性
    // （包括用户手动设置的命名属性及数组元素属性），有则通过属性系统返回；
    // 否则按索引访问元素（越界时返回null）。
    std::string key = std::to_string(index);
    if (array->hasProperty(key)) {
      return jvalue(array->getProperty(key),
                    std::static_pointer_cast<JObject>(array), key);
    }
    return jvalue(array->At(index), array, index);
  }
  auto objectLike = getObjectLike();
  if (objectLike) {
    std::string key = std::to_string(index);
    return jvalue(objectLike->getProperty(key), objectLike, key);
  }
  return jvalue(JUndefined{});
}

jvalue &jvalue::operator=(const ValueVariant &value) {
  value_ = value;

  if (accessType_ == AccessType::ObjectProperty) {
    auto objectLike = objectTarget_.lock();
    if (objectLike) {
      objectLike->setProperty(propertyName_, value);
    }
    return *this;
  }

  if (accessType_ == AccessType::ArrayIndex) {
    auto array = arrayTarget_.lock();
    if (array) {
      array->setElement(arrayIndex_, value);
    }
  }

  return *this;
}

const ValueVariant &jvalue::getValue() const { return value_; }

ValueVariant &jvalue::getValue() { return value_; }

jvalue::operator ValueVariant() const { return value_; }

bool jvalue::isUndefined() const {
  return std::holds_alternative<JUndefined>(value_);
}

bool jvalue::isNull() const {
  return std::holds_alternative<std::nullptr_t>(value_);
}

bool jvalue::isNullish() const { return isUndefined() || isNull(); }

std::shared_ptr<JObject> jvalue::getObjectLike() const {
  if (auto objectPtr = std::get_if<std::shared_ptr<JObject>>(&value_)) {
    return *objectPtr;
  }
  if (auto stringPtr = std::get_if<std::shared_ptr<JString>>(&value_)) {
    return std::static_pointer_cast<JObject>(*stringPtr);
  }
  if (auto arrayPtr = std::get_if<std::shared_ptr<JArray>>(&value_)) {
    return std::static_pointer_cast<JObject>(*arrayPtr);
  }
  if (auto functionPtr = std::get_if<std::shared_ptr<JFunction>>(&value_)) {
    return std::static_pointer_cast<JObject>(*functionPtr);
  }
  if (auto datePtr = std::get_if<std::shared_ptr<JDate>>(&value_)) {
    return std::static_pointer_cast<JObject>(*datePtr);
  }
  return nullptr;
}

std::shared_ptr<JArray> jvalue::getArray() const {
  if (auto arrayPtr = std::get_if<std::shared_ptr<JArray>>(&value_)) {
    return *arrayPtr;
  }
  return nullptr;
}

// =======================
// jvalue::to<T> 特化实现
// =======================

template <> bool jvalue::to<bool>() const { return utils::toBoolean(value_); }

template <> int32_t jvalue::to<int32_t>() const {
  return static_cast<int32_t>(utils::toNumber(value_));
}

template <> uint32_t jvalue::to<uint32_t>() const {
  return static_cast<uint32_t>(utils::toNumber(value_));
}

template <> int64_t jvalue::to<int64_t>() const {
  return static_cast<int64_t>(utils::toNumber(value_));
}

template <> uint64_t jvalue::to<uint64_t>() const {
  return static_cast<uint64_t>(utils::toNumber(value_));
}

template <> std::string jvalue::to<std::string>() const {
  return utils::valueToString(value_);
}

// =======================
// jarray 实现
// =======================

void jarray::push(const ValueVariant &value) {
  auto array = getArray();
  if (array) {
    array->Push(value);
  }
}

void jarray::push(const jvalue &val) { push(val.getValue()); }

ValueVariant jarray::pop() {
  auto array = getArray();
  if (array) {
    return array->Pop();
  }
  return JUndefined{};
}

size_t jarray::length() const {
  auto array = getArray();
  if (array) {
    return array->Size();
  }
  return 0;
}

// =======================
// property_iterator 实现
// =======================

jvalue::property_iterator::property_iterator() : index_(0) {}

jvalue::property_iterator::property_iterator(
    const std::shared_ptr<JObject> &obj,
    const std::shared_ptr<const std::vector<std::string>> &names,
    size_t index)
    : object_(obj), names_(names), index_(index) {}

jvalue::property_iterator::reference
jvalue::property_iterator::operator*() const {
  const auto &name = (*names_)[index_];
  return {name, jvalue(object_->getProperty(name), object_, name)};
}

jvalue::property_iterator &jvalue::property_iterator::operator++() {
  ++index_;
  return *this;
}

jvalue::property_iterator jvalue::property_iterator::operator++(int) {
  auto tmp = *this;
  ++index_;
  return tmp;
}

bool jvalue::property_iterator::operator==(
    const property_iterator &other) const {
  return index_ == other.index_;
}

bool jvalue::property_iterator::operator!=(
    const property_iterator &other) const {
  return index_ != other.index_;
}

// =======================
// property_range 实现
// =======================

jvalue::property_range::property_range(property_iterator b, property_iterator e)
    : begin_(std::move(b)), end_(std::move(e)) {}

jvalue::property_iterator jvalue::property_range::begin() const {
  return begin_;
}

jvalue::property_iterator jvalue::property_range::end() const { return end_; }

// =======================
// jvalue 迭代范围方法
// =======================

jvalue::property_range jvalue::properties() const {
  auto obj = getObjectLike();
  if (!obj) {
    return property_range(property_iterator(), property_iterator());
  }
  // 只调用一次 getPropertyNames，begin 和 end 共享同一份数据
  auto names = std::make_shared<const std::vector<std::string>>(obj->getPropertyNames());
  auto sz = names->size();
  return property_range(property_iterator(obj, names, 0),
                        property_iterator(obj, names, sz));
}

// =======================
// jvalue begin/end/size/empty
// =======================

jvalue::iterator jvalue::begin() const {
  auto obj = getObjectLike();
  if (obj) {
    auto names = std::make_shared<const std::vector<std::string>>(obj->getPropertyNames());
    return property_iterator(obj, names, 0);
  }
  return property_iterator();
}

jvalue::iterator jvalue::end() const {
  auto obj = getObjectLike();
  if (obj) {
    auto names = std::make_shared<const std::vector<std::string>>(obj->getPropertyNames());
    auto sz = names->size();
    return property_iterator(obj, names, sz);
  }
  return property_iterator();
}

size_t jvalue::size() const {
  auto obj = getObjectLike();
  if (obj)
    return obj->getPropertyNames().size();
  return 0;
}

bool jvalue::empty() const { return size() == 0; }

// =======================
// jarray::element_iterator 实现
// =======================

jarray::element_iterator::element_iterator() : index_(0) {}

jarray::element_iterator::element_iterator(const std::shared_ptr<JArray> &arr,
                                           size_t index)
    : array_(arr), index_(index) {}

jarray::element_iterator::reference
jarray::element_iterator::operator*() const {
  // Route through public APIs to create an index-bound jvalue.
  return jvalue(array_)[index_];
}

jarray::element_iterator &jarray::element_iterator::operator++() {
  ++index_;
  return *this;
}

jarray::element_iterator jarray::element_iterator::operator++(int) {
  auto tmp = *this;
  ++index_;
  return tmp;
}

bool jarray::element_iterator::operator==(const element_iterator &other) const {
  return index_ == other.index_;
}

bool jarray::element_iterator::operator!=(const element_iterator &other) const {
  return index_ != other.index_;
}

// =======================
// jarray::element_range 实现
// =======================

jarray::element_range::element_range(element_iterator b, element_iterator e)
    : begin_(std::move(b)), end_(std::move(e)) {}

jarray::element_iterator jarray::element_range::begin() const { return begin_; }

jarray::element_iterator jarray::element_range::end() const { return end_; }

// =======================
// jarray 迭代方法
// =======================

jarray::element_range jarray::elements() const {
  auto arr = getArray();
  if (!arr) {
    return element_range(element_iterator(), element_iterator());
  }
  return element_range(element_iterator(arr, 0),
                       element_iterator(arr, arr->Size()));
}

jarray::iterator jarray::begin() const {
  auto arr = getArray();
  if (arr) {
    return element_iterator(arr, 0);
  }
  return element_iterator();
}

jarray::iterator jarray::end() const {
  auto arr = getArray();
  if (arr) {
    return element_iterator(arr, arr->Size());
  }
  return element_iterator();
}

size_t jarray::size() const {
  auto arr = getArray();
  if (arr)
    return arr->Size();
  return 0;
}

bool jarray::empty() const { return size() == 0; }

} // namespace jobject
