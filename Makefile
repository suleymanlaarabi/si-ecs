CMAKE ?= cmake
BUILD_DIR ?= cmake-build-bench
BUILD_TYPE ?= Release
BENCH_ARGS ?=
QUERY_BENCH_ARGS ?= $(BENCH_ARGS)
ENTITY_BENCH_ARGS ?= $(BENCH_ARGS)

.PHONY: bench-configure bench-query bench-entity-component bench

bench-configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_TESTING=OFF

bench-query: bench-configure
	$(CMAKE) --build $(BUILD_DIR) --target ecs_bench_query
	./$(BUILD_DIR)/ecs_bench_query $(QUERY_BENCH_ARGS)

bench-entity-component: bench-configure
	$(CMAKE) --build $(BUILD_DIR) --target ecs_bench_entity_component
	./$(BUILD_DIR)/ecs_bench_entity_component $(ENTITY_BENCH_ARGS)

bench: bench-query bench-entity-component
