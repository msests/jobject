#include "JObject.h"

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace jobject {

// =======================
// JObject 实现
// =======================

JObject::JObject() { initializeCommonProperties(); }

void JObject::initializeCommonProperties() {
  // 不在这里添加toString属性，避免无限递归
  // toString将在getPropertyInternal中按需创建
}

void JObject::keepOrder(bool enable) { keepOrder_ = enable; }

bool JObject::defineProperty(const std::string &name,
                             const PropertyDescriptor &descriptor) {
  if (properties_.find(name) == properties_.end()) {
    insertionOrder_.push_back(name);
  }
  properties_[name] = descriptor;
  return true;
}

bool JObject::deleteProperty(const std::string &name) {
  auto it = properties_.find(name);
  if (it != properties_.end()) {
    if (it->second.configurable) {
      properties_.erase(it);
      auto orderIt =
          std::find(insertionOrder_.begin(), insertionOrder_.end(), name);
      if (orderIt != insertionOrder_.end()) {
        insertionOrder_.erase(orderIt);
      }
      return true;
    }
  }
  return false;
}

bool JObject::hasProperty(const std::string &name) const {
  return properties_.find(name) != properties_.end();
}

std::vector<std::string> JObject::getPropertyNames() const {
  std::vector<std::string> names;
  names.reserve(properties_.size());
  if (keepOrder_) {
    for (const auto &name : insertionOrder_) {
      auto it = properties_.find(name);
      if (it != properties_.end() && it->second.enumerable) {
        names.push_back(name);
      }
    }
  } else {
    for (const auto &pair : properties_) {
      if (pair.second.enumerable) {
        names.push_back(pair.first);
      }
    }
  }
  return names;
}

ValueVariant JObject::getProperty(const std::string &name) const {
  return getPropertyInternal(name);
}

ValueVariant JObject::getPropertyInternal(const std::string &name) const {
  auto it = properties_.find(name);
  if (it != properties_.end()) {
    const auto &descriptor = it->second;
    if (descriptor.getter) {
      return descriptor.getter();
    }
    return descriptor.value;
  }

  // 处理内置属性
  if (name == "toString") {
    auto toStringFunc = std::make_shared<JFunction>(
        "toString", [this](const std::vector<ValueVariant> &) -> ValueVariant {
          return std::make_shared<JString>(this->toString());
        });
    return toStringFunc;
  }

  return JUndefined{};
}

bool JObject::setProperty(const std::string &name, const ValueVariant &value) {
  auto it = properties_.find(name);
  if (it != properties_.end()) {
    auto &descriptor = it->second;
    if (descriptor.setter) {
      descriptor.setter(value);
      return true;
    }
    if (descriptor.writable) {
      descriptor.value = value;
      return true;
    }
    return false;
  } else {
    // 创建新属性
    PropertyDescriptor descriptor;
    descriptor.value = value;
    return defineProperty(name, descriptor);
  }
}

void JObject::setPropertyValue(const std::string &name,
                               const ValueVariant &value) {
  setProperty(name, value);
}

void JObject::setPropertyValue(size_t index, const ValueVariant &value) {
  setProperty(std::to_string(index), value);
}

std::string JObject::toString() const { return "[object Object]"; }

// =======================
// JString 实现
// =======================

JString::JString(const std::string &str) : value_(str) {
  initializeStringProperties();
}

JString::JString(const char *str) : value_(str ? str : "") {
  initializeStringProperties();
}

void JString::initializeStringProperties() {
  // length属性
  jobject::utils::def_prop_ex(
      *this, "length",
      [this]() -> ValueVariant {
        return static_cast<uint32_t>(value_.length());
      },
      nullptr, false, false, false);

  // 其他方法将在getPropertyInternal中按需创建，避免递归
}

size_t JString::Size() const { return value_.size(); }

bool JString::Empty() const { return value_.empty(); }

void JString::Clear() { value_.clear(); }

char JString::At(size_t index) const {
  if (index >= value_.size())
    return '\0';
  return value_[index];
}

char JString::Front() const { return value_.empty() ? '\0' : value_.front(); }

char JString::Back() const { return value_.empty() ? '\0' : value_.back(); }

std::string JString::toString() const { return value_; }

void JString::setValue(const std::string &value) { value_ = value; }

ValueVariant JString::getPropertyInternal(const std::string &name) const {
  // 处理JString特有的方法
  if (name == "concat") {
    auto concatFunc = std::make_shared<JFunction>(
        "concat",
        [this](const std::vector<ValueVariant> &args) -> ValueVariant {
          std::string result = value_;
          for (const auto &arg : args) {
            result += utils::valueToString(arg);
          }
          return std::make_shared<JString>(result);
        });
    return concatFunc;
  } else if (name == "indexOf") {
    auto indexOfFunc = std::make_shared<JFunction>(
        "indexOf",
        [this](const std::vector<ValueVariant> &args) -> ValueVariant {
          if (args.empty())
            return static_cast<int32_t>(-1);
          std::string searchStr = utils::valueToString(args[0]);
          size_t pos = value_.find(searchStr);
          return pos == std::string::npos ? static_cast<int32_t>(-1)
                                          : static_cast<int32_t>(pos);
        });
    return indexOfFunc;
  } else if (name == "lastIndexOf") {
    auto lastIndexOfFunc = std::make_shared<JFunction>(
        "lastIndexOf",
        [this](const std::vector<ValueVariant> &args) -> ValueVariant {
          if (args.empty())
            return static_cast<int32_t>(-1);
          std::string searchStr = utils::valueToString(args[0]);
          size_t pos = value_.rfind(searchStr);
          return pos == std::string::npos ? static_cast<int32_t>(-1)
                                          : static_cast<int32_t>(pos);
        });
    return lastIndexOfFunc;
  }

  // 调用父类方法处理通用属性
  return JObject::getPropertyInternal(name);
}

// =======================
// JArray 实现
// =======================

JArray::JArray(size_t size) : value_(size) { initializeArrayProperties(); }

JArray::JArray(const std::vector<ValueVariant> &values) : value_(values) {
  initializeArrayProperties();
}

void JArray::initializeArrayProperties() {
  // length属性
  jobject::utils::def_prop_ex(
      *this, "length",
      [this]() -> ValueVariant { return static_cast<uint32_t>(value_.size()); },
      [this](const ValueVariant &val) {
        if (std::holds_alternative<uint32_t>(val)) {
          uint32_t newSize = std::get<uint32_t>(val);
          value_.resize(newSize);
        }
      },
      true, false, false);

  updateLength();

  // 其他方法将在getPropertyInternal中按需创建，避免递归
}

void JArray::updateLength() {
  // 更新数字索引属性
  for (size_t i = 0; i < value_.size(); ++i) {
    jobject::utils::def_prop_ex(
        *this, std::to_string(i), ([this, i]() -> ValueVariant {
          return i < value_.size() ? value_[i] : ValueVariant(nullptr);
        }),
        ([this, i](const ValueVariant &val) {
          if (i < value_.size()) {
            value_[i] = val;
          }
        }),
        true, true, true);
  }
}

size_t JArray::Size() const { return value_.size(); }

bool JArray::Empty() const { return value_.empty(); }

void JArray::Clear() {
  value_.clear();
  updateLength();
}

void JArray::Push(const ValueVariant &value) {
  value_.push_back(value);
  updateLength();
}

ValueVariant JArray::Pop() {
  if (value_.empty())
    return JUndefined{};
  ValueVariant result = value_.back();
  value_.pop_back();
  updateLength();
  return result;
}

ValueVariant JArray::At(size_t index) const {
  if (index >= value_.size())
    return JUndefined{};
  return value_[index];
}

ValueVariant JArray::Front() const {
  return value_.empty() ? ValueVariant{JUndefined{}} : value_.front();
}

ValueVariant JArray::Back() const {
  return value_.empty() ? ValueVariant{JUndefined{}} : value_.back();
}

void JArray::setElement(size_t index, const ValueVariant &value) {
  if (index >= value_.size()) {
    value_.resize(index + 1);
  }
  value_[index] = value;
  updateLength();
}

std::string JArray::toString() const {
  std::ostringstream oss;
  for (size_t i = 0; i < value_.size(); ++i) {
    if (i > 0)
      oss << ",";
    oss << utils::valueToString(value_[i]);
  }
  return oss.str();
}

ValueVariant JArray::getPropertyInternal(const std::string &name) const {
  // 处理JArray特有的方法
  if (name == "push") {
    auto pushFunc = std::make_shared<JFunction>(
        "push", [this](const std::vector<ValueVariant> &args) -> ValueVariant {
          // 注意：这里我们需要修改数组，但方法是const的
          // 我们需要const_cast来绕过这个限制
          auto *mutableThis = const_cast<JArray *>(this);
          for (const auto &arg : args) {
            mutableThis->value_.push_back(arg);
          }
          mutableThis->updateLength();
          return static_cast<uint32_t>(mutableThis->value_.size());
        });
    return pushFunc;
  } else if (name == "pop") {
    auto popFunc = std::make_shared<JFunction>(
        "pop", [this](const std::vector<ValueVariant> &) -> ValueVariant {
          auto *mutableThis = const_cast<JArray *>(this);
          if (mutableThis->value_.empty())
            return JUndefined{};
          ValueVariant result = mutableThis->value_.back();
          mutableThis->value_.pop_back();
          mutableThis->updateLength();
          return result;
        });
    return popFunc;
  } else if (name == "shift") {
    auto shiftFunc = std::make_shared<JFunction>(
        "shift", [this](const std::vector<ValueVariant> &) -> ValueVariant {
          auto *mutableThis = const_cast<JArray *>(this);
          if (mutableThis->value_.empty())
            return JUndefined{};
          ValueVariant result = mutableThis->value_.front();
          mutableThis->value_.erase(mutableThis->value_.begin());
          mutableThis->updateLength();
          return result;
        });
    return shiftFunc;
  } else if (name == "unshift") {
    auto unshiftFunc = std::make_shared<JFunction>(
        "unshift",
        [this](const std::vector<ValueVariant> &args) -> ValueVariant {
          auto *mutableThis = const_cast<JArray *>(this);
          mutableThis->value_.insert(mutableThis->value_.begin(), args.begin(),
                                     args.end());
          mutableThis->updateLength();
          return static_cast<uint32_t>(mutableThis->value_.size());
        });
    return unshiftFunc;
  } else if (name == "splice") {
    auto spliceFunc = std::make_shared<JFunction>(
        "splice",
        [this](const std::vector<ValueVariant> &args) -> ValueVariant {
          auto *mutableThis = const_cast<JArray *>(this);
          if (args.empty())
            return utils::createArray();

          int32_t start = 0;
          if (args.size() > 0 && std::holds_alternative<int32_t>(args[0])) {
            start = std::get<int32_t>(args[0]);
          }

          size_t deleteCount = mutableThis->value_.size();
          if (args.size() > 1 && std::holds_alternative<int32_t>(args[1])) {
            deleteCount = std::max(0, std::get<int32_t>(args[1]));
          }

          if (start < 0)
            start = std::max(
                0, static_cast<int32_t>(mutableThis->value_.size()) + start);
          size_t startIndex =
              std::min(static_cast<size_t>(start), mutableThis->value_.size());
          deleteCount =
              std::min(deleteCount, mutableThis->value_.size() - startIndex);

          // 创建被删除的元素数组
          auto deletedArray = utils::createArray();
          for (size_t i = 0; i < deleteCount; ++i) {
            deletedArray->getValue().push_back(
                mutableThis->value_[startIndex + i]);
          }

          // 删除元素
          mutableThis->value_.erase(mutableThis->value_.begin() + startIndex,
                                    mutableThis->value_.begin() + startIndex +
                                        deleteCount);

          // 插入新元素
          if (args.size() > 2) {
            mutableThis->value_.insert(mutableThis->value_.begin() + startIndex,
                                       args.begin() + 2, args.end());
          }

          mutableThis->updateLength();
          return deletedArray;
        });
    return spliceFunc;
  } else if (name == "slice") {
    auto sliceFunc = std::make_shared<JFunction>(
        "slice", [this](const std::vector<ValueVariant> &args) -> ValueVariant {
          int32_t start = 0;
          int32_t end = static_cast<int32_t>(value_.size());

          if (args.size() > 0 && std::holds_alternative<int32_t>(args[0])) {
            start = std::get<int32_t>(args[0]);
          }
          if (args.size() > 1 && std::holds_alternative<int32_t>(args[1])) {
            end = std::get<int32_t>(args[1]);
          }

          if (start < 0)
            start = std::max(0, static_cast<int32_t>(value_.size()) + start);
          if (end < 0)
            end = std::max(0, static_cast<int32_t>(value_.size()) + end);

          size_t startIndex =
              std::min(static_cast<size_t>(start), value_.size());
          size_t endIndex = std::min(static_cast<size_t>(end), value_.size());

          if (startIndex >= endIndex)
            return utils::createArray();

          std::vector<ValueVariant> sliced(value_.begin() + startIndex,
                                           value_.begin() + endIndex);
          return std::make_shared<JArray>(sliced);
        });
    return sliceFunc;
  }

  // 调用父类方法处理通用属性
  return JObject::getPropertyInternal(name);
}

// =======================
// JFunction 实现
// =======================

JFunction::JFunction(const std::string &name, FunctionType func)
    : name_(name), function_(func) {
  initializeFunctionProperties();
}

void JFunction::initializeFunctionProperties() {
  // name属性
  jobject::utils::def_prop_ex(
      *this, "name",
      [this]() -> ValueVariant { return std::make_shared<JString>(name_); },
      nullptr, false, false, true);

  // length属性（参数个数，这里简化为0）
  jobject::utils::def_prop_val(*this, "length", static_cast<uint32_t>(0), false,
                               false, true);

  // call方法将在getPropertyInternal中按需创建，避免递归
}

ValueVariant JFunction::Call(const std::vector<ValueVariant> &args) {
  if (function_) {
    return function_(args);
  }
  return nullptr;
}

std::string JFunction::toString() const {
  return "function " + name_ + "() { [native code] }";
}

void JFunction::setName(const std::string &name) { name_ = name; }

ValueVariant JFunction::getPropertyInternal(const std::string &name) const {
  // 处理JFunction特有的方法
  if (name == "call") {
    auto callFunc = std::make_shared<JFunction>(
        "call", [this](const std::vector<ValueVariant> &args) -> ValueVariant {
          return const_cast<JFunction *>(this)->Call(args);
        });
    return callFunc;
  }

  // 调用父类方法处理通用属性
  return JObject::getPropertyInternal(name);
}

// =======================
// JDate 实现
// =======================

JDate::JDate() : time_(std::chrono::system_clock::now()) {
  initializeDateProperties();
}

JDate::JDate(const std::chrono::system_clock::time_point &time) : time_(time) {
  initializeDateProperties();
}

JDate::JDate(int64_t timestamp)
    : time_(std::chrono::system_clock::from_time_t(timestamp / 1000)) {
  initializeDateProperties();
}

void JDate::initializeDateProperties() {
  // 日期方法将在getPropertyInternal中按需创建，避免递归
}

int64_t JDate::getTime() const {
  auto duration = time_.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(duration)
      .count();
}

void JDate::setTime(int64_t timestamp) {
  time_ = std::chrono::system_clock::from_time_t(timestamp / 1000);
}

std::string JDate::toString() const {
  auto time_t = std::chrono::system_clock::to_time_t(time_);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
  return ss.str();
}

ValueVariant JDate::getPropertyInternal(const std::string &name) const {
  // 处理JDate特有的方法
  if (name == "getTime") {
    auto getTimeFunc = std::make_shared<JFunction>(
        "getTime", [this](const std::vector<ValueVariant> &) -> ValueVariant {
          return static_cast<uint64_t>(this->getTime());
        });
    return getTimeFunc;
  } else if (name == "setTime") {
    auto setTimeFunc = std::make_shared<JFunction>(
        "setTime",
        [this](const std::vector<ValueVariant> &args) -> ValueVariant {
          auto *mutableThis = const_cast<JDate *>(this);
          if (!args.empty()) {
            if (std::holds_alternative<uint64_t>(args[0])) {
              mutableThis->setTime(std::get<uint64_t>(args[0]));
            } else if (std::holds_alternative<int32_t>(args[0])) {
              mutableThis->setTime(std::get<int32_t>(args[0]));
            }
          }
          return static_cast<uint64_t>(mutableThis->getTime());
        });
    return setTimeFunc;
  }

  // 调用父类方法处理通用属性
  return JObject::getPropertyInternal(name);
}

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
