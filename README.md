# EDF Tools

Original file from: [gitlab.com/kittopiacreator/edf-tools](https://gitlab.com/kittopiacreator/edf-tools)

A asset-convert tool for Earth Defender Force.

## Usage

Download the latest file from [Release page](https://github.com/AUK233/edf-tools/releases)

### Convert files

Just drag the file to the .exe, or run the .exe on terminal then pass the file path as argument.

The output will be put to the original file's location.

The filename extension should be correct, Like:

- Acceptable: `EXAMPLE.rab`, Unacceptable: `EXAMPLE.rabbackup`
- Acceptable: `EXAMPLE_MDB.xml`, Unacceptable: `EXAMPLE.xml`

Support file types:

- `.canm` <-> `.xml`
- `.cas` <-> `.xml`
- `.mdb` <-> `.xml`
- `.mrab` <-> Folder
- `.rab` <-> Folder
- `.efarc` <-> Folder
- `.txt` -> `.bvm`
- `.mab` <-> `.xml`
- `.mtab` <-> `.xml`
- `.rmpa` <-> `.xml` (EDF6 Only)
- `.sgo` <-> `.xml`

### Command line

```batch
"EDF Tool.exe" [optional1] [optional2] [optional3] <path>
```

A directly input `path` will be auto-recognized and converted. If it is a folder, it will be packed without compression as a `RAB` file.

`optional1` arguments:
- `-br`: Use `path` as input folder. Convert supported binary data to text.
- `-bw`: Use `path` as input folder. Convert supported text to binary data.
- `-pd`: Use `path` as input folder. Archive a folder to `RAB`, `optional2` is required.
- `-px`: Same as above, but it will process `XML` files in `MODEL` folder.

`optional2` arguments:
- `-in` (Default compression)
- `-fc` (No compression)
- `-mc` (Multithreading, but only P-Core)
- `-mt` (Multithreading compression)
- `-st` (Multi-core compression, require `optional3` as cores count)

#### Example:

convert folder to `.rab` `.mrab` `.efarc`:

```batch
"EDF Tool.exe" -px -st 6 ANTHILL301
```
In ANTHILL301, `XML` files within `MODEL` folder will be converted to `MDB`.
Then it will use 6 cores to compress the files, finally packaging them into `RAB`.

### 3dsmax support

Download the [source code](https://github.com/AUK233/edf-tools/archive/refs/heads/master.zip) and import the script in folder `3dsmax Scripts`, then run the script and select the tool on "Utilities" panel.

Alsoly, refer [readme.txt](./3dmax%20Scripts/readme.txt).

It's still in an early stage.

## Build

Required: [Visual Studio 2026](https://visualstudio.microsoft.com/downloads)
