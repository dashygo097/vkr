PROJECT_DIR := $(CURDIR)
BUILD_DIR ?= $(PROJECT_DIR)/build
BUILD_TYPE ?= Debug
GENERATOR ?= Ninja

CMAKE ?= cmake
CTEST ?= ctest

CMAKE_CONFIG := $(BUILD_DIR)/config.cmake
CMAKE_ARGS ?=
BUILD_ARGS ?=
CTEST_ARGS ?=

.PHONY: all configure config build rebuild reconfigure clean distclean test help

all: build

$(CMAKE_CONFIG): $(PROJECT_DIR)/cmake/config.cmake
	@mkdir -p "$(BUILD_DIR)"
	@if [ ! -f "$(CMAKE_CONFIG)" ]; then \
		echo "==> Creating local build config: $(CMAKE_CONFIG)"; \
		cp "$(PROJECT_DIR)/cmake/config.cmake" "$(CMAKE_CONFIG)"; \
	fi

configure config: $(CMAKE_CONFIG)
	@echo "==> Configuring $(BUILD_TYPE) build in $(BUILD_DIR)"
	@$(CMAKE) -S "$(PROJECT_DIR)" -B "$(BUILD_DIR)" \
		-G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" \
		$(CMAKE_ARGS)

build: configure
	@echo "==> Building project"
	@$(CMAKE) --build "$(BUILD_DIR)" --parallel $(BUILD_ARGS)

rebuild: distclean build

reconfigure: $(CMAKE_CONFIG)
	@echo "==> Reconfiguring project"
	@$(CMAKE) -S "$(PROJECT_DIR)" -B "$(BUILD_DIR)" \
		-G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" \
		$(CMAKE_ARGS)

clean:
	@if [ -f "$(BUILD_DIR)/CMakeCache.txt" ]; then \
		echo "==> Cleaning CMake build targets"; \
		$(CMAKE) --build "$(BUILD_DIR)" --target clean; \
	else \
		echo "==> Nothing to clean; $(BUILD_DIR) is not configured"; \
	fi

distclean:
	@echo "==> Removing build directory"
	@rm -rf "$(BUILD_DIR)"

test: configure
	@echo "==> Running tests"
	@$(CTEST) --test-dir "$(BUILD_DIR)" --output-on-failure $(CTEST_ARGS)

help:
	@echo "Targets:"
	@echo "  make                  Configure and build"
	@echo "  make configure        Run CMake configure"
	@echo "  make build            Configure and build"
	@echo "  make reconfigure      Re-run CMake configure"
	@echo "  make rebuild          Remove build directory, then build"
	@echo "  make clean            Run the CMake clean target"
	@echo "  make distclean        Remove the build directory"
	@echo "  make test             Run CTest"
	@echo ""
	@echo "Variables:"
	@echo "  BUILD_DIR=$(BUILD_DIR)"
	@echo "  BUILD_TYPE=$(BUILD_TYPE)"
	@echo "  GENERATOR=$(GENERATOR)"
	@echo "  CMAKE_ARGS=$(CMAKE_ARGS)"
	@echo "  BUILD_ARGS=$(BUILD_ARGS)"
	@echo "  CTEST_ARGS=$(CTEST_ARGS)"
