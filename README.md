# LibUI Android
This is an a basic C API over the classic Android View UI system. It also copies the [libui-ng](https://github.com/libui-ng/libui-ng)
API, so this allows libui C code to be run on Android, with no modification (requires a little bit of glue)

Use cases:
- Use [LibUI Lua bindings](https://github.com/zevv/libuilua) and allow in-app plugins to manipulate the UI
- Very limited and lightweight alternative to Qt (or React Native)

How to use:
- Copy the files into your project
- Submodule?
- Symlink these files into your project

- [x] uiButton, uiLabel, progress bar
- [x] Click events
- [x] uiBox
- [x] TabView (swipe, tab bar)
- [x] Access strings.xml text from string ID
- [ ] Set widget text, visibility, hidden, destroy
- [ ] Tables
- [ ] Set action bar buttons (Window close/title?)
- [ ] Dropdowns, entries, ratio, combo box, sliders
- [ ] File dialog
- [x] UI call queue support (uiQueue)