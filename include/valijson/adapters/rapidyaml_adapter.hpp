/**
 * @file
 *
 * @brief   Adapter implementation for the RapidYAML parser library.
 *
 * Include this file in your program to enable support for RapidYAML.
 *
 * This file defines the following classes (not in this order):
 *  - RymlAdapter
 *  - RymlArray
 *  - RymlArrayValueIterator
 *  - RymlFrozenValue
 *  - RymlObject
 *  - RymlObjectMember
 *  - RymlObjectMemberIterator
 *  - RymlValue
 *
 * Due to the dependencies that exist between these classes, the ordering of
 * class declarations and definitions may be a bit confusing. The best place to
 * start is RymlAdapter. This class definition is actually very small, since
 * most of the functionality is inherited from the BasicAdapter class. Most of
 * the classes in this file are provided as template arguments to the inherited
 * BasicAdapter class.
 */

#pragma once

#include <optional>
#include <string>

#include <ryml.hpp>
#include <ryml_std.hpp>
#include <c4/charconv.hpp>

#include <valijson/internal/adapter.hpp>
#include <valijson/internal/basic_adapter.hpp>
#include <valijson/internal/frozen_value.hpp>
#include <valijson/exceptions.hpp>

namespace valijson {
namespace adapters {

class RymlAdapter;
class RymlArrayValueIterator;
class RymlObjectMemberIterator;

typedef std::pair<std::string, RymlAdapter> RymlObjectMember;

/**
 * @brief  Light weight wrapper for a RapidYAML array value.
 *
 * This class is a light weight wrapper for a RapidYAML sequence node. It
 * provides a minimum set of container functions and typedefs that allow it to
 * be used as an iterable container.
 *
 * An instance of this class contains a single ConstNodeRef to the underlying
 * RapidYAML sequence node, so there is very little overhead associated with
 * copy construction and passing by value.
 */
class RymlArray
{
  public:
    typedef RymlArrayValueIterator const_iterator;
    typedef RymlArrayValueIterator iterator;

    /// Construct a RymlArray referencing an empty array singleton.
    RymlArray() : m_value(emptyArrayRef()) {}

    /**
     * @brief   Construct a RymlArray referencing a specific RapidYAML node.
     *
     * @param   value   ConstNodeRef referencing a sequence node
     *
     * Note that this constructor will throw an exception if the value is not
     * a sequence.
     */
    RymlArray(const ryml::ConstNodeRef &value) : m_value(value)
    {
        if (!value.is_seq()) {
            throwRuntimeError("Value is not an array.");
        }
    }

    /// Return an iterator for the first element of the array.
    RymlArrayValueIterator begin() const;

    /// Return an iterator for one-past the last element of the array.
    RymlArrayValueIterator end() const;

    /// Return the number of elements in the array.
    size_t size() const
    {
        return static_cast<size_t>(m_value.num_children());
    }

  private:
    static ryml::ConstNodeRef emptyArrayRef()
    {
        static ryml::Tree s_emptyTree = ryml::parse_in_arena(ryml::to_csubstr("[]"));
        return s_emptyTree.docref(0);
    }

    /// Reference to the contained sequence node.
    ryml::ConstNodeRef m_value;
};

/**
 * @brief  Light weight wrapper for a RapidYAML object (map) value.
 *
 * This class is a light weight wrapper for a RapidYAML map node. It provides
 * a minimum set of container functions and typedefs that allow it to be used
 * as an iterable container.
 */
class RymlObject
{
  public:
    typedef RymlObjectMemberIterator const_iterator;
    typedef RymlObjectMemberIterator iterator;

    /// Construct a RymlObject referencing an empty object singleton.
    RymlObject() : m_value(emptyObjectRef()) {}

    /**
     * @brief   Construct a RymlObject referencing a specific RapidYAML node.
     *
     * @param   value  ConstNodeRef referencing a map node
     *
     * Note that this constructor will throw an exception if the value is not
     * a map.
     */
    RymlObject(const ryml::ConstNodeRef &value) : m_value(value)
    {
        if (!value.is_map()) {
            throwRuntimeError("Value is not an object.");
        }
    }

