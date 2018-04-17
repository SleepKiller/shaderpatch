
#include "describe_material.hpp"
#include "compose_exception.hpp"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <string>
#include <string_view>

#include <gsl/gsl>

namespace sp {

using namespace std::literals;

namespace {

constexpr auto read_constant_offset(std::string_view string)
{
   const auto allowed_numeric_part = "0123456789"sv;
   const auto allowed_index_part = "xyzw"sv;

   const auto npos = std::string_view::npos;

   if (string.length() != 3 || string[1] != '.' ||
       allowed_numeric_part.find(string[0]) == npos ||
       allowed_index_part.find(string[2]) == npos) {
      throw compose_exception<std::runtime_error>(
         "Bad constant offset described for material property "sv);
   }

   gsl::index offset = (string[0] - '0') * 4;

   if (string[2] == 'x') offset += 0;
   if (string[2] == 'y') offset += 1;
   if (string[2] == 'z') offset += 2;
   if (string[2] == 'w') offset += 3;

   return offset;
}

void read_prop(const std::string& prop_key, const YAML::Node value,
               const YAML::Node property_mappings, std::array<float, 32>& constants)
{
   if (!property_mappings[prop_key]) {
      throw compose_exception<std::runtime_error>("Undescribed material property "sv,
                                                  std::quoted(prop_key),
                                                  " encountered."sv);
   }

   auto desc = property_mappings[prop_key];

   const auto type = desc["Type"s].as<std::string>();
   const std::array<float, 2> range{desc["Range"s][0].as<float>(),
                                    desc["Range"s][1].as<float>()};
   const auto offset = read_constant_offset(desc["Constant"s].as<std::string>());

   if (type == "scaler"sv) {
      constants[offset] = std::clamp(value.as<float>(), range[0], range[1]);
   }
   else {
      auto count = 0;

      // clang-format off
      if (type == "vec2"sv) count = 2;
      else if (type == "vec3"sv) count = 3;
      else if (type == "vec4"sv) count = 4;
      else {
         // clang-format on

         throw compose_exception<std::runtime_error>("Invalid material property type "sv,
                                                     std::quoted(prop_key),
                                                     " encountered in description."sv);
      }

      for (auto i = 0; i < count; ++i) {
         constants[offset + i] =
            std::clamp(value[i].as<float>(), range[0], range[1]);
      }
   }
}

void read_texture_slot(const std::string& texture_key, const YAML::Node value,
                       const YAML::Node texture_mappings,
                       std::array<std::string, 8>& constants)
{
   if (!texture_mappings[texture_key]) {
      throw compose_exception<std::runtime_error>("Undescribed material texture "sv,
                                                  std::quoted(texture_key),
                                                  " encountered."sv);
   }

   auto index = texture_mappings[texture_key]["Slot"s].as<int>();

   if (index < 5 || index > 12) {
      throw compose_exception<std::runtime_error>("Invalid material texture description"sv,
                                                  std::quoted(texture_key),
                                                  " encountered."sv);
   }

   constants[index - 5] = value.as<std::string>();
}

auto glmify_constants(const std::array<float, 32>& constants)
{
   std::array<glm::vec4, 8> vec4_constants;

   static_assert(sizeof(constants) == sizeof(vec4_constants));

   std::memcpy(vec4_constants.data(), constants.data(), sizeof(constants));

   return vec4_constants;
}
}

auto describe_material(const YAML::Node description, const YAML::Node material) -> Material_info
{
   Material_info info;

   info.rendertype = material["RenderType"s].as<std::string>();
   info.overridden_rendertype =
      description["Overridden RenderType"s].as<std::string>();

   std::array<float, 32> constants{};

   for (auto prop : material["Material"s]) {
      try {
         read_prop(prop.first.as<std::string>(), prop.second,
                   description["Property Mappings"s], constants);
      }
      catch (std::exception& e) {
         throw compose_exception<std::runtime_error>(
            "Error occured while processing material property "sv,
            std::quoted(prop.first.as<std::string>()), ": "sv, e.what());
      }
   }

   info.constants = glmify_constants(constants);

   for (auto texture : material["Textures"s]) {
      try {
         read_texture_slot(texture.first.as<std::string>(), texture.second,
                           description["Texture Mappings"s], info.textures);
      }
      catch (std::exception& e) {
         throw compose_exception<std::runtime_error>(
            "Error occured while processing material texture "sv,
            std::quoted(texture.first.as<std::string>()), ": "sv, e.what());
      }
   }

   return info;
}
}
