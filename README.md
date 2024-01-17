# LibUI Framework

This is an Android View backend for [libui-ng](https://github.com/libui-ng/libui-ng). It allows you to create native Android apps
in C, Rust, or [any other language](https://github.com/libui-ng/libui-ng#user-content-language-bindings).

## Short term goals
- Implement basic UI layouts without writing hundreds of lines of boilerplate
- Write apps in dozens of different languages
- Be able to run the same code on Windows, MacOS, Linux, and Android and get a similar layout
- Make it easy to drop libuifw in existing projects

It's a *superset* of the LibUI API, giving you basic widgets as well as most things relevant for mobile app development.

## Use cases
- Use [LibUI Lua bindings](https://github.com/zevv/libuilua) and allow in-app plugins to manipulate the UI
- Very limited and lightweight alternative to Qt (or React Native)
- Drop libuifw in existing projects to help speed up development

## How to use
- Copy the files into your project
- Submodule?
- Symlink these files into your project

## Roadmap
- [x] uiButton, uiLabel, progress bar
- [x] Click events
- [x] uiBox
- [x] uiTab (swipe, tab bar)
- [x] Access strings.xml text from string ID
- [x] Emulate 'activities' on runtime (uiScreenSwitch)
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
