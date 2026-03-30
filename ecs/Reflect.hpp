#pragma once
#include <cstdint>

enum class ValueType {
    u8,
    u16,
    u32,
    u64,
    i8,
    i16,
    i32,
    i64,
    f32,
    f64,
    cstr,
    array
};

template <std::size_t N>
struct FixedString {
    char data[N + 1]{};

    explicit consteval FixedString(const char (&str)[N]) {
        for (std::size_t i = 0; i < N; ++i) {
            data[i] = str[i];
        }
        data[N] = '\0';
    }

    [[nodiscard]] consteval const char* to_cstr() const {
        return this->data;
    }
};


struct StructMember {
    const char* name = nullptr;
    ValueType valueType = ValueType::cstr;
};

template <std::size_t size>
consteval StructMember member(FixedString<size> name, const ValueType type) {
    static auto str = Name;
    return {
        .name = str.to_cstr(),
        .valueType = type
    };
}

struct StructDef {
    StructMember* members = nullptr;
    uint32_t count = 0;
};

template <StructMember ... Member>
class Reflect {
    static StructMember members[sizeof...(Member)] = {Member...};

    [[nodiscard]] static StructDef* reflectDef() {
        return {
            .members = members,
            .count = sizeof...(Member)
        };
    }
};

