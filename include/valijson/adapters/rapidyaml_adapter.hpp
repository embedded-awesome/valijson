/**
 * @file
 *
 * @brief   Adapter implementation for the RapidYAML parser library.
 *
 * Include this file in your program to enable support for RapidYAML.
 *
 * This file defines the following classes (not in this order):
 *  - RapidYAMLAdapter
 *  - RapidYAMLArray
 *  - RapidYAMLArrayValueIterator
 *  - RapidYAMLFrozenValue
 *  - RapidYAMLObject
 *  - RapidYAMLObjectMember
 *  - RapidYAMLObjectMemberIterator
 *  - RapidYAMLValue
 *
 * Due to the dependencies that exist between these classes, the ordering of
 * class declarations and definitions may be a bit confusing. The best place to
 * start is RapidYAMLAdapter. This class definition is actually very small,
 * since most of the functionality is inherited from the BasicAdapter class.
 * Most of the classes in this file are provided as template arguments to the
 * inherited BasicAdapter class.
 */

#pragma once

#include <memory>
#if defined(__cpp_lib_optional) || __cplusplus >= 201703L
#include <optional>
#else
#include <experimental/optional>
#endif
#include <ryml.hpp>
#include <ryml_std.hpp>
#include <string>

#include <utility>
#include <valijson/exceptions.hpp>
#include <valijson/internal/adapter.hpp>
#include <valijson/internal/basic_adapter.hpp>
#include <valijson/internal/frozen_value.hpp>

namespace valijson {
namespace adapters {

#if defined(__cpp_lib_optional) || __cplusplus >= 201703L
template <typename T> using Optional = std::optional<T>;
using std::make_optional;
#else
template <typename T> using Optional = std::experimental::optional<T>;
using std::experimental::make_optional;
#endif

class RapidYAMLAdapter;
class RapidYAMLArrayValueIterator;
class RapidYAMLObjectMemberIterator;

typedef std::pair<std::string, RapidYAMLAdapter> RapidYAMLObjectMember;

/**
 * @brief  Light weight wrapper for a RapidYAML array value.
 *
 * This class is light weight wrapper for a RapidYAML array. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * RapidYAML value, assumed to be a sequence, so there is very little overhead
 * associated with copy construction and passing by value.
 */
class RapidYAMLArray
{
  public:
    typedef RapidYAMLArrayValueIterator const_iterator;
    typedef RapidYAMLArrayValueIterator iterator;

    /// Construct a RapidYAMLArray referencing an empty array.
    RapidYAMLArray() : m_node(emptyArray) {}

    /**
     * @brief   Construct a RapidYAMLArray referencing a specific
     * RapidYAML value.
     *
     * @param   node   reference to a RapidYAML node
     *
     * Note that this constructor will throw an exception if the value is not
     * a sequence.
     */
    RapidYAMLArray(ryml::ConstNodeRef node) : m_node(node)
    {
        if (!node.is_seq()) {
            throwRuntimeError("Value is not an array.");
        }
    }

    /**
     * @brief   Return an iterator for the first element of the array.
     *
     * The iterator returned by this function is effectively the iterator
     * returned by the underlying RapidYAML implementation.
     */
    RapidYAMLArrayValueIterator begin() const;

    /**
     * @brief   Return an iterator for one-past the last element of the array.
     *
     * The iterator returned by this function is effectively the iterator
     * returned by the underlying RapidYAML implementation.
     */
    RapidYAMLArrayValueIterator end() const;

    /// Return the number of elements in the array
    size_t size() const
    {
        return m_node.num_children();
    }

  private:
    /**
     * @brief   Return a reference to a RapidYAML tree that is an empty
     * sequence.
     *
     * Note that the value returned by this function is a singleton.
     */

    static inline ryml::Tree emptyArrayTree;
    static inline ryml::ConstNodeRef emptyArray = [] {
        static std::string emptyArrayStr = R"(
            []
          )";
        if (emptyArrayTree.empty()) {
            emptyArrayTree =
                ryml::parse_in_place(ryml::to_substr(emptyArrayStr));
        }
        return emptyArrayTree.rootref();
    }();

