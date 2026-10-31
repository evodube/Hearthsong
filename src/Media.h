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

// Talks to the music app through Windows' own media controls (the ones behind the keyboard's media keys
// and the volume pop-up) and its volume through the Windows volume mixer. No Spotify account, no internet.
// Everything runs on one background thread; the game only reads snapshots and queues commands.
// No game headers here (this file is compiled without CommonLibSSE).

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace Media
{
	struct Snapshot
	{
		bool        ready = false;       // Windows' media controls answered
		bool        unavailable = false;  // they never will (old Windows): the widget says so
		bool        hasSession = false;  // the music app is running and has shown up in Windows' media controls
		std::string app;                 // app id Windows reports (e.g. "Spotify.exe")
		std::vector<std::string> sessions;  // every app Windows' media controls know about right now
		bool        appRunning = false;     // the music app's program is running (even if Windows doesn't list it)
		std::string title, artist, album;
		bool        playing = false;
		bool        shuffle = false, shuffleKnown = false;
		int         repeat = 0;  // 0 off, 1 one track, 2 whole list
		bool        repeatKnown = false;
		bool        canPlayPause = false, canNext = false, canPrev = false, canSeek = false;
		double      position = 0.0, duration = 0.0;  // seconds
		std::chrono::steady_clock::time_point positionAt{};  // when position was true (extrapolate while playing)
		float       volume = -1.0f;  // 0..1 in the Windows volume mixer; -1 = app not found there
		bool        muted = false;
		std::uint64_t artVersion = 0;  // changes when the album art changes
	};

	struct Settings
	{
		std::string targetApp = "Spotify";  // matched (case-insensitive) against the app id and the program file name
		bool        anyPlayer = false;      // no match: use whatever Windows says is playing
	};

	void Start(const Settings& a_settings);  // starts the background thread (once)
	void SetSettings(const Settings& a_settings);
	Snapshot Get();
	double   CurrentPosition(const Snapshot& a_s);  // position extrapolated to now

	// Album art as RGBA8 pixels. Returns false when a_version is already the newest.
	bool TakeArt(std::uint64_t& a_version, std::vector<std::uint8_t>& a_rgba, int& a_w, int& a_h);

	// Commands (queued; the background thread sends them)
	void PlayPause();
	void Next();
	void Previous();
	void ToggleShuffle();
	void CycleRepeat();
	void Seek(double a_seconds);
	void SetVolume(float a_volume);  // 0..1
	void ChangeVolume(float a_delta);
	void OpenApp();  // starts the music app (spotify: link)
}
