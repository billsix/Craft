.DEFAULT_GOAL := help

USE_GRAPHICS ?= 1
USE_EMACS ?= 1

CONTAINER_CMD ?= $(shell command -v podman >/dev/null 2>&1 && echo podman || echo docker)
CONTAINER_NAME = craft

# USE_EMACS=1 (the default) bind-mounts the vendored .emacs.d/ into the container so
# an interactive `make shell` can USE its clangd/lsp-mode C config + packages (:U chowns
# to the container user, :z relabels for SELinux). USE_EMACS=0 skips the mount. To
# REFRESH the vendored elpa packages, use `make update-emacs-packages`.
ifeq ($(USE_EMACS),1)
  ELPA_MOUNT = -v $(CURDIR)/entrypoint/dotfiles/.emacs.d/:/root/.emacs.d/:U,z
else
  ELPA_MOUNT =
endif

# Extra flags for every container `run`. Auto-set when running nested inside a
# runClaudeInContainer/runCrushInContainer sandbox (which exports NESTED_PODMAN=1,
# making --cgroups=disabled apply so podman-in-podman works); empty -- and
# byte-identical behavior -- on a normal host. On `run` lines only, never `build`
# (podman build rejects --cgroups). Convention: runClaudeInContainer
# tasks/reference/nested-podman-design.md.
PODMAN_RUN_FLAGS ?= $(if $(filter 1,$(NESTED_PODMAN)),--cgroups=disabled)

FILES_TO_MOUNT = -v $(shell pwd):/craft/:Z \
                 -v ./entrypoint/shell.sh:/usr/local/bin/shell.sh:Z \
                 -v ./entrypoint/format.sh:/usr/local/bin/format.sh:Z \
                 -v ./entrypoint/lint.sh:/usr/local/bin/lint.sh:Z \
                 -v ./entrypoint/buildDebug.sh:/usr/local/bin/buildDebug.sh:Z

# Craft is a GLFW + OpenGL GUI game: pass the host display through so `make shell`
# (and `make run`) can open the window.
USE_X = -e DISPLAY=$(DISPLAY) \
	-v /tmp/.X11-unix:/tmp/.X11-unix \
	--security-opt label=type:container_runtime_t
WAYLAND_FLAGS_FOR_CONTAINER = -e "WAYLAND_DISPLAY=${WAYLAND_DISPLAY}" \
                              -e "XDG_RUNTIME_DIR=${XDG_RUNTIME_DIR}" \
                              -v "${XDG_RUNTIME_DIR}:${XDG_RUNTIME_DIR}"

# shell / shell-exec share ONE run invocation (defined once so the two can never
# drift). shell is interactive; shell-exec runs a script/command with no TTY.
SHELL_RUN_FLAGS = --entrypoint /bin/bash \
	$(FILES_TO_MOUNT) \
	$(ELPA_MOUNT) \
	$(USE_X) \
	$(WAYLAND_FLAGS_FOR_CONTAINER)
SHELL_EXEC_ARGS = -c 'cd /craft && $(if $(CMD),$(CMD),exec bash $(SCRIPT))'


.PHONY: all
all: shell ## Build the image and get a shell in it

.PHONY: image
image: ## Build podman image (builds Craft via Meson at image-build time)
	$(CONTAINER_CMD) build \
	                 --build-arg USE_GRAPHICS=$(USE_GRAPHICS) \
	                 --build-arg USE_EMACS=$(USE_EMACS) \
	                 -t $(CONTAINER_NAME) \
	                 .

.PHONY: build
build: image ## Re-run the Meson build inside the container against the mounted tree (writes ./builddir)
	$(CONTAINER_CMD) run --rm $(PODMAN_RUN_FLAGS) \
		$(FILES_TO_MOUNT) \
		$(CONTAINER_NAME) \
		/usr/local/bin/buildDebug.sh

.PHONY: shell
shell: ## Get a shell in an ephemeral container (X11 + Wayland passthrough for the game)
	$(CONTAINER_CMD) run -it --rm $(PODMAN_RUN_FLAGS) \
		$(SHELL_RUN_FLAGS) \
		$(CONTAINER_NAME) \
		/usr/local/bin/shell.sh

.PHONY: run
run: image ## Run the installed craft client (X11 + Wayland passthrough)
	$(CONTAINER_CMD) run -it --rm $(PODMAN_RUN_FLAGS) \
		$(SHELL_RUN_FLAGS) \
		$(CONTAINER_NAME) \
		/usr/local/bin/shell.sh -c 'exec craft'

