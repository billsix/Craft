.DEFAULT_GOAL := help

.PHONY: all
all: clean image html ## Build the debug and release versions


.PHONY: clean
clean: ## delete all non-version controlled files
	git clean -fdx .


.PHONY: debug
debug: ## build Craft in debug mode
	cmake -S. -BdebugBuild
	mkdir -p debugBuildInstall
	cmake -DCMAKE_INSTALL_PREFIX=./debugBuildInstall -DENABLE_VULKAN_RENDERER=NO \
	-DENABLE_OPENGL_CORE_PROFILE_RENDERER=YES -DCMAKE_BUILD_TYPE=Debug -S. -BdebugBuild
	cmake --build debugBuild
	cmake --install debugBuild


.PHONY: release
release: ## build Craft in release mode
	cmake -S. -BreleaseBuild
	mkdir -p releaseBuildInstall
	cmake -DCMAKE_INSTALL_PREFIX=./releaseBuildInstall  -DENABLE_VULKAN_RENDERER=NO \
	-DENABLE_OPENGL_CORE_PROFILE_RENDERER=YES -DCMAKE_BUILD_TYPE=Release  -S. -BreleaseBuild
	cmake --build releaseBuild
	cmake --install releaseBuild


.PHONY: sanitize
sanitize: ## build+run the src/-scoped ASan & UBSan(trap) smoke harness (gate)
	@status=0; \
	for kind in address undefined; do \
	  echo "=== sanitizer gate: $$kind ==="; \
	  cmake -S. -BsanitizeBuild-$$kind \
	    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
	    -DCMAKE_BUILD_TYPE=Debug \
	    -DENABLE_SANITIZER_GATE=YES -DSANITIZER_GATE_KIND=$$kind \
	    -DENABLE_VULKAN_RENDERER=NO -DENABLE_OPENGL_CORE_PROFILE_RENDERER=YES \
	    || status=1; \
	  cmake --build sanitizeBuild-$$kind --target craft_smoke || status=1; \
	  echo "--- running $$kind harness ---"; \
	  ASAN_OPTIONS=detect_leaks=0 ./sanitizeBuild-$$kind/craft_smoke || status=1; \
	done; \
	echo "=== sanitizer gate exit status: $$status ==="; \
	exit $$status


.PHONY: help
help:
	@grep --extended-regexp '^[a-zA-Z0-9_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
