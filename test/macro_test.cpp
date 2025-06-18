#include "../src/JObject.h"
#include <iostream>

using namespace jobject;

class TestObject : public JObject {
private:
    int value1_ = 10;
    int value2_ = 20;
    std::string name_ = "test";

public:
    void setupProperties() {
        // 方法1：使用括号包围带逗号的 lambda
        DEF_PROP_EX(*this, "method1", 
            ([this, &value1 = value1_, &value2 = value2_]() -> ValueVariant {
                return static_cast<int32_t>(value1 + value2);
            }), 
            nullptr, true, true, true);

        // 方法2：使用类型别名
        auto getter = [this, &val1 = value1_, &val2 = value2_]() -> ValueVariant {
            return static_cast<int32_t>(val1 * val2);
        };
        auto setter = [this, &val1 = value1_](const ValueVariant& v) {
            if (std::holds_alternative<int32_t>(v)) {
                val1 = std::get<int32_t>(v);
            }
        };
        DEF_PROP_EX(*this, "method2", getter, setter, true, true, true);

        // 方法3：使用新的可变参数宏
        DEF_PROP_EX_VA(*this, "method3", 
            [this, &name = name_]() -> ValueVariant {
                return utils::createString(name + "_processed");
            },
            [this, &name = name_](const ValueVariant& v) {
                if (std::holds_alternative<std::shared_ptr<JString>>(v)) {
                    auto str = std::get<std::shared_ptr<JString>>(v);
                    name = str->getValue();
                }
            },
            true, true, true);
    }

    void testProperties() {
        std::cout << "测试属性访问：\n";
        
        // 测试 method1
        auto result1 = getProperty("method1");
        if (std::holds_alternative<int32_t>(result1)) {
            std::cout << "method1 = " << std::get<int32_t>(result1) << std::endl;
        }

        // 测试 method2
        auto result2 = getProperty("method2");
        if (std::holds_alternative<int32_t>(result2)) {
            std::cout << "method2 = " << std::get<int32_t>(result2) << std::endl;
        }

        // 设置新值
        setProperty("method2", static_cast<int32_t>(5));
        result2 = getProperty("method2");
        if (std::holds_alternative<int32_t>(result2)) {
            std::cout << "method2 (after set) = " << std::get<int32_t>(result2) << std::endl;
        }

        // 测试 method3
        auto result3 = getProperty("method3");
        if (std::holds_alternative<std::shared_ptr<JString>>(result3)) {
            auto str = std::get<std::shared_ptr<JString>>(result3);
            std::cout << "method3 = " << str->toString() << std::endl;
        }
    }
};

int main() {
    std::cout << "=== 宏中 Lambda 逗号问题解决方案测试 ===\n\n";
    
    TestObject obj;
    obj.setupProperties();
    obj.testProperties();
    
    std::cout << "\n=== 所有测试完成 ===\n";
    return 0;
} 