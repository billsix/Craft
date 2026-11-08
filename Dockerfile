FROM registry.fedoraproject.org/fedora:45

# A GLFW + OpenGL GUI game; USE_GRAPHICS pulls in the X11/Mesa (and Vulkan) libs so the
# window can open via `make shell` display passthrough. USE_EMACS adds the Emacs binary
# (clangd itself is always installed by 01-install-base.sh). Emacs's packages are never
# installed here: `make shell` bind-mounts the vendored entrypoint/dotfiles/.emacs.d/elpa
# and init.el byte-compiles it on Emacs's first start.
ARG USE_GRAPHICS=1
ARG USE_EMACS=0

# System-package installation lives in per-group scripts (entrypoint/0N-install-*.sh),
# host-runnable with no container runtime; WHICH optional groups run is decided by the
# ARG `if` blocks here. The dnf cache mount / tsflags sed / keepcache stay in the
# Dockerfile (build plumbing). 01-install-base.sh also does `dnf upgrade`.
COPY entrypoint/01-install-base.sh entrypoint/02-install-graphics.sh \
     entrypoint/03-install-emacs.sh /usr/local/bin/

RUN --mount=type=cache,target=/var/cache/libdnf5 \
    --mount=type=cache,target=/var/lib/dnf \
    sed -i -e "s@tsflags=nodocs@#tsflags=nodocs@g" /etc/dnf/dnf.conf && \
    echo "keepcache=True" >> /etc/dnf/dnf.conf && \
    /usr/local/bin/01-install-base.sh && \
    if [ "$USE_GRAPHICS" = "1" ]; then /usr/local/bin/02-install-graphics.sh; fi && \
    if [ "$USE_EMACS" = "1" ]; then /usr/local/bin/03-install-emacs.sh; fi && \
    echo 'set debuginfod enabled off' > /root/.gdbinit

# Emacs config (init.el, helm.el, preferences.el, install-melpa-packages.el) into /root.
# .dockerignore keeps the vendored elpa/ out; the Makefile's ELPA_MOUNT supplies it.
COPY entrypoint/dotfiles/ /root/

COPY entrypoint/buildDebug.sh entrypoint/format.sh entrypoint/lint.sh entrypoint/shell.sh /usr/local/bin/
COPY entrypoint/entrypoint.sh /

# Craft source: meson build files, first-party src/, vendored deps/, resources, server.
COPY .clang-format /craft/
COPY meson.build meson_options.txt /craft/
COPY src /craft/src/
COPY deps /craft/deps/
COPY shaders /craft/shaders/
COPY textures /craft/textures/
COPY tests /craft/tests/
COPY world.py server.py builder.py /craft/

# Build + install the client at image-build time (ready-to-run image).
RUN /usr/local/bin/buildDebug.sh

ENTRYPOINT ["/entrypoint.sh"]
