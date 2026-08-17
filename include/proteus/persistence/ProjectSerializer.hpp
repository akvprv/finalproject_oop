#pragma once

#include "proteus/core/Circuit.hpp"
#include "proteus/ui/CanvasModel.hpp"

#include <filesystem>
#include <string>

namespace proteus {

struct ProjectDocument {
    Circuit circuit;
    CanvasSettings canvas;
};

class ProjectSerializer {
public:
    static std::string serialize(const ProjectDocument& document);
    static ProjectDocument deserialize(const std::string& json);
    static void saveFile(const ProjectDocument& document,
                         const std::filesystem::path& path);
    static ProjectDocument loadFile(const std::filesystem::path& path);
};

}
