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
    // toString方法
    auto toStringFunc = std::make_shared<JFunction>("toString", 
        [this](const std::vector<ValueVariant>&) -> ValueVariant {
            return std::make_shared<JString>(this->toString());
        });
    
    PropertyDescriptor toStringDesc;
    toStringDesc.value = toStringFunc;
    toStringDesc.writable = true;
    toStringDesc.enumerable = false;
    toStringDesc.configurable = true;
    defineProperty("toString", toStringDesc);
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
    } else {
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
    PropertyDescriptor lengthDesc;
    lengthDesc.getter = [this]() -> ValueVariant {
        return static_cast<uint32_t>(value_.length());
    };
    lengthDesc.writable = false;
    lengthDesc.enumerable = false;
    lengthDesc.configurable = false;
    defineProperty("length", lengthDesc);

    // concat方法
    auto concatFunc = std::make_shared<JFunction>("concat",
        [this](const std::vector<ValueVariant>& args) -> ValueVariant {
            std::string result = value_;
            for (const auto& arg : args) {
                result += utils::valueToString(arg);
            }
            return std::make_shared<JString>(result);
        });
    PropertyDescriptor concatDesc;
    concatDesc.value = concatFunc;
    concatDesc.writable = true;
    concatDesc.enumerable = false;
    concatDesc.configurable = true;
    defineProperty("concat", concatDesc);

    // indexOf方法
    auto indexOfFunc = std::make_shared<JFunction>("indexOf",
        [this](const std::vector<ValueVariant>& args) -> ValueVariant {
            if (args.empty()) return static_cast<int32_t>(-1);
            std::string searchStr = utils::valueToString(args[0]);
            size_t pos = value_.find(searchStr);
            return pos == std::string::npos ? static_cast<int32_t>(-1) : static_cast<int32_t>(pos);
        });
    PropertyDescriptor indexOfDesc;
    indexOfDesc.value = indexOfFunc;
    indexOfDesc.writable = true;
    indexOfDesc.enumerable = false;
    indexOfDesc.configurable = true;
    defineProperty("indexOf", indexOfDesc);

    // lastIndexOf方法
    auto lastIndexOfFunc = std::make_shared<JFunction>("lastIndexOf",
        [this](const std::vector<ValueVariant>& args) -> ValueVariant {
            if (args.empty()) return static_cast<int32_t>(-1);
            std::string searchStr = utils::valueToString(args[0]);
            size_t pos = value_.rfind(searchStr);
            return pos == std::string::npos ? static_cast<int32_t>(-1) : static_cast<int32_t>(pos);
        });
    PropertyDescriptor lastIndexOfDesc;
    lastIndexOfDesc.value = lastIndexOfFunc;
    lastIndexOfDesc.writable = true;
    lastIndexOfDesc.enumerable = false;
    lastIndexOfDesc.configurable = true;
    defineProperty("lastIndexOf", lastIndexOfDesc);
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
    PropertyDescriptor lengthDesc;
    lengthDesc.getter = [this]() -> ValueVariant {
        return static_cast<uint32_t>(value_.size());
    };
    lengthDesc.setter = [this](const ValueVariant& val) {
        if (std::holds_alternative<uint32_t>(val)) {
            uint32_t newSize = std::get<uint32_t>(val);
            value_.resize(newSize);
        }
    };
    lengthDesc.enumerable = false;
    lengthDesc.configurable = false;
    defineProperty("length", lengthDesc);

    // push方法
    auto pushFunc = std::make_shared<JFunction>("push",
        [this](const std::vector<ValueVariant>& args) -> ValueVariant {
            for (const auto& arg : args) {
                value_.push_back(arg);
            }
            return static_cast<uint32_t>(value_.size());
        });
    PropertyDescriptor pushDesc;
    pushDesc.value = pushFunc;
    pushDesc.writable = true;
    pushDesc.enumerable = false;
    pushDesc.configurable = true;
    defineProperty("push", pushDesc);

    // pop方法
    auto popFunc = std::make_shared<JFunction>("pop",
        [this](const std::vector<ValueVariant>&) -> ValueVariant {
            if (value_.empty()) return nullptr;
            ValueVariant result = value_.back();
            value_.pop_back();
            return result;
        });
    PropertyDescriptor popDesc;
    popDesc.value = popFunc;
    popDesc.writable = true;
    popDesc.enumerable = false;
    popDesc.configurable = true;
    defineProperty("pop", popDesc);

    // shift方法
    auto shiftFunc = std::make_shared<JFunction>("shift",
        [this](const std::vector<ValueVariant>&) -> ValueVariant {
            if (value_.empty()) return nullptr;
            ValueVariant result = value_.front();
            value_.erase(value_.begin());
            return result;
        });
    PropertyDescriptor shiftDesc;
    shiftDesc.value = shiftFunc;
    shiftDesc.writable = true;
    shiftDesc.enumerable = false;
    shiftDesc.configurable = true;
    defineProperty("shift", shiftDesc);

    // unshift方法
    auto unshiftFunc = std::make_shared<JFunction>("unshift",
        [this](const std::vector<ValueVariant>& args) -> ValueVariant {
            value_.insert(value_.begin(), args.begin(), args.end());
            return static_cast<uint32_t>(value_.size());
        });
    PropertyDescriptor unshiftDesc;
    unshiftDesc.value = unshiftFunc;
    unshiftDesc.writable = true;
    unshiftDesc.enumerable = false;
    unshiftDesc.configurable = true;
    defineProperty("unshift", unshiftDesc);

    // splice方法（简化版本）
    auto spliceFunc = std::make_shared<JFunction>("splice",
        [this](const std::vector<ValueVariant>& args) -> ValueVariant {
            if (args.empty()) return utils::createArray();
            
            int32_t start = 0;
            if (args.size() > 0 && std::holds_alternative<int32_t>(args[0])) {
                start = std::get<int32_t>(args[0]);
            }
            
            size_t deleteCount = value_.size();
            if (args.size() > 1 && std::holds_alternative<int32_t>(args[1])) {
                deleteCount = std::max(0, std::get<int32_t>(args[1]));
            }
            
            if (start < 0) start = std::max(0, static_cast<int32_t>(value_.size()) + start);
            size_t startIndex = std::min(static_cast<size_t>(start), value_.size());
            deleteCount = std::min(deleteCount, value_.size() - startIndex);
            
            // 创建被删除的元素数组
            auto deletedArray = utils::createArray();
            for (size_t i = 0; i < deleteCount; ++i) {
                deletedArray->getValue().push_back(value_[startIndex + i]);
            }
            
            // 删除元素
            value_.erase(value_.begin() + startIndex, value_.begin() + startIndex + deleteCount);
            
            // 插入新元素
            if (args.size() > 2) {
                value_.insert(value_.begin() + startIndex, args.begin() + 2, args.end());
            }
            
            return deletedArray;
        });
    PropertyDescriptor spliceDesc;
    spliceDesc.value = spliceFunc;
    spliceDesc.writable = true;
    spliceDesc.enumerable = false;
    spliceDesc.configurable = true;
    defineProperty("splice", spliceDesc);

    // slice方法
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
    PropertyDescriptor sliceDesc;
    sliceDesc.value = sliceFunc;
    sliceDesc.writable = true;
    sliceDesc.enumerable = false;
    sliceDesc.configurable = true;
    defineProperty("slice", sliceDesc);

    updateLength();
}

