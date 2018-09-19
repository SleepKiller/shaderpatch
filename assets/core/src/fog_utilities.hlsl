#ifndef FOG_UTILS_INCLUDED
#define FOG_UTILS_INCLUDED

#include "constants_list.hlsl"
#include "ext_constants_list.hlsl"

namespace fog
{

float get_eye_distance(float3 positionWS)
{
   return distance(view_positionWS, positionWS);
}

float3 apply(float3 color, float eye_distance)
{
   if (fog_enabled) {
      float fog = (fog_range.y - eye_distance) * fog_range.z;

      return lerp(fog_color, color, saturate(fog));
   }
   else {
      return color;
   }
}

}
#endif