    /// Return an iterator for the first object member.
    RymlObjectMemberIterator begin() const;

    /// Return an iterator for one-past the last object member.
    RymlObjectMemberIterator end() const;

    /**
     * @brief   Return an iterator for the object member with the specified
     *          property name.
     *
     * If an object member with the specified name does not exist, the iterator
     * returned will be the same as the iterator returned by the end() function.
     *
     * @param   propertyName  property name to search for
     */
    RymlObjectMemberIterator find(const std::string &propertyName) const;

    /// Returns the number of members belonging to this object.
    size_t size() const
    {
        return static_cast<size_t>(m_value.num_children());
    }

  private:
    static ryml::ConstNodeRef emptyObjectRef()
    {
        static ryml::Tree s_emptyTree = ryml::parse_in_arena(ryml::to_csubstr("{}"));
        return s_emptyTree.docref(0);
    }

    /// Reference to the contained map node.
    ryml::ConstNodeRef m_value;
};

/**
 * @brief   Stores an independent copy of a RapidYAML value.
 *
 * This class allows a RapidYAML value to be stored independently of its
 * original document. It serializes the node back to YAML text and re-parses
 * it into a new tree so that no references to the original tree are kept.
 *
 * @see FrozenValue
 */
class RymlFrozenValue : public FrozenValue
{
  public:
    /**
     * @brief  Make an independent copy of a RapidYAML node.
     *
     * @param  source  the ConstNodeRef to be copied
     */
    explicit RymlFrozenValue(const ryml::ConstNodeRef &source)
    {
        // Serialise the sub-tree to YAML text and re-parse into a private tree
        // so that the frozen value owns its data completely.
        ryml::emitrs_yaml(*(source.tree()), source.id(), &m_serialized);
        m_tree = ryml::parse_in_arena(ryml::to_csubstr(m_serialized));
    }

    FrozenValue *clone() const override
    {
        return new RymlFrozenValue(getRootNode());
    }

    bool equalTo(const Adapter &other, bool strict) const override;

  private:
    ryml::ConstNodeRef getRootNode() const
    {
        // Navigate past stream/doc wrapper nodes to the actual content node.
        ryml::ConstNodeRef root = m_tree.rootref();
        if (root.is_stream() && root.num_children() > 0) {
            root = root.first_child();
        }
        if (root.is_doc() && root.num_children() > 0) {
            root = root.first_child();
        }
        return root;
    }

    /// YAML text of the serialised node (owns the string data for m_tree).
    std::string m_serialized;
    /// The re-parsed tree that owns the frozen value.
    ryml::Tree m_tree;
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
 * RapidYAML stores all scalar values as string views with no built-in type
 * information, so this adapter uses hasStrictTypes() = false, meaning that
 * all non-null scalars are returned as strings and type inference is
 * performed by the BasicAdapter's maybe*() helpers.
 *
 * @see BasicAdapter
 */
class RymlValue
{
  public:
    /// Construct a wrapper for the empty object singleton.
    RymlValue() : m_value(emptyObjectRef()) {}

    /// Construct a wrapper for a specific RapidYAML node.
    explicit RymlValue(const ryml::ConstNodeRef &value) : m_value(value) {}

    /**
     * @brief   Create a new RymlFrozenValue instance that contains the value
     *          referenced by this RymlValue instance.
     *
     * @returns pointer to a new RymlFrozenValue instance, belonging to the
     * caller.
     */
    FrozenValue *freeze() const
    {
        return new RymlFrozenValue(m_value);
    }

    /**
     * @brief   Optionally return a RymlArray instance.
     *
     * If the referenced node is a sequence, this function will return a
     * std::optional containing a RymlArray instance referencing it.
     * Otherwise it will return an empty optional.
     */
    std::optional<RymlArray> getArrayOptional() const
    {
        if (m_value.is_seq()) {
            return std::make_optional(RymlArray(m_value));
        }
        return {};
    }

    /**
     * @brief   Retrieve the number of elements in the array.
     *
     * @param   result  reference to size_t to set with result
     * @returns true if the number of elements was retrieved, false otherwise.
     */
    bool getArraySize(size_t &result) const
    {
        if (m_value.is_seq()) {
            result = static_cast<size_t>(m_value.num_children());
            return true;
        }
        return false;
    }

