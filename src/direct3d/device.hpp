#pragma once

#include "../shader_constants.hpp"
#include "../shader_database.hpp"
#include "../texture.hpp"
#include "../texture_database.hpp"
#include "../user_config.hpp"
#include "com_ptr.hpp"
#include "render_state_block.hpp"
#include "shader_constant.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include <d3d9.h>

namespace sp::direct3d {

class Device : public IDirect3DDevice9 {
public:
   Device(Com_ptr<IDirect3DDevice9> device, const HWND window,
          const glm::ivec2 resolution) noexcept;

   HRESULT __stdcall QueryInterface(const IID& iid, void** object) noexcept override;
   ULONG __stdcall AddRef() noexcept override;
   ULONG __stdcall Release() noexcept override;

   HRESULT __stdcall TestCooperativeLevel() noexcept override;

   UINT __stdcall GetAvailableTextureMem() noexcept override;

   HRESULT __stdcall EvictManagedResources() noexcept override;

   HRESULT __stdcall GetDirect3D(IDirect3D9** d3d9) noexcept override;
   HRESULT __stdcall GetDeviceCaps(D3DCAPS9* caps) noexcept override;
   HRESULT __stdcall GetDisplayMode(UINT swap_chain, D3DDISPLAYMODE* mode) noexcept override;
   HRESULT __stdcall GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* parameters) noexcept override;

   HRESULT __stdcall SetCursorProperties(UINT x_hot_spot, UINT y_hot_spot,
                                         IDirect3DSurface9* cursor_bitmap) noexcept override;
   void __stdcall SetCursorPosition(int x, int y, DWORD flags) noexcept override;
   BOOL __stdcall ShowCursor(BOOL show) noexcept override;

