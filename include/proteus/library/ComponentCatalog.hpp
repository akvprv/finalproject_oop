#pragma once

#include "proteus/core/Circuit.hpp"

#include <string>
#include <vector>

namespace proteus {

struct CatalogEntry {
    std::string typeName;
    std::string displayName;
    std::string category;
    std::string description;
};

class ComponentCatalog {
public:
    ComponentCatalog();

    const std::vector<CatalogEntry>& entries() const noexcept;
    std::vector<std::string> categories() const;
    std::vector<CatalogEntry> search(
        const std::string& text,
        const std::string& category = {}) const;
    Component& create(Circuit& circuit,
                      const std::string& typeName,
                      const std::string& label,
                      Point position) const;

private:
    std::vector<CatalogEntry> entries_;
};

}