void JArray::updateLength() {
    // 更新数字索引属性
    for (size_t i = 0; i < value_.size(); ++i) {
        PropertyDescriptor indexDesc;
        indexDesc.getter = [this, i]() -> ValueVariant {
            return i < value_.size() ? value_[i] : ValueVariant(nullptr);
        };
        indexDesc.setter = [this, i](const ValueVariant& val) {
            if (i < value_.size()) {
                value_[i] = val;
            }
        };
        indexDesc.enumerable = true;
        indexDesc.configurable = true;
        defineProperty(std::to_string(i), indexDesc);
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

// =======================
// JFunction 实现
// =======================

JFunction::JFunction(const std::string& name, FunctionType func) 
    : name_(name), function_(func) {
    initializeFunctionProperties();
}

void JFunction::initializeFunctionProperties() {
    // name属性
    PropertyDescriptor nameDesc;
    nameDesc.getter = [this]() -> ValueVariant {
        return std::make_shared<JString>(name_);
    };
    nameDesc.writable = false;
    nameDesc.enumerable = false;
    nameDesc.configurable = true;
    defineProperty("name", nameDesc);

    // length属性（参数个数，这里简化为0）
    PropertyDescriptor lengthDesc;
    lengthDesc.value = static_cast<uint32_t>(0);
    lengthDesc.writable = false;
    lengthDesc.enumerable = false;
    lengthDesc.configurable = true;
    defineProperty("length", lengthDesc);

    // call方法
    auto callFunc = std::make_shared<JFunction>("call",
        [this](const std::vector<ValueVariant>& args) -> ValueVariant {
            return this->Call(args);
        });
    PropertyDescriptor callDesc;
    callDesc.value = callFunc;
    callDesc.writable = true;
    callDesc.enumerable = false;
    callDesc.configurable = true;
    defineProperty("call", callDesc);
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
    // getTime方法
    auto getTimeFunc = std::make_shared<JFunction>("getTime",
        [this](const std::vector<ValueVariant>&) -> ValueVariant {
            return static_cast<uint64_t>(this->getTime());
        });
    PropertyDescriptor getTimeDesc;
    getTimeDesc.value = getTimeFunc;
    getTimeDesc.writable = true;
    getTimeDesc.enumerable = false;
    getTimeDesc.configurable = true;
    defineProperty("getTime", getTimeDesc);

    // setTime方法
    auto setTimeFunc = std::make_shared<JFunction>("setTime",
        [this](const std::vector<ValueVariant>& args) -> ValueVariant {
            if (!args.empty()) {
                if (std::holds_alternative<uint64_t>(args[0])) {
                    this->setTime(std::get<uint64_t>(args[0]));
                } else if (std::holds_alternative<int32_t>(args[0])) {
                    this->setTime(std::get<int32_t>(args[0]));
                }
            }
            return static_cast<uint64_t>(this->getTime());
        });
    PropertyDescriptor setTimeDesc;
    setTimeDesc.value = setTimeFunc;
    setTimeDesc.writable = true;
    setTimeDesc.enumerable = false;
    setTimeDesc.configurable = true;
    defineProperty("setTime", setTimeDesc);
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

// =======================
// 工具函数实现
// =======================

namespace utils {

ValueType getValueType(const ValueVariant& value) {
    return std::visit([](const auto& v) -> ValueType {
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
    }, value);
}

std::string valueToString(const ValueVariant& value) {
    return std::visit([](const auto& v) -> std::string {
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
    }, value);
}

bool toBoolean(const ValueVariant& value) {
    return std::visit([](const auto& v) -> bool {
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
