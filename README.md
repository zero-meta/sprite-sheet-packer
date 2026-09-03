# Sprite Sheet Packer CLI

A Qt 6 command-line sprite-sheet generator for macOS, Linux, and Windows. This
branch builds a native CLI only; the legacy Qt Widgets GUI and PVR texture path
are not part of the build.

## Features

- Rectangular and polygon packing
- Trimming, scaling, rotation, borders, power-of-two and square sheets
- Optional edge-pixel extrusion for repeat sampling with linear filtering
- Multiple sheets and project scaling variants
- PNG, WebP, and JPEG texture output
- Cocos2d plist, Corona/Solar2D, generic JSON, PixiJS, Phaser, and Godot metadata
- `.ssp`, JSON project, and TexturePacker `.tps` input

The metadata exporters are implemented in C++, so Qt QML is not required.

## Dependencies

The required Qt modules are `Core`, `Gui`, and `Xml`. On macOS with Homebrew:

```sh
brew install qtbase cmake ninja
```

`qtimageformats` is optional. Install it when additional input image formats
such as WebP or TIFF, or WebP texture output, are needed:

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
provided by Qt and installed image plugins. `--list-texture-formats` reports
the available texture writers.

Select WebP or JPEG output and quality with:

```sh
build/sprite-sheet-packer path/to/sprites path/to/output \
  --texture-format webp --webp-quality 80
```

WebP preserves transparency. JPEG is accepted only when packed sprite pixels
are opaque; use PNG or WebP for transparent sprites. Passing
`--pixel-format RGB888` explicitly discards Alpha and permits JPEG output. The
project fields `imageFormat`, `webpQuality`, and `jpgQuality` provide the same
settings. Ensure the target runtime can decode the chosen texture format.

Use `--extrude N` to reserve `N` pixels around every rectangular frame and
fill them by duplicating its edge pixels. The frame coordinates in metadata
still describe only the original sprite. Projects use the integer `extrude`
field as a default and may override selected frames with ordered glob rules:

```json
{
  "extrude": 0,
  "extrudeRules": [
    {"pattern": "tiles/repeat/**", "pixels": 2},
    {"pattern": "**/*_repeat.png", "pixels": 1},
    {"pattern": "tiles/repeat/no_extrude/**", "pixels": 0}
  ]
}
```

Patterns match normalized source frame names with `/` separators and support
`*`, `?`, and recursive `**`. Rules are evaluated in array order and the last
match wins. An explicit `--extrude N`, including `--extrude 0`, overrides all
project rules. If identical images use different values, their shared packed
frame uses the largest extrusion. Edge extrusion is intentionally unavailable
with polygon packing.

Use pngquant as an optional lossy post-process when a smaller distribution
file is more important than exact source pixels:

```sh
build/sprite-sheet-packer path/to/sprites path/to/output \
  --pngquant --pngquant-quality 80-95
```

If `pngquant` is not installed, cannot process the image, or produces a larger
file, the original lossless PNG is kept and publishing still succeeds. A
project can enable the same behavior with `"pngOptMode": "Lossy"` and can set
`"pngQuantQuality": "80-95"`. Use `--no-pngquant` to override that project
setting.

Project `srcList` entries may independently be image files or directories,
and relative entries are resolved from the project file's directory.
Directories are searched recursively, so one project may mix folders and
individual files from different locations. Directory entries preserve paths
relative to the directory's parent; individually listed files use only their
base file names. Avoid duplicate final frame names.

The two Corona/Solar2D formats produce the same compact JSON structure but use
different frame names: `corona` keeps only the sprite file name, while
`corona2` preserves the normalized logical source path and removes extensions.
For mixed `srcList` input, directory entries retain their directory namespace
and hierarchy; individually listed files are placed at the logical root using
their base names. Set `prependSmartFolderName` to `false` to remove the first
directory component. Inputs that produce the same logical frame name are
rejected instead of silently overwriting one another.

Legacy embedded optipng/libimagequant code and PVRTexTool-based PVR/PKM output
are intentionally excluded; pngquant support invokes the external executable
for PNG output when requested.

## License

See [LICENSE.md](LICENSE.md).
