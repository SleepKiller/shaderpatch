
#include "sol_create_usertypes.hpp"
#include "../logger.hpp"
#include "constant_buffer_builder.hpp"
#include "material.hpp"
#include "properties_view.hpp"

#include <glm/glm.hpp>
#include <sol/sol.hpp>

using namespace std::literals;

namespace sp::material {

namespace {

template<typename T, std::size_t length>
struct Vec_constructor {
};

template<typename T>
struct Vec_constructor<T, 2> {
   using type = T(typename T::value_type, typename T::value_type);
};

template<typename T>
struct Vec_constructor<T, 3> {
   using type = T(typename T::value_type, typename T::value_type, typename T::value_type);
};

template<typename T>
struct Vec_constructor<T, 4> {
   using type = T(typename T::value_type, typename T::value_type,
                  typename T::value_type, typename T::value_type);
};

template<typename T>
using Vec_constructor_t = typename Vec_constructor<T, T::length()>::type;

template<typename T>
void create_vec_type(sol::state& lua, const char* name) noexcept
{
   auto type = lua.new_usertype<T>(
      name,
      sol::constructors<T(), T(typename T::value_type), Vec_constructor_t<T>>(),

      sol::meta_function::addition,
      sol::resolve<T(const T&, const T&)>(glm::operator+),

      sol::meta_function::subtraction,
      sol::resolve<T(const T&, const T&)>(glm::operator-),

      sol::meta_function::multiplication,
      sol::resolve<T(const T&, const T&)>(glm::operator*),

      sol::meta_function::division,
      sol::resolve<T(const T&, const T&)>(glm::operator/));

   if constexpr (!std::is_unsigned_v<typename T::value_type>) {
      type[sol::meta_function::unary_minus] =
         sol::resolve<T(const T&)>(glm::operator-);
   }

   if constexpr (T::length() >= 1) type["x"sv] = &T::x;
   if constexpr (T::length() >= 2) type["y"sv] = &T::y;
   if constexpr (T::length() >= 3) type["z"sv] = &T::z;
   if constexpr (T::length() >= 4) type["w"sv] = &T::w;
}

}

void sol_create_usertypes(sol::state& lua) noexcept
{
   create_vec_type<glm::vec2>(lua, "float2");
   create_vec_type<glm::vec3>(lua, "float3");
   create_vec_type<glm::vec4>(lua, "float4");

   create_vec_type<glm::ivec2>(lua, "int2");
   create_vec_type<glm::ivec3>(lua, "int3");
   create_vec_type<glm::ivec4>(lua, "int4");

   create_vec_type<glm::uvec2>(lua, "uint2");
   create_vec_type<glm::uvec3>(lua, "uint3");
   create_vec_type<glm::uvec4>(lua, "uint4");

   auto constant_buffer_builder = lua.new_usertype<Constant_buffer_builder>(
      "constant_buffer_builder",
      sol::constructors<Constant_buffer_builder(std::string_view)>());

   constant_buffer_builder["set"sv] = &Constant_buffer_builder::set;
   constant_buffer_builder["complete"sv] = &Constant_buffer_builder::complete;

   auto properties_view = lua.new_usertype<Properties_view>("properties_view");

   properties_view["get_float"sv] = &Properties_view::get<float>;
   properties_view["get_float2"sv] = &Properties_view::get<glm::vec2>;
   properties_view["get_float3"sv] = &Properties_view::get<glm::vec3>;
   properties_view["get_float4"sv] = &Properties_view::get<glm::vec4>;

   properties_view["get_int"sv] = &Properties_view::get<std::int32_t>;
   properties_view["get_int2"sv] = &Properties_view::get<glm::ivec2>;
   properties_view["get_int3"sv] = &Properties_view::get<glm::ivec3>;
   properties_view["get_int4"sv] = &Properties_view::get<glm::ivec4>;

   properties_view["get_uint"sv] = &Properties_view::get<std::uint32_t>;
   properties_view["get_uint2"sv] = &Properties_view::get<glm::uvec2>;
   properties_view["get_uint3"sv] = &Properties_view::get<glm::uvec3>;
   properties_view["get_uint4"sv] = &Properties_view::get<glm::uvec4>;

   properties_view["get_bool"sv] = &Properties_view::get<bool>;

   lua.new_enum("constant_buffer_bind_flag"sv, "none"sv, Constant_buffer_bind::none,
                "vs"sv, Constant_buffer_bind::vs, "ps"sv, Constant_buffer_bind::ps);

   auto log = lua["log"sv].get_or_create<sol::table>();

   log["str_info"sv] = [](const std::string_view str) noexcept {
      log_fmt(Log_level::info, str);
   };
   log["str_warning"sv] = [](const std::string_view str) noexcept {
      log_fmt(Log_level::warning, str);
   };
   log["str_error"sv] = [](const std::string_view str) noexcept {
      log_fmt(Log_level::error, str);
   };

   lua.do_string(R"(
      log.info = function(...) log.str_info(string.format(...)) end
      log.warning = function(...) log.str_warning(string.format(...)) end
      log.error = function(...) log.str_error(string.format(...)) end
   )"sv);

   auto math = lua["math2"sv].get_or_create<sol::table>();

   math["exp2"sv] = sol::resolve<double(double)>(std::exp2);
   math["rcp"sv] = [](double v) { return 1.0 / v; };
   math["sign"sv] = [](double v) { return v < 0.0 ? -1.0 : 1.0; };
}

}
