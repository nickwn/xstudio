#include "rhi.hpp"
#include "rhi_vk.hpp"

#include <Windows.h>

namespace xs
{
namespace rhi
{
namespace impl_
{
    struct context_plat_data
    {
        HINSTANCE instance;
    };

    struct surface_plat_data
    {
        HWND hwnd;
    };

	LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
}


rendering_surface::rendering_surface(const rendering_context& ctx, std::wstring name, uint32_t height, uint32_t width) :
    width_(width),
    height_(height),
    surface_gfx_data_ptr_(std::make_unique<impl_::surface_gfx_data>()),
    surface_plat_data_ptr_(std::make_unique<impl_::surface_plat_data>())
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = impl_::WndProc; // TODO
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = ctx.context_plat_data_ptr_->instance;
	wcex.hIcon = LoadIcon(ctx.context_plat_data_ptr_->instance, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = TEXT("test");
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

	if (!RegisterClassEx(&wcex))
	{
		
	}

	int32_t screen_width = GetSystemMetrics(SM_CXSCREEN);
	int32_t screen_height = GetSystemMetrics(SM_CYSCREEN);
	int32_t window_x = screen_width / 2 - width_ / 2;
	int32_t window_y = screen_height / 2 - height_ / 2;

	surface_plat_data_ptr_->hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		name.c_str(),
		name.c_str(),
		WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		window_x, window_y,
		width_, height_,
		nullptr, nullptr,
		ctx.context_plat_data_ptr_->instance,
		nullptr
	);

	ShowWindow(surface_plat_data_ptr_->hwnd, SW_SHOW);
	SetForegroundWindow(surface_plat_data_ptr_->hwnd);
	SetFocus(surface_plat_data_ptr_->hwnd);
}

} // namespace rhi
} // namespace xs