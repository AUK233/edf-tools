# Index


# Read assembly code

[You can get a function description of the assembly code from here.](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/shader-model-5-assembly--directx-hlsl-)

Here are some known assembly to HLSL:
| Assembly Code | Intrinsic Function |
|---|---|
|discard|discard|
|dp(2,3,4)|v0 = dot(v1,v2)|
|round_ne|v0 = round(v1)|
|round_pi|v0 = ceil(v1)|
|round_ni|v0 = floor(v1)|
|round_z|v0 = trunc(v1)|
|frc|v0 = frac(v1)|
|mad|v0 = v1*v2+v3|
|ld_indexable|v0 = texture_name.Load(v1)|
|eq|v0 = (v1 == v2)|
