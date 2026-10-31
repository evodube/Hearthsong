// Hearthsong - an in-game music widget for Skyrim SE / AE
// Copyright (C) 2026 evodube
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// Additional permissions: the Modding Exception and the GPL-3.0 Linking
// Exception from CommonLibSSE-NG apply (see EXCEPTIONS.txt).
// Source code: https://github.com/evodube/Hearthsong

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
	void                 InstallInputBlocker();  // first kPostLoadGame / kNewGame: after every mod that hooks input at the main menu
	void                 OnDataLoaded();      // finds Skyrim's music sound category
	void                 OnGameLoaded();      // a save was loaded / new game: the widget may show
	void                 OnPresent(IDXGISwapChain* a_swapChain);  // render thread, inside Present
	int                  SehFilter(EXCEPTION_POINTERS* a_info, const char* a_where);
}