    /// Reference to the contained value
    ryml::ConstNodeRef m_node;
};

/**
 * @brief  Light weight wrapper for a RapidYAML object.
 *
 * This class is light weight wrapper for a RapidYAML object. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * RapidYAML value, assumed to be a map, so there is very little overhead
 * associated with copy construction and passing by value.
 */
class RapidYAMLObject
{
  public:
    typedef RapidYAMLObjectMemberIterator const_iterator;
    typedef RapidYAMLObjectMemberIterator iterator;

    /// Construct a RapidYAMLObject referencing an empty object singleton.
    RapidYAMLObject() : m_node(emptyObject) {}

    /**
     * @brief   Construct a RapidYAMLObject referencing a specific
     * RapidYAML value.
     *
     * @param   node  reference to a RapidYAML value
     *
     * Note that this constructor will throw an exception if the value is not
     * a map.
     */
    RapidYAMLObject(ryml::ConstNodeRef node) : m_node(node)
    {
        if (!node.is_map()) {
            throwRuntimeError("Value is not an object.");
        }
    }

    /**
     * @brief   Return an iterator for this first object member
     *
     * The iterator returned by this function is effectively a wrapper around
     * the iterator value returned by the underlying RapidYAML
     * implementation.
     */
    RapidYAMLObjectMemberIterator begin() const;

    /**
     * @brief   Return an iterator for an invalid object member that indicates
     *          the end of the collection.
     *
     * The iterator returned by this function is effectively a wrapper around
     * the iterator value returned by the underlying RapidYAML
     * implementation.
     */
    RapidYAMLObjectMemberIterator end() const;

    /**
     * @brief   Return an iterator for the object member with the specified
     *          property name.
     *
     * If an object member with the specified name does not exist, the iterator
     * returned will be the same as the iterator returned by the end() function.
     *
     * @param   propertyName  property name to search for
     */
    RapidYAMLObjectMemberIterator find(const std::string &propertyName) const;

    /// Returns the number of members belonging to this object.
    size_t size() const
    {
        return m_node.num_children();
    }

  private:
    /**
     * @brief   Return a reference to a RapidYAML tree that is empty map.
     *
     * Note that the value returned by this function is a singleton.
     */
    static inline ryml::Tree emptyObjectTree;
    static inline ryml::ConstNodeRef emptyObject = [] {
        static std::string empty_object_string = R"(
            {}
          )";
        if (RapidYAMLObject::emptyObjectTree.empty()) {
            RapidYAMLObject::emptyObjectTree =
                ryml::parse_in_place(ryml::to_substr(empty_object_string));
        }
        return RapidYAMLObject::emptyObjectTree.rootref();
    }();

    /// Reference to the contained object
    ryml::ConstNodeRef m_node;
};

/**
 * @brief   Stores an independent copy of a RapidYAML value.
 *
 * This class allows a RapidYAML value to be stored independent of its
 * original document.
 *
 * @see FrozenValue
 */
class RapidYAMLFrozenValue : public FrozenValue
{
  public:
    /**
     * @brief  Make a copy of a RapidYAML node
     *
     * @param  source  the RapidYAML node to be copied
     */
    explicit RapidYAMLFrozenValue(ryml::ConstNodeRef source)
        : m_tree(std::make_shared<ryml::Tree>())
    {
        // Clone the node's tree and store it
        *m_tree = *source.tree();
    }

    FrozenValue *clone() const override
    {
        // Create a new tree copy
        auto newValue = std::make_unique<RapidYAMLFrozenValue>(*this);
        return newValue.release();
    }

    bool equalTo(const Adapter &other, bool strict) const override;

    /**
     * @brief   Get the root node of the frozen tree
     *
     * @returns ConstNodeRef to the root of the stored tree
     */
    ryml::ConstNodeRef getRoot() const
    {
        return m_tree->rootref();
    }

  private:
    /// Stored RapidYAML tree
    std::shared_ptr<ryml::Tree> m_tree;
};

/**
 * @brief   Light weight wrapper for a RapidYAML value.
 *
 * This class is passed as an argument to the BasicAdapter template class,
 * and is used to provide access to a RapidYAML value. This class is
 * responsible for the mechanics of actually reading a RapidYAML value,
 * whereas the BasicAdapter class is responsible for the semantics of type
 * comparisons and conversions.
 *
 * The functions that need to be provided by this class are defined implicitly
 * by the implementation of the BasicAdapter template class.
 *
 * @see BasicAdapter
 */
