#define VK_USE_PLATFORM_WIN32_KHR
#include "rhi.hpp"
#include "rhi_vk.hpp"

#include <iostream>

#define NOMINMAX
#include <Windows.h>
#include <fcntl.h>
#include <io.h>

PFN_vkCreateDebugUtilsMessengerEXT fpCreateDebugUtilsMessengerEXT_g;
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger) 
{
	return fpCreateDebugUtilsMessengerEXT_g(instance, pCreateInfo, pAllocator, pMessenger);
}

PFN_vkDestroyDebugUtilsMessengerEXT fpDestroyDebugUtilsMessengerEXT_g;
VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator)
{
	fpDestroyDebugUtilsMessengerEXT_g(instance, messenger, pAllocator);
}

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
		static int id_timer = -1;

		surface* p_this = nullptr;

		if (msg == WM_NCCREATE)
		{
			p_this = static_cast<surface*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
			SetLastError(0);
			if (!SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(p_this)))
			{
				if (GetLastError() != 0)
				{
					return FALSE;
				}
			}
		}
		else
		{
			p_this = reinterpret_cast<surface*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		}

		switch (msg) {
		case WM_CREATE:
			SetTimer(hWnd, id_timer = 1, 10, NULL);
			break;
		case WM_MOUSEWHEEL:
			for (const auto& cb : p_this->scroll_callbacks_)
			{
				cb(GET_WHEEL_DELTA_WPARAM(wParam));
			}
			break;
		case WM_KEYDOWN:
			for (const auto& cb : p_this->key_callbacks_)
			{
				cb(wParam);
			}
			break;
		case WM_DESTROY:
			KillTimer(hWnd, 1);
			DestroyWindow(hWnd);
			PostQuitMessage(0);
			break;
		case WM_SIZE:
			switch (wParam)
			{
			case SIZE_MINIMIZED:
				KillTimer(hWnd, 1);
				id_timer = -1;
				break;

			case SIZE_RESTORED:
			case SIZE_MAXIMIZED:
				if (id_timer == -1)
				{
					SetTimer(hWnd, id_timer = 1, 10, NULL);
				}
				break;
			}
			return 0L;
		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
			break;
		}
	}

	VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
		void* pUserData) 
	{
		std::string full_msg = std::string(p_callback_data->pMessage) + "\n";
		_RPT0(_CRT_WARN, full_msg.c_str());

		if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			__debugbreak();
		}

		return VK_FALSE;
	}

	static const WORD max_console_lines = 500;

	// from http://dslweb.nwnexus.com/~ast/dload/guicon.htm
#ifdef _DEBUG
	void setup_console()
	{
		int hConHandle;
		long lStdHandle;
		CONSOLE_SCREEN_BUFFER_INFO coninfo;
		FILE* fp;
		// allocate a console for this app
		AllocConsole();

		// set the screen buffer to be big enough to let us scroll text
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
		coninfo.dwSize.Y = max_console_lines;
		SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

		// redirect unbuffered STDOUT to the console
		lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
		hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
		fp = _fdopen(hConHandle, "w");
		*stdout = *fp;
		setvbuf(stdout, NULL, _IONBF, 0);

		// redirect unbuffered STDIN to the console
		lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
		hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
		fp = _fdopen(hConHandle, "r");
		*stdin = *fp;
		setvbuf(stdin, NULL, _IONBF, 0);

		// redirect unbuffered STDERR to the console
		lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
		hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
		fp = _fdopen(hConHandle, "w");
		*stderr = *fp;
		setvbuf(stderr, NULL, _IONBF, 0);

		// make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog point to console as well
		std::ios::sync_with_stdio();
	}
#endif
}