   HRESULT __stdcall CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* presentation_parameters,
                                               IDirect3DSwapChain9** swap_chain) noexcept override;

   HRESULT __stdcall GetSwapChain(UINT swap_chain_index,
                                  IDirect3DSwapChain9** swap_chain) noexcept override;
   UINT __stdcall GetNumberOfSwapChains() noexcept override;

   HRESULT __stdcall Reset(D3DPRESENT_PARAMETERS* presentation_parameters) noexcept override;

   HRESULT __stdcall Present(const RECT* source_rect, const RECT* dest_rect,
                             HWND dest_window_override,
                             const RGNDATA* dirty_region) noexcept override;

   HRESULT __stdcall GetBackBuffer(UINT swap_chain, UINT back_buffer_index,
                                   D3DBACKBUFFER_TYPE type,
                                   IDirect3DSurface9** back_buffer) noexcept override;
   HRESULT __stdcall GetRasterStatus(UINT swap_chain,
                                     D3DRASTER_STATUS* raster_status) noexcept override;

   HRESULT __stdcall SetDialogBoxMode(BOOL enable_dialogs) noexcept override;

   void __stdcall SetGammaRamp(UINT swap_chain_index, DWORD flags,
                               const D3DGAMMARAMP* ramp) noexcept override;
   void __stdcall GetGammaRamp(UINT swap_chain_index, D3DGAMMARAMP* ramp) noexcept override;

   HRESULT __stdcall CreateTexture(UINT width, UINT height, UINT levels,
                                   DWORD usage, D3DFORMAT format, D3DPOOL pool,
                                   IDirect3DTexture9** texture,
                                   HANDLE* shared_handle) noexcept override;
   HRESULT __stdcall CreateVolumeTexture(UINT width, UINT height, UINT depth,
                                         UINT levels, DWORD usage,
                                         D3DFORMAT format, D3DPOOL pool,
                                         IDirect3DVolumeTexture9** volume_texture,
                                         HANDLE* shared_handle) noexcept override;
   HRESULT __stdcall CreateCubeTexture(UINT edge_length, UINT levels, DWORD usage,
                                       D3DFORMAT format, D3DPOOL pool,
                                       IDirect3DCubeTexture9** cube_texture,
                                       HANDLE* shared_handle) noexcept override;
   HRESULT __stdcall CreateVertexBuffer(UINT length, DWORD usage, DWORD fvf,
                                        D3DPOOL pool,
                                        IDirect3DVertexBuffer9** vertex_buffer,
                                        HANDLE* shared_handle) noexcept override;
   HRESULT __stdcall CreateIndexBuffer(UINT length, DWORD usage,
                                       D3DFORMAT format, D3DPOOL pool,
                                       IDirect3DIndexBuffer9** index_buffer,
                                       HANDLE* shared_handle) noexcept override;
   HRESULT __stdcall CreateRenderTarget(UINT width, UINT height, D3DFORMAT format,
                                        D3DMULTISAMPLE_TYPE multi_sample,
                                        DWORD multisample_quality, BOOL lockable,
                                        IDirect3DSurface9** surface,
                                        HANDLE* shared_handle) noexcept override;
   HRESULT __stdcall CreateDepthStencilSurface(
      UINT width, UINT height, D3DFORMAT format,
      D3DMULTISAMPLE_TYPE multi_sample, DWORD multi_sample_quality, BOOL discard,
      IDirect3DSurface9** surface, HANDLE* shared_handle) noexcept override;

   HRESULT __stdcall UpdateSurface(IDirect3DSurface9* source_surface,
                                   const RECT* source_rect,
                                   IDirect3DSurface9* destination_surface,
                                   const POINT* dest_point) noexcept override;
   HRESULT __stdcall UpdateTexture(IDirect3DBaseTexture9* source_texture,
                                   IDirect3DBaseTexture9* destination_texture) noexcept override;

   HRESULT __stdcall GetRenderTargetData(IDirect3DSurface9* render_target,
                                         IDirect3DSurface9* dest_surface) noexcept override;
   HRESULT __stdcall GetFrontBufferData(UINT swap_chain_index,
                                        IDirect3DSurface9* dest_surface) noexcept override;

   HRESULT __stdcall StretchRect(IDirect3DSurface9* source_surface,
                                 const RECT* source_rect,
                                 IDirect3DSurface9* dest_surface, const RECT* dest_rect,
                                 D3DTEXTUREFILTERTYPE filter) noexcept override;

   HRESULT __stdcall ColorFill(IDirect3DSurface9* surface, const RECT* rect,
                               D3DCOLOR color) noexcept override;

   HRESULT __stdcall CreateOffscreenPlainSurface(UINT width, UINT height,
                                                 D3DFORMAT format, D3DPOOL pool,
                                                 IDirect3DSurface9** surface,
                                                 HANDLE* shared_handle) noexcept override;

   HRESULT __stdcall SetRenderTarget(DWORD render_target_index,
                                     IDirect3DSurface9* render_target) noexcept override;
   HRESULT __stdcall GetRenderTarget(DWORD render_target_index,
                                     IDirect3DSurface9** render_target) noexcept override;

   HRESULT __stdcall SetDepthStencilSurface(IDirect3DSurface9* new_z_stencil) noexcept override;
   HRESULT __stdcall GetDepthStencilSurface(IDirect3DSurface9** z_stencil_surface) noexcept override;

   HRESULT __stdcall BeginScene() noexcept override;
   HRESULT __stdcall EndScene() noexcept override;

   HRESULT __stdcall Clear(DWORD count, const D3DRECT* rects, DWORD flags,
                           D3DCOLOR color, float z, DWORD stencil) noexcept override;

   HRESULT __stdcall SetTransform(D3DTRANSFORMSTATETYPE state,
                                  const D3DMATRIX* matrix) noexcept override;
   HRESULT __stdcall GetTransform(D3DTRANSFORMSTATETYPE state,
                                  D3DMATRIX* matrix) noexcept override;
   HRESULT __stdcall MultiplyTransform(D3DTRANSFORMSTATETYPE state,
                                       const D3DMATRIX* matrix) noexcept override;

   HRESULT __stdcall SetViewport(const D3DVIEWPORT9* viewport) noexcept override;
   HRESULT __stdcall GetViewport(D3DVIEWPORT9* viewport) noexcept override;

   HRESULT __stdcall SetMaterial(const D3DMATERIAL9* material) noexcept override;
   HRESULT __stdcall GetMaterial(D3DMATERIAL9* material) noexcept override;

   HRESULT __stdcall SetLight(DWORD index, const D3DLIGHT9* light) noexcept override;
   HRESULT __stdcall GetLight(DWORD index, D3DLIGHT9* light) noexcept override;

   HRESULT __stdcall LightEnable(DWORD index, BOOL enabled) noexcept override;
   HRESULT __stdcall GetLightEnable(DWORD index, BOOL* enabled) noexcept override;

   HRESULT __stdcall SetClipPlane(DWORD index, const float* plane) noexcept override;
   HRESULT __stdcall GetClipPlane(DWORD index, float* plane) noexcept override;

   HRESULT __stdcall SetRenderState(D3DRENDERSTATETYPE state, DWORD value) noexcept override;
   HRESULT __stdcall GetRenderState(D3DRENDERSTATETYPE state,
                                    DWORD* value) noexcept override;

   HRESULT __stdcall CreateStateBlock(D3DSTATEBLOCKTYPE type,
                                      IDirect3DStateBlock9** state_block) noexcept override;
   HRESULT __stdcall BeginStateBlock() noexcept override;
   HRESULT __stdcall EndStateBlock(IDirect3DStateBlock9** state_block) noexcept override;

   HRESULT __stdcall SetClipStatus(const D3DCLIPSTATUS9* clip_status) noexcept override;
   HRESULT __stdcall GetClipStatus(D3DCLIPSTATUS9* clip_status) noexcept override;

   HRESULT __stdcall GetTexture(DWORD stage,
                                IDirect3DBaseTexture9** texture) noexcept override;
   HRESULT __stdcall SetTexture(DWORD stage,
                                IDirect3DBaseTexture9* texture) noexcept override;

   HRESULT __stdcall GetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type,
                                          DWORD* value) noexcept override;
   HRESULT __stdcall SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type,
                                          DWORD value) noexcept override;

   HRESULT __stdcall GetSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE type,
                                     DWORD* value) noexcept override;
   HRESULT __stdcall SetSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE type,
                                     DWORD value) noexcept override;

   HRESULT __stdcall ValidateDevice(DWORD* num_passes) noexcept override;

   HRESULT __stdcall SetPaletteEntries(UINT palette_number,
                                       const PALETTEENTRY* entries) noexcept override;
   HRESULT __stdcall GetPaletteEntries(UINT palette_number,
                                       PALETTEENTRY* entries) noexcept override;

   HRESULT __stdcall SetCurrentTexturePalette(UINT palette_number) noexcept override;
   HRESULT __stdcall GetCurrentTexturePalette(UINT* palette_number) noexcept override;

   HRESULT __stdcall SetScissorRect(const RECT* rect) noexcept override;
   HRESULT __stdcall GetScissorRect(RECT* rect) noexcept override;

   HRESULT __stdcall SetSoftwareVertexProcessing(BOOL software) noexcept override;
   BOOL __stdcall GetSoftwareVertexProcessing() noexcept override;

   HRESULT __stdcall SetNPatchMode(float segments) noexcept override;
   float __stdcall GetNPatchMode() noexcept override;

   HRESULT __stdcall DrawPrimitive(D3DPRIMITIVETYPE primitive_type, UINT start_vertex,
                                   UINT primitive_count) noexcept override;
   HRESULT __stdcall DrawIndexedPrimitive(D3DPRIMITIVETYPE primitive_type,
                                          INT base_vertex_index, UINT min_vertex_index,
                                          UINT num_vertices, UINT start_Index,
                                          UINT prim_Count) noexcept override;
   HRESULT __stdcall DrawPrimitiveUP(D3DPRIMITIVETYPE primitive_type,
                                     UINT primitive_count,
                                     const void* vertexstreamzerodata,
                                     UINT vertexstreamzerostride) noexcept override;
   HRESULT __stdcall DrawIndexedPrimitiveUP(
      D3DPRIMITIVETYPE primitive_type, UINT min_vertex_index, UINT num_vertices,
      UINT primitive_count, const void* index_data, D3DFORMAT index_data_format,
      const void* vertex_stream_zero_data,
      UINT vertex_stream_zero_stride) noexcept override;

   HRESULT __stdcall ProcessVertices(UINT src_start_index, UINT dest_index,
                                     UINT vertex_count,
                                     IDirect3DVertexBuffer9* dest_buffer,
                                     IDirect3DVertexDeclaration9* vertex_decl,
                                     DWORD flags) noexcept override;

   HRESULT __stdcall CreateVertexDeclaration(const D3DVERTEXELEMENT9* vertex_elements,
                                             IDirect3DVertexDeclaration9** decl) noexcept override;

   HRESULT __stdcall SetVertexDeclaration(IDirect3DVertexDeclaration9* decl) noexcept override;
   HRESULT __stdcall GetVertexDeclaration(IDirect3DVertexDeclaration9** decl) noexcept override;

   HRESULT __stdcall SetFVF(DWORD fvf) noexcept override;
   HRESULT __stdcall GetFVF(DWORD* fvf) noexcept override;

   HRESULT __stdcall CreateVertexShader(const DWORD* function,
                                        IDirect3DVertexShader9** shader) noexcept override;

   HRESULT __stdcall SetVertexShader(IDirect3DVertexShader9* shader) noexcept override;
   HRESULT __stdcall GetVertexShader(IDirect3DVertexShader9** shader) noexcept override;

   HRESULT __stdcall SetVertexShaderConstantF(UINT start_register,
                                              const float* constant_data,
                                              UINT vector4f_count) noexcept override;
   HRESULT __stdcall GetVertexShaderConstantF(UINT start_register, float* constant_data,
                                              UINT vector4f_count) noexcept override;
   HRESULT __stdcall SetVertexShaderConstantI(UINT start_register,
                                              const int* constant_data,
                                              UINT vector4i_count) noexcept override;
   HRESULT __stdcall GetVertexShaderConstantI(UINT start_register, int* constant_data,
                                              UINT vector4i_count) noexcept override;
   HRESULT __stdcall SetVertexShaderConstantB(UINT start_register,
                                              const BOOL* constant_data,
                                              UINT bool_count) noexcept override;
   HRESULT __stdcall GetVertexShaderConstantB(UINT start_register, BOOL* constant_data,
                                              UINT bool_count) noexcept override;

   HRESULT __stdcall SetStreamSource(UINT stream_number,
                                     IDirect3DVertexBuffer9* stream_data,
                                     UINT offset_in_bytes, UINT stride) noexcept override;
   HRESULT __stdcall GetStreamSource(UINT stream_number,
                                     IDirect3DVertexBuffer9** stream_data,
                                     UINT* offset_in_bytes,
                                     UINT* stride) noexcept override;

   HRESULT __stdcall SetStreamSourceFreq(UINT stream_number, UINT setting) noexcept override;
   HRESULT __stdcall GetStreamSourceFreq(UINT stream_number,
                                         UINT* setting) noexcept override;

   HRESULT __stdcall SetIndices(IDirect3DIndexBuffer9* index_data) noexcept override;
   HRESULT __stdcall GetIndices(IDirect3DIndexBuffer9** index_data) noexcept override;

   HRESULT __stdcall CreatePixelShader(const DWORD* function,
                                       IDirect3DPixelShader9** shader) noexcept override;

   HRESULT __stdcall SetPixelShader(IDirect3DPixelShader9* shader) noexcept override;
   HRESULT __stdcall GetPixelShader(IDirect3DPixelShader9** shader) noexcept override;

   HRESULT __stdcall SetPixelShaderConstantF(UINT start_register,
                                             const float* constant_data,
                                             UINT vector4f_count) noexcept override;
   HRESULT __stdcall GetPixelShaderConstantF(UINT Start_Register, float* constant_data,
                                             UINT vector4f_count) noexcept override;
   HRESULT __stdcall SetPixelShaderConstantI(UINT start_register,
                                             const int* constant_data,
                                             UINT vector4i_count) noexcept override;
   HRESULT __stdcall GetPixelShaderConstantI(UINT start_register, int* constant_data,
                                             UINT vector4i_count) noexcept override;
   HRESULT __stdcall SetPixelShaderConstantB(UINT start_register,
                                             const BOOL* constant_data,
                                             UINT bool_count) noexcept override;
   HRESULT __stdcall GetPixelShaderConstantB(UINT start_register, BOOL* constant_data,
                                             UINT bool_count) noexcept override;

   HRESULT __stdcall DrawRectPatch(UINT handle, const float* num_segs,
                                   const D3DRECTPATCH_INFO* rect_patch_info) noexcept override;
   HRESULT __stdcall DrawTriPatch(UINT handle, const float* num_segs,
                                  const D3DTRIPATCH_INFO* tri_patch_info) noexcept override;

   HRESULT __stdcall DeletePatch(UINT handle) noexcept override;

   HRESULT __stdcall CreateQuery(D3DQUERYTYPE type,
                                 IDirect3DQuery9** query) noexcept override;

