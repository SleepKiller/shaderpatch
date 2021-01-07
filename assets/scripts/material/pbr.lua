
constant_buffer_bind = constant_buffer_bind_flag.ps
fail_safe_texture_index = 0

function make_constant_buffer(props)
   local cb = constant_buffer_builder.new([[
      float3 base_color;
      float base_metallicness;
      float base_roughness;
      float ao_strength;
      float emissive_power;
   ]])

   cb:set("base_color", props:get_float3("BaseColor", float3.new(1.0, 1.0, 1.0)))
   cb:set("base_metallicness", props:get_float("Metallicness", 1.0))
   cb:set("base_roughness", props:get_float("Roughness", 1.0))
   cb:set("ao_strength", props:get_float("AOStrength", 1.0))
   cb:set("emissive_power", props:get_float("EmissivePower", 1.0))

   return cb:complete()
end

function fill_resource_vec(props, resource_props, resources)

   resources:add(resource_props["AlbedoMap"] or "$grey")
   resources:add(resource_props["NormalMap"] or "$null_normalmap")
   resources:add(resource_props["MetallicRoughnessMap"] or "")
   resources:add(resource_props["AOMap"] or "$null_ao")
   resources:add(resource_props["EmissiveMap"] or "")

   -- TEMP IBL Stuff
   resources:add(resource_props["TEMP_ibl_dfg"] or "")
   resources:add(resource_props["TEMP_ibl_specular"] or "")
   resources:add(resource_props["TEMP_ibl_diffuse"] or "")

end
