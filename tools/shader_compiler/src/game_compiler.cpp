
#include "game_compiler.hpp"
#include "compiler_helpers.hpp"
#include "compose_exception.hpp"
#include "magic_number.hpp"
#include "shader_metadata.hpp"
#include "shader_variations.hpp"
#include "synced_io.hpp"
#include "ucfb_writer.hpp"

#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>

#include <boost/filesystem.hpp>

#include <d3dcompiler.h>
#include <gsl/gsl>

using namespace std::literals;

namespace fs = boost::filesystem;

namespace sp {

Game_compiler::Game_compiler(nlohmann::json definition, const fs::path& definition_path,
                             const fs::path& source_file_dir, const fs::path& output_dir)
{
   Expects(fs::is_directory(source_file_dir) && fs::is_directory(output_dir));

   _source_path =
      source_file_dir /
      definition.value("source_name"s,
                       definition_path.stem().replace_extension(".fx"s).string());

   if (!fs::exists(_source_path)) {
      throw compose_exception<std::runtime_error>("Source file "sv, _source_path,
                                                  " does not exist"sv);
   }

   const auto output_path =
      output_dir / definition_path.stem().replace_extension(".shader"s);

   if (fs::exists(output_path) && (std::max(fs::last_write_time(definition_path),
                                            date_test_shader_file(_source_path)) <
                                   fs::last_write_time(output_path))) {
      return;
   }

   synced_print("Munging shader "sv, definition_path.filename().string(), "..."sv);

   _render_type = definition["rendertype"];

   fs::load_string_file(_source_path, _source);

   const auto metadata = definition.value("metadata", nlohmann::json::object());

   for (const auto& state_def : definition["states"]) {
      _states.emplace_back(compile_state(state_def, metadata));
   }

   save(output_path);
}

void Game_compiler::save(const boost::filesystem::path& output_path)
{
   auto file = ucfb::open_file_for_output(output_path.string());

   ucfb::Writer writer{file};

   auto shdr = writer.emplace_child("SHDR"_mn);

   shdr.emplace_child("RTYP"_mn).write(_render_type);
   shdr.emplace_child("NAME"_mn).write(_render_type);
   shdr.emplace_child("INFO"_mn).write(std::uint32_t{1u}, std::uint8_t{26u});

   auto pipe = shdr.emplace_child("PIPE"_mn);
   pipe.emplace_child("INFO"_mn).write(std::uint32_t{1u},
                                       static_cast<std::uint32_t>(_states.size()),
                                       static_cast<std::uint32_t>(_vs_shaders.size()),
                                       static_cast<std::uint32_t>(_ps_shaders.size()));

   auto vsp_ = pipe.emplace_child("VSP_"_mn);

   for (const auto& shader : _vs_shaders) {
      auto vs__ = vsp_.emplace_child("VS__"_mn);

      vs__.write(gsl::make_span(shader).size_bytes());
      vs__.write(gsl::make_span(shader));
   }

   auto psp_ = pipe.emplace_child("PSP_"_mn);

   for (const auto& shader : _ps_shaders) {
      auto ps__ = psp_.emplace_child("PS__"_mn);

      ps__.write(gsl::make_span(shader).size_bytes());
      ps__.write(gsl::make_span(shader));
   }

   for (std::uint32_t i = 0u; i < _states.size(); ++i) {
      const auto& state = _states[i];

      auto stat = pipe.emplace_child("STAT"_mn);
      stat.emplace_child("INFO"_mn).write(i, static_cast<std::uint32_t>(
                                                state.passes.size()));

      for (std::uint32_t j = 0u; j < state.passes.size(); ++j) {
         const auto& pass = state.passes[j];

         auto pass_chunk = stat.emplace_child("PASS"_mn);
         pass_chunk.emplace_child("INFO"_mn).write(pass.flags);

         for (const auto& vs_ref : pass.vs_shaders) {
            pass_chunk.emplace_child("PVS_"_mn).write(vs_ref.flags, vs_ref.index);
         }

         pass_chunk.emplace_child("PPS_"_mn).write(pass.ps_index);
      }
   }
}

auto Game_compiler::compile_state(const nlohmann::json& state_def,
                                  const nlohmann::json& parent_metadata)
   -> Game_compiler::State
{
   State state{std::uint32_t{state_def["id"]}};

   auto& passes = state.passes;

   auto metadata = state_def.value("metadata", nlohmann::json::object());
   metadata.update(parent_metadata);

   const std::string state_name = state_def["name"s];

   for (const auto& pass_def : state_def["passes"]) {
      passes.emplace_back(compile_pass(pass_def, metadata, state_name));
   }

   return state;
}

auto Game_compiler::compile_pass(const nlohmann::json& pass_def,
                                 const nlohmann::json& parent_metadata,
                                 std::string_view state_name) -> Game_compiler::Pass
{
   Pass pass{};

   pass.flags = get_pass_flags(pass_def);

   auto metadata = pass_def.value("metadata", nlohmann::json::object());
   metadata.update(parent_metadata);

   const auto vs_target = pass_def.value("vs_target", "vs_3_0");
   const auto ps_target = pass_def.value("ps_target", "ps_3_0");

   const std::string vs_entry_point = pass_def["vertex_shader"];
   const std::string ps_entry_point = pass_def["pixel_shader"];

   for (auto& variation : get_shader_variations(pass_def["skinned"], pass_def["lighting"],
                                                pass_def["vertex_color"])) {
      pass.vs_shaders.emplace_back(compile_vertex_shader(metadata, vs_entry_point, vs_target,
                                                         variation, state_name));
   }

   pass.ps_index =
      compile_pixel_shader(metadata, ps_entry_point, ps_target, state_name);

   return pass;
}

auto Game_compiler::compile_vertex_shader(const nlohmann::json& parent_metadata,
                                          const std::string& entry_point,
                                          const std::string& target,
                                          Shader_variation& variation,
                                          std::string_view state_name) -> Vertex_shader_ref
{
   Shader_cache_index cache_index{entry_point, std::move(variation.definitions)};

   const auto cached = _vs_cache.find(cache_index);

   if (cached != std::end(_vs_cache)) return cached->second;

   Com_ptr<ID3DBlob> error_message;
   Com_ptr<ID3DBlob> shader;

   auto result =
      D3DCompile(_source.data(), _source.length(), _source_path.string().data(),
                 cache_index.definitions.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE,
                 entry_point.data(), target.data(), compiler_flags, NULL,
                 shader.clear_and_assign(), error_message.clear_and_assign());

   if (result != S_OK) {
      throw std::runtime_error{static_cast<char*>(error_message->GetBufferPointer())};
   }
   else if (error_message) {
      std::cout << static_cast<char*>(error_message->GetBufferPointer());
   }

   const auto shader_index = static_cast<std::uint32_t>(_vs_shaders.size());

   _vs_shaders.emplace_back(
      embed_meta_data(parent_metadata, _render_type, state_name, entry_point,
                      target, variation.flags, make_dword_span(*shader)));

   return _vs_cache[cache_index] = {variation.flags, shader_index};
}

auto Game_compiler::compile_pixel_shader(const nlohmann::json& parent_metadata,
                                         const std::string& entry_point,
                                         const std::string& target,
                                         std::string_view state_name) -> Pixel_shader_ref
{
   const auto cached = _ps_cache.find(entry_point);

   if (cached != std::end(_ps_cache)) return cached->second;

   Com_ptr<ID3DBlob> error_message;
   Com_ptr<ID3DBlob> shader;

   auto result =
      D3DCompile(_source.data(), _source.length(), _source_path.string().data(),
                 nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry_point.data(),
                 target.data(), compiler_flags, NULL, shader.clear_and_assign(),
                 error_message.clear_and_assign());

   if (result != S_OK) {
      throw std::runtime_error{static_cast<char*>(error_message->GetBufferPointer())};
   }
   else if (error_message) {
      std::cout << static_cast<char*>(error_message->GetBufferPointer());
   }

   const auto shader_index = static_cast<std::uint32_t>(_ps_shaders.size());

   _ps_shaders.emplace_back(embed_meta_data(parent_metadata, _render_type,
                                            state_name, entry_point, target, {},
                                            make_dword_span(*shader)));

   return _ps_cache[entry_point] = shader_index;
}

auto Game_compiler::get_pass_flags(const nlohmann::json& pass_def) -> Pass_flags
{
   Pass_flags flags = Pass_flags::none;

   const std::string transform = pass_def["transform"];

   if (pass_def["lighting"]) {
      flags |= Pass_flags::lighting;
   }
   else {
      flags |= Pass_flags::nolighting;
   }

   if (transform == "none"sv) {
      flags |= Pass_flags::notransform;

      return flags;
   }
   else if (transform == "position"sv) {
      flags |= Pass_flags::position;
   }
   else if (transform == "normals"sv) {
      flags |= Pass_flags::normals;
   }
   else if (transform == "binormals"sv) {
      flags |= Pass_flags::binormals;
   }
   else {
      throw std::runtime_error{"Invalid transform value: "s + transform};
   }

   if (pass_def["vertex_color"]) flags |= Pass_flags::vertex_color;
   if (pass_def["texture_coords"]) flags |= Pass_flags::texcoords;

   return flags;
}
}
