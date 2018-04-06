
#include "munge_materials.hpp"
#include "patch_material.hpp"
#include "req_file_helpers.hpp"
#include "synced_io.hpp"

#include <array>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace sp {

using namespace std::literals;

namespace {

struct Hardcoded_material_flags {
   bool transparent = false;
   bool hard_edged = false;
   bool double_sided = false;
   bool statically_lit = false;
};

auto munge_material(const fs::path& material_path, const fs::path& output_file_path)
   -> Hardcoded_material_flags
{
   auto root_node = YAML::LoadFile(material_path.string());

   if (!root_node["Material"s]) {
      throw std::runtime_error{"Null Material YAML node."s};
   }

   auto material_node = root_node["Material"s];

   Material_info material;

   material.rendertype = material_node["RenderType"s].as<std::string>();
   material.overridden_rendertype = "normal"s;

   material.constants[0].x = material_node["BaseColor"s][0].as<float>();
   material.constants[0].y = material_node["BaseColor"s][1].as<float>();
   material.constants[0].z = material_node["BaseColor"s][2].as<float>();

   material.constants[0].w = material_node["Metallicness"s].as<float>();
   material.constants[1].x = material_node["Roughness"s].as<float>();

   material.constants[1].y = material_node["AOStrength"s].as<float>();
   material.constants[1].z = material_node["EmissiveStrength"s].as<float>();

   if (!root_node["Flags"s]) {
      throw std::runtime_error{"Null Flags YAML node."s};
   }

   auto flags_node = root_node["Flags"s];

   Hardcoded_material_flags flags;

   flags.transparent = flags_node["Transparent"s].as<bool>();
   flags.hard_edged = flags_node["HardEdged"s].as<bool>();
   flags.double_sided = flags_node["DoubleSided"s].as<bool>();
   flags.statically_lit = flags_node["StaticallyLit"s].as<bool>();

   if (!root_node["Textures"s]) {
      throw std::runtime_error{"Null Textures YAML node."s};
   }

   auto textures_node = root_node["Textures"s];

   material.textures[0] = textures_node["AlbedoMap"s].as<std::string>();
   material.textures[1] = textures_node["NormalMap"s].as<std::string>();
   material.textures[2] = textures_node["MetallicRoughnessMap"s].as<std::string>();
   material.textures[3] = textures_node["AOMap"s].as<std::string>();
   material.textures[4] = textures_node["LightMap"s].as<std::string>();
   material.textures[5] = textures_node["EmissiveMap"s].as<std::string>();

   std::vector<std::pair<std::string, std::vector<std::string>>> required_files;

   auto& required_sp_textures =
      required_files.emplace_back("sptex"s, std::vector<std::string>{}).second;

   for (const auto& texture : material.textures) {
      if (!texture.empty()) required_sp_textures.emplace_back(texture);
   }

   write_patch_material(output_file_path, material);
   emit_req_file(fs::change_extension(output_file_path, ".texture.req"s), required_files);

   return flags;
}

void fixup_munged_models(
   const fs::path& output_dir,
   const std::unordered_map<std::string, std::vector<fs::path>>& texture_references,
   std::vector<std::pair<std::string, Hardcoded_material_flags>> munged_materials)
{
   for (const auto& [name, flags] : munged_materials) {
      if (!texture_references.count(name)) continue;

      for (const auto& file_ref : texture_references.at(name)) {
         auto output_file_path = output_dir / file_ref.filename();

         if (!fs::exists(output_file_path) ||
             (fs::last_write_time(output_file_path) < fs::last_write_time(file_ref))) {
            fs::copy_file(file_ref, output_file_path);

            const auto ext = fs::extension(file_ref);

            auto req_file_path = fs::change_extension(file_ref, ext + ".req"s);

            if (fs::exists(req_file_path)) {
               auto output_req_file_path =
                  fs::change_extension(output_file_path, ext + ".req"s);

               fs::copy_file(req_file_path, output_req_file_path);
            }
         }

         synced_print("One day "sv, file_ref.filename(), " will be edited."sv);
      }
   }
}
}

void munge_materials(const fs::path& output_dir,
                     const std::unordered_map<std::string, std::vector<fs::path>>& texture_references,
                     const std::unordered_map<std::string, fs::path>& files)
{
   std::vector<std::pair<std::string, Hardcoded_material_flags>> munged_materials;

   for (auto& file : files) {
      try {
         if (file.second.extension() == ".mtrl"s) {
            synced_print("Munging "sv, file.first, "..."sv);

            const auto output_file_path =
               output_dir / file.second.stem().replace_extension(".texture"s);

            if (fs::exists(output_file_path) &&
                (fs::last_write_time(file.second) <
                 fs::last_write_time(output_file_path))) {
               continue;
            }

            const auto flags = munge_material(file.second, output_file_path);

            munged_materials.emplace_back(file.second.stem().string(), flags);
         }
      }
      catch (std::exception& e) {
         synced_error_print("Error munging "sv, file.first, ':', e.what());
      }
   }

   fixup_munged_models(output_dir, texture_references, munged_materials);
}
}
