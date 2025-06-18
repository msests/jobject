#include "JObject.h"
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>

namespace jobject {

  // =======================
  // JObject 实现
  // =======================

  JObject::JObject() {
    initializeCommonProperties();
  }

  void JObject::initializeCommonProperties() {
    // 不在这里添加toString属性，避免无限递归
    // toString将在getPropertyInternal中按需创建
  }

  bool JObject::defineProperty(const std::string& name, const PropertyDescriptor& descriptor) {
    properties_[name] = descriptor;
    return true;
  }

  bool JObject::deleteProperty(const std::string& name) {
    auto it = properties_.find(name);
    if (it != properties_.end()) {
      if (it->second.configurable) {
        properties_.erase(it);
        return true;
      }
    }
    return false;
  }

  bool JObject::hasProperty(const std::string& name) const {
    return properties_.find(name) != properties_.end();
  }

  std::vector<std::string> JObject::getPropertyNames() const {
    std::vector<std::string> names;
    for (const auto& pair : properties_) {
      if (pair.second.enumerable) {
        names.push_back(pair.first);
      }
    }
    return names;
  }

  ValueVariant JObject::getProperty(const std::string& name) const {
    return getPropertyInternal(name);
  }

  ValueVariant JObject::getPropertyInternal(const std::string& name) const {
    auto it = properties_.find(name);
    if (it != properties_.end()) {
      const auto& descriptor = it->second;
      if (descriptor.getter) {
        return descriptor.getter();
      }
      return descriptor.value;
    }

    // 处理内置属性
    if (name == "toString") {
      auto toStringFunc = std::make_shared<JFunction>("toString",
        [this](const std::vector<ValueVariant>&) -> ValueVariant {
          return std::make_shared<JString>(this->toString());
        });
      return toStringFunc;
    }

    return nullptr;
  }

  bool JObject::setProperty(const std::string& name, const ValueVariant& value) {
    auto it = properties_.find(name);
    if (it != properties_.end()) {
      auto& descriptor = it->second;
      if (descriptor.setter) {
        descriptor.setter(value);
        return true;
      }
      if (descriptor.writable) {
        descriptor.value = value;
        return true;
      }
      return false;
    }
    else {
      // 创建新属性
      PropertyDescriptor descriptor;
      descriptor.value = value;
      return defineProperty(name, descriptor);
    }
  }

  ValueVariant JObject::operator[](const std::string& name) const {
    return getPropertyInternal(name);
  }

  ValueVariant JObject::operator[](const char* name) const {
    return getPropertyInternal(std::string(name));
  }

  ValueVariant JObject::operator[](int index) const {
    return getPropertyInternal(std::to_string(index));
  }

  ValueVariant JObject::operator[](size_t index) const {
    return getPropertyInternal(std::to_string(index));
  }

  void JObject::setPropertyValue(const std::string& name, const ValueVariant& value) {
    setProperty(name, value);
  }

  void JObject::setPropertyValue(int index, const ValueVariant& value) {
    setProperty(std::to_string(index), value);
  }

  void JObject::setPropertyValue(size_t index, const ValueVariant& value) {
    setProperty(std::to_string(index), value);
  }

  std::string JObject::toString() const {
    return "[object Object]";
  }

  // =======================
  // JString 实现
  // =======================

  JString::JString(const std::string& str) : value_(str) {
    initializeStringProperties();
  }

  JString::JString(const char* str) : value_(str ? str : "") {
    initializeStringProperties();
  }

  void JString::initializeStringProperties() {
    // length属性
    DEF_PROP_EX(*this, "length",
      [this]() -> ValueVariant {
        return static_cast<uint32_t>(value_.length());
      },
      nullptr, false, false, false);

    // 其他方法将在getPropertyInternal中按需创建，避免递归
  }

  size_t JString::Size() const {
    return value_.size();
  }

  bool JString::Empty() const {
    return value_.empty();
  }

  void JString::Clear() {
    value_.clear();
  }

  char JString::At(size_t index) const {
    if (index >= value_.size()) return '\0';
    return value_[index];
  }

  char JString::Front() const {
    return value_.empty() ? '\0' : value_.front();
  }

  char JString::Back() const {
    return value_.empty() ? '\0' : value_.back();
  }

  std::string JString::toString() const {
    return value_;
  }

  void JString::setValue(const std::string& value) {
    value_ = value;
  }

