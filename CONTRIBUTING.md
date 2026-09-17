# Contributing

Bug reports and pull requests are welcome. Please include a minimal reproducible game state for rules issues.

Before opening a pull request:

1. Build the project with CMake.
2. Run `ctest --test-dir build --output-on-failure`.
3. Keep opponent hands, the face-down offer, and deck order out of policy observations.
4. Do not add scans or artwork from the commercial game.

Contributions are accepted under the MIT License.
