#include <gtest/gtest.h>

#include <valijson/adapters/rapidyaml_adapter.hpp>
#include <valijson/utils/rapidyaml_utils.hpp>

class TestRymlAdapter : public testing::Test
{
};

TEST_F(TestRymlAdapter, BasicArrayIteration)
{
    const unsigned int numElements = 10;

    // Build a YAML sequence manually using ryml's tree API.
    ryml::Tree document;
    ryml::NodeRef root = document.rootref();
    root |= ryml::SEQ;
    for (unsigned int i = 0; i < numElements; i++) {
        ryml::NodeRef child = root.append_child();
        child << static_cast<double>(i);
    }

    valijson::adapters::RymlAdapter adapter(document.rootref());

#if VALIJSON_USE_EXCEPTIONS
    ASSERT_NO_THROW(adapter.getArray());
    ASSERT_ANY_THROW(adapter.getBool());
    ASSERT_ANY_THROW(adapter.getDouble());
    ASSERT_ANY_THROW(adapter.getObject());
    ASSERT_ANY_THROW(adapter.getString());
#endif

    // Ensure that the array contains the expected number of elements.
    EXPECT_EQ(numElements, adapter.getArray().size());

    // Ensure that the elements are returned in the order they were inserted.
    unsigned int expectedValue = 0;
    for (const valijson::adapters::RymlAdapter value : adapter.getArray()) {
        ASSERT_TRUE(value.isString());
        ASSERT_FALSE(value.isNumber());
        ASSERT_TRUE(value.maybeDouble());
        EXPECT_EQ(double(expectedValue), value.getDouble());
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over.
    EXPECT_EQ(numElements, expectedValue);
}

TEST_F(TestRymlAdapter, BasicObjectIteration)
{
    const unsigned int numElements = 10;

    // Build a YAML map manually using ryml's tree API.
    ryml::Tree document;
    ryml::NodeRef root = document.rootref();
    root |= ryml::MAP;
    for (unsigned int i = 0; i < numElements; i++) {
        std::string key = std::to_string(i);
        ryml::csubstr keyView = ryml::to_csubstr(key);
        ryml::NodeRef child = root.append_child();
        child << ryml::key(keyView);
        child << static_cast<double>(i);
    }

    valijson::adapters::RymlAdapter adapter(document.rootref());

#if VALIJSON_USE_EXCEPTIONS
    ASSERT_NO_THROW(adapter.getObject());
    ASSERT_ANY_THROW(adapter.getArray());
    ASSERT_ANY_THROW(adapter.getBool());
    ASSERT_ANY_THROW(adapter.getDouble());
    ASSERT_ANY_THROW(adapter.getString());
#endif

    // Ensure that the object contains the expected number of members.
    EXPECT_EQ(numElements, adapter.getObject().size());

    // Ensure that the members are returned in the order they were inserted.
    unsigned int expectedValue = 0;
    for (const valijson::adapters::RymlAdapter::ObjectMember member :
         adapter.getObject()) {
        ASSERT_TRUE(member.second.isString());
        ASSERT_FALSE(member.second.isNumber());
        ASSERT_TRUE(member.second.maybeDouble());
        EXPECT_EQ(std::to_string(expectedValue), member.first);
        EXPECT_EQ(double(expectedValue), member.second.getDouble());
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over.
    EXPECT_EQ(numElements, expectedValue);
}

TEST_F(TestRymlAdapter, BasicObjectMemberAccess)
{
    const unsigned int numElements = 10;

    ryml::Tree document;
    ryml::NodeRef root = document.rootref();
    root |= ryml::MAP;
    for (unsigned int i = 0; i < numElements; i++) {
        std::string key = std::to_string(i);
        ryml::csubstr keyView = ryml::to_csubstr(key);
        ryml::NodeRef child = root.append_child();
        child << ryml::key(keyView);
        child << static_cast<double>(i);
    }

    valijson::adapters::RymlAdapter adapter(document.rootref());
    const auto adapterObject = adapter.asObject();

    // Ensure that accessing an element that exists produces the expected result.
    const auto result3 = adapterObject.find("3");
    EXPECT_NE(result3, adapterObject.end());
    EXPECT_EQ(result3->second.asDouble(), 3);

    // Ensure that accessing an element that does not exist returns end().
    const auto result12 = adapterObject.find("12");
    EXPECT_EQ(result12, adapterObject.end());
}

TEST_F(TestRymlAdapter, NullValue)
{
    // Parse YAML with a null scalar value.
    ryml::Tree document = ryml::parse_in_arena(ryml::to_csubstr("~"));
    valijson::adapters::RymlAdapter adapterTilde(
        valijson::utils::getDocumentNode(document));
    EXPECT_TRUE(adapterTilde.isNull());
    EXPECT_FALSE(adapterTilde.isString());

    ryml::Tree document2 = ryml::parse_in_arena(ryml::to_csubstr("null"));
    valijson::adapters::RymlAdapter adapterNull(
        valijson::utils::getDocumentNode(document2));
    EXPECT_TRUE(adapterNull.isNull());
    EXPECT_FALSE(adapterNull.isString());
}

TEST_F(TestRymlAdapter, BoolValue)
{
    // Parse YAML booleans and verify maybeBool() and asBool().
    ryml::Tree docTrue = ryml::parse_in_arena(ryml::to_csubstr("true"));
    valijson::adapters::RymlAdapter adapterTrue(
        valijson::utils::getDocumentNode(docTrue));
    ASSERT_TRUE(adapterTrue.maybeBool());
    EXPECT_TRUE(adapterTrue.asBool());

    ryml::Tree docFalse = ryml::parse_in_arena(ryml::to_csubstr("false"));
    valijson::adapters::RymlAdapter adapterFalse(
        valijson::utils::getDocumentNode(docFalse));
    ASSERT_TRUE(adapterFalse.maybeBool());
    EXPECT_FALSE(adapterFalse.asBool());
}
