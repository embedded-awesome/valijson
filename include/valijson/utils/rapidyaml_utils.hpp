#pragma once

#include <iostream>
#include <memory>
#include <string>

#include <ryml.hpp>
#include <valijson/utils/file_utils.hpp>

namespace valijson {
namespace utils {

template<class CharContainer>
inline size_t file_get_contents(const char *filename, CharContainer *v)
{
    std::FILE *fp = std::fopen(filename, "rb");
    if (!fp) {
        std::cerr << "Failed to open file '" << filename << "'." << std::endl;
        return 0;
    }
    // RYML_CHECK_MSG(fp != nullptr, "could not open file");
    std::fseek(fp, 0, SEEK_END);
    long sz = std::ftell(fp);
    v->resize(static_cast<typename CharContainer::size_type>(sz));
    if(sz)
    {
        std::rewind(fp);
        std::fread(&(*v)[0], 1, v->size(), fp);
        // RYML_CHECK(ret == (size_t)sz);
    }
    std::fclose(fp);
    return v->size();
}

std::vector<std::shared_ptr<std::vector<char>>> loaded_files;

inline bool loadDocument(const std::string &path, ryml::Tree &document)
{
    try {
        std::shared_ptr<std::vector<char>> file_contents = std::make_shared<std::vector<char>>();
        file_get_contents(path.c_str(), file_contents.get());
        document = ryml::parse_in_place(file_contents->data());
        loaded_files.push_back(file_contents);
        return true;
    } catch (const std::exception &ex) {
        std::cout << "rapidyaml failed to parse the document '" << ex.what()
                  << std::endl;
        return false;
    }
}

} // namespace utils
} // namespace valijson
