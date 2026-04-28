#include <gtest/gtest.h>

#include <valijson/adapters/rapidyaml_adapter.hpp>

class TestRapidYAMLAdapter : public testing::Test
{
};

TEST_F(TestRapidYAMLAdapter, BasicArrayIteration)
{
    const unsigned int numElements = 10;

    // Create a Json document that consists of an array of numbers
    ryml::Tree document;
    auto root = document.rootref();

    root |= ryml::SEQ; 

    for (unsigned int i = 0; i < numElements; i++) {
        root.append_child() << static_cast<double>(i);
    }

    // Ensure that wrapping the document preserves the array and does not allow
    // it to be cast to other types
    valijson::adapters::RapidYAMLAdapter adapter(document);
#if VALIJSON_USE_EXCEPTIONS
    ASSERT_NO_THROW(adapter.getArray());
    ASSERT_ANY_THROW(adapter.getBool());
    ASSERT_ANY_THROW(adapter.getDouble());
    ASSERT_ANY_THROW(adapter.getObject());
    ASSERT_ANY_THROW(adapter.getString());
#endif

    // Ensure that the array contains the expected number of elements
    EXPECT_EQ(numElements, adapter.getArray().size());

    // Ensure that the elements are returned in the order they were inserted
    unsigned int expectedValue = 0;
    for (const valijson::adapters::RapidYAMLAdapter value : adapter.getArray()) {
        ASSERT_TRUE(value.isString());
        ASSERT_TRUE(value.isNumber());
        ASSERT_TRUE(value.maybeDouble());
        EXPECT_EQ(double(expectedValue), value.getDouble());
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ(numElements, expectedValue);
}

TEST_F(TestRapidYAMLAdapter, BasicObjectIteration)
{
    const unsigned int numElements = 10;

    // Create a document that consists of an object that maps
    // numeric strings their corresponding numeric values
    ryml::Tree document;
    auto root = document.rootref();
    root |= ryml::MAP; 

    for (uint32_t i = 0; i < numElements; i++) {
        std::string key = std::to_string(i);
        root.append_child() << ryml::key(key) << static_cast<double>(i);
    }

    // allow it to be cast to other types
    valijson::adapters::RapidYAMLAdapter adapter(document);
#if VALIJSON_USE_EXCEPTIONS
    ASSERT_NO_THROW(adapter.getObject());
    ASSERT_ANY_THROW(adapter.getArray());
    ASSERT_ANY_THROW(adapter.getBool());
    ASSERT_ANY_THROW(adapter.getDouble());
    ASSERT_ANY_THROW(adapter.getString());
#endif

    // Ensure that the object contains the expected number of members
    EXPECT_EQ(numElements, adapter.getObject().size());

    // Ensure that the members are returned in the order they were inserted
    unsigned int expectedValue = 0;
    for (const valijson::adapters::RapidYAMLAdapter::ObjectMember member :
         adapter.getObject()) {
        ASSERT_TRUE(member.second.isString());
        ASSERT_TRUE(member.second.isNumber());
        ASSERT_TRUE(member.second.maybeDouble());
        EXPECT_EQ(std::to_string(expectedValue), member.first);
        EXPECT_EQ(double(expectedValue), member.second.getDouble());
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ(numElements, expectedValue);
}

TEST_F(TestRapidYAMLAdapter, BasicObjectMemberAccess)
{
    const unsigned int numElements = 10;

    // Create a document that consists of an object that maps
    // numeric strings their corresponding numeric values
    ryml::Tree document;
    auto root = document.rootref();
    root |= ryml::MAP;

    for (uint32_t i = 0; i < numElements; i++) {
        std::string key = std::to_string(i);
        root.append_child() << ryml::key(key) << static_cast<double>(i);
    }
    valijson::adapters::RapidYAMLAdapter adapter(document);
    const auto adapterObject = adapter.asObject();

    // Ensure that accessing an element that exists produces the expected result.
    const auto result3 = adapterObject.find("3");
    EXPECT_NE(result3, adapterObject.end());
    EXPECT_EQ(result3->second.asDouble(), 3);

    // Ensure that accessing an element that does not exists.
    const auto result12 = adapterObject.find("12");
    EXPECT_EQ(result12, adapterObject.end());
}
