#pragma once
#include <memory>
#include "TypedTable.hpp"
#include "World.hpp"

struct DefaultScene;

using SceneId = uint16_t;

struct SceneRecord {
    std::unique_ptr<World> world;
};

class Engine : TypedTable<SceneRecord> {
    SceneId currentScene = 0;

    Engine() {
        this->initScene<DefaultScene>();
        this->setScene<DefaultScene>();
    }

public:
    template <typename T>
    void initScene() {
        if (!this->isRegistered<T>()) {
            this->registerElement<T>({
                .world = std::make_unique<World>()
            });

            if constexpr (IsPlugin<T>) {
                // ReSharper disable once CppRedundantTemplateKeyword
                this->getScene<T>().template registerPlugin<T>();
            }
        }
    }

    template <typename T>
    World& getScene() {
        return *this->getElement<T>().world;
    }

    template <typename T>
    void setScene() {
        this->currentScene = this->id<T>();
    }

    void progress() {
        this->getElementById(this->currentScene).world->progress();
    }
};