class RapidYAMLValue
{
  public:
    /// Construct a wrapper for the empty object singleton
    RapidYAMLValue() : m_node(emptyObject) {}

    /// Construct a wrapper for a specific RapidYAML node
    RapidYAMLValue(ryml::ConstNodeRef node) : m_node(node) {}

    /**
     * @brief   Create a new RapidYAMLFrozenValue instance that contains the
     *          value referenced by this RapidYAMLValue instance.
     *
     * @returns pointer to a new RapidYAMLFrozenValue instance, belonging to
     * the caller.
     */
    FrozenValue *freeze() const
    {
        return new RapidYAMLFrozenValue(m_node);
    }

    /**
     * @brief   Optionally return a RapidYAMLArray instance.
     *
     * If the referenced RapidYAML value is a sequence, this function will
     * return a std::optional containing a RapidYAMLArray instance
     * referencing the array.
     *
     * Otherwise it will return an empty optional.
     */
    Optional<RapidYAMLArray> getArrayOptional() const
    {
        if (m_node.is_seq()) {
            return make_optional(RapidYAMLArray(m_node));
        }

        return {};
    }

    /**
     * @brief   Retrieve the number of elements in the array
     *
     * If the referenced RapidYAML value is a sequence, this function will
     * retrieve the number of elements in the array and store it in the output
     * variable provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of elements was retrieved, false otherwise.
     */
    bool getArraySize(size_t &result) const
    {
        if (m_node.is_seq()) {
            result = m_node.num_children();
            return true;
        }

        return false;
    }

    bool getBool(bool &result) const
    {
        if (m_node.has_val()) {
            try {
                m_node >> result;
                // result = m_node.as<bool>();
                return true;
            } catch (...) {
                return false;
            }
        }

        return false;
    }

    bool getDouble(double &result) const
    {
        if (m_node.has_val()) {
            try {
                m_node >> result;
                // result = m_node.as<double>();
                return true;
            } catch (...) {
                return false;
            }
        }

        return false;
    }

    bool getInteger(int64_t &result) const
    {
        if (m_node.has_val()) {
            try {
                m_node >> result;
                // result = m_node.as<int64_t>();
                return true;
            } catch (...) {
                return false;
            }
        }
        return false;
    }

    /**
     * @brief   Optionally return a RapidYAMLObject instance.
     *
     * If the referenced RapidYAML value is a map, this function will
     * return a std::optional containing a RapidYAMLObject instance
     * referencing the map.
     *
     * Otherwise it will return an empty optional.
     */
    Optional<RapidYAMLObject> getObjectOptional() const
    {
        if (m_node.is_map()) {
            return make_optional(RapidYAMLObject(m_node));
        }

        return {};
    }

    /**
     * @brief   Retrieve the number of members in the object
     *
     * If the referenced RapidYAML value is a map, this function will
     * retrieve the number of members in the map and store it in the output
     * variable provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of members was retrieved, false otherwise.
     */
    bool getObjectSize(size_t &result) const
    {
        if (m_node.is_map()) {
            result = m_node.num_children();
            return true;
        }

        return false;
    }

    bool getString(std::string &result) const
    {
        if (m_node.has_val()) {
            m_node >> result;
            // result = m_node.as<std::string>();
            return true;
        }

        return false;
    }

    static bool hasStrictTypes()
    {
        return false;
    }

    bool isArray() const
    {
        return m_node.is_seq();
    }

    bool isBool() const
    {
        if (!m_node.has_val()) {
            return false;
        }
        bool dummy;
        return c4::yml::read(m_node, &dummy);
    }

    bool isDouble() const
    {
        if (!m_node.has_val()) {
            return false;
        }
        double dummy;
        return c4::yml::read(m_node, &dummy);
    }

    bool isInteger() const
    {
        if (!m_node.has_val()) {
            return false;
        }
        int64_t dummy;
        return c4::yml::read(m_node, &dummy);
    }

