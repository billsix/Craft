.DEFAULT_GOAL := help

.PHONY: all
all: clean image html ## Build the debug and release versions


.PHONY: clean
clean: ## delete all non-version controlled files
	git clean -fdx .


.PHONY: debug
debug: ## build Craft in debug mode
	mkdir -p debugBuild
	mkdir -p debugBuildInstall
	cmake -DCMAKE_INSTALL_PREFIX=./debugBuildInstall -DENABLE_VULKAN_RENDERER=NO \
	-DENABLE_OPENGL_CORE_PROFILE_RENDERER=YES -DCMAKE_BUILD_TYPE=Debug -S. -BdebugBuild
	cmake --build debugBuild
	cmake --install debugBuild


.PHONY: release
release: ## build Craft in release mode
	mkdir -p releaseBuild
	mkdir -p releaseBuildInstall
	cmake -DCMAKE_INSTALL_PREFIX=./releaseBuildInstall  -DENABLE_VULKAN_RENDERER=NO \
	-DENABLE_OPENGL_CORE_PROFILE_RENDERER=YES -DCMAKE_BUILD_TYPE=Release  -S. -BreleaseBuild
	cmake --build releaseBuild
	cmake --install releaseBuild


.PHONY: help
help:
	@grep --extended-regexp '^[a-zA-Z0-9_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
