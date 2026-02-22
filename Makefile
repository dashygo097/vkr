PROJECT_DIR = $(shell pwd)
CMAKE_DIR = $(PROJECT_DIR)/cmake
BUILD_DIR = $(PROJECT_DIR)/build
SHADER_SRC_ROOT = $(PROJECT_DIR)/shaders
SHADER_OUT_ROOT = $(BUILD_DIR)/bin/shaders

# Build configuration
BUILD_TYPE ?= Debug 
GENERATOR ?= Ninja 

# Detect generator command
ifeq ($(GENERATOR),Ninja)
  BUILD_CMD = ninja -C $(BUILD_DIR)
else
  BUILD_CMD = cmake --build $(BUILD_DIR)
endif

.PHONY: all config build shaders clean reconfigure help

all: build

# Configure CMake project
config:
	@echo "==> Configuring project..."
	@mkdir -p $(BUILD_DIR)
	@if [ ! -f $(BUILD_DIR)/config.cmake ]; then \
		echo "Copying default config.cmake to build directory..."; \
		cp $(CMAKE_DIR)/config.cmake $(BUILD_DIR)/config.cmake; \
	else \
		echo "Using existing config.cmake in build directory"; \
	fi

# Compile shaders
shaders:
	@echo "==> Transferring shaders..."
	@mkdir -p $(SHADER_OUT_ROOT)
	@find $(SHADER_SRC_ROOT) -type f \( -name "*.vert" -o -name "*.frag" \) 2>/dev/null | while read -r src; do \
		rel="$${src#$(SHADER_SRC_ROOT)/}"; \
		base="$${rel%.*}"; \
		ext="$${rel##*.}"; \
		name="$$(basename "$$base")"; \
		out_dir="$(SHADER_OUT_ROOT)/$$(dirname "$$rel")"; \
		mkdir -p "$$out_dir"; \
		out="$$out_dir/$${ext}_$${name}.spv"; \
		cp "$$src" "$$out_dir/$$name.$${ext}"; \
		echo "  ✓ Copy $$src → $$out"; \
	done || true

# Build the project (includes shader compilation)
build: config shaders
	@if [ ! -f $(BUILD_DIR)/CMakeCache.txt ]; then \
		echo "Running CMake with generator: $(GENERATOR)"; \
		cd $(BUILD_DIR) && cmake $(PROJECT_DIR) \
			-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
			-G $(GENERATOR); \
	fi
	@echo "==> Building project with $(GENERATOR)..."
	@$(BUILD_CMD)

# Force reconfigure
reconfigure:
	@echo "==> Forcing reconfiguration..."
	@rm -f $(BUILD_DIR)/CMakeCache.txt
	@$(MAKE) config

# Clean build artifacts
clean:
	@echo "==> Cleaning build directory..."
	@rm -rf $(BUILD_DIR)

# Help message
help:
	@echo "Available targets:"
	@echo "  all              - Configure, compile shaders, and build (default)"
	@echo "  config           - Configure CMake project"
	@echo "  shaders          - Copy shader files to the correct paths"
	@echo "  build            - Build the project (includes config and shaders)"
	@echo "  reconfigure      - Force CMake reconfiguration"
	@echo "  clean            - Remove build directory"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  BUILD_TYPE       - Build type (Debug, Release, RelWithDebInfo, MinSizeRel)"
	@echo "                     Current: $(BUILD_TYPE)"
	@echo "  GENERATOR        - CMake generator (Ninja, 'Unix Makefiles', etc.)"
	@echo "                     Current: $(GENERATOR)"
	@echo ""
	@echo "Paths:"
	@echo "  SHADER_SRC_ROOT  - Source shader directory"
	@echo "                     Current: $(SHADER_SRC_ROOT)"
	@echo "  SHADER_OUT_ROOT  - Compiled shader output directory"
	@echo "                     Current: $(SHADER_OUT_ROOT)"
