# Project tit (aka LibUI-Android)

This is an Android View backend for [libui-ng](https://github.com/libui-ng/libui-ng). It allows you to create native Android UIs
using the same C API you use to create cross-platform desktop apps using LibUI.

Since libui-ng has bindings for many different programming languages, this also allows you to create UIs in Rust, Nim, Zig, and many others.

Once finished, this project can let you:
- Implement basic app functionality without writing hundreds of lines of boilerplate
- Write apps in your favorite compiled language
- Add modding support to your app, and allow UI to be created and modified on runtime
- Run the same code on Windows, MacOS, Linux, and Android

Since the scope of this project is limited, this is not a replacement for React Native. It won't give you low-level
hardware APIs, and it won't let you fine-tune UI layouts. It will only act as a very basic layer allowing you to create
basic UI widgets from compiled languages.

## Use cases:
- Use [LibUI Lua bindings](https://github.com/zevv/libuilua) and allow in-app plugins to manipulate the UI
- Very limited and lightweight alternative to Qt (or React Native)
- Easy to drop in existing projects (one Java file, two C files)

## How to use:
- Copy the files into your project
- Submodule?
- Symlink these files into your project

## Roadmap
- [x] uiButton, uiLabel, progress bar
- [x] Click events
- [x] uiBox
- [x] uiTab (swipe, tab bar)
- [x] Access strings.xml text from string ID
- [x] Create 'activities' on runtime (uiScreenSwitch)
- [x] UI call queue support (uiQueue)
- [ ] uiImage
- [ ] Set widget text, visibility, hidden, destroy
- [ ] Tables, rows, columns, background colors, etc
- [ ] Set action bar buttons (Window close/title?)
- [ ] Dropdowns, entries, ratio, combo box, sliders
- [ ] File dialog
- [ ] ScrollView (extension, WIP upstream)

## Planned
- iOS Swift UI support
- Strip out as much build system bloat as possible, allow Android apps to be written entirely in C (similar to https://github.com/cnlohr/rawdrawandroid)
Or - just a minimal starter template for LibUI, with as much stuff stripped out as possible.
