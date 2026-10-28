PROJECT_DIR := $(CURDIR)
BUILD_DIR ?= $(PROJECT_DIR)/build
INSTALL_DIR ?= $(BUILD_DIR)/install

CMAKE ?= cmake
CMAKE_ARGS ?=
BUILD_ARGS ?=
INSTALL_ARGS ?=

.PHONY: all build install clean help

all: build

build:
	@echo "==> Configuring VKR in $(BUILD_DIR)"
	@$(CMAKE) -S "$(PROJECT_DIR)" -B "$(BUILD_DIR)" \
		$(CMAKE_ARGS)
	@echo "==> Building VKR"
	@$(CMAKE) --build "$(BUILD_DIR)" --parallel $(BUILD_ARGS)

install:
	@echo "==> Installing VKR package into $(INSTALL_DIR)"
	@$(CMAKE) --install "$(BUILD_DIR)" --prefix "$(INSTALL_DIR)" $(INSTALL_ARGS)

clean:
	@if [ -f "$(BUILD_DIR)/CMakeCache.txt" ]; then \
		echo "==> Cleaning CMake build targets"; \
		$(CMAKE) --build "$(BUILD_DIR)" --target clean $(BUILD_ARGS); \
	else \
		echo "==> Nothing to clean; $(BUILD_DIR) is not configured"; \
	fi

help:
	@echo "Targets:"
	@echo "  make / make build     Configure and build VKR"
	@echo "  make install          Install the existing VKR build"
	@echo "  make clean            Clean the VKR build"
	@echo ""
	@echo "Variables:"
	@echo "  BUILD_DIR=$(BUILD_DIR)"
	@echo "  INSTALL_DIR=$(INSTALL_DIR)"
	@echo "  CMAKE_ARGS=$(CMAKE_ARGS)"
	@echo "  BUILD_ARGS=$(BUILD_ARGS)"
	@echo "  INSTALL_ARGS=$(INSTALL_ARGS)"
	@echo ""
	@echo "Examples consume the installed package independently:"
	@echo "  make -C examples CMAKE_ARGS=\"-DCMAKE_PREFIX_PATH=$(INSTALL_DIR)\""
