#include <criterion/criterion.h>

#include "System.hpp"
#include "../ecs/World.hpp"


Test(system_condition, basique) {
    World world;
    enum class MyState {
        A,
        B
    };
    static int count = 0;
    count = 0;

    world.initSingleton<MyState>();
    world.getSingleton<MyState>() = MyState::A;

    auto sys = task<[]() {
        count += 1;
    }>().condition<InState<MyState::A> >(world);

    world.system(Phase::Update, sys);

    world.progress();

    cr_assert_eq(count, 1);
    world.getSingleton<MyState>() = MyState::B;
    world.progress();

    cr_assert_eq(count, 1);
}
