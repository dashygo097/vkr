PROJECT_DIR := $(CURDIR)
BUILD_DIR ?= $(PROJECT_DIR)/build

CMAKE ?= cmake
CMAKE_ARGS ?=
BUILD_ARGS ?=

.PHONY: all build clean help

all: build

build:
	@echo "==> Configuring examples in $(BUILD_DIR)"
	@$(CMAKE) -S "$(PROJECT_DIR)" -B "$(BUILD_DIR)" \
		$(CMAKE_ARGS)
	@echo "==> Building examples"
	@$(CMAKE) --build "$(BUILD_DIR)" --parallel $(BUILD_ARGS)

clean:
	@if [ -f "$(BUILD_DIR)/CMakeCache.txt" ]; then \
		echo "==> Cleaning example build targets"; \
		$(CMAKE) --build "$(BUILD_DIR)" --target clean $(BUILD_ARGS); \
	else \
		echo "==> Nothing to clean; $(BUILD_DIR) is not configured"; \
	fi

help:
	@echo "Targets:"
	@echo "  make / make build     Configure and build examples using the VKR package"
	@echo "  make clean            Clean the example build"
	@echo ""
	@echo "Variables:"
	@echo "  BUILD_DIR=$(BUILD_DIR)"
	@echo "  CMAKE_ARGS=$(CMAKE_ARGS)"
	@echo "  BUILD_ARGS=$(BUILD_ARGS)"
	@echo ""
	@echo "First configuration with a custom VKR installation:"
	@echo "  make CMAKE_ARGS=\"-DCMAKE_PREFIX_PATH=/path/to/vkr\""
