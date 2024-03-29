# LibUI Framework

This is an Android View backend for [libui-ng](https://github.com/libui-ng/libui-ng). It enables you to write basic Android UIs entirely
in C or [any other language](https://github.com/libui-ng/libui-ng#user-content-language-bindings) that libui has bindings for.

This library is entirely compatible with libui-ng, but some extra controls and features are added, such as `uiScroll`.

## Short term goals
- Perfect compatibility with libui
- [Write a minimal PoC app in C](https://github.com/petabyt/us)
- Write basic apps in dozens of different languages
- Make it easier to drop this library in existing projects

## Use cases
- Use [LibUI Lua bindings](https://github.com/zevv/libuilua) and allow in-app plugins to manipulate the UI
- Very limited and lightweight alternative to Qt (or React Native)
- Drop libuifw in existing projects to help speed up development

## How to use
- Copy the files into your project
- Submodule?
- Symlink these files into your project

## Roadmap
- [x] Complete most basic controls
- [x] Click events
- [x] uiTab (swipable on AndroidX, or tab bar)
- [x] Access strings.xml text from string ID (`uiGet`)
- [x] UI call queue support (`uiQueue`)
- [x] `uiScroll` (extension, WIP upstream)
- [ ] `uiImage`
- [ ] Set control text, visibility, hidden, destroy
- [ ] Tables
- [ ] ratio, combo box, sliders
- [ ] File dialog

## Planned
- iOS uikit support
