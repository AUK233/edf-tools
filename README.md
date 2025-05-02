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
- `.mrab` <-> Folder include model and textures
- `.txt` <-> `.bvm`
- `.rab` <-> Folder include models and textures
- `.mab` -> `.xml`
- `.mtab` -> `.xml`
- `.rmpa` -> `.xml`
- `.sgo` -> `.xml`

### Convert model files

convert folder back to `.rab` or `.mrab`:

```batch
"EDF Tool.exe" /ARCHIVE [optional1] [optional2] <Folder Name>
```

Example:
```batch
"EDF Tool.exe" /ARCHIVE ANTHILL301
```

Acceptable `optional1` arguments: `-fc`(use a faster packing method, but it will bloat the filesize significantly), `-mt`(use all threads to compress the file, if there are really enough files), `-mc`(select P-Core for compression), `-cmtn`(use 4 cores to compress files if you don't add `optional2`), pass the number of cores to `optional2`

### 3dsmax support

Download the [source code](https://github.com/AUK233/edf-tools/archive/refs/heads/master.zip) and import the script in folder `3dsmax Scripts`, then run the script and select the tool on "Utilities" panel.

Alsoly, refer [readme.txt](./3dmax%20Scripts/readme.txt).

It's still in an early stage.

## Build

Required: [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/?cid=learn-onpage-download-install-visual-studio-page-cta)