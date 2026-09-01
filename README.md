# Sprite Sheet Packer CLI

A Qt 6 command-line sprite-sheet generator for macOS, Linux, and Windows. This
branch builds a native CLI only; the legacy Qt Widgets GUI and PVR texture path
are not part of the build.

## Features

- Rectangular and polygon packing
- Trimming, scaling, rotation, borders, power-of-two and square sheets
- Multiple sheets and project scaling variants
- PNG texture output
- Cocos2d plist, Corona/Solar2D, generic JSON, PixiJS, Phaser, and Godot metadata
- `.ssp`, JSON project, and TexturePacker `.tps` input

The metadata exporters are implemented in C++, so Qt QML is not required.

## Dependencies

The required Qt modules are `Core`, `Gui`, and `Xml`. On macOS with Homebrew:

```sh
brew install qtbase cmake ninja
```

`qtimageformats` is optional. Install it when additional input image formats
such as WebP or TIFF are needed:

```sh
brew install qtimageformats
```

## Build

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase)" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The executable is `build/sprite-sheet-packer`.

## Usage

Pack a directory and write a PNG plus PixiJS JSON:

```sh
build/sprite-sheet-packer path/to/sprites path/to/output \
  --format pixijs \
  --output-name player
```

Build using settings and output paths stored in a project:

```sh
build/sprite-sheet-packer path/to/project.ssp
```

An explicit destination overrides the one stored in a project:

```sh
build/sprite-sheet-packer path/to/project.ssp path/to/output
```

Run `sprite-sheet-packer --help` for all packing options, or
`sprite-sheet-packer --list-formats` for the built-in metadata formats.
`sprite-sheet-packer --list-image-formats` reports the input formats currently
provided by Qt and installed image plugins.

The two Corona/Solar2D formats produce the same compact JSON structure but use
different frame names: `corona` keeps only the sprite file name, while
`corona2` keeps `direct-parent/file-name` and removes extensions.

Only PNG texture output is currently supported. Legacy optipng/pngquant
optimization and PVRTexTool-based PVR/PKM output are intentionally excluded.

## License

See [LICENSE.md](LICENSE.md).
