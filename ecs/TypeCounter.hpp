#pragma once

namespace reflection {
    template <typename>
    class __attribute__((visibility("default"))) TypeCounter {
    public:
        static inline uint16_t current_id = 0;

        template <typename T>
        __attribute__((visibility("default"))) static uint16_t id() noexcept {
            static const uint16_t type_id = current_id++;
            return type_id;
        }
    };
} // namespace reflection

template <typename T>
std::string type_name_string() {
    std::string value = __PRETTY_FUNCTION__;
    const std::string key = "T = ";
    const std::size_t begin = value.find(key);
    if (begin == std::string::npos) {
        return value;
    }
    const std::size_t start = begin + key.size();
    const std::size_t end = value.find_first_of(";]", start);
    if (end == std::string::npos) {
        return value.substr(start);
    }
    return value.substr(start, end - start);
}
