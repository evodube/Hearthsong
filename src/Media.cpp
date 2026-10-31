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

// Windows media controls (GlobalSystemMediaTransportControls) + volume mixer (Core Audio).
// Compiled without the precompiled header: C++/WinRT and CommonLibSSE's WinAPI shim don't mix.

#include "Media.h"

#ifndef NOMINMAX
#	define NOMINMAX
#endif
#include <Windows.h>
#include <unknwn.h>
#include <shellapi.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <tlhelp32.h>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>

#include <spdlog/spdlog.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#include <stb_image.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <mutex>
#include <thread>

namespace wmc = winrt::Windows::Media::Control;
namespace wss = winrt::Windows::Storage::Streams;
using winrt::Windows::Media::MediaPlaybackAutoRepeatMode;

namespace Media
{
	namespace
	{
		enum class Cmd
		{
			PlayPause,
			Next,
			Previous,
			Shuffle,
			Repeat,
			Seek,
			Volume,
			VolumeDelta,
			Open
		};
		struct Command
		{
			Cmd    cmd;
			double value = 0.0;
		};

		// Heap-allocated and never freed: the detached thread may still run while the game exits.
		struct Shared
		{
			std::mutex              mtx;
			std::condition_variable cv;
			std::deque<Command>     cmds;
			Snapshot                snap;
			Settings                settings;
			std::uint64_t           artVersion = 0;
			std::vector<std::uint8_t> art;
			int                     artW = 0, artH = 0;
		};
		Shared&           S = *new Shared;
		std::atomic<bool> g_started{ false };

		std::string Lower(std::string a_s)
		{
			std::transform(a_s.begin(), a_s.end(), a_s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return a_s;
		}

		void Push(Cmd a_cmd, double a_value = 0.0)
		{
			{
				std::lock_guard lk(S.mtx);
				if (a_cmd == Cmd::Volume && !S.cmds.empty() && S.cmds.back().cmd == Cmd::Volume) {
					S.cmds.back().value = a_value;  // dragging the slider: only the newest value matters
				} else if (S.cmds.size() < 64) {
					S.cmds.push_back({ a_cmd, a_value });
				}
			}
			S.cv.notify_one();
		}

		// ------------------------------------------------------------ volume mixer
		std::string ProcessName(DWORD a_pid)
		{
			std::string out;
			if (HANDLE h = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, a_pid)) {
				wchar_t path[MAX_PATH]{};
				DWORD   n = MAX_PATH;
				if (::QueryFullProcessImageNameW(h, 0, path, &n)) {
					std::wstring p(path, n);
					const auto   slash = p.find_last_of(L"\\/");
					out = winrt::to_string(slash == std::wstring::npos ? p : p.substr(slash + 1));
				}
				::CloseHandle(h);
			}
			return Lower(out);
		}

		// Is a program whose file name contains a_needle running? (Toolhelp snapshot, no rights needed)
		bool ProcessRunning(const std::string& a_needle)
		{
			if (a_needle.empty()) {
				return false;
			}
			HANDLE snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (snap == INVALID_HANDLE_VALUE) {
				return false;
			}
			PROCESSENTRY32W pe{};
			pe.dwSize = sizeof pe;
			bool found = false;
			for (BOOL ok = ::Process32FirstW(snap, &pe); ok && !found; ok = ::Process32NextW(snap, &pe)) {
				found = Lower(winrt::to_string(pe.szExeFile)).find(a_needle) != std::string::npos;
			}
			::CloseHandle(snap);
			return found;
		}