.PHONY: shell-exec
# shell-exec depends on image (NOT format) so a batch run does not reformat the
# source as a side effect -- a runner must not mutate the tree.
shell-exec: image ## Run a script/command in the container env (no TTY): make shell-exec SCRIPT=path | CMD='...'
	@[ -n "$(SCRIPT)$(CMD)" ] || { echo 'usage: make shell-exec SCRIPT=<mounted path> | CMD="..."'; exit 2; }
	$(CONTAINER_CMD) run --rm $(PODMAN_RUN_FLAGS) \
		$(SHELL_RUN_FLAGS) \
		$(CONTAINER_NAME) \
		/usr/local/bin/shell.sh $(SHELL_EXEC_ARGS)

.PHONY: format
format: image ## Format first-party C/C++ with clang-format in the container (reproducible)
	$(CONTAINER_CMD) run --rm $(PODMAN_RUN_FLAGS) \
		--entrypoint /bin/bash \
		$(FILES_TO_MOUNT) \
		$(CONTAINER_NAME) \
		/usr/local/bin/format.sh

.PHONY: lint
lint: build ## Advisory static analysis (clang-tidy bug checks + cppcheck); never fatal
	$(CONTAINER_CMD) run --rm $(PODMAN_RUN_FLAGS) \
		--entrypoint /bin/bash \
		$(FILES_TO_MOUNT) \
		$(CONTAINER_NAME) \
		/usr/local/bin/lint.sh

.PHONY: image-export
image-export: ## Export the OCI image to a timestamped tar in the repo root
	$(CONTAINER_CMD) save $(CONTAINER_NAME) -o $(CONTAINER_NAME)-$(shell date +%m-%d-%Y_%H-%M-%S).tar

.PHONY: image-import
image-import: ## Import an OCI image tar: make image-import FILE=foo.tar
	$(CONTAINER_CMD) load -i $(FILE)

# Refresh the vendored Emacs packages. Forces USE_EMACS=1 and rebuilds the image
# first. Then, in the container, wipes ONLY the elpa/ subtree (init.el /
# preferences.el / helm.el / install-melpa-packages.el are hand-written config,
# left untouched) and reinstalls from MELPA into the host's bind-mounted
# .emacs.d/elpa. Finally strips compiled *.elc/*.eln (machine-specific) and
# force-stages the elpa tree. Needs network. Mirrors billsEmacsConfigs/c + gltron.
.PHONY: update-emacs-packages
update-emacs-packages: ## USE_EMACS=1: rebuild image, wipe+reinstall elpa, strip *.elc/*.eln, git add -f
	$(MAKE) image USE_EMACS=1
	$(CONTAINER_CMD) run --rm $(PODMAN_RUN_FLAGS) \
		-v $(CURDIR)/entrypoint/dotfiles/.emacs.d/:/root/.emacs.d/:U,z \
		--entrypoint /bin/bash \
		$(CONTAINER_NAME) \
		-c 'mkdir -p /root/.emacs.d/elpa; find /root/.emacs.d/elpa -mindepth 1 -delete; \
		    emacs --batch --load /root/.emacs.d/install-melpa-packages.el'
	mkdir -p $(CURDIR)/entrypoint/dotfiles/.emacs.d/elpa && cd $(CURDIR)/entrypoint/dotfiles/.emacs.d/elpa && \
		find . \( -iname '*.elc' -o -iname '*.eln' \) -delete && \
		git add -A -f .
	@echo "Done: reinstalled packages, stripped *.elc/*.eln, staged elpa -- review and commit."

# --- Re-vendor a third-party source dep from upstream at a ref (branch/tag/SHA or version).
# Each overwrites deps/<dep> and prints the vendored version; run on the HOST (git/curl/
# unzip/python3 + cp, no container). Convention: the personal overlay Makefile contract
# ("update-<dep> targets for deps vendored BY SOURCE"); plan + recipes:
# tasks/add-dep-update-targets.md. After a bump, commit as '<dep> <ref>' (git-history pin).
.PHONY: update-imgui
update-imgui: ## Re-vendor Dear ImGui: make update-imgui IMGUI_REF=v1.92.5 (tag|branch|SHA)
	@[ -n "$(IMGUI_REF)" ] || { echo 'usage: make update-imgui IMGUI_REF=<tag|branch|SHA>'; exit 2; }
	bash tools/update-imgui.sh "$(IMGUI_REF)"