    bool getBool(bool &result) const
    {
        if (!m_value.has_val()) {
            return false;
        }
        const ryml::csubstr v = m_value.val();
        if (v == "true" || v == "True" || v == "TRUE") {
            result = true;
            return true;
        }
        if (v == "false" || v == "False" || v == "FALSE") {
            result = false;
            return true;
        }
        return false;
    }

    bool getDouble(double &result) const
    {
        if (!m_value.has_val()) {
            return false;
        }
        return c4::from_chars(m_value.val(), &result);
    }

    bool getInteger(int64_t &result) const
    {
        if (!m_value.has_val()) {
            return false;
        }
        return c4::from_chars(m_value.val(), &result);
    }

    /**
     * @brief   Optionally return a RymlObject instance.
     *
     * If the referenced node is a map, this function will return a
     * std::optional containing a RymlObject instance referencing it.
     * Otherwise it will return an empty optional.
     */
    std::optional<RymlObject> getObjectOptional() const
    {
        if (m_value.is_map()) {
            return std::make_optional(RymlObject(m_value));
        }
        return {};
    }

    /**
     * @brief   Retrieve the number of members in the object.
     *
     * @param   result  reference to size_t to set with result
     * @returns true if the number of members was retrieved, false otherwise.
     */
    bool getObjectSize(size_t &result) const
    {
        if (m_value.is_map()) {
            result = static_cast<size_t>(m_value.num_children());
            return true;
        }
        return false;
    }

    bool getString(std::string &result) const
    {
        if (!m_value.has_val()) {
            return false;
        }
        const ryml::csubstr v = m_value.val();
        result.assign(v.str, v.len);
        return true;
    }

    static bool hasStrictTypes()
    {
        return false;
    }

    bool isArray() const
    {
        return m_value.is_seq();
    }

    bool isBool() const
    {
        return false;
    }

    bool isDouble() const
    {
        return false;
    }

    bool isInteger() const
    {
        return false;
    }

    bool isNull() const
    {
        if (m_value.is_map() || m_value.is_seq()) {
            return false;
        }
        if (!m_value.has_val()) {
            return false;
        }
        return m_value.val_is_null();
    }

    bool isNumber() const
    {
        return false;
    }

    bool isObject() const
    {
        return m_value.is_map();
    }

    bool isString() const
    {
        // All non-null scalars are treated as strings (non-strict mode).
        if (!m_value.has_val()) {
            return false;
        }
        return !isNull();
    }

  private:
    static ryml::ConstNodeRef emptyObjectRef()
    {
        static ryml::Tree s_emptyTree = ryml::parse_in_arena(ryml::to_csubstr("{}"));
        return s_emptyTree.docref(0);
    }

    /// Reference to the contained RapidYAML node.
    ryml::ConstNodeRef m_value;
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
class RymlAdapter
    : public BasicAdapter<RymlAdapter, RymlArray, RymlObjectMember,
                          RymlObject, RymlValue>
{
  public:
    /// Construct a RymlAdapter that contains an empty object.
    RymlAdapter() : BasicAdapter() {}

    /// Construct a RymlAdapter containing a specific RapidYAML node.
    explicit RymlAdapter(const ryml::ConstNodeRef &value)
        : BasicAdapter(RymlValue{value})
    {
    }
};

/**
 * @brief   Class for iterating over values held in a YAML sequence.
 *
 * This class provides an array iterator that dereferences as an instance of
 * RymlAdapter representing a value stored in the array.
 *
 * @see RymlArray
 */
class RymlArrayValueIterator
{
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = RymlAdapter;
    using difference_type = std::ptrdiff_t;
    using pointer = RymlAdapter *;
    using reference = RymlAdapter &;

    /**
     * @brief   Construct a new RymlArrayValueIterator using an existing
     *          RapidYAML child iterator.
     *
     * @param   itr  RapidYAML child iterator to store
     */
    explicit RymlArrayValueIterator(
        const c4::yml::detail::child_iterator<c4::yml::ConstNodeRef> &itr)
        : m_itr(itr)
    {
    }