context::context(void* params) :
	context_gfx_data_ptr_(std::make_unique<impl_::context_gfx_data>()),
	context_plat_data_ptr_(std::make_unique<impl_::context_plat_data>()) // potential problem
{
#ifdef _DEBUG
	impl_::setup_console();

	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
#endif

	context_plat_data_ptr_->instance = *reinterpret_cast<HINSTANCE*>(params);
	vk::ApplicationInfo app_info = vk::ApplicationInfo()
		.setPApplicationName("application name")
		.setApplicationVersion(0)
		.setPEngineName("!(unity||unreal)")
		.setEngineVersion(0)
		.setApiVersion(VK_API_VERSION_1_1);

	static std::vector<const char*> layers = { "VK_LAYER_KHRONOS_validation" };
	static std::vector<const char*> extensions = { VK_KHR_SURFACE_EXTENSION_NAME, "VK_KHR_win32_surface", 
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME };

	auto inst_info = vk::InstanceCreateInfo()
		.setPApplicationInfo(&app_info)
		.setEnabledLayerCount(layers.size())
		.setPpEnabledLayerNames(layers.data())
		.setEnabledExtensionCount(extensions.size())
		.setPpEnabledExtensionNames(extensions.data());

	context_gfx_data_ptr_->instance = vk::createInstanceUnique(inst_info);

	vk::DebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info = vk::DebugUtilsMessengerCreateInfoEXT()
		.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
		.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
		.setPfnUserCallback(impl_::debugCallback)
		.setPUserData(nullptr);

	fpCreateDebugUtilsMessengerEXT_g = (PFN_vkCreateDebugUtilsMessengerEXT) context_gfx_data_ptr_->instance->getProcAddr("vkCreateDebugUtilsMessengerEXT");
	fpDestroyDebugUtilsMessengerEXT_g = (PFN_vkDestroyDebugUtilsMessengerEXT) context_gfx_data_ptr_->instance->getProcAddr("vkDestroyDebugUtilsMessengerEXT");
	context_gfx_data_ptr_->debug_utils_messenger = context_gfx_data_ptr_->instance->createDebugUtilsMessengerEXTUnique(debug_utils_messenger_create_info);
}

context::~context() {}

void context::log(std::string_view str)
{
	_RPT0(_CRT_WARN, str.data());
}

surface::surface(const context& ctx, std::wstring name, std::uint32_t width, std::uint32_t height) :
    width_(width),
    height_(height),
    surface_gfx_data_ptr_(std::make_unique<impl_::surface_gfx_data>()),
    surface_plat_data_ptr_(std::make_unique<impl_::surface_plat_data>())
{
	static TCHAR app_name[] = TEXT("xstudio");

	HINSTANCE hInstance = ctx.context_plat_data_ptr_->instance;
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = impl_::WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = app_name;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL, TEXT("Call to RegisterClassEx failed!"),
			TEXT("Windows Desktop Guided Tour"), NULL);
	}

	surface_plat_data_ptr_->hwnd = CreateWindow(
		app_name,
		name.c_str(),
		WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT,
		width_, height_,
		NULL,
		NULL,
		hInstance,
		reinterpret_cast<LPVOID>(this)
	);

	if (!surface_plat_data_ptr_->hwnd)
	{
		MessageBox(NULL, TEXT("Call to CreateWindow failed!"),
			TEXT("Windows Desktop Guided Tour"), NULL);
	}

	ShowWindow(surface_plat_data_ptr_->hwnd, SW_SHOW);
	SetForegroundWindow(surface_plat_data_ptr_->hwnd);
	SetFocus(surface_plat_data_ptr_->hwnd);

	vk::Win32SurfaceCreateInfoKHR createInfo = vk::Win32SurfaceCreateInfoKHR()
		.setHinstance(ctx.context_plat_data_ptr_->instance)
		.setHwnd(surface_plat_data_ptr_->hwnd);
	surface_gfx_data_ptr_->surface = ctx.context_gfx_data_ptr_->instance->createWin32SurfaceKHR(createInfo);
}

surface::~surface() {}

} // namespace rhi
} // namespace xs