.PHONY: update-glfw
update-glfw: ## Re-vendor GLFW: make update-glfw GLFW_REF=3.4 (tag|branch|SHA)
	@[ -n "$(GLFW_REF)" ] || { echo 'usage: make update-glfw GLFW_REF=<tag|branch|SHA>'; exit 2; }
	bash tools/update-glfw.sh "$(GLFW_REF)"

.PHONY: update-curl
update-curl: ## Re-vendor curl: make update-curl CURL_REF=curl-8_18_0 (git tag|SHA)
	@[ -n "$(CURL_REF)" ] || { echo 'usage: make update-curl CURL_REF=<git tag|SHA>'; exit 2; }
	bash tools/update-curl.sh "$(CURL_REF)"

.PHONY: update-lodepng
update-lodepng: ## Re-vendor LodePNG: make update-lodepng LODEPNG_REF=<commit|branch|tag>
	@[ -n "$(LODEPNG_REF)" ] || { echo 'usage: make update-lodepng LODEPNG_REF=<commit|branch|tag>'; exit 2; }
	bash tools/update-lodepng.sh "$(LODEPNG_REF)"

.PHONY: update-gl3w
update-gl3w: ## Re-vendor + regenerate gl3w: make update-gl3w GL3W_REF=<commit|branch|tag>
	@[ -n "$(GL3W_REF)" ] || { echo 'usage: make update-gl3w GL3W_REF=<commit|branch|tag>'; exit 2; }
	bash tools/update-gl3w.sh "$(GL3W_REF)"

.PHONY: update-sqlite
update-sqlite: ## Re-vendor SQLite: make update-sqlite SQLITE_REF=3.47.2 [SQLITE_YEAR=2024] (year needed for old versions)
	@[ -n "$(SQLITE_REF)" ] || { echo 'usage: make update-sqlite SQLITE_REF=<version, e.g. 3.47.2> [SQLITE_YEAR=<year>]'; exit 2; }
	bash tools/update-sqlite.sh "$(SQLITE_REF)" "$(SQLITE_YEAR)"

.PHONY: update-tinycthread
update-tinycthread: ## Re-vendor tinycthread (default master): make update-tinycthread TINYCTHREAD_REF=<commit>
	bash tools/update-tinycthread.sh $(TINYCTHREAD_REF)

.PHONY: update-noise
update-noise: ## Re-vendor noise from fogleman/Craft (default master): make update-noise NOISE_REF=<commit>
	bash tools/update-noise.sh $(NOISE_REF)

# ----------------------------------------------------------------------------
# Host CMake build (Windows / macOS, or Linux without the container).
#
# The container targets above are the Linux dev path and build with Meson. CMake
# stays the cross-platform build (Meson here is Linux-only -- see
# tasks/add-container-template.md); on Win/Mac these are the only make-based path.
# They run cmake on the HOST, not in the container.
# ----------------------------------------------------------------------------

.PHONY: clean
clean: ## [host] delete all non-version-controlled files (git clean -fdx)
	git clean -fdx .

.PHONY: debug
debug: ## [host] build Craft in debug mode via CMake (OpenGL) -> ./debugBuildInstall
	cmake -S. -BdebugBuild
	mkdir -p debugBuildInstall
	cmake -DCMAKE_INSTALL_PREFIX=./debugBuildInstall -DENABLE_VULKAN_RENDERER=NO \
	-DENABLE_OPENGL_CORE_PROFILE_RENDERER=YES -DCMAKE_BUILD_TYPE=Debug -S. -BdebugBuild
	cmake --build debugBuild
	cmake --install debugBuild

.PHONY: release
release: ## [host] build Craft in release mode via CMake (OpenGL) -> ./releaseBuildInstall
	cmake -S. -BreleaseBuild
	mkdir -p releaseBuildInstall
	cmake -DCMAKE_INSTALL_PREFIX=./releaseBuildInstall  -DENABLE_VULKAN_RENDERER=NO \
	-DENABLE_OPENGL_CORE_PROFILE_RENDERER=YES -DCMAKE_BUILD_TYPE=Release  -S. -BreleaseBuild
	cmake --build releaseBuild
	cmake --install releaseBuild

.PHONY: sanitize
sanitize: ## [host] build+run the src/-scoped ASan & UBSan(trap) smoke harness via CMake (gate)
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