    bool isNull() const
    {
        return m_node.invalid();
    }

    bool isNumber() const
    {
        if (!m_node.has_val()) {
            return false;
        }
        return isInteger() || isDouble();
    }

    bool isObject() const
    {
        return m_node.is_map();
    }

    bool isString() const
    {
        if (!m_node.has_val()) {
            return false;
        }
        return m_node.has_val();
    }

  private:
    /// Shared RapidYAML node used to represent an empty object
    static inline ryml::Tree emptyObjectTree;
    static inline ryml::ConstNodeRef emptyObject = [] {
        static std::string empty_object_string = R"(
          {}
        )";
        if (emptyObjectTree.empty()) {
            emptyObjectTree =
                ryml::parse_in_place(ryml::to_substr(empty_object_string));
        }
        return emptyObjectTree.rootref();
    }();

    /// Reference to the contained RapidYAML value.
    ryml::ConstNodeRef m_node;
};

/**
 * @brief   An implementation of the Adapter interface supporting RapidYAML.
 *
 * This class is defined in terms of the BasicAdapter template class, which
 * helps to ensure that all of the Adapter implementations behave consistently.
 *
 * @see Adapter
 * @see BasicAdapter
 */
class RapidYAMLAdapter : public BasicAdapter<RapidYAMLAdapter, RapidYAMLArray,
                                             RapidYAMLObjectMember,
                                             RapidYAMLObject, RapidYAMLValue>
{
  public:
    /// Construct a RapidYAMLAdapter that contains an empty object
    RapidYAMLAdapter() : BasicAdapter() {}

    /// Construct a RapidYAMLAdapter containing a specific RapidYAML node
    RapidYAMLAdapter(ryml::ConstNodeRef node)
        : BasicAdapter(RapidYAMLValue{node})
    {
    }
};

/**
 * @brief   Class for iterating over values held in a YAML array.
 *
 * This class provides a YAML array iterator that dereferences as an instance of
 * RapidYAMLAdapter representing a value stored in the array. It has been
 * implemented using standard C++ iterator traits.
 *
 * @see RapidYAMLArray
 */
class RapidYAMLArrayValueIterator
{
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = RapidYAMLAdapter;
    using difference_type = RapidYAMLAdapter;
    using pointer = RapidYAMLAdapter *;
    using reference = RapidYAMLAdapter &;

    /**
     * @brief   Construct a new RapidYAMLArrayValueIterator using an existing
     *          RapidYAML node iterator.
     *
     * @param   node   Parent sequence node
     * @param   index  Current index in the sequence
     */
    RapidYAMLArrayValueIterator(ryml::ConstNodeRef node, size_t index = 0)
        : m_node(node), m_index(index)
    {
    }

    /// Returns a RapidYAMLAdapter that contains the value of the current
    /// element.
    RapidYAMLAdapter operator*() const
    {
        if (m_index < m_node.num_children()) {
            return RapidYAMLAdapter(m_node[m_index]);
        }
        return RapidYAMLAdapter();
    }

    DerefProxy<RapidYAMLAdapter> operator->() const
    {
        return DerefProxy<RapidYAMLAdapter>(**this);
    }

    /**
     * @brief   Compare this iterator against another iterator.
     *
     * Note that this compares the node references and indices.
     *
     * @param   other  iterator to compare against
     *
     * @returns true   if the iterators are equal, false otherwise.
     */
    bool operator==(const RapidYAMLArrayValueIterator &other) const
    {
        return m_index == other.m_index && m_node.id() == other.m_node.id();
    }

    bool operator!=(const RapidYAMLArrayValueIterator &other) const
    {
        return !(m_index == other.m_index && m_node.id() == other.m_node.id());
    }

    const RapidYAMLArrayValueIterator &operator++()
    {
        m_index++;
        return *this;
    }

    RapidYAMLArrayValueIterator operator++(int)
    {
        RapidYAMLArrayValueIterator iterator_pre(m_node, m_index);
        ++(*this);
        return iterator_pre;
    }

    void advance(std::ptrdiff_t n)
    {
        m_index += n;
    }

  private:
    ryml::ConstNodeRef m_node;
    size_t m_index;
};

