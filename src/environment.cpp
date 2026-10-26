#include <cpm/args/environment.hpp>

#include <cstdlib>

namespace cpm::args {

Environment system_environment() {
    return [](std::string_view name) -> std::optional<std::string> {
        const char* value = std::getenv(std::string(name).c_str());
        if (value == nullptr) return std::nullopt;
        return std::string(value);
    };
}

}  // namespace cpm::args