		// All volume-mixer entries of the target program, on every active output device.
		std::vector<winrt::com_ptr<ISimpleAudioVolume>> FindVolumes(const std::string& a_needle)
		{
			std::vector<winrt::com_ptr<ISimpleAudioVolume>> out;
			if (a_needle.empty()) {
				return out;
			}
			winrt::com_ptr<IMMDeviceEnumerator> en;
			if (FAILED(::CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), en.put_void()))) {
				return out;
			}
			winrt::com_ptr<IMMDeviceCollection> devices;
			if (FAILED(en->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, devices.put()))) {
				return out;
			}
			UINT count = 0;
			devices->GetCount(&count);
			for (UINT d = 0; d < count; ++d) {
				winrt::com_ptr<IMMDevice> dev;
				if (FAILED(devices->Item(d, dev.put()))) {
					continue;
				}
				winrt::com_ptr<IAudioSessionManager2> mgr;
				if (FAILED(dev->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, mgr.put_void()))) {
					continue;
				}
				winrt::com_ptr<IAudioSessionEnumerator> sessions;
				if (FAILED(mgr->GetSessionEnumerator(sessions.put()))) {
					continue;
				}
				int n = 0;
				sessions->GetCount(&n);
				for (int i = 0; i < n; ++i) {
					winrt::com_ptr<IAudioSessionControl> ctl;
					if (FAILED(sessions->GetSession(i, ctl.put()))) {
						continue;
					}
					auto ctl2 = ctl.try_as<IAudioSessionControl2>();
					if (!ctl2) {
						continue;
					}
					DWORD pid = 0;
					if (FAILED(ctl2->GetProcessId(&pid)) || pid == 0) {
						continue;
					}
					if (ProcessName(pid).find(a_needle) == std::string::npos) {
						continue;
					}
					if (auto vol = ctl.try_as<ISimpleAudioVolume>()) {
						out.push_back(vol);
					}
				}
			}
			return out;
		}

		// ------------------------------------------------------------ album art
		struct Box
		{
			int x = 0, y = 0, w = 0, h = 0;
		};

		// Spotify hands Windows the cover on a flat dark background with its logo and name underneath.
		// Find the cover itself: rows/columns mostly different from the background colour. Returns false
		// (keep the whole picture) when the picture doesn't look like that.
		bool FindCover(const std::uint8_t* a_px, int a_w, int a_h, Box& a_out)
		{
			if (a_w < 32 || a_h < 32) {
				return false;
			}
			auto at = [&](int x, int y) { return a_px + (static_cast<std::size_t>(y) * a_w + x) * 4; };
			// background: the four corners must agree
			const std::uint8_t* c[4] = { at(1, 1), at(a_w - 2, 1), at(1, a_h - 2), at(a_w - 2, a_h - 2) };
			for (int i = 1; i < 4; ++i) {
				for (int k = 0; k < 3; ++k) {
					if (std::abs(static_cast<int>(c[i][k]) - c[0][k]) > 12) {
						return false;
					}
				}
			}
			auto differs = [&](int x, int y) {
				const auto* p = at(x, y);
				return std::abs(p[0] - c[0][0]) + std::abs(p[1] - c[0][1]) + std::abs(p[2] - c[0][2]) > 36;
			};
			// rows where much of the picture isn't background; the cover is the longest unbroken run of them
			// (the logo and name below it are separated from the cover by plain background rows)
			int top = -1, bottom = -1, runStart = -1;
			for (int y = 0; y <= a_h; ++y) {
				bool cover = false;
				if (y < a_h) {
					int n = 0;
					for (int x = 0; x < a_w; ++x) {
						n += differs(x, y);
					}
					cover = n >= a_w * 4 / 10;
				}
				if (cover && runStart < 0) {
					runStart = y;
				} else if (!cover && runStart >= 0) {
					if (top < 0 || (y - 1) - runStart > bottom - top) {
						top = runStart;
						bottom = y - 1;
					}
					runStart = -1;
				}
			}
			if (top < 0) {
				return false;
			}
			// same for the columns within those rows (longest unbroken run)
			int left = -1, right = -1;
			runStart = -1;
			for (int x = 0; x <= a_w; ++x) {
				bool cover = false;
				if (x < a_w) {
					int n = 0;
					for (int y = top; y <= bottom; ++y) {
						n += differs(x, y);
					}
					cover = n >= (bottom - top + 1) * 4 / 10;
				}
				if (cover && runStart < 0) {
					runStart = x;
				} else if (!cover && runStart >= 0) {
					if (left < 0 || (x - 1) - runStart > right - left) {
						left = runStart;
						right = x - 1;
					}
					runStart = -1;
				}
			}
			if (left < 0) {
				return false;
			}
			// covers are square: take the width (a dark cover edge can shorten the rows found), keep it inside
			const int side = right - left + 1;
			if (side < a_w * 3 / 10 || side > a_w - 2 || top + side > a_h) {
				return false;  // no real margin around it: not the branded layout
			}
			if (bottom - top + 1 < side * 6 / 10) {
				return false;
			}
			a_out = { left, top, side, side };
			return true;
		}

		bool ReadArt(const wss::IRandomAccessStreamReference& a_ref, bool a_spotify, std::vector<std::uint8_t>& a_rgba, int& a_w, int& a_h)
		{
			if (!a_ref) {
				return false;
			}
			auto stream = a_ref.OpenReadAsync().get();
			const auto size = static_cast<std::uint32_t>((std::min)(stream.Size(), static_cast<std::uint64_t>(16u << 20)));
			if (size == 0) {
				return false;
			}
			wss::DataReader reader(stream.GetInputStreamAt(0));
			const auto      got = reader.LoadAsync(size).get();
			std::vector<std::uint8_t> bytes(got);
			reader.ReadBytes(bytes);
			int   w = 0, h = 0, n = 0;
			auto* px = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &w, &h, &n, 4);
			if (!px) {
				return false;
			}
			// Spotify: just the cover, without the dark border and the logo/name under it
			Box box{ 0, 0, w, h };
			if (a_spotify) {
				if (Box cover; FindCover(px, w, h, cover)) {
					spdlog::info("Album art: cover {}x{} at {},{} cut out of Spotify's {}x{} picture", cover.w, cover.h, cover.x, cover.y, w, h);
					box = cover;
				}
			}
			// keep it small: nearest-neighbour down to 512 px on the long side
			const int step = (std::max)(1, ((std::max)(box.w, box.h) + 511) / 512);
			a_w = box.w / step;
			a_h = box.h / step;
			a_rgba.resize(static_cast<std::size_t>(a_w) * a_h * 4);
			for (int y = 0; y < a_h; ++y) {
				for (int x = 0; x < a_w; ++x) {
					std::memcpy(&a_rgba[(static_cast<std::size_t>(y) * a_w + x) * 4], px + (static_cast<std::size_t>(box.y + y * step) * w + box.x + x * step) * 4, 4);
				}
			}
			stbi_image_free(px);
			return true;
		}

		// ------------------------------------------------------------ the thread
		wmc::GlobalSystemMediaTransportControlsSession Pick(const wmc::GlobalSystemMediaTransportControlsSessionManager& a_mgr, const Settings& a_set)
		{
			const auto needle = Lower(a_set.targetApp);
			if (!needle.empty()) {
				for (const auto& s : a_mgr.GetSessions()) {
					if (Lower(winrt::to_string(s.SourceAppUserModelId())).find(needle) != std::string::npos) {
						return s;
					}
				}
			}
			if (a_set.anyPlayer || needle.empty()) {
				return a_mgr.GetCurrentSession();
			}
			return nullptr;
		}

		void Run(wmc::GlobalSystemMediaTransportControlsSessionManager& a_mgr, const Command& a_c, const wmc::GlobalSystemMediaTransportControlsSession& a_s,
			const std::string& a_volNeedle)
		{
			switch (a_c.cmd) {
			case Cmd::Open:
				::ShellExecuteW(nullptr, L"open", L"spotify:", nullptr, nullptr, SW_SHOWNOACTIVATE);
				return;
			case Cmd::Volume:
			case Cmd::VolumeDelta:
				{
					for (auto& v : FindVolumes(a_volNeedle)) {
						float cur = 0.0f;
						v->GetMasterVolume(&cur);
						const float next = std::clamp(a_c.cmd == Cmd::Volume ? static_cast<float>(a_c.value) : cur + static_cast<float>(a_c.value), 0.0f, 1.0f);
						v->SetMasterVolume(next, nullptr);
						if (next > 0.0f) {
							v->SetMute(FALSE, nullptr);
						}
					}
					return;
				}
			default:
				break;
			}
			if (!a_s) {
				return;
			}
			switch (a_c.cmd) {
			case Cmd::PlayPause:
				a_s.TryTogglePlayPauseAsync().get();
				break;
			case Cmd::Next:
				a_s.TrySkipNextAsync().get();
				break;
			case Cmd::Previous:
				a_s.TrySkipPreviousAsync().get();
				break;
			case Cmd::Shuffle:
				{
					const auto info = a_s.GetPlaybackInfo();
					bool       on = false;
					if (auto v = info.IsShuffleActive()) {
						on = v.Value();
					}
					a_s.TryChangeShuffleActiveAsync(!on).get();
					break;
				}
			case Cmd::Repeat:
				{
					const auto info = a_s.GetPlaybackInfo();
					auto       mode = MediaPlaybackAutoRepeatMode::None;
					if (auto v = info.AutoRepeatMode()) {
						mode = v.Value();
					}
					// off -> whole list -> one track -> off (Spotify's order)
					const auto next = mode == MediaPlaybackAutoRepeatMode::None ? MediaPlaybackAutoRepeatMode::List :
					                  mode == MediaPlaybackAutoRepeatMode::List ? MediaPlaybackAutoRepeatMode::Track :
					                                                              MediaPlaybackAutoRepeatMode::None;
					a_s.TryChangeAutoRepeatModeAsync(next).get();
					break;
				}
			case Cmd::Seek:
				{
					const auto tl = a_s.GetTimelineProperties();
					const auto ticks = tl.StartTime().count() + static_cast<std::int64_t>(a_c.value * 10'000'000.0);
					a_s.TryChangePlaybackPositionAsync(ticks).get();
					break;
				}
			default:
				break;
			}
			(void)a_mgr;
		}

		void ThreadMain()
		{
			try {
				winrt::init_apartment(winrt::apartment_type::multi_threaded);
			} catch (...) {
			}
			::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

			wmc::GlobalSystemMediaTransportControlsSessionManager mgr{ nullptr };
			try {
				mgr = wmc::GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
			} catch (const winrt::hresult_error& e) {
				spdlog::error("Windows media controls not available ({:08X} {}) - needs Windows 10 version 1809 or newer",
					static_cast<std::uint32_t>(e.code()), winrt::to_string(e.message()));
			} catch (...) {
				spdlog::error("Windows media controls not available");
			}
			if (!mgr) {
				std::lock_guard lk(S.mtx);
				S.snap.unavailable = true;
				return;
			}
			spdlog::info("Windows media controls ready");

			std::string lastTrack, lastApp;
			bool        lastHad = false;
			auto        nextProps = std::chrono::steady_clock::time_point{};
			auto        nextVolume = std::chrono::steady_clock::time_point{};
			Snapshot    snap;
			snap.ready = true;
			std::string volNeedle;
			std::string lastList = "-";
			auto        nextRunning = std::chrono::steady_clock::time_point{};

			for (;;) {
				std::deque<Command> cmds;
				Settings            set;
				{
					std::unique_lock lk(S.mtx);
					S.cv.wait_for(lk, std::chrono::milliseconds(250), [] { return !S.cmds.empty(); });
					cmds.swap(S.cmds);
					set = S.settings;
				}
				volNeedle = Lower(set.targetApp);
				const auto now = std::chrono::steady_clock::now();
				bool       forceProps = false;
				try {
					{
						std::vector<std::string> list;
						std::string              joined;
						for (const auto& s : mgr.GetSessions()) {
							list.push_back(winrt::to_string(s.SourceAppUserModelId()));
							joined += (joined.empty() ? "" : ", ") + list.back();
						}
						if (joined != lastList) {
							spdlog::info("Apps in Windows' media controls: {}", joined.empty() ? "none" : joined);
							lastList = joined;
						}
						snap.sessions = std::move(list);
					}
					if (now >= nextRunning) {
						nextRunning = now + std::chrono::seconds(2);
						const bool running = ProcessRunning(volNeedle);
						if (running != snap.appRunning) {
							spdlog::info("{} is {}running", set.targetApp, running ? "" : "not ");
						}
						snap.appRunning = running;
					}
					auto session = Pick(mgr, set);
					for (const auto& c : cmds) {
						try {
							Run(mgr, c, session, volNeedle);
						} catch (const winrt::hresult_error& e) {
							spdlog::warn("Media command {} failed: {:08X}", static_cast<int>(c.cmd), static_cast<std::uint32_t>(e.code()));
						}
						forceProps = true;
						nextVolume = {};
					}
					snap.hasSession = static_cast<bool>(session);
					if (snap.hasSession != lastHad) {
						spdlog::info(snap.hasSession ? "Music app found in Windows' media controls" : "No music app in Windows' media controls");
						lastHad = snap.hasSession;
					}
					if (session) {
						snap.app = winrt::to_string(session.SourceAppUserModelId());
						if (snap.app != lastApp) {
							spdlog::info("Controlling: {}", snap.app);
							lastApp = snap.app;
						}
						const auto info = session.GetPlaybackInfo();
						snap.playing = info.PlaybackStatus() == wmc::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
						const auto ctl = info.Controls();
						snap.canPlayPause = ctl.IsPlayPauseToggleEnabled() || ctl.IsPlayEnabled() || ctl.IsPauseEnabled();
						snap.canNext = ctl.IsNextEnabled();
						snap.canPrev = ctl.IsPreviousEnabled();
						snap.canSeek = ctl.IsPlaybackPositionEnabled();
						if (auto v = info.IsShuffleActive()) {
							snap.shuffle = v.Value();
							snap.shuffleKnown = true;
						} else {
							snap.shuffleKnown = false;
						}
						if (auto v = info.AutoRepeatMode()) {
							const auto m = v.Value();
							snap.repeat = m == MediaPlaybackAutoRepeatMode::Track ? 1 : (m == MediaPlaybackAutoRepeatMode::List ? 2 : 0);
							snap.repeatKnown = true;
						} else {
							snap.repeatKnown = false;
						}
						const auto tl = session.GetTimelineProperties();
						const double start = tl.StartTime().count() / 1e7, end = tl.EndTime().count() / 1e7;
						double       pos = tl.Position().count() / 1e7 - start;
						if (snap.playing) {  // the app only reports now and then: add the time since its last report
							const auto since = winrt::clock::now() - tl.LastUpdatedTime();
							const double s = std::chrono::duration<double>(since).count();
							if (s > 0.0 && s < 6 * 3600.0) {
								pos += s;
							}
						}
						snap.duration = (std::max)(0.0, end - start);
						snap.position = std::clamp(pos, 0.0, snap.duration > 0.0 ? snap.duration : pos);
						snap.positionAt = now;

						if (forceProps || now >= nextProps) {
							nextProps = now + std::chrono::milliseconds(1000);
							const auto props = session.TryGetMediaPropertiesAsync().get();
							snap.title = winrt::to_string(props.Title());
							snap.artist = winrt::to_string(props.Artist());
							snap.album = winrt::to_string(props.AlbumTitle());
							const std::string key = snap.app + "|" + snap.title + "|" + snap.artist + "|" + snap.album;
							if (key != lastTrack) {
								lastTrack = key;
								std::vector<std::uint8_t> px;
								int                       w = 0, h = 0;
								bool                      ok = false;
								try {
									ok = ReadArt(props.Thumbnail(), Lower(snap.app).find("spotify") != std::string::npos, px, w, h);
								} catch (...) {
									ok = false;
								}
								std::lock_guard lk(S.mtx);
								S.art = ok ? std::move(px) : std::vector<std::uint8_t>{};
								S.artW = ok ? w : 0;
								S.artH = ok ? h : 0;
								snap.artVersion = ++S.artVersion;
							}
						}
					} else {
						snap.title.clear();
						snap.artist.clear();
						snap.album.clear();
						snap.playing = false;
						snap.position = snap.duration = 0.0;
						if (!lastTrack.empty()) {
							lastTrack.clear();
							std::lock_guard lk(S.mtx);
							S.art.clear();
							S.artW = S.artH = 0;
							snap.artVersion = ++S.artVersion;
						}
					}
				} catch (const winrt::hresult_error& e) {
					spdlog::warn("Media controls: {:08X} {}", static_cast<std::uint32_t>(e.code()), winrt::to_string(e.message()));
				} catch (...) {
					spdlog::warn("Media controls: unexpected error");
				}

				if (now >= nextVolume) {
					nextVolume = now + std::chrono::milliseconds(500);
					snap.volume = -1.0f;
					snap.muted = false;
					try {
						for (auto& v : FindVolumes(volNeedle)) {
							float vol = 0.0f;
							BOOL  mute = FALSE;
							v->GetMasterVolume(&vol);
							v->GetMute(&mute);
							snap.volume = (std::max)(snap.volume, vol);
							snap.muted = mute != FALSE;
						}
					} catch (...) {
					}
				}

				std::lock_guard lk(S.mtx);
				S.snap = snap;
			}
		}
	}

	void Start(const Settings& a_settings)
	{
		SetSettings(a_settings);
		if (g_started.exchange(true)) {
			return;
		}
		std::thread(ThreadMain).detach();
	}

	void SetSettings(const Settings& a_settings)
	{
		std::lock_guard lk(S.mtx);
		S.settings = a_settings;
	}

	Snapshot Get()
	{
		std::lock_guard lk(S.mtx);
		return S.snap;
	}

	double CurrentPosition(const Snapshot& a_s)
	{
		double p = a_s.position;
		if (a_s.playing && a_s.positionAt.time_since_epoch().count()) {
			p += std::chrono::duration<double>(std::chrono::steady_clock::now() - a_s.positionAt).count();
		}
		return a_s.duration > 0.0 ? std::clamp(p, 0.0, a_s.duration) : (std::max)(0.0, p);
	}

	bool TakeArt(std::uint64_t& a_version, std::vector<std::uint8_t>& a_rgba, int& a_w, int& a_h)
	{
		std::lock_guard lk(S.mtx);
		if (a_version == S.artVersion) {
			return false;
		}
		a_version = S.artVersion;
		a_rgba = S.art;
		a_w = S.artW;
		a_h = S.artH;
		return true;
	}

	void PlayPause() { Push(Cmd::PlayPause); }
	void Next() { Push(Cmd::Next); }
	void Previous() { Push(Cmd::Previous); }
	void ToggleShuffle() { Push(Cmd::Shuffle); }
	void CycleRepeat() { Push(Cmd::Repeat); }
	void Seek(double a_seconds) { Push(Cmd::Seek, a_seconds); }
	void SetVolume(float a_volume) { Push(Cmd::Volume, a_volume); }
	void ChangeVolume(float a_delta) { Push(Cmd::VolumeDelta, a_delta); }
	void OpenApp() { Push(Cmd::Open); }
}
