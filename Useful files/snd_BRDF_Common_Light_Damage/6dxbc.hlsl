cbuffer xgl_system : register( b0 )
{
	float4x4 g_xgl_view;
	float4x4 g_xgl_view_inverse;
	float4x4 g_xgl_projection;
	float4x4 g_xgl_view_projection;
	float4x4 g_xgl_view_projection_inverse;
	uint2 g_xgl_target_dimension;
	float g_xgl_time = 0;
	int g_xgl_id = 0;
	float3 g_xgl_light_vector0 = {0.699999988,-0.699999988,-0.699999988};
	float3 g_xgl_light_color0 = {1,1,1};
	float3 g_xgl_light_specular_color0  = {1,1,1};
	float3 g_xgl_light_vector1 = {-0.699999988,0.699999988,0.699999988};
	float3 g_xgl_light_color1 = {0.200000003,0.200000003,0.200000003};
	float3 g_xgl_light_specular_color1 = {1,1,1};
	float3 g_xgl_light_vector2 = {0,0,0};
	float3 g_xgl_light_color2 = {0,0,0};
	float3 g_xgl_light_specular_color2 = {1,1,1};
	float3 g_xgl_light_vector3 = {0,0,0};
	float3 g_xgl_light_color3 = {0,0,0};
	float3 g_xgl_light_specular_color3 = {0,0,0};
	float3 g_xgl_ambient_color = {0.200000003,0.200000003,0.200000003};
	float g_pad2;
	float4 g_xgl_fog_fade_param = {0,0,0,0};
	float4 g_xgl_fog_color = {1,1,1,0};
}

cbuffer xgl_user_param : register(b2)
{
  float3 diffuse = {0.5,0.5,0.5};
  float3 specular_color = {1,1,1};
  float specular_diffuse_blend = 0.5;
  float3 light_color = {1,1,1};
  float normal_amount = 1;
  float metallic = 1;
  float roughness = 1;
  float damage_test = 0;
  float2 damage_uv_mul = {10,10};
  float2 damage_mask_uv_mul = {10,10};
  float damage_normal_amount= 1;
  float3 damage_specular_color = {1,1,1};
  float3 damage_edge_color = {1,1,1};
  float3 damage_max_color = {0.200000003,0.100000001,0.100000001};
  float damage_metallic = 0;
  float damage_roughness = 0;
  float damage_light_mask = 0;

  struct
  {
    float3 m_org;
    uint m_line_pitch;
    float3 m_inv_grid_size;
    uint m_slice_pitch;
  } sys_damage_map_param;

}

Texture2D albedo;
SamplerState albedo_sampler;

Texture2D normal;
SamplerState normal_sampler;

Texture2D param_r_m_occ_light;
SamplerState param_r_m_occ_light_sampler;

Texture2D damage_dist;
SamplerState damage_dist_sampler;

Texture2D damage_diffuse;
SamplerState damage_diffuse_sampler;

Texture2D damage_normal;
SamplerState damage_normal_sampler;

StructuredBuffer<uint> sys_damage_map;

#define cmp -

void main(
  float4 v0 : SV_POSITION0,
  float3 v1 : EYE_VECTOR0,
  float3 v2 : VIEW_POSITION0,
  float3 v3 : LOCAL_POSITION0,
  float3 v4 : NORMAL0,
  float3 v5 : BINORMAL0,
  float3 v6 : TANGENT0,
  float2 v7 : TEXCOORD0,
  out float4 o0 : SV_TARGET0,
  out float4 o1 : SV_TARGET1,
  out float4 o2 : SV_TARGET2,
  out float4 o3 : SV_TARGET3,
  out float4 o4 : SV_TARGET5,
  out float4 o6 : SV_TARGET7)
{
  float4 r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11;
  uint4 bitmask, uiDest;
  float4 fDest;

  r0.x = dot(v4.xyz, v4.xyz);
  r0.x = rsqrt(r0.x);
  r0.xyz = v4.xyz * r0.xxx;
  r0.w = dot(v5.xyz, v5.xyz);
  r0.w = rsqrt(r0.w);
  r1.xyz = v5.xyz * r0.www;
  r0.w = dot(v6.xyz, v6.xyz);
  r0.w = rsqrt(r0.w);
  r2.xyz = v6.xyz * r0.www;
  r3.xyzw = param_r_m_occ_light.Sample(param_r_m_occ_light_sampler, v7.xy).wxyz;
  r3.x = saturate(r3.x);
  r4.xyz = albedo.Sample(albedo_sampler, v7.xy).xyz;
  r4.xyz = diffuse.xyz * r4.xyz;
  r3.yz = metallic * r3.zy;
  r0.w = specular_diffuse_blend * r3.y;
  r5.xyz = r4.xyz * specular_color.xyz + -specular_color.xyz;
  r5.xyz = r0.www * r5.xyz + specular_color.xyz;
  r0.w = 1 + -r3.x;
  r5.xyz = r5.xyz * r0.www;
  r6.xyz = -sys_damage_map_param.m_org.xyz + v3.xyz;
  r6.xyz = sys_damage_map_param.m_inv_grid_size.xyz * r6.xyz;
  r7.xyz = frac(r6.xyz);
  r6.xyz = (int3)r6.xyz;
  r8.xy = (int2)r6.xy + int2(1,1);
  r6.w = r8.y;
  r8.z = r6.x;
  r9.xyzw = mad((int4)r6.yyww, sys_damage_map_param.m_line_pitch, (int4)r8.zxzx);
  r9.xyzw = mad((int4)r6.zzzz, sys_damage_map_param.m_slice_pitch, (int4)r9.xyzw);
  r10.xyzw = (uint4)r9.xyzw >> int4(4,4,4,4);
  r11.x = sys_damage_map[r10.x].x;
  r11.y = sys_damage_map[r10.y].x;
  r11.z = sys_damage_map[r10.z].x;
  r11.w = sys_damage_map[r10.w].x;
  bitmask.x = ((~(-1 << 4)) << 1) & 0xffffffff;  r9.x = (((uint)r9.x << 1) & bitmask.x) | ((uint)0 & ~bitmask.x);
  bitmask.y = ((~(-1 << 4)) << 1) & 0xffffffff;  r9.y = (((uint)r9.y << 1) & bitmask.y) | ((uint)0 & ~bitmask.y);
  bitmask.z = ((~(-1 << 4)) << 1) & 0xffffffff;  r9.z = (((uint)r9.z << 1) & bitmask.z) | ((uint)0 & ~bitmask.z);
  bitmask.w = ((~(-1 << 4)) << 1) & 0xffffffff;  r9.w = (((uint)r9.w << 1) & bitmask.w) | ((uint)0 & ~bitmask.w);
  r9.xyzw = (uint4)r11.xyzw >> (uint4)r9.xyzw;
  r9.xyzw = (int4)r9.xyzw & int4(3,3,3,3);
  r9.xyzw = (uint4)r9.xyzw;
  r9.xyzw = float4(0.333333343,0.333333343,0.333333343,0.333333343) * r9.xyzw;
  r6.xyz = (int3)r6.xyz + int3(0,0,1);
  r8.w = r6.x;
  r8.xyzw = mad((int4)r6.yyww, sys_damage_map_param.m_line_pitch, (int4)r8.wxwx);
  r6.xyzw = mad((int4)r6.zzzz, sys_damage_map_param.m_slice_pitch, (int4)r8.xyzw);
  r8.xyzw = (uint4)r6.xyzw >> int4(4,4,4,4);
  r10.x = sys_damage_map[r8.x].x;
  r10.y = sys_damage_map[r8.y].x;
  r10.z = sys_damage_map[r8.z].x;
  r10.w = sys_damage_map[r8.w].x;
  bitmask.x = ((~(-1 << 4)) << 1) & 0xffffffff;  r6.x = (((uint)r6.x << 1) & bitmask.x) | ((uint)0 & ~bitmask.x);
  bitmask.y = ((~(-1 << 4)) << 1) & 0xffffffff;  r6.y = (((uint)r6.y << 1) & bitmask.y) | ((uint)0 & ~bitmask.y);
  bitmask.z = ((~(-1 << 4)) << 1) & 0xffffffff;  r6.z = (((uint)r6.z << 1) & bitmask.z) | ((uint)0 & ~bitmask.z);
  bitmask.w = ((~(-1 << 4)) << 1) & 0xffffffff;  r6.w = (((uint)r6.w << 1) & bitmask.w) | ((uint)0 & ~bitmask.w);
  r6.xyzw = (uint4)r10.xyzw >> (uint4)r6.xyzw;
  r6.xyzw = (int4)r6.xyzw & int4(3,3,3,3);
  r6.xyzw = (uint4)r6.xyzw;
  r6.xyzw = r6.xyzw * float4(0.333333343,0.333333343,0.333333343,0.333333343) + -r9.xyzw;
  r6.xyzw = r7.zzzz * r6.xyzw + r9.xyzw;
  r6.zw = r6.zw + -r6.xy;
  r6.xy = r7.yy * r6.zw + r6.xy;
  r1.w = r6.y + -r6.x;
  r1.w = r7.x * r1.w + r6.x;
  r6.xy = damage_mask_uv_mul.xy * v7.xy;
  r2.w = damage_dist.Sample(damage_dist_sampler, r6.xy).x;
  r1.w = -r2.w + r1.w;
  r1.w = -r3.x * damage_light_mask + r1.w;
  r2.w = cmp(0 < r1.w);
  if (r2.w) {
    r6.xy = damage_uv_mul.xy * v7.xy;
    r7.xyz = damage_normal.Sample(damage_normal_sampler, r6.xy).xyw;
    r7.x = r7.x * r7.z;
    r6.zw = float2(-0.5,-0.5) + r7.xy;
    r6.zw = r6.zw + r6.zw;
    r2.w = dot(r6.zw, r6.zw);
    r2.w = 1 + -r2.w;
    r2.w = max(9.99999975e-06, r2.w);
    r2.w = sqrt(r2.w);
    r6.zw = damage_normal_amount * r6.zw;
    r7.xyz = r6.www * r1.xyz;
    r7.xyz = r6.zzz * r2.xyz + r7.xyz;
    r7.xyz = r2.www * r0.xyz + r7.xyz;
    r2.w = dot(r7.xyz, r7.xyz);
    r2.w = rsqrt(r2.w);
    r7.xyz = r7.xyz * r2.www;
    r6.xyz = damage_diffuse.Sample(damage_diffuse_sampler, r6.xy).xyz;
    r2.w = saturate(r1.w);
    r8.xyz = damage_max_color.xyz + -r6.xyz;
    r6.xyz = r2.www * r8.xyz + r6.xyz;
    r1.w = cmp(r1.w < 0.100000001);
    r6.xyz = r1.www ? damage_edge_color.xyz : r6.xyz;
    r5.xyz = r1.www ? r5.xyz : damage_specular_color.xyz;
    r3.y = r1.w ? r3.y : damage_metallic;
    r3.z = r1.w ? r3.z : damage_roughness;
    r8.xyz = float3(0,0,0);
  } else {
    r9.xyz = normal.Sample(normal_sampler, v7.xy).xyw;
    r9.x = r9.x * r9.z;
    r9.xy = float2(-0.5,-0.5) + r9.xy;
    r9.xy = r9.xy + r9.xy;
    r1.w = dot(r9.xy, r9.xy);
    r1.w = 1 + -r1.w;
    r1.w = max(9.99999975e-06, r1.w);
    r1.w = sqrt(r1.w);
    r9.xy = normal_amount * r9.xy;
    r1.xyz = r9.yyy * r1.xyz;
    r1.xyz = r9.xxx * r2.xyz + r1.xyz;
    r0.xyz = r1.www * r0.xyz + r1.xyz;
    r1.x = dot(r0.xyz, r0.xyz);
    r1.x = rsqrt(r1.x);
    r7.xyz = r1.xxx * r0.xyz;
    r0.xyz = light_color.xyz * r4.xyz;
    r8.xyz = r0.xyz * r3.xxx;
    r6.xyz = r4.xyz * r0.www;
  }
  o2.xyz = r7.xyz * r3.www;
  o4.w = g_xgl_id;
  o0.xyz = r6.xyz;
  o0.w = 1;
  o1.xyz = r5.xyz;
  o1.w = 1;
  o2.w = 1;
  o3.xy = r3.yz;
  o3.zw = float2(0,1);
  o4.xyz = v2.xyz;
  o6.xyz = r8.xyz;
  o6.w = 1;
  return;
}