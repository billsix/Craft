.DEFAULT_GOAL := help

.PHONY: all
all: clean image html ## Build the debug and release versions


.PHONY: clean
clean: ## delete all non-version controlled files
	git clean -fdx .

.PHONY: debug
debug: ## build Craft in debug mode
	mkdir debugBuild
	mkdir debugBuildInstall
	cd debugBuild && \
		cmake -DCMAKE_INSTALL_PREFIX=../debugBuildInstall -DENABLE_VULKAN_RENDERER=NO \
		-DENABLE_OPENGL_CORE_PROFILE_RENDERER=YES -DCMAKE_BUILD_TYPE=Debug ../ && \
		cmake --build  . --target all && \
		cmake --build  . --target install


.PHONY: release
release: ## build Craft in release mode
	mkdir releaseBuild
	mkdir releaseBuildInstall
	cd releaseBuild && \
		cmake -DCMAKE_INSTALL_PREFIX=../releaseBuildInstall -DCMAKE_BUILD_TYPE=Release ../ && \
		cmake --build  . --target all && \
		cmake --build  . --target install


.PHONY: help
help:
	@grep --extended-regexp '^[a-zA-Z0-9_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