    /// Returns a RymlAdapter that contains the value of the current element.
    RymlAdapter operator*() const
    {
        return RymlAdapter(*m_itr);
    }

    DerefProxy<RymlAdapter> operator->() const
    {
        return DerefProxy<RymlAdapter>(**this);
    }

    bool operator==(const RymlArrayValueIterator &other) const
    {
        return m_itr == other.m_itr;
    }

    bool operator!=(const RymlArrayValueIterator &other) const
    {
        return !(m_itr == other.m_itr);
    }

    const RymlArrayValueIterator &operator++()
    {
        ++m_itr;
        return *this;
    }

    RymlArrayValueIterator operator++(int)
    {
        RymlArrayValueIterator pre(m_itr);
        ++(*this);
        return pre;
    }

    void advance(std::ptrdiff_t n)
    {
        for (std::ptrdiff_t i = 0; i < n; ++i) {
            ++m_itr;
        }
    }

  private:
    c4::yml::detail::child_iterator<c4::yml::ConstNodeRef> m_itr;
};

/**
 * @brief   Class for iterating over the members belonging to a YAML map.
 *
 * This class provides an object member iterator that dereferences as an
 * instance of RymlObjectMember representing one of the members of the object.
 *
 * @see RymlObject
 * @see RymlObjectMember
 */
class RymlObjectMemberIterator
{
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = RymlObjectMember;
    using difference_type = std::ptrdiff_t;
    using pointer = RymlObjectMember *;
    using reference = RymlObjectMember &;

    /**
     * @brief   Construct an iterator from a RapidYAML child iterator.
     *
     * @param   itr  RapidYAML child iterator to store
     */
    explicit RymlObjectMemberIterator(
        const c4::yml::detail::child_iterator<c4::yml::ConstNodeRef> &itr)
        : m_itr(itr)
    {
    }

    /**
     * @brief   Returns a RymlObjectMember containing the key and value of the
     *          current map member.
     */
    RymlObjectMember operator*() const
    {
        const ryml::ConstNodeRef node = *m_itr;
        const ryml::csubstr k = node.key();
        return RymlObjectMember(std::string(k.str, k.len), RymlAdapter(node));
    }

    DerefProxy<RymlObjectMember> operator->() const
    {
        return DerefProxy<RymlObjectMember>(**this);
    }

    bool operator==(const RymlObjectMemberIterator &other) const
    {
        return m_itr == other.m_itr;
    }

    bool operator!=(const RymlObjectMemberIterator &other) const
    {
        return !(m_itr == other.m_itr);
    }

    const RymlObjectMemberIterator &operator++()
    {
        ++m_itr;
        return *this;
    }

    RymlObjectMemberIterator operator++(int)
    {
        RymlObjectMemberIterator pre(m_itr);
        ++(*this);
        return pre;
    }

  private:
    c4::yml::detail::child_iterator<c4::yml::ConstNodeRef> m_itr;
};

/// Specialisation of the AdapterTraits template struct for RymlAdapter.
template <> struct AdapterTraits<valijson::adapters::RymlAdapter>
{
    typedef ryml::Tree DocumentType;

    static std::string adapterName()
    {
        return "RymlAdapter";
    }
};

inline bool RymlFrozenValue::equalTo(const Adapter &other, bool strict) const
{
    return RymlAdapter(getRootNode()).equalTo(other, strict);
}

inline RymlArrayValueIterator RymlArray::begin() const
{
    return RymlArrayValueIterator(m_value.begin());
}

inline RymlArrayValueIterator RymlArray::end() const
{
    return RymlArrayValueIterator(m_value.end());
}

inline RymlObjectMemberIterator RymlObject::begin() const
{
    return RymlObjectMemberIterator(m_value.begin());
}

inline RymlObjectMemberIterator RymlObject::end() const
{
    return RymlObjectMemberIterator(m_value.end());
}

inline RymlObjectMemberIterator
RymlObject::find(const std::string &propertyName) const
{
    for (auto itr = begin(); itr != end(); ++itr) {
        if ((*itr).first == propertyName) {
            return itr;
        }
    }
    return end();
}

} // namespace adapters
} // namespace valijson
