#pragma once

#include "proteus/persistence/ProjectSerializer.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace proteus {

class History {
public:
    explicit History(std::size_t capacity = 100);

    void reset(const ProjectDocument& document);
    void record(const ProjectDocument& document);
    bool canUndo() const noexcept;
    bool canRedo() const noexcept;
    ProjectDocument undo();
    ProjectDocument redo();
    std::size_t size() const noexcept;

private:
    std::size_t capacity_;
    std::vector<std::string> snapshots_;
    std::size_t cursor_{0};
};

}
