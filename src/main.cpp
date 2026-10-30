#include "Widget.h"

namespace
{
	void SetupLog()
	{
		auto path = SKSE::log::log_directory();
		if (!path) {
			return;
		}
		// keep the last runs: Hearthsong.log (this run), Hearthsong.1.log, Hearthsong.2.log
		{
			const auto      dir = *path;
			std::error_code ec;
			std::filesystem::remove(dir / "Hearthsong.2.log", ec);
			std::filesystem::rename(dir / "Hearthsong.1.log", dir / "Hearthsong.2.log", ec);
			std::filesystem::rename(dir / "Hearthsong.log", dir / "Hearthsong.1.log", ec);
		}
		*path /= "Hearthsong.log";
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
		auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));
		log->set_level(spdlog::level::info);
		log->flush_on(spdlog::level::info);
		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v");
	}

	// ------------------------------------------------------------------ Present hook
	// Same approach as Hearthframe: patch the game's own swap chain (no dummy swap chain, which breaks
	// ENB Frame Generation). With Hearthframe installed too, each hook calls the one before it.
	using PresentFn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
	PresentFn         g_originalPresent = nullptr;
	std::atomic<bool> g_presentDisabled{ false };

	void SafeOnPresent(IDXGISwapChain* a_sc)
	{
		__try {
			Widget::OnPresent(a_sc);
		} __except (Widget::SehFilter(GetExceptionInformation(), "OnPresent")) {
			g_presentDisabled = true;
		}
	}

	HRESULT STDMETHODCALLTYPE HookedPresent(IDXGISwapChain* a_sc, UINT a_sync, UINT a_flags)
	{
		if (!g_presentDisabled) {
			SafeOnPresent(a_sc);
		}
		return g_originalPresent(a_sc, a_sync, a_flags);
	}

	bool PatchPresentSlot(IDXGISwapChain* a_sc)
	{
		auto** vtable = *reinterpret_cast<void***>(a_sc);
		if (vtable[8] == reinterpret_cast<void*>(&HookedPresent)) {
			return true;
		}
		DWORD old = 0;
		if (!::VirtualProtect(&vtable[8], sizeof(void*), PAGE_EXECUTE_READWRITE, &old)) {
			spdlog::error("  VirtualProtect failed ({})", ::GetLastError());
			return false;
		}
		g_originalPresent = reinterpret_cast<PresentFn>(vtable[8]);
		vtable[8] = reinterpret_cast<void*>(&HookedPresent);
		::VirtualProtect(&vtable[8], sizeof(void*), old, &old);
		spdlog::info("  hooked Present of swap chain {}", static_cast<void*>(a_sc));
		return true;
	}

	IDXGISwapChain* GetGameSwapChain()
	{
		auto* renderer = RE::BSGraphics::Renderer::GetSingleton();
		return renderer ? renderer->data.renderWindows[0].swapChain : nullptr;
	}

	bool SafeInstallPresentHook()
	{
		__try {
			auto* sc = GetGameSwapChain();
			return sc && PatchPresentSlot(sc);
		} __except (Widget::SehFilter(GetExceptionInformation(), "InstallPresentHook")) {
			return false;
		}
	}

	bool SafeInstallInputSink()
	{
		__try {
			return Widget::InstallInputSink();
		} __except (Widget::SehFilter(GetExceptionInformation(), "InstallInputSink")) {
			return false;
		}
	}

	void OnMessage(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kDataLoaded:
			{
				const auto& dbg = Widget::GetDebugSwitches();
				if (dbg.hookPresent) {
					if (SafeInstallPresentHook()) {
						spdlog::info("Present hook installed");
					} else {
						spdlog::error("Could not install the Present hook - the widget will not appear");
					}
				} else {
					spdlog::warn("Present hook DISABLED by bHookPresent=0");
				}
				if (dbg.inputSink) {
					if (SafeInstallInputSink()) {
						spdlog::info("Input sink registered");
					} else {
						spdlog::error("Could not register for input events - the keys will not work");
					}
				} else {
					spdlog::warn("Input sink DISABLED by bInputSink=0");
				}
				Widget::OnDataLoaded();
				break;
			}
		case SKSE::MessagingInterface::kPostLoadGame:
		case SKSE::MessagingInterface::kNewGame:
			Widget::OnGameLoaded();
			break;
		default:
			break;
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	SetupLog();
	spdlog::info("Hearthsong {} loaded, SKSE {}", SKSE::PluginDeclaration::GetSingleton()->GetVersion().string("."sv),
		REL::Version::unpack(a_skse->SKSEVersion()).string("."sv));
	const auto ver = REL::Module::get().version();
	spdlog::info("Game version {}", ver.string("."sv));
	Widget::LoadSettings();
	if (!SKSE::GetMessagingInterface()->RegisterListener(OnMessage)) {
		spdlog::error("Could not register with the SKSE messaging interface");
		return false;
	}
	return true;
}
