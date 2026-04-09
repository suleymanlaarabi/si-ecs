CMAKE ?= cmake
BUILD_TYPE ?= Release
SANITIZER_BUILD_DIR ?= cmake-build-asan
DEBUG_BUILD_DIR ?= cmake-build-debug
RELEASE_BUILD_DIR ?= cmake-build-release
VEC_LOG_DIR ?= build-logs
PROBE_BIN ?= $(abspath $(VEC_LOG_DIR)/vectorization-probe)
RELEASE_VEC_FLAGS ?= -O3 -march=native -fopt-info-vec-optimized
PROBE_VEC_FLAGS ?= -O3 -march=native -fopt-info-vec-optimized

.PHONY: sanitize-configure sanitize-build sanitize-test sanitizers test run release release-vec release-vec-main vectorize-probe clean-vec-logs

test:
	$(CMAKE) -S . -B $(DEBUG_BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
	$(CMAKE) --build $(DEBUG_BUILD_DIR)
	ctest --test-dir $(DEBUG_BUILD_DIR) --output-on-failure

run:
	$(CMAKE) -S . -B $(DEBUG_BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug
	$(CMAKE) --build $(DEBUG_BUILD_DIR) --target untitled
	./$(DEBUG_BUILD_DIR)/untitled

release:
	$(CMAKE) -S . -B $(RELEASE_BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	$(CMAKE) --build $(RELEASE_BUILD_DIR) --target untitled

release-vec:
	mkdir -p $(VEC_LOG_DIR)
	$(CMAKE) -S . -B $(RELEASE_BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE="$(RELEASE_VEC_FLAGS)"
	$(CMAKE) --build $(RELEASE_BUILD_DIR) --target clean
	$(CMAKE) --build $(RELEASE_BUILD_DIR) --target untitled

release-vec-main:
	$(MAKE) release-vec 2>&1 | grep -E 'main.cpp:|ecs/System.hpp:'

vectorize-probe:
	mkdir -p $(VEC_LOG_DIR)
	rm -f $(PROBE_BIN)
	g++ -std=c++23 $(PROBE_VEC_FLAGS) tools/vectorization_probe.cpp -o $(PROBE_BIN)
	@echo "Probe binary: $(PROBE_BIN)"

clean-vec-logs:
	rm -rf $(VEC_LOG_DIR)

sanitize-configure:
	$(CMAKE) -S . -B $(SANITIZER_BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -DECS_ENABLE_SANITIZERS=ON

sanitize-build: sanitize-configure
	$(CMAKE) --build $(SANITIZER_BUILD_DIR)

sanitize-test: sanitize-build
	ctest --test-dir $(SANITIZER_BUILD_DIR) --output-on-failure

sanitizers: sanitize-test
