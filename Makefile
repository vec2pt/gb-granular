# Variables
PROJECT_NAME = granular

# Tools
GBDK_DIR := resources/gbdk
GBDK_VERSION = 4.5.0
LCC := $(GBDK_DIR)/bin/lcc
LCC_FLAGS += -debug
PNG2ASSET := $(GBDK_DIR)/bin/png2asset
PNG2ASSET_FLAGS += -spr8x8 -no_palettes -noflip -keep_palette_order

# Dirs
SRC_DIR := src
OBJ_DIR := obj
BUILD_DIR := build
ASSETS_DIR := assets

# Assets
ASSETS_PNG := $(wildcard $(ASSETS_DIR)/*.png)
ASSETS_C := $(ASSETS_PNG:%.png=$(SRC_DIR)/%.c)
ASSETS_O := $(ASSETS_PNG:%.png=$(OBJ_DIR)/%.o)

# Sources
SOURCES_C := $(wildcard $(SRC_DIR)/*.c)
SOURCES_O := $(notdir $(SOURCES_C))
SOURCES_O := $(SOURCES_O:%.c=$(OBJ_DIR)/%.o)

# GB File
GB_FILE := $(BUILD_DIR)/$(PROJECT_NAME).gb

# Build
all: prepare $(GBDK_DIR) $(GB_FILE)

$(GB_FILE): $(ASSETS_O) $(SOURCES_O)
	$(LCC) $(LCC_FLAGS) -o $(GB_FILE) $(SOURCES_O) $(ASSETS_O)

# Build .c files for assets
$(SRC_DIR)/$(ASSETS_DIR)/%.c: $(ASSETS_DIR)/%.png
	$(PNG2ASSET) $< $(PNG2ASSET_FLAGS) -tiles_only -keep_palette_order -c $@

$(SRC_DIR)/$(ASSETS_DIR)/logo.c: $(ASSETS_DIR)/logo.png
	$(PNG2ASSET) $< $(PNG2ASSET_FLAGS) -map -tile_origin 112 -keep_palette_order -c $@

# Build .o files
$(OBJ_DIR)/$(ASSETS_DIR)/%.o: $(SRC_DIR)/$(ASSETS_DIR)/%.c
	$(LCC) $(LCC_FLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(LCC) $(LCC_FLAGS) -c -o $@ $<

# GBDK-2020
$(GBDK_DIR):
	@wget -nc -q https://github.com/gbdk-2020/gbdk-2020/releases/download/$(GBDK_VERSION)/gbdk-linux64.tar.gz -P resources
	@tar xf resources/gbdk-linux64.tar.gz -C resources
	@rm resources/gbdk-linux64.tar.gz

prepare:
	@mkdir -p $(SRC_DIR)/$(ASSETS_DIR) $(OBJ_DIR)/$(ASSETS_DIR) $(BUILD_DIR)

clean:
	rm -rf $(SRC_DIR)/$(ASSETS_DIR) $(OBJ_DIR) $(BUILD_DIR)
