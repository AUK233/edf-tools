# Header
It has 0x30 bytes? It's all BigEndian.
Each group is 0x20 bytes.

| Offset | data size | description |
|---|---|---|
| +0 | 4 | header|
| +4 | 4 | version? OK, It's also 1|
| +8 | 4 | route main node count?|
| +C | 4 | route main node offset?|
| +10 | 4 | shape main node count?|
| +14 | 4 | shape main node offset?|
| +18 | 4 | camera main node count?|
| +1C | 4 | camera main node offset?|
| +20 | 4 | point main node count?|
| +24 | 4 | point main node offset?|
| +28 | 8 | 16-byte alignment?|

They need to read the main node first, then get the sub nodes from the main node.
The sub node's data is under the current main node, not under the main node.

Note: String data needs to be 16-byte aligned

## Node Structure
It has 0x20 bytes?

| Offset | data size | description |
|---|---|---|
| +0 | 4 | Always -1? |
| +4 | 4 | unknown |
| +8 | 4 | node name offset, possible null |
| +C | 4 | sub node count? |
| +10 | 4 | sub node offset? |
| +14 | 12 | 16-byte alignment?|


### [Return to Header](#Header)


## Sub Node Structure
It has 0x14 bytes?

| Offset | data size | description |
|---|---|---|
| +0 | 4 | Always -1? |
| +4 | 4 | unknown |
| +8 | 4 | node name offset, possible null |
| +C | 4 | sub node count? |
| +10 | 4 | sub node offset? |

### [Return to Header](#Header)

### Point Node Structure
It has 0x34 bytes?

| Offset | data size | description |
|---|---|---|
| +0 | 4 | unknown |
| +4 | 12 | float3, xyz |
| +10 | 4 | unknown |
| +14 | 12 | float3, xyz |
| +20 | 8 | maybe int64 |
| +28 | 4 | node name offset |
| +2C | 8 | maybe int64 |

#### [Return to Header](#Header)
