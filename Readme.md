# VibeKidbright IDE 3.7.2

ESP-IDF IDE and AI assistant for classic KidBright32 boards, with a local Unity simulator. This release includes the existing board Build/Flash workflow, paired C/JavaScript generation, simulator validation and native macOS SDK installation.

- [Release notes](docs/releases/3.7.2.md)
- [Local simulator guide](docs/LOCAL-SIMULATOR-3.7.1-TH.md)
- [macOS setup and recovery](docs/MACOS-ESP-IDF-RECOVERY-TH.md)

The simulator implements a checked subset of board behavior; it does not execute arbitrary ESP-IDF firmware or emulate Wi-Fi/FreeRTOS. Firmware and simulator programs remain separate files.

## Build and check

Use a Node version accepted by package.json and the Tauri build prerequisites for the target OS.

```sh
npm ci
npm test
npm run build:release
cargo test --locked --manifest-path src-tauri/Cargo.toml --lib
cargo check --locked --manifest-path src-tauri/Cargo.toml
npm run tauri -- build --ci
```

Push a version tag such as `v3.7.2` to run the Windows and macOS installer workflows. Both attach artifacts to the same **draft** release without changing the latest public release. macOS packaging waits for native Intel and Apple Silicon SDK installation and firmware-build checks. Signing/notarization requires the corresponding repository secrets.

Local verification was performed on Windows. Actual macOS installer/firmware results must be read from Actions for the release tag. GUI installation, upgrades and Flash on real hardware still require hands-on verification before public distribution. ESP-IDF is downloaded separately from the IDE installer.
