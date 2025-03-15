# Header
It has 0x30 bytes.

| Offset | data size | description |
|---|---|---|
| +0 | 4 | header|
| +4 | 4 | version: 4.1 is 00020000, 5 is 03020000|
| +8 | 4 | CANM data offset|
| +C | 4 | TControl count|
| +10 | 4 | TControl offset|
| +14 | 4 | VControl count|
| +18 | 4 | VControl offset|
| +1C | 4 | AnmGroup count|
| +20 | 4 | AnmGroup offset|
| +24 | 4 | BoneList count|
| +28 | 4 | BoneList offset|
| +2C | 4 | UnnamedC block offset. note: structure is CasDataGroup.|

## TControl Structure
It has 12 bytes.

| Offset | data size | description |
|---|---|---|
| +0 | 4 | name offset|
| +4 | 4 | Number data count|
| +8 | 4 | Number data offset|

Number is Int32. Index of animation name in CANM.
### [Return to Header](#Header)

## VControl Structure
It has 0x14 bytes.

| Offset | data size | description |
|---|---|---|
| +0 | 4 | name offset |
| +4 | 4 | Int32 |
| +8 | 4 | Int32 |
| +C | 4 | FP32 |
| +10 | 4 | Int32 |

### [Return to Header](#Header)

## AnmGroup Structure
It has 12 bytes.

| Offset | data size | description |
|---|---|---|
| +0 | 4 | name offset|
| +4 | 4 | MCAnm node count|
| +8 | 4 | MCAnm node offset|

### [Return to Header](#Header)

## MCAnm Structure
It has 0x24 bytes.

- 0x00 - : Int32, start offset of string.
- 0x04 - : Int32, start offset of AnmData1.
- 0x08 - : Int32, AnmData2 amount.
- 0x0C - : Int32, start offset of AnmData2.
- 0x10 - : Int32, start offset of ptr1. note: structure is CasDataGroup.
- 0x14 - : Int32, start offset of ptr2. note: structure is CasDataGroup.
- 0x18 - : Int32, start offset of ptr3. note: structure is CasDataGroup.
- 0x1C - : Int32, determines the type of the next value.
- 0x20 - : 0x1C = 0, it is Float. 0x1C = 2, it is Int32, Like a TControl index. 0x1C = 1, it is Int32?
note: 5th int of 1st structure of ptr2 is TControl index.

### [Return to AnmGroup](#AnmGroup_Structure)

## AnmData1 Structure
It has 0x20 bytes.

- 0x00 - : Int32
- 0x04 - : Float, it seems to be action time.
- 0x08 - : Int32, start offset of CasDataGroup.
- 0x0C - : Int32, determines the type of the next value.
- 0x10 - : 0xC = 0, it is Float. 0xC = 2, it is Int32, Like a TControl index. 0xC = 1, it is Int32?
- 0x14 - : Int32
- 0x18 - : Int32
- 0x1C - : Int32

AnmData2, like AnmData1

### [Return to MCAnm](#MCAnm_Structure)

## CasDataGroup Structure
It has 8 bytes.

| Offset | data size | description |
|---|---|---|
| +0 | 4 | CasDataCommon node count|
| +4 | 4 | CasDataCommon node offset|

### CasDataCommon Structure
It has 52 bytes. (4.1 is 36 bytes)

| Offset | data size | description |
|---|---|---|
| +0 | 4 | type, |

#### [Return to Header](#Header)

## CASBoneList Structure
It has 4 bytes.

| Offset | data size | description |
|---|---|---|
| +0 | 4 | name offset |

### [Return to Header](#Header)
