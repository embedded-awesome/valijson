#pragma once

#include <iostream>
#include <string>

#include <ryml.hpp>
#include <ryml_std.hpp>

#include <valijson/utils/file_utils.hpp>

namespace valijson {
namespace utils {

/**
 * @brief  Load a YAML or JSON document from a file into a RapidYAML tree.
 *
 * @param  path      path to the file to load
 * @param  document  tree to populate
 *
 * @returns true if the document was loaded successfully, false otherwise
 */
inline bool loadDocument(const std::string &path, ryml::Tree &document)
{
    std::string content;
    if (!loadFile(path, content)) {
        std::cerr << "Failed to load file '" << path << "'." << std::endl;
        return false;
    }

#if VALIJSON_USE_EXCEPTIONS
    // Install a per-tree error callback that throws instead of aborting.
    // The callback must not return, so we throw std::runtime_error.
    struct ErrorState {
        bool failed = false;
        std::string message;
    } errorState;

    ryml::Callbacks cb;
    cb.set_error_basic([](ryml::csubstr msg, ryml::ErrorDataBasic const&, void *user_data) {
        auto *state = static_cast<ErrorState *>(user_data);
        state->failed = true;
        state->message.assign(msg.str, msg.len);
        throw std::runtime_error(state->message);
    });
    cb.set_error_parse([](ryml::csubstr msg, ryml::ErrorDataParse const&, void *user_data) {
        auto *state = static_cast<ErrorState *>(user_data);
        state->failed = true;
        state->message.assign(msg.str, msg.len);
        throw std::runtime_error(state->message);
    });
    cb.set_user_data(&errorState);
    document = ryml::Tree(cb);

    try {
        ryml::parse_in_arena(ryml::to_csubstr(path), ryml::to_csubstr(content), &document);
    } catch (const std::runtime_error &e) {
        std::cerr << "RapidYAML failed to parse '" << path << "': "
                  << e.what() << std::endl;
        return false;
    }
#else
    ryml::parse_in_arena(ryml::to_csubstr(path), ryml::to_csubstr(content), &document);
#endif

    return true;
}

/**
 * @brief  Navigate to the content node of a RapidYAML document tree.
 *
 * When ryml parses YAML, it creates a stream node at the root with a document
 * node as its first child. For most use cases (single-document YAML/JSON),
 * callers want a ConstNodeRef pointing directly to the document content.
 *
 * @param  tree  the parsed tree
 * @returns ConstNodeRef pointing to the first document's content, or
 *          the root node if the tree layout differs from the expected pattern.
 */
inline ryml::ConstNodeRef getDocumentNode(const ryml::Tree &tree)
{
    ryml::ConstNodeRef root = tree.rootref();
    if (root.is_stream() && root.num_children() > 0) {
        return root.first_child();
    }
    return root;
}

} // namespace utils
} // namespace valijson