/**
 * @brief   Class for iterating over the members belonging to a YAML object.
 *
 * This class provides a YAML object iterator that dereferences as an instance
 * of RapidYAMLObjectMember representing one of the members of the object.
 *
 * @see RapidYAMLObject
 * @see RapidYAMLObjectMember
 */
class RapidYAMLObjectMemberIterator
{
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = RapidYAMLObjectMember;
    using difference_type = RapidYAMLObjectMember;
    using pointer = RapidYAMLObjectMember *;
    using reference = RapidYAMLObjectMember &;

    /**
     * @brief   Construct an iterator from a RapidYAML parent node and index.
     *
     * @param   node   Parent map node
     * @param   index  Current index in the map
     */
    RapidYAMLObjectMemberIterator(ryml::ConstNodeRef node, size_t index = 0)
        : m_node(node), m_index(index)
    {
    }

    /**
     * @brief   Returns a RapidYAMLObjectMember that contains the key and
     * value belonging to the object member identified by the iterator.
     */
    RapidYAMLObjectMember operator*() const
    {
        if (m_index < m_node.num_children()) {
            auto child = m_node[m_index];
            std::string key;
            if (child.has_key()) {
                key = {child.key().str, child.key().len};
            }
            return RapidYAMLObjectMember(key, RapidYAMLAdapter(child));
        }
        return RapidYAMLObjectMember("", RapidYAMLAdapter());
    }

    DerefProxy<RapidYAMLObjectMember> operator->() const
    {
        return DerefProxy<RapidYAMLObjectMember>(**this);
    }

    /**
     * @brief   Compare this iterator with another iterator.
     *
     * Note that this compares the node references and indices.
     *
     * @param   other  Iterator to compare with
     *
     * @returns true if the underlying iterators are equal, false otherwise
     */
    bool operator==(const RapidYAMLObjectMemberIterator &other) const
    {
        return m_index == other.m_index && m_node.id() == other.m_node.id();
    }

    bool operator!=(const RapidYAMLObjectMemberIterator &other) const
    {
        return !(m_index == other.m_index && m_node.id() == other.m_node.id());
    }

    const RapidYAMLObjectMemberIterator &operator++()
    {
        m_index++;
        return *this;
    }

    RapidYAMLObjectMemberIterator operator++(int)
    {
        RapidYAMLObjectMemberIterator iterator_pre(m_node, m_index);
        ++(*this);
        return iterator_pre;
    }

  private:
    /// Internal copy of the parent node
    ryml::ConstNodeRef m_node;
    /// Current index in the map/sequence
    size_t m_index;
};

/// Specialisation of the AdapterTraits template struct for RapidYAMLAdapter.
template <> struct AdapterTraits<valijson::adapters::RapidYAMLAdapter>
{
    typedef ryml::Tree DocumentType;

    static std::string adapterName()
    {
        return "RapidYAMLAdapter";
    }
};

inline bool RapidYAMLFrozenValue::equalTo(const Adapter &other,
                                          bool strict) const
{
    return RapidYAMLAdapter(getRoot()).equalTo(other, strict);
}

inline RapidYAMLArrayValueIterator RapidYAMLArray::begin() const
{
    return RapidYAMLArrayValueIterator(m_node, 0);
}

inline RapidYAMLArrayValueIterator RapidYAMLArray::end() const
{
    return RapidYAMLArrayValueIterator(m_node, m_node.num_children());
}

inline RapidYAMLObjectMemberIterator RapidYAMLObject::begin() const
{
    return RapidYAMLObjectMemberIterator(m_node, 0);
}

inline RapidYAMLObjectMemberIterator RapidYAMLObject::end() const
{
    return RapidYAMLObjectMemberIterator(m_node, m_node.num_children());
}

inline RapidYAMLObjectMemberIterator
RapidYAMLObject::find(const std::string &propertyName) const
{
    for (size_t i = 0; i < m_node.num_children(); ++i) {
        auto child = m_node[i];
        if (child.has_key()) {
            std::string key = {child.key().str, child.key().len};
            if (key == propertyName) {
                return RapidYAMLObjectMemberIterator(m_node, i);
            }
        }
    }
    return end();
}

} // namespace adapters
} // namespace valijson