private:
   ~Device();

   constexpr static auto water_slot = 8;
   constexpr static auto refraction_slot = 9;
   constexpr static auto cubemap_projection_slot = 15;

   void bind_water_texture() noexcept;
   void bind_refraction_texture() noexcept;

   void update_refraction_texture() noexcept;

   const Com_ptr<IDirect3DDevice9> _device;
   const HWND _window;

   Com_ptr<IDirect3DTexture9> _water_texture;
   Com_ptr<IDirect3DTexture9> _refraction_texture;

   std::function<void()> _on_ps_shader_set{};

   bool _window_dirty = true;
   bool _imgui_bootstrapped = false;
   bool _imgui_active = false;
   bool _fake_device_loss = false;

   glm::ivec2 _resolution;

   bool _water_refraction = false;

   User_config _config{"shader patch.ini"s};

   Ps_3f_shader_constant<constants::ps::fog_range> _fog_range_const;
   Ps_3f_shader_constant<constants::ps::fog_color> _fog_color_const;
   Ps_4f_shader_constant<constants::ps::rt_resolution> _rt_resolution_const;
   Vs_1f_shader_constant<constants::vs::time> _time_vs_const;

   Shader_database _shaders;
   Texture_database _textures;

   const std::chrono::steady_clock::time_point _device_start{
      std::chrono::steady_clock::now()};

   Render_state_block _state_block;

   std::atomic<ULONG> _ref_count{1};
};
}
