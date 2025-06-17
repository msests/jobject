#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <variant>
#include <chrono>
#include <optional>

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
using ValueVariant = std::variant<
    std::nullptr_t,                           // Null
    bool,                                     // Boolean
    int32_t,                                  // Int32
    uint32_t,                                 // UInt32
    uint64_t,                                 // UInt64
    double,                                   // Double
    std::shared_ptr<JString>,                 // String
    std::shared_ptr<JArray>,                  // Array
    std::shared_ptr<JObject>,                 // Object
    std::shared_ptr<JFunction>,               // Function
    std::shared_ptr<JDate>                    // Date
>;

// 属性描述符
struct PropertyDescriptor {
    ValueVariant value;
    bool writable = true;
    bool enumerable = true;
    bool configurable = true;
    std::function<ValueVariant()> getter = nullptr;
    std::function<void(const ValueVariant&)> setter = nullptr;
};

// 基础对象类
class JObject {
public:
    JObject();
    virtual ~JObject() = default;

    // 属性管理
    virtual bool defineProperty(const std::string& name, const PropertyDescriptor& descriptor);
    virtual bool deleteProperty(const std::string& name);
    virtual bool hasProperty(const std::string& name) const;
    virtual std::vector<std::string> getPropertyNames() const;

    // 属性访问
    virtual ValueVariant getProperty(const std::string& name) const;
    virtual bool setProperty(const std::string& name, const ValueVariant& value);

    // 操作符重载
    ValueVariant operator[](const std::string& name) const;
    ValueVariant operator[](const char* name) const;
    ValueVariant operator[](int index) const;
    ValueVariant operator[](size_t index) const;

    // 设置属性的辅助方法
    void setPropertyValue(const std::string& name, const ValueVariant& value);
    void setPropertyValue(int index, const ValueVariant& value);
    void setPropertyValue(size_t index, const ValueVariant& value);

    // 数据上下文
    void* data = nullptr;

    // 类型相关
    virtual ValueType getType() const { return ValueType::Object; }
    virtual std::string toString() const;

protected:
    std::unordered_map<std::string, PropertyDescriptor> properties_;
    void initializeCommonProperties();

private:
    // 内部属性访问辅助方法
    ValueVariant getPropertyInternal(const std::string& name) const;
};

// 字符串类
class JString : public JObject {
public:
    JString(const std::string& str = "");
    JString(const char* str);

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
    const std::string& getValue() const { return value_; }
    void setValue(const std::string& value);

private:
    std::string value_;
    void initializeStringProperties();
};

// 数组类
class JArray : public JObject {
public:
    JArray(size_t size = 0);
    JArray(const std::vector<ValueVariant>& values);

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
    void setElement(size_t index, const ValueVariant& value);

    // 获取底层数组
    const std::vector<ValueVariant>& getValue() const { return value_; }
    std::vector<ValueVariant>& getValue() { return value_; }

private:
    std::vector<ValueVariant> value_;
    void initializeArrayProperties();
    void updateLength();
};

// 函数类
class JFunction : public JObject {
public:
    using FunctionType = std::function<ValueVariant(const std::vector<ValueVariant>&)>;
    
    JFunction(const std::string& name = "", FunctionType func = nullptr);

    // C++方法
    ValueVariant Call(const std::vector<ValueVariant>& args);

    // 重写基类方法
    ValueType getType() const override { return ValueType::Function; }
    std::string toString() const override;

    // 获取函数信息
    const std::string& getName() const { return name_; }
    void setName(const std::string& name);

private:
    std::string name_;
    FunctionType function_;
    void initializeFunctionProperties();
};

// 日期类
class JDate : public JObject {
public:
    JDate();
    JDate(const std::chrono::system_clock::time_point& time);
    JDate(int64_t timestamp); // 毫秒时间戳

    // 重写基类方法
    ValueType getType() const override { return ValueType::Date; }
    std::string toString() const override;

    // 日期相关方法
    int64_t getTime() const; // 获取毫秒时间戳
    void setTime(int64_t timestamp);

private:
    std::chrono::system_clock::time_point time_;
    void initializeDateProperties();
};

// 工具函数
namespace utils {
    ValueType getValueType(const ValueVariant& value);
    std::string valueToString(const ValueVariant& value);
    bool isNumber(const ValueVariant& value);
    double toNumber(const ValueVariant& value);
    bool toBoolean(const ValueVariant& value);
    
    // 创建不同类型的值
    std::shared_ptr<JObject> createObject();
    std::shared_ptr<JString> createString(const std::string& str = "");
    std::shared_ptr<JArray> createArray(size_t size = 0);
    std::shared_ptr<JFunction> createFunction(const std::string& name = "", 
                                            JFunction::FunctionType func = nullptr);
    std::shared_ptr<JDate> createDate();
}

} // namespace jobject
