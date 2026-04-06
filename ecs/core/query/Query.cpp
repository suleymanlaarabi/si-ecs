#include "Query.hpp"

#include "Table.hpp"

bool Query::matchTable(const Table& table) const {
    if ((this->bloom & table.bloom) != this->bloom) {
        return false;
    }
    for (uint i = 0; i < this->required_count; i++) {
        if (!table.hasComponent(this->required[i])) {
            return false;
        }
    }
    for (uint i = 0; i < this->excluded_count; i++) {
        if (table.hasComponent(this->excluded[i])) {
            return false;
        }
    }
    return true;
}
