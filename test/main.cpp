#include "../src/JObject.h"
#include <iostream>
#include <cassert>

using namespace jobject;
using namespace jobject::utils;

void testBasicTypes() {
    std::cout << "=== 测试基本类型 ===" << std::endl;
    
    // 测试null
    ValueVariant nullValue = nullptr;
    std::cout << "null: " << valueToString(nullValue) << std::endl;
    
    // 测试boolean
    ValueVariant boolValue = true;
    std::cout << "boolean: " << valueToString(boolValue) << std::endl;
    
    // 测试整数
    ValueVariant intValue = static_cast<int32_t>(42);
    std::cout << "int32: " << valueToString(intValue) << std::endl;
    
    // 测试浮点数
    ValueVariant doubleValue = 3.14159;
    std::cout << "double: " << valueToString(doubleValue) << std::endl;
}

void testString() {
    std::cout << "\n=== 测试字符串 ===" << std::endl;
    
    auto str = createString("Hello World");
    std::cout << "字符串: " << str->toString() << std::endl;
    std::cout << "长度: " << valueToString(str->getProperty("length")) << std::endl;
    
    // 测试concat
    auto concatFunc = std::get<std::shared_ptr<JFunction>>(str->getProperty("concat"));
    auto result = concatFunc->Call({createString(" - 测试")});
    auto resultStr = std::get<std::shared_ptr<JString>>(result);
    std::cout << "concat结果: " << resultStr->toString() << std::endl;
    
    // 测试indexOf
    auto indexOfFunc = std::get<std::shared_ptr<JFunction>>(str->getProperty("indexOf"));
    auto indexResult = indexOfFunc->Call({createString("World")});
    std::cout << "indexOf 'World': " << valueToString(indexResult) << std::endl;
}

void testArray() {
    std::cout << "\n=== 测试数组 ===" << std::endl;
    
    auto arr = createArray();
    std::cout << "空数组长度: " << valueToString(arr->getProperty("length")) << std::endl;
    
    // 测试push
    auto pushFunc = std::get<std::shared_ptr<JFunction>>(arr->getProperty("push"));
    pushFunc->Call({static_cast<int32_t>(1), static_cast<int32_t>(2), static_cast<int32_t>(3)});
    std::cout << "push后长度: " << valueToString(arr->getProperty("length")) << std::endl;
    std::cout << "数组内容: " << arr->toString() << std::endl;
    
    // 测试访问元素
    std::cout << "arr[0]: " << valueToString(arr->At(0)) << std::endl;
    std::cout << "arr[1]: " << valueToString(arr->At(1)) << std::endl;
    
    // 测试pop
    auto popFunc = std::get<std::shared_ptr<JFunction>>(arr->getProperty("pop"));
    auto poppedValue = popFunc->Call({});
    std::cout << "pop的值: " << valueToString(poppedValue) << std::endl;
    std::cout << "pop后数组: " << arr->toString() << std::endl;
}

void testObject() {
    std::cout << "\n=== 测试对象 ===" << std::endl;
    
    auto obj = createObject();
    
    // 设置属性
    obj->setProperty("name", createString("测试对象"));
    obj->setProperty("age", static_cast<int32_t>(25));
    obj->setProperty("active", true);
    
    std::cout << "name: " << valueToString(obj->getProperty("name")) << std::endl;
    std::cout << "age: " << valueToString(obj->getProperty("age")) << std::endl;
    std::cout << "active: " << valueToString(obj->getProperty("active")) << std::endl;
    
    // 测试operator[]
    std::cout << "obj[\"name\"]: " << valueToString(obj->operator[]("name")) << std::endl;
    
    // 测试属性枚举
    auto propNames = obj->getPropertyNames();
    std::cout << "可枚举属性: ";
    for (const auto& name : propNames) {
        std::cout << name << " ";
    }
    std::cout << std::endl;
}

void testFunction() {
    std::cout << "\n=== 测试函数 ===" << std::endl;
    
    auto addFunc = createFunction("add", [](const std::vector<ValueVariant>& args) -> ValueVariant {
        if (args.size() >= 2) {
            double a = toNumber(args[0]);
            double b = toNumber(args[1]);
            return a + b;
        }
        return 0.0;
    });
    
    std::cout << "函数名: " << valueToString(addFunc->getProperty("name")) << std::endl;
    std::cout << "函数字符串: " << addFunc->toString() << std::endl;
    
    // 调用函数
    auto result = addFunc->Call({static_cast<int32_t>(10), static_cast<int32_t>(20)});
    std::cout << "add(10, 20) = " << valueToString(result) << std::endl;
}

void testDate() {
    std::cout << "\n=== 测试日期 ===" << std::endl;
    
    auto date = createDate();
    std::cout << "当前时间: " << date->toString() << std::endl;
    
    // 获取时间戳
    auto getTimeFunc = std::get<std::shared_ptr<JFunction>>(date->getProperty("getTime"));
    auto timestamp = getTimeFunc->Call({});
    std::cout << "时间戳: " << valueToString(timestamp) << std::endl;
}

void testPropertyDescriptor() {
    std::cout << "\n=== 测试属性描述符 ===" << std::endl;
    
    auto obj = createObject();
    
    // 定义一个带getter/setter的属性
    PropertyDescriptor descriptor;
    descriptor.getter = []() -> ValueVariant {
        return createString("这是一个getter属性");
    };
    descriptor.setter = [](const ValueVariant& value) {
        std::cout << "setter被调用，值: " << valueToString(value) << std::endl;
    };
    descriptor.enumerable = true;
    descriptor.configurable = true;
    
    obj->defineProperty("special", descriptor);
    
    std::cout << "getter属性值: " << valueToString(obj->getProperty("special")) << std::endl;
    
    // 测试setter
    obj->setProperty("special", createString("新值"));
    
    // 测试不可写属性
    PropertyDescriptor readOnlyDesc;
    readOnlyDesc.value = createString("只读属性");
    readOnlyDesc.writable = false;
    readOnlyDesc.enumerable = true;
    readOnlyDesc.configurable = false;
    
    obj->defineProperty("readonly", readOnlyDesc);
    std::cout << "只读属性: " << valueToString(obj->getProperty("readonly")) << std::endl;
    
    // 尝试修改只读属性
    bool success = obj->setProperty("readonly", createString("尝试修改"));
    std::cout << "修改只读属性" << (success ? "成功" : "失败") << std::endl;
}

int main() {
    std::cout << "JObject 系统测试程序" << std::endl;
    std::cout << "===================" << std::endl;
    
    try {
        testBasicTypes();
        testString();
        testArray();
        testObject();
        testFunction();
        testDate();
        testPropertyDescriptor();
        
        std::cout << "\n所有测试完成！" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "测试出错: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 