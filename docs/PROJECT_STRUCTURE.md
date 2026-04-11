# Project Structure

Cyn0 keeps source code, data, assets, Qt resources, packaging files, and third-party code in separate top-level directories. This keeps the GitHub repository readable while still allowing Qt Creator to load everything from `Cyn0.pro`.

## Directories

- `src/`: C++ implementation files.
- `include/`: Project headers.
- `ui/`: Qt Designer `.ui` forms.
- `data/json/`: Application JSON databases.
- `assets/`: Runtime assets such as icons, fonts, images, and sounds.
- `styles/`: QSS themes and style sheets.
- `resources/qrc/`: Qt resource collection files. These files map the organized repository paths back to stable runtime resource paths such as `:/json/index.json`.
- `third_party/`: Vendored dependencies. QHotkey lives here.
- `packaging/`: Platform-specific packaging files.
- `archive/`: Old backup/reference files kept out of the application build.

## Qt Resource Rule

Application code should use stable resource paths:

- `:/json/...` for JSON databases.
- `:/qss/...` for styles.
- `:/fonts/...` for embedded fonts.
- `:/icon/...` for icons.

Physical files may move inside the repository, but `resources/qrc/*.qrc` should preserve those runtime aliases so application code does not need path churn.

## Generated Files

Build outputs, qmake cache files, MOC files, RCC files, object files, and local IDE files should not be committed. `Cyn0.pro` directs generated qmake files into `build/generated/` for in-place builds.