  ValueVariant JString::getPropertyInternal(const std::string& name) const {
    // 处理JString特有的方法
    if (name == "concat") {
      auto concatFunc = std::make_shared<JFunction>("concat",
        [this](const std::vector<ValueVariant>& args) -> ValueVariant {
          std::string result = value_;
          for (const auto& arg : args) {
            result += utils::valueToString(arg);
          }
          return std::make_shared<JString>(result);
        });
      return concatFunc;
    }
    else if (name == "indexOf") {
      auto indexOfFunc = std::make_shared<JFunction>("indexOf",
        [this](const std::vector<ValueVariant>& args) -> ValueVariant {
          if (args.empty()) return static_cast<int32_t>(-1);
          std::string searchStr = utils::valueToString(args[0]);
          size_t pos = value_.find(searchStr);
          return pos == std::string::npos ? static_cast<int32_t>(-1) : static_cast<int32_t>(pos);
        });
      return indexOfFunc;
    }
    else if (name == "lastIndexOf") {
      auto lastIndexOfFunc = std::make_shared<JFunction>("lastIndexOf",
        [this](const std::vector<ValueVariant>& args) -> ValueVariant {
          if (args.empty()) return static_cast<int32_t>(-1);
          std::string searchStr = utils::valueToString(args[0]);
          size_t pos = value_.rfind(searchStr);
          return pos == std::string::npos ? static_cast<int32_t>(-1) : static_cast<int32_t>(pos);
        });
      return lastIndexOfFunc;
    }

    // 调用父类方法处理通用属性
    return JObject::getPropertyInternal(name);
  }

  // =======================
  // JArray 实现
  // =======================

  JArray::JArray(size_t size) : value_(size) {
    initializeArrayProperties();
  }

  JArray::JArray(const std::vector<ValueVariant>& values) : value_(values) {
    initializeArrayProperties();
  }

  void JArray::initializeArrayProperties() {
    // length属性
    DEF_PROP_EX(*this, "length",
      [this]() -> ValueVariant {
        return static_cast<uint32_t>(value_.size());
      },
      [this](const ValueVariant& val) {
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
      DEF_PROP_EX(*this, std::to_string(i),
        ([this, i]() -> ValueVariant {
          return i < value_.size() ? value_[i] : ValueVariant(nullptr);
          }),
        ([this, i](const ValueVariant& val) {
          if (i < value_.size()) {
            value_[i] = val;
          }
          }),
        true, true, true);
    }
  }

  size_t JArray::Size() const {
    return value_.size();
  }

  bool JArray::Empty() const {
    return value_.empty();
  }

  void JArray::Clear() {
    value_.clear();
    updateLength();
  }

  ValueVariant JArray::At(size_t index) const {
    if (index >= value_.size()) return nullptr;
    return value_[index];
  }

  ValueVariant JArray::Front() const {
    return value_.empty() ? nullptr : value_.front();
  }

  ValueVariant JArray::Back() const {
    return value_.empty() ? nullptr : value_.back();
  }

  ValueVariant JArray::operator[](size_t index) const {
    return At(index);
  }

  void JArray::setElement(size_t index, const ValueVariant& value) {
    if (index >= value_.size()) {
      value_.resize(index + 1);
    }
    value_[index] = value;
    updateLength();
  }

  std::string JArray::toString() const {
    std::ostringstream oss;
    for (size_t i = 0; i < value_.size(); ++i) {
      if (i > 0) oss << ",";
      oss << utils::valueToString(value_[i]);
    }
    return oss.str();
  }

  ValueVariant JArray::getPropertyInternal(const std::string& name) const {
    // 处理JArray特有的方法
    if (name == "push") {
      auto pushFunc = std::make_shared<JFunction>("push",
        [this](const std::vector<ValueVariant>& args) -> ValueVariant {
          // 注意：这里我们需要修改数组，但方法是const的
          // 我们需要const_cast来绕过这个限制
          auto* mutableThis = const_cast<JArray*>(this);
          for (const auto& arg : args) {
            mutableThis->value_.push_back(arg);
          }
          mutableThis->updateLength();
          return static_cast<uint32_t>(mutableThis->value_.size());
        });
      return pushFunc;
    }
    else if (name == "pop") {
      auto popFunc = std::make_shared<JFunction>("pop",
        [this](const std::vector<ValueVariant>&) -> ValueVariant {
          auto* mutableThis = const_cast<JArray*>(this);
          if (mutableThis->value_.empty()) return nullptr;
          ValueVariant result = mutableThis->value_.back();
          mutableThis->value_.pop_back();
          mutableThis->updateLength();
          return result;
        });
      return popFunc;
    }
    else if (name == "shift") {
      auto shiftFunc = std::make_shared<JFunction>("shift",
        [this](const std::vector<ValueVariant>&) -> ValueVariant {
          auto* mutableThis = const_cast<JArray*>(this);
          if (mutableThis->value_.empty()) return nullptr;
          ValueVariant result = mutableThis->value_.front();
          mutableThis->value_.erase(mutableThis->value_.begin());
          mutableThis->updateLength();
          return result;
        });
      return shiftFunc;
    }
    else if (name == "unshift") {
      auto unshiftFunc = std::make_shared<JFunction>("unshift",
        [this](const std::vector<ValueVariant>& args) -> ValueVariant {
          auto* mutableThis = const_cast<JArray*>(this);
          mutableThis->value_.insert(mutableThis->value_.begin(), args.begin(), args.end());
          mutableThis->updateLength();
          return static_cast<uint32_t>(mutableThis->value_.size());
        });
      return unshiftFunc;
    }
    else if (name == "splice") {
      auto spliceFunc = std::make_shared<JFunction>("splice",
        [this](const std::vector<ValueVariant>& args) -> ValueVariant {
          auto* mutableThis = const_cast<JArray*>(this);
          if (args.empty()) return utils::createArray();

          int32_t start = 0;
          if (args.size() > 0 && std::holds_alternative<int32_t>(args[0])) {
            start = std::get<int32_t>(args[0]);
          }

          size_t deleteCount = mutableThis->value_.size();
          if (args.size() > 1 && std::holds_alternative<int32_t>(args[1])) {
            deleteCount = std::max(0, std::get<int32_t>(args[1]));
          }

          if (start < 0) start = std::max(0, static_cast<int32_t>(mutableThis->value_.size()) + start);
          size_t startIndex = std::min(static_cast<size_t>(start), mutableThis->value_.size());
          deleteCount = std::min(deleteCount, mutableThis->value_.size() - startIndex);

          // 创建被删除的元素数组
          auto deletedArray = utils::createArray();
          for (size_t i = 0; i < deleteCount; ++i) {
            deletedArray->getValue().push_back(mutableThis->value_[startIndex + i]);
          }

          // 删除元素
          mutableThis->value_.erase(mutableThis->value_.begin() + startIndex,
            mutableThis->value_.begin() + startIndex + deleteCount);

          // 插入新元素
          if (args.size() > 2) {
            mutableThis->value_.insert(mutableThis->value_.begin() + startIndex,
              args.begin() + 2, args.end());
          }

          mutableThis->updateLength();
          return deletedArray;
        });
      return spliceFunc;
    }
    else if (name == "slice") {
      auto sliceFunc = std::make_shared<JFunction>("slice",
        [this](const std::vector<ValueVariant>& args) -> ValueVariant {
          int32_t start = 0;
          int32_t end = static_cast<int32_t>(value_.size());

          if (args.size() > 0 && std::holds_alternative<int32_t>(args[0])) {
            start = std::get<int32_t>(args[0]);
          }
          if (args.size() > 1 && std::holds_alternative<int32_t>(args[1])) {
            end = std::get<int32_t>(args[1]);
          }

          if (start < 0) start = std::max(0, static_cast<int32_t>(value_.size()) + start);
          if (end < 0) end = std::max(0, static_cast<int32_t>(value_.size()) + end);

          size_t startIndex = std::min(static_cast<size_t>(start), value_.size());
          size_t endIndex = std::min(static_cast<size_t>(end), value_.size());

          if (startIndex >= endIndex) return utils::createArray();

          std::vector<ValueVariant> sliced(value_.begin() + startIndex, value_.begin() + endIndex);
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

  JFunction::JFunction(const std::string& name, FunctionType func)
    : name_(name), function_(func) {
    initializeFunctionProperties();
  }

  void JFunction::initializeFunctionProperties() {
    // name属性
    DEF_PROP_EX(*this, "name",
      [this]() -> ValueVariant {
        return std::make_shared<JString>(name_);
      },
      nullptr, false, false, true);

    // length属性（参数个数，这里简化为0）
    DEF_PROP_VAL(*this, "length", static_cast<uint32_t>(0), false, false, true);

    // call方法将在getPropertyInternal中按需创建，避免递归
  }

  ValueVariant JFunction::Call(const std::vector<ValueVariant>& args) {
    if (function_) {
      return function_(args);
    }
    return nullptr;
  }

  std::string JFunction::toString() const {
    return "function " + name_ + "() { [native code] }";
  }

  void JFunction::setName(const std::string& name) {
    name_ = name;
  }

  ValueVariant JFunction::getPropertyInternal(const std::string& name) const {
    // 处理JFunction特有的方法
    if (name == "call") {
      auto callFunc = std::make_shared<JFunction>("call",
        [this](const std::vector<ValueVariant>& args) -> ValueVariant {
          return const_cast<JFunction*>(this)->Call(args);
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

  JDate::JDate(const std::chrono::system_clock::time_point& time) : time_(time) {
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
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
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

  ValueVariant JDate::getPropertyInternal(const std::string& name) const {
    // 处理JDate特有的方法
    if (name == "getTime") {
      auto getTimeFunc = std::make_shared<JFunction>("getTime",
        [this](const std::vector<ValueVariant>&) -> ValueVariant {
          return static_cast<uint64_t>(this->getTime());
        });
      return getTimeFunc;
    }
    else if (name == "setTime") {
      auto setTimeFunc = std::make_shared<JFunction>("setTime",
        [this](const std::vector<ValueVariant>& args) -> ValueVariant {
          auto* mutableThis = const_cast<JDate*>(this);
          if (!args.empty()) {
            if (std::holds_alternative<uint64_t>(args[0])) {
              mutableThis->setTime(std::get<uint64_t>(args[0]));
            }
            else if (std::holds_alternative<int32_t>(args[0])) {
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
  // 工具函数实现
  // =======================

  namespace utils {

    ValueType getValueType(const ValueVariant& value) {
      return std::visit([](const auto& v) -> ValueType {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
          return ValueType::Null;
        }
        else if constexpr (std::is_same_v<T, bool>) {
          return ValueType::Boolean;
        }
        else if constexpr (std::is_same_v<T, int32_t>) {
          return ValueType::Int32;
        }
        else if constexpr (std::is_same_v<T, uint32_t>) {
          return ValueType::UInt32;
        }
        else if constexpr (std::is_same_v<T, uint64_t>) {
          return ValueType::UInt64;
        }
        else if constexpr (std::is_same_v<T, double>) {
          return ValueType::Double;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<JString>>) {
          return ValueType::String;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<JArray>>) {
          return ValueType::Array;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<JObject>>) {
          return ValueType::Object;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<JFunction>>) {
          return ValueType::Function;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<JDate>>) {
          return ValueType::Date;
        }
        return ValueType::Null;
        }, value);
    }

    std::string valueToString(const ValueVariant& value) {
      return std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
          return "null";
        }
        else if constexpr (std::is_same_v<T, bool>) {
          return v ? "true" : "false";
        }
        else if constexpr (std::is_same_v<T, int32_t>) {
          return std::to_string(v);
        }
        else if constexpr (std::is_same_v<T, uint32_t>) {
          return std::to_string(v);
        }
        else if constexpr (std::is_same_v<T, uint64_t>) {
          return std::to_string(v);
        }
        else if constexpr (std::is_same_v<T, double>) {
          return std::to_string(v);
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<JString>>) {
          return v ? v->toString() : "null";
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<JArray>>) {
          return v ? v->toString() : "null";
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<JObject>>) {
          return v ? v->toString() : "null";
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<JFunction>>) {
          return v ? v->toString() : "null";
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<JDate>>) {
          return v ? v->toString() : "null";
        }
        return "undefined";
        }, value);
    }

    bool isNumber(const ValueVariant& value) {
      return std::visit([](const auto& v) -> bool {
        using T = std::decay_t<decltype(v)>;
        return std::is_same_v<T, int32_t> ||
          std::is_same_v<T, uint32_t> ||
          std::is_same_v<T, uint64_t> ||
          std::is_same_v<T, double>;
        }, value);
    }

    double toNumber(const ValueVariant& value) {
      return std::visit([](const auto& v) -> double {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
          return 0.0;
        }
        else if constexpr (std::is_same_v<T, bool>) {
          return v ? 1.0 : 0.0;
        }
        else if constexpr (std::is_same_v<T, int32_t>) {
          return static_cast<double>(v);
        }
        else if constexpr (std::is_same_v<T, uint32_t>) {
          return static_cast<double>(v);
        }
        else if constexpr (std::is_same_v<T, uint64_t>) {
          return static_cast<double>(v);
        }
        else if constexpr (std::is_same_v<T, double>) {
          return v;
        }
        else {
          return std::nan("");
        }
        }, value);
    }

    bool toBoolean(const ValueVariant& value) {
      return std::visit([](const auto& v) -> bool {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
          return false;
        }
        else if constexpr (std::is_same_v<T, bool>) {
          return v;
        }
        else if constexpr (std::is_same_v<T, int32_t>) {
          return v != 0;
        }
        else if constexpr (std::is_same_v<T, uint32_t>) {
          return v != 0;
        }
        else if constexpr (std::is_same_v<T, uint64_t>) {
          return v != 0;
        }
        else if constexpr (std::is_same_v<T, double>) {
          return v != 0.0 && !std::isnan(v);
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<JString>>) {
          return v && !v->Empty();
        }
        else {
          return v != nullptr;
        }
        }, value);
    }

    std::shared_ptr<JObject> createObject() {
      return std::make_shared<JObject>();
    }

    std::shared_ptr<JString> createString(const std::string& str) {
      return std::make_shared<JString>(str);
    }

    std::shared_ptr<JArray> createArray(size_t size) {
      return std::make_shared<JArray>(size);
    }

    std::shared_ptr<JFunction> createFunction(const std::string& name, JFunction::FunctionType func) {
      return std::make_shared<JFunction>(name, func);
    }

    std::shared_ptr<JDate> createDate() {
      return std::make_shared<JDate>();
    }

  } // namespace utils

} // namespace jobject
