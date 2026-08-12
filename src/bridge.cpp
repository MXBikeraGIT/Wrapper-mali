name: Build Bionic Vulkan Wrapper

on:
  push:
    branches: [ main, master ]
  pull_request:
    branches: [ main, master ]
  workflow_dispatch:

# Grants GITHUB_TOKEN write access to create releases and upload build assets
permissions:
  contents: write

jobs:
  build:
    runs-on: ubuntu-latest

    steps:
      - name: Checkout repository
        uses: actions/checkout@v4

      - name: Install Dependencies & Cross-Compiler
        run: |
          sudo apt-get update
          sudo apt-get install -y g++-aarch64-linux-gnu zstd libvulkan-dev

      - name: Build Shared Library
        run: |
          mkdir -p usr/lib
          aarch64-linux-gnu-g++ -shared -fPIC -O3 \
            src/bridge.cpp \
            src/logic.cpp \
            src/output.cpp \
            -Iinclude -Isrc -I. -ldl -o usr/lib/libvulkan_wrapper.so

      - name: Package Archive
        run: |
          tar -I 'zstd --ultra -22' -cvf wrapper.tzst usr/

      - name: Upload Artifacts
        uses: actions/upload-artifact@v4
        with:
          name: bionic-vulkan-wrapper
          path: |
            usr/lib/libvulkan_wrapper.so
            wrapper.tzst

      - name: Create or Update GitHub Release
        if: github.event_name == 'push' && (github.ref == 'refs/heads/main' || github.ref == 'refs/heads/master')
        uses: softprops/action-gh-release@v2
        env:
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
          NODE_TLS_REJECT_UNAUTHORIZED: '0'
          NODE_OPTIONS: '--use-system-ca'
        with:
          tag_name: "testing"
          name: "Nightly / Testing Build"
          body: "Automated release build with Lee Gao dispatch tables, extension filtering, and SPIR-V patching."
          draft: false
          prerelease: true
          files: |
            usr/lib/libvulkan_wrapper.so
            wrapper.tzst
