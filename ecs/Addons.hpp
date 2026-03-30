#pragma once

class Table;
class World;

struct DeltaTime {
    float value = 0.f;

    static DeltaTime& fromWorldTable(World& world, const Table& table);
};
