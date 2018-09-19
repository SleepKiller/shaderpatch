
#include "constants_list.hlsl"
#include "ext_constants_list.hlsl"
#include "fog_utilities.hlsl"
#include "generic_vertex_input.hlsl"
#include "vertex_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_utilities.hlsl"

Texture2D<float4> diffuse_map : register(ps_3_0, s0);
Texture2D<float3> detail_map : register(ps_3_0, s1);
Texture2D<float3> projected_texture : register(ps_3_0, s2);
Texture2D<float4> shadow_map : register(ps_3_0, s3);

SamplerState linear_wrap_sampler;

float4 blend_constants : register(ps, c[0]);

float2 lighting_factor : register(c[CUSTOM_CONST_MIN]);
float4 texture_transforms[4] : register(vs, c[CUSTOM_CONST_MIN + 1]);

const static bool use_transparency = NORMAL_USE_TRANSPARENCY;
const static bool use_hardedged_test = NORMAL_USE_HARDEDGED_TEST;
const static bool use_shadow_map = NORMAL_USE_SHADOW_MAP;
const static bool use_projected_texture = NORMAL_USE_PROJECTED_TEXTURE;

struct Vs_output_unlit
{
   float4 position : SV_Position;
   float2 diffuse_texcoords : TEXCOORD0;
   float2 detail_texcoords : TEXCOORD1;
   float4 material_color_fade : TEXCOORD2;
   float fog_eye_distance : DEPTH;
};

Vs_output_unlit unlit_main_vs(Vertex_input input)
{
   Vs_output_unlit output;
    
   Transformer transformer = create_transformer(input);

   float3 positionWS = transformer.positionWS();

   output.position = transformer.positionPS();
   output.fog_eye_distance = fog::get_eye_distance(positionWS);

   output.diffuse_texcoords = transformer.texcoords(texture_transforms[0],
                                                    texture_transforms[1]);  
   output.detail_texcoords = transformer.texcoords(texture_transforms[2],
                                                   texture_transforms[3]);

   Near_scene near_scene = calculate_near_scene_fade(positionWS);

   output.material_color_fade.rgb = hdr_info.z * lighting_factor.x + lighting_factor.y;
   output.material_color_fade.a = saturate(near_scene.fade);
   output.material_color_fade *= get_material_color(input.color());

   return output;
}

struct Vs_output
{
   float4 positionPS : SV_Position;

   float2 diffuse_texcoords : TEXCOORD0;
   float2 detail_texcoords : TEXCOORD1;
   float4 projection_texcoords : TEXCOORD2;
   float4 shadow_texcoords : TEXCOORD3;

   float3 positionWS : TEXCOORD4;
   float3 normalWS : TEXCOORD5;

   float4 material_color_fade : TEXCOORD6;
   float3 static_lighting : TEXCOORD7;

   float fog_eye_distance : TEXCOORD8;
};

Vs_output main_vs(Vertex_input input)
{
   Vs_output output;

   Transformer transformer = create_transformer(input);

   float3 positionWS = transformer.positionWS();
   float3 normalWS = transformer.normalWS();

   output.positionWS = positionWS;
   output.normalWS = normalWS;
   output.positionPS = transformer.positionPS();
   output.fog_eye_distance = fog::get_eye_distance(positionWS);

   output.diffuse_texcoords = transformer.texcoords(texture_transforms[0],
                                                    texture_transforms[1]);  
   output.detail_texcoords = transformer.texcoords(texture_transforms[2],
                                                   texture_transforms[3]);

   output.projection_texcoords = mul(float4(positionWS, 1.0), light_proj_matrix);
   output.shadow_texcoords = transform_shadowmap_coords(positionWS);

   Near_scene near_scene = calculate_near_scene_fade(positionWS);

   output.material_color_fade = get_material_color(input.color());
   output.material_color_fade.a *= saturate(near_scene.fade);
   output.static_lighting = get_static_diffuse_color(input.color());

   return output;
}

struct Ps_input_unlit
{
   float2 diffuse_texcoords : TEXCOORD0;
   float2 detail_texcoords : TEXCOORD1;
   float4 material_color_fade : TEXCOORD2;
   float fog_eye_distance : DEPTH;
};

