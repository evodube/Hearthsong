#pragma once

struct IDXGISwapChain;

namespace Widget
{
	struct DebugSwitches
	{
		bool hookPresent = true;  // [Debug] bHookPresent
		bool inputSink = true;    // [Debug] bInputSink
		bool logInput = false;    // [Debug] bLogInput
	};

	void                 LoadSettings();  // plugin load
	const DebugSwitches& GetDebugSwitches();
	bool                 InstallInputSink();  // kDataLoaded
	void                 OnDataLoaded();      // finds Skyrim's music sound category
	void                 OnGameLoaded();      // a save was loaded / new game: the widget may show
	void                 OnPresent(IDXGISwapChain* a_swapChain);  // render thread, inside Present
	int                  SehFilter(EXCEPTION_POINTERS* a_info, const char* a_where);
}
