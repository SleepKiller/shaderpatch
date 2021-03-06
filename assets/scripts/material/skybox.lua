
constant_buffer_bind = constant_buffer_bind_flag.ps
fail_safe_texture_index = 0

function make_constant_buffer(props)
   local cb = constant_buffer_builder.new([[
      float emissive_power;
   ]])


   cb:set("emissive_power", math2.exp2(props:get_float("EmissivePower", 0.0)))

   return cb:complete()
end

function fill_resource_vec(props, resource_props, resources)
   
   resources:add(resource_props["Skybox"] or "")
   resources:add(resource_props["SkyboxEmissive"] or "")

end

function get_shader_flags(props, flags)

   if props:get_bool("UseEmissiveMap", false) then
      flags:add("SKYBOX_USE_EMISSIVE_MAP")
   end
   
end