float get_glow_factor(float4 diffuse_map_color)
{
   if (use_hardedged_test || use_transparency) return blend_constants.a;
   
   return lerp(blend_constants.b, diffuse_map_color.a, blend_constants.a);
}

float4 unlit_main_ps(Ps_input_unlit input) : SV_Target0
{
   const float4 diffuse_map_color = diffuse_map.Sample(linear_wrap_sampler, 
                                                       input.diffuse_texcoords);
   const float3 detail_color = detail_map.Sample(linear_wrap_sampler, input.detail_texcoords);

   float3 color = diffuse_map_color.rgb * input.material_color_fade.rgb;
   color = (color * detail_color) * 2.0;

   const float glow_factor = get_glow_factor(diffuse_map_color);
   const float3 glow_color = color * glow_factor + color;
     
   color = glow_color * glow_factor + color;

   float alpha = input.material_color_fade.a;

   if (use_transparency) {
      if (use_hardedged_test && diffuse_map_color.a < 0.5) discard;

      alpha *= diffuse_map_color.a;
   }
   else if (use_hardedged_test) {
      if (diffuse_map_color.a < 0.5) discard;
   }

   color = fog::apply(color.rgb, input.fog_eye_distance);

   return float4(color, alpha);
}

struct Ps_input
{
   float2 diffuse_texcoords : TEXCOORD0;
   float2 detail_texcoords : TEXCOORD1;
   float4 projection_texcoords : TEXCOORD2;
   float4 shadow_texcoords : TEXCOORD3;

   float3 positionWS : TEXCOORD4;
   float3 normalWS : TEXCOORD5;

   float4 material_color_fade : TEXCOORD6;
   float3 static_lighting : TEXCOORD7;

   float fog_eye_distance : TEXCOORD8;
};

float4 main_ps(Ps_input input) : SV_Target0
{
   const float4 diffuse_map_color = diffuse_map.Sample(linear_wrap_sampler, 
                                                       input.diffuse_texcoords);
   const float3 detail_color = detail_map.Sample(linear_wrap_sampler, input.detail_texcoords);

   float3 diffuse_color = diffuse_map_color.rgb * input.material_color_fade.rgb;
   diffuse_color = diffuse_color * detail_color * 2.0;
   
   const float3 projection_texture_color = 
      use_projected_texture ? sample_projected_light(projected_texture, 
                                                     input.projection_texcoords) :
                              0.0;

   // Calculate lighting.
   Lighting lighting = light::calculate(normalize(input.normalWS), input.positionWS,
                                        input.static_lighting, use_projected_texture,
                                        projection_texture_color);

   lighting.color = lighting.color * lighting_factor.x + lighting_factor.y;

   float3 color = 0.0;

   // Apply lighting to diffuse colour.
   color += lighting.color;
   color *= diffuse_color; 

   if (use_shadow_map) {
      const float2 shadow_texcoords = input.shadow_texcoords.xy / input.shadow_texcoords.w;

      const float shadow_map_value =
         use_shadow_map ? shadow_map.SampleLevel(linear_wrap_sampler, shadow_texcoords, 0).a
                        : 1.0;

      float shadow = 1.0 - (lighting.intensity * (1.0 - shadow_map_value));

      // Linear Rendering Normalmap Hack
      color = lerp(color * shadow, diffuse_color * hdr_info.z * shadow, rt_multiply_blending);
   }
   else {

      // Linear Rendering Normalmap Hack
      color = lerp(color, diffuse_color * hdr_info.z, rt_multiply_blending);
   }

   float alpha;

   if (use_transparency) {
      if (use_hardedged_test) {
         if (diffuse_map_color.a < 0.5) discard;

         alpha = diffuse_map_color.a * input.material_color_fade.a;
      }
      else {
         alpha = 
            lerp(1.0, diffuse_map_color.a, blend_constants.b) * input.material_color_fade.a;
      }
   }
   else {
      if (use_hardedged_test && diffuse_map_color.a < 0.5) discard;

      alpha = input.material_color_fade.a;
   }

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, alpha);
}
