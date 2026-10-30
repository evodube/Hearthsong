#pragma once

struct IDXGISwapChain;

namespace Widget
{
	struct DebugSwitches
	{
		bool hookPresent = true;  // [Debug] bHookPresent
		bool inputSink = true;    // [Debug] bInputSink
		bool blockInput = true;   // [Debug] bBlockInput: in cursor mode the game and other mods get no keys
		bool logInput = false;    // [Debug] bLogInput
	};

	void                 LoadSettings();  // plugin load
	const DebugSwitches& GetDebugSwitches();
	bool                 InstallInputSink();  // kDataLoaded
	void                 PrepareInputBlocker();  // plugin load (reserves trampoline space)
	void                 InstallInputBlocker();  // first kPostLoadGame / kNewGame: after every mod that hooks input at the main menu
	void                 OnDataLoaded();      // finds Skyrim's music sound category
	void                 OnGameLoaded();      // a save was loaded / new game: the widget may show
	void                 OnPresent(IDXGISwapChain* a_swapChain);  // render thread, inside Present
	int                  SehFilter(EXCEPTION_POINTERS* a_info, const char* a_where);
}
