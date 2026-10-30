#include "Widget.h"

#include "Media.h"
#include "Theme.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_impl_dx11.h>

#include <cfloat>

namespace fs = std::filesystem;

namespace Widget
{
	namespace
	{
		// ================================================================ settings
		constexpr float kMinW = 240.0f, kMaxW = 1000.0f, kMinH = 84.0f, kMaxH = 680.0f;  // widget size at 1080p
		constexpr float kDefW = 340.0f, kDefH = 380.0f;

		enum KeySlot
		{
			kKeyToggle,
			kKeyCursor,
			kKeyUndo,
			kKeyPlayPause,
			kKeyNext,
			kKeyPrevious,
			kKeyVolUp,
			kKeyVolDown,
			kKeyCount
		};
		const char* const kKeyIni[kKeyCount] = { "iToggleKey", "iCursorKey", "iUndoKey", "iPlayPauseKey", "iNextKey", "iPreviousKey", "iVolumeUpKey", "iVolumeDownKey" };
		const char* const kKeySection[kKeyCount] = { "General", "General", "Keys", "Keys", "Keys", "Keys", "Keys", "Keys" };
		const int         kKeyDefault[kKeyCount] = { VK_F7, VK_F6, 'Z', 0, 0, 0, 0, 0 };
		const char* const kKeyLabel[kKeyCount] = { "Show / hide widget", "Cursor mode", "Undo move (with Ctrl)", "Play / pause", "Next track", "Previous track",
			"Volume up", "Volume down" };
		const char* const kKeyHelp[kKeyCount] = {
			"Shows or hides the widget. Works any time you're playing.",
			"Shows a cursor so you can click, drag and resize the widget. While it's on, the game and other mods get no keys or clicks.",
			"In cursor mode: Ctrl + this key puts the widget back where it was before the last move or resize.",
			"Works without the cursor, any time you're playing. Your keyboard's own media keys work too.",
			"Works without the cursor.",
			"Works without the cursor.",
			"Raises the music app's volume in the Windows volume mixer by the volume step.",
			"Lowers the music app's volume in the Windows volume mixer by the volume step.",
		};

		struct Config
		{
			int   keyVk[kKeyCount]{};
			bool  cursorHold = false;  // cursor mode while the key is held, instead of press on / press off
			bool  visible = true;
			float mouseSpeed = 1.0f;
			float uiScale = 1.0f;  // settings window
			float fontSize = 18.0f;
			// widget
			float posX = 0.70f, posY = 0.06f;  // top-left corner, as a fraction of the screen
			float w = kDefW, h = kDefH;        // size at 1080p and 100 % scale
			int   scalePct = 100;
			int   opacityPct = 95;
			bool  showHeader = true, showArt = true, showProgress = true, showSound = true;
			bool  locked = false;
			// safety
			bool  leaveCursorInCombat = true;
			bool  noCursorInCombat = true;
			int   dragDelayMs = 150;
			int   dragDistance = 6;
			// hiding
			bool  autoHideInCombat = false;
			float showAgainAfter = 2.0f;
			bool  hideInMenus = true;
			bool  hideWhenNoPlayer = false;
			// sound
			bool        muteGameMusic = true;
			int         volumeStep = 5;
			std::string musicApp = "Spotify";
			bool        anyPlayer = false;
		} cfg;

		DebugSwitches dbg;
		std::string   g_iniPath;

		std::uint32_t VirtualKeyToDIK(int a_vk)
		{
			if (a_vk <= 0) {
				return 0;
			}
			const UINT sc = ::MapVirtualKeyW(static_cast<UINT>(a_vk), MAPVK_VK_TO_VSC_EX);
			if (sc == 0) {
				return 0;
			}
			if ((sc & 0xFF00) == 0xE000 || (sc & 0xFF00) == 0xE100) {
				return (sc & 0x7F) | 0x80;  // extended keys: DirectInput sets the high bit
			}
			return sc & 0xFF;
		}

		int DIKToVirtualKey(std::uint32_t a_dik)
		{
			const UINT sc = (a_dik & 0x7F) | ((a_dik & 0x80) ? 0xE000u : 0u);
			return static_cast<int>(::MapVirtualKeyW(sc, MAPVK_VSC_TO_VK_EX));
		}

		std::atomic<std::uint32_t> g_keyDik[kKeyCount]{};

		void RefreshKeyCodes()
		{
			for (int i = 0; i < kKeyCount; ++i) {
				g_keyDik[i] = VirtualKeyToDIK(cfg.keyVk[i]);
			}
		}

		std::string ToUtf8(const std::wstring& a_s)
		{
			if (a_s.empty()) {
				return {};
			}
			const int   n = ::WideCharToMultiByte(CP_UTF8, 0, a_s.c_str(), static_cast<int>(a_s.size()), nullptr, 0, nullptr, nullptr);
			std::string out(static_cast<std::size_t>(n), '\0');
			::WideCharToMultiByte(CP_UTF8, 0, a_s.c_str(), static_cast<int>(a_s.size()), out.data(), n, nullptr, nullptr);
			return out;
		}

		std::string KeyName(int a_vk)
		{
			if (a_vk <= 0) {
				return "None";
			}
			const std::uint32_t dik = VirtualKeyToDIK(a_vk);
			wchar_t             name[64]{};
			const LONG          lparam = static_cast<LONG>(((dik & 0x7F) << 16) | ((dik & 0x80) ? (1 << 24) : 0));
			if (dik && ::GetKeyNameTextW(lparam, name, 64) > 0) {
				return ToUtf8(name);
			}
			return fmt::format("Key {}", a_vk);
		}

		// ---- INI
		float ReadFloat(const char* a_sec, const char* a_key, float a_def, float a_min, float a_max)
		{
			char buf[64]{};
			::GetPrivateProfileStringA(a_sec, a_key, fmt::format("{}", a_def).c_str(), buf, sizeof buf, g_iniPath.c_str());
			const float v = static_cast<float>(std::atof(buf));
			return std::isfinite(v) ? std::clamp(v, a_min, a_max) : a_def;
		}
		int ReadInt(const char* a_sec, const char* a_key, int a_def, int a_min, int a_max)
		{
			return std::clamp(static_cast<int>(::GetPrivateProfileIntA(a_sec, a_key, a_def, g_iniPath.c_str())), a_min, a_max);
		}
		bool ReadBool(const char* a_sec, const char* a_key, bool a_def)
		{
			return ::GetPrivateProfileIntA(a_sec, a_key, a_def ? 1 : 0, g_iniPath.c_str()) != 0;
		}
		void Write(const char* a_sec, const char* a_key, const std::string& a_v)
		{
			::WritePrivateProfileStringA(a_sec, a_key, a_v.c_str(), g_iniPath.c_str());
		}
		std::string B(bool a_v) { return a_v ? "1" : "0"; }
		std::string F(float a_v) { return fmt::format("{:.4f}", a_v); }

		void SaveAll()
		{
			for (int i = 0; i < kKeyCount; ++i) {
				Write(kKeySection[i], kKeyIni[i], std::to_string(cfg.keyVk[i]));
			}
			Write("General", "bCursorHold", B(cfg.cursorHold));
			Write("General", "bVisible", B(cfg.visible));
			Write("General", "fMouseSpeed", fmt::format("{:.2f}", cfg.mouseSpeed));
			Write("General", "fUIScale", fmt::format("{:.2f}", cfg.uiScale));
			Write("Widget", "fPosX", F(cfg.posX));
			Write("Widget", "fPosY", F(cfg.posY));
			Write("Widget", "fWidth", fmt::format("{:.0f}", cfg.w));
			Write("Widget", "fHeight", fmt::format("{:.0f}", cfg.h));
			Write("Widget", "iScale", std::to_string(cfg.scalePct));
			Write("Widget", "iOpacity", std::to_string(cfg.opacityPct));
			Write("Widget", "bShowTitleBar", B(cfg.showHeader));
			Write("Widget", "bShowAlbumArt", B(cfg.showArt));
			Write("Widget", "bShowProgress", B(cfg.showProgress));
			Write("Widget", "bShowSound", B(cfg.showSound));
			Write("Widget", "bLocked", B(cfg.locked));
			Write("Safety", "bLeaveCursorInCombat", B(cfg.leaveCursorInCombat));
			Write("Safety", "bNoCursorInCombat", B(cfg.noCursorInCombat));
			Write("Safety", "iDragDelayMs", std::to_string(cfg.dragDelayMs));
			Write("Safety", "iDragDistance", std::to_string(cfg.dragDistance));
			Write("Hiding", "bAutoHideInCombat", B(cfg.autoHideInCombat));
			Write("Hiding", "fShowAgainAfter", fmt::format("{:.1f}", cfg.showAgainAfter));
			Write("Hiding", "bHideInMenus", B(cfg.hideInMenus));
			Write("Hiding", "bHideWhenNoPlayer", B(cfg.hideWhenNoPlayer));
			Write("Sound", "bMuteGameMusic", B(cfg.muteGameMusic));
			Write("Sound", "iVolumeStep", std::to_string(cfg.volumeStep));
			Write("Sound", "sMusicApp", cfg.musicApp);
			Write("Sound", "bAnyPlayer", B(cfg.anyPlayer));
		}

		Media::Settings MediaSettings()
		{
			Media::Settings s;
			s.targetApp = cfg.musicApp;
			s.anyPlayer = cfg.anyPlayer;
			return s;
		}

		// ================================================================ state
		struct InputMsg
		{
			enum class Kind
			{
				MouseMove,
				MouseButton,
				Wheel,
				Key,
				Char
			} kind;
			float    x = 0.0f, y = 0.0f;
			int      button = 0;
			ImGuiKey key = ImGuiKey_None;
			bool     down = false;
		};
		std::mutex            g_inputMtx;
		std::vector<InputMsg> g_input;

		std::atomic<bool> g_cursor{ false };      // cursor mode is on (applied in Present)
		std::atomic<bool> g_wantCursor{ false };  // requested by the input thread
		std::atomic<bool> g_toggleVisible{ false };
		std::atomic<bool> g_undoRequest{ false };
		std::atomic<bool> g_escRequest{ false };
		std::atomic<bool> g_shiftDown{ false }, g_ctrlDown{ false }, g_altDown{ false };
		std::atomic<int>  g_captureSlot{ -1 };       // settings: waiting for a key for this slot
		std::atomic<int>  g_capturedVk{ 0 };         // result (-1 cancelled, -2 cleared)
		std::atomic<bool> g_inputDisabled{ false };
		std::atomic<bool> g_gameLoaded{ false };
		std::atomic<bool> g_inCombatInput{ false };  // combat state as last seen by Present
		std::atomic<bool> g_pausedInput{ false };

		bool                 g_imguiReady = false, g_imguiFailed = false;
		IDXGISwapChain*      g_sc = nullptr;
		ID3D11Device*        g_dev = nullptr;
		ID3D11DeviceContext* g_ctx = nullptr;
		float                g_displayW = 1920.0f, g_displayH = 1080.0f;
		float                g_mouseX = 0.0f, g_mouseY = 0.0f;
		std::chrono::steady_clock::time_point g_lastFrame{};
		bool                 g_pendingFonts = false;
		bool                 g_settingsOpen = false;
		int                  g_settingsTab = 0;
		bool                 g_dirty = false;
		float                g_dirtyAt = 0.0f;
		float                g_time = 0.0f;
		float                g_alpha = 0.0f;   // whole widget fades in/out
		float                g_combatEndedAt = -100.0f;
		bool                 g_wasInCombat = false;
		ImFont*              g_fText = nullptr;   // Inter Regular, widget text
		ImFont*              g_fSemi = nullptr;   // Inter SemiBold, song title
		ImFont*              g_fTitle = nullptr;  // Cinzel, "Now Playing"
		float                g_fontK = 1.0f;      // widget scale the fonts were made for
		ID3D11ShaderResourceView* g_art = nullptr;
		int                  g_artW = 0, g_artH = 0;
		std::uint64_t        g_artVersion = 0;
		float                g_volDrag = -1.0f;   // volume slider value while dragging (and a moment after)
		float                g_volDragUntil = 0.0f;
		bool                 g_controlsBlocked = false;
		std::uintptr_t       g_dispatchInput = 0;  // input blocker: the function we replaced the call to (game, or a mod in front of it)
		std::string          g_status;            // short message under the settings footer
		char                 g_appBuf[64]{};

		struct Rect
		{
			float x, y, w, h;  // x, y: screen fractions; w, h: size at 1080p
		};
		std::vector<Rect> g_undo;

		// drag / resize
		enum class Grab
		{
			None,
			Move,
			N,
			S,
			E,
			W,
			NE,
			NW,
			SE,
			SW
		};
		struct DragState
		{
			Grab   mode = Grab::None;
			bool   active = false;  // passed the delay and the distance
			ImVec2 start{};
			float  startTime = 0.0f;
			ImVec2 pos0{}, size0{};  // panel in pixels when the press started
		} g_drag;

		void MarkDirty()
		{
			g_dirty = true;
			g_dirtyAt = g_time;
		}

		void PushInput(const InputMsg& a_m)
		{
			std::lock_guard lk(g_inputMtx);
			if (g_input.size() < 1024) {
				g_input.push_back(a_m);
			}
		}

		void Notify(std::string a_text)
		{
			if (auto* t = SKSE::GetTaskInterface()) {
				t->AddTask([a_text]() { RE::SendHUDMessage::ShowHUDMessage(a_text.c_str()); });
			}
		}

		// ================================================================ game state
		bool PlayerInCombat()
		{
			auto* pc = RE::PlayerCharacter::GetSingleton();
			return pc && pc->IsInCombat();
		}

		bool GamePaused()
		{
			auto* ui = RE::UI::GetSingleton();
			return !ui || ui->GameIsPaused() || ui->IsMenuOpen(RE::MainMenu::MENU_NAME) || ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME);
		}

		bool ConsoleOpen()
		{
			auto* ui = RE::UI::GetSingleton();
			return ui && ui->IsMenuOpen(RE::Console::MENU_NAME);
		}

		// Another mod's menu or a conversation has the player's controls: don't take the mouse over.
		bool OthersBusy()
		{
			auto* ui = RE::UI::GetSingleton();
			if (ui && ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME)) {
				return true;
			}
			auto* cm = RE::ControlMap::GetSingleton();
			using UEFlag = RE::ControlMap::UEFlag;
			return !g_controlsBlocked && cm &&
			       !cm->AreControlsEnabled(static_cast<UEFlag>(static_cast<std::uint32_t>(UEFlag::kMovement) | static_cast<std::uint32_t>(UEFlag::kLooking)));
		}

		// ---- Skyrim's music: turned down while the music app plays
		RE::BGSSoundCategory* g_musicCategory = nullptr;
		bool                  g_musicMuted = false;
		float                 g_musicSaved = 1.0f;

		void FindMusicCategory()
		{
			auto* dh = RE::TESDataHandler::GetSingleton();
			if (!dh) {
				return;
			}
			for (auto* cat : dh->GetFormArray<RE::BGSSoundCategory>()) {
				if (!cat) {
					continue;
				}
				std::string name = cat->GetFullName() ? cat->GetFullName() : "";
				std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				if (cat->IsMenuCategory() && name.find("music") != std::string::npos) {
					g_musicCategory = cat;
					break;
				}
			}
			if (g_musicCategory) {
				spdlog::info("Skyrim's music volume: sound category {:08X} \"{}\"", g_musicCategory->GetFormID(), g_musicCategory->GetFullName());
			} else {
				spdlog::warn("Skyrim's music sound category not found - \"Mute Skyrim's music\" won't do anything");
			}
		}

		void SetGameMusicMuted(bool a_mute)
		{
			if (a_mute == g_musicMuted || !g_musicCategory) {
				return;
			}
			g_musicMuted = a_mute;
			auto* cat = g_musicCategory;
			if (auto* t = SKSE::GetTaskInterface()) {
				t->AddTask([cat, a_mute]() {
					if (a_mute) {
						g_musicSaved = cat->GetCategoryVolume();
						cat->SetCategoryVolume(0.0f);
					} else {
						cat->SetCategoryVolume(g_musicSaved);
					}
				});
			}
		}

		// ================================================================ cursor mode
		void ApplyCursor(bool a_on)
		{
			if (g_cursor == a_on) {
				return;
			}
			g_cursor = a_on;
			g_wantCursor = a_on;
			using UEFlag = RE::ControlMap::UEFlag;
			// the player can still walk; looking, attacking, activating and menu keys belong to the widget
			constexpr auto blocked = static_cast<UEFlag>(
				static_cast<std::uint32_t>(UEFlag::kLooking) | static_cast<std::uint32_t>(UEFlag::kFighting) |
				static_cast<std::uint32_t>(UEFlag::kActivate) | static_cast<std::uint32_t>(UEFlag::kMenu) |
				static_cast<std::uint32_t>(UEFlag::kWheelZoom) | static_cast<std::uint32_t>(UEFlag::kPOVSwitch));
			// With the input blocker the game gets no presses at all. Without it (bBlockInput=0, or it couldn't be
			// installed) the game's own control switches are the fallback.
			if (!g_dispatchInput || !dbg.blockInput) {
				if (auto* cm = RE::ControlMap::GetSingleton()) {
					cm->ToggleControls(blocked, !a_on, false);
				}
				g_controlsBlocked = a_on;
			}
			if (g_imguiReady) {
				ImGui::GetIO().MouseDrawCursor = a_on;
			}
			if (!a_on) {
				g_settingsOpen = false;
				g_drag = {};
				g_captureSlot = -1;
				std::lock_guard lk(g_inputMtx);
				g_input.clear();
			}
			spdlog::info("Cursor mode {}", a_on ? "on" : "off");
		}

		// Input thread: may the cursor come up right now?
		bool CursorAllowed(bool a_tellPlayer)
		{
			if (!g_gameLoaded || g_pausedInput || OthersBusy()) {
				return false;
			}
			if (cfg.noCursorInCombat && g_inCombatInput) {
				if (a_tellPlayer) {
					Notify("Hearthsong: the widget can't be moved during combat");
				}
				return false;
			}
			return true;
		}

		// ================================================================ input (game input events)
		wchar_t KeyToChar(std::uint32_t a_dik, bool a_shift, bool a_altGr)
		{
			const UINT sc = (a_dik & 0x7F) | ((a_dik & 0x80) ? 0xE000u : 0u);
			HKL        layout = ::GetKeyboardLayout(::GetWindowThreadProcessId(::GetForegroundWindow(), nullptr));
			const UINT vk = ::MapVirtualKeyExW(sc, MAPVK_VSC_TO_VK_EX, layout);
			if (vk == 0) {
				return 0;
			}
			BYTE state[256]{};
			if (a_shift) {
				state[VK_SHIFT] = state[VK_LSHIFT] = 0x80;
			}
			if (::GetKeyState(VK_CAPITAL) & 1) {
				state[VK_CAPITAL] = 0x01;
			}
			if (a_altGr) {
				state[VK_CONTROL] = state[VK_LCONTROL] = state[VK_MENU] = state[VK_RMENU] = 0x80;
			}
			wchar_t   buf[8]{};
			const int n = ::ToUnicodeEx(vk, sc, state, buf, 8, 0x4, layout);
			return (n == 1 && buf[0] >= 0x20 && buf[0] != 0x7F) ? buf[0] : 0;
		}

		ImGuiKey KeyToImGui(std::uint32_t a_dik)
		{
			switch (a_dik) {
			case 0x01: return ImGuiKey_Escape;
			case 0xCB: return ImGuiKey_LeftArrow;
			case 0xCD: return ImGuiKey_RightArrow;
			case 0xC8: return ImGuiKey_UpArrow;
			case 0xD0: return ImGuiKey_DownArrow;
			case 0x0E: return ImGuiKey_Backspace;
			case 0xD3: return ImGuiKey_Delete;
			case 0x1C: return ImGuiKey_Enter;
			case 0x9C: return ImGuiKey_KeypadEnter;
			case 0xC7: return ImGuiKey_Home;
			case 0xCF: return ImGuiKey_End;
			case 0x0F: return ImGuiKey_Tab;
			case 0x1E: return ImGuiKey_A;
			case 0x2E: return ImGuiKey_C;
			case 0x2F: return ImGuiKey_V;
			case 0x2D: return ImGuiKey_X;
			case 0x2C: return ImGuiKey_Z;
			default: return ImGuiKey_None;
			}
		}

		bool TextBoxFocused()
		{
			return g_imguiReady && g_cursor && ImGui::GetCurrentContext() && ImGui::GetIO().WantTextInput;
		}

		void HandleInput(RE::InputEvent* const* a_event)
		{
			if (!a_event) {
				return;
			}
			for (auto* e = *a_event; e; e = e->next) {
				if (e->GetEventType() == RE::INPUT_EVENT_TYPE::kMouseMove) {
					if (g_cursor) {
						auto* mm = static_cast<RE::MouseMoveEvent*>(e);
						PushInput({ InputMsg::Kind::MouseMove, static_cast<float>(mm->mouseInputX), static_cast<float>(mm->mouseInputY) });
					}
					continue;
				}
				auto* btn = e->AsButtonEvent();
				if (!btn) {
					continue;
				}
				const auto code = btn->GetIDCode();
				const bool down = btn->IsDown();
				const bool up = btn->IsUp();
				if (!down && !up) {
					continue;  // held
				}
				const bool keyboard = e->GetDevice() == RE::INPUT_DEVICE::kKeyboard;
				if (keyboard) {
					if (code == 0x2A || code == 0x36) {
						g_shiftDown = down;
						if (g_cursor) {
							InputMsg m{ InputMsg::Kind::Key };
							m.key = ImGuiMod_Shift;
							m.down = down;
							PushInput(m);
						}
					} else if (code == 0x1D || code == 0x9D) {
						g_ctrlDown = down;
						if (g_cursor) {
							InputMsg m{ InputMsg::Kind::Key };
							m.key = ImGuiMod_Ctrl;
							m.down = down;
							PushInput(m);
						}
					} else if (code == 0x38 || code == 0xB8) {
						g_altDown = down;
					}
				}

				// Settings: "press a key" for one of the keys
				if (keyboard && down && g_captureSlot >= 0) {
					constexpr std::uint32_t unsafe[] = { 0x0F, 0x38, 0xB8, 0x1D, 0x9D, 0x2A, 0x36, 0xDB, 0xDC, 0x1C, 0x9C };
					if (code == 0x01) {
						g_capturedVk = -1;  // Esc: keep the old key
					} else if (code == 0x0E || code == 0xD3) {
						g_capturedVk = -2;  // Backspace / Delete: no key
					} else if (std::find(std::begin(unsafe), std::end(unsafe), code) == std::end(unsafe)) {
						const int vk = DIKToVirtualKey(code);
						g_capturedVk = vk > 0 ? vk : -1;
					} else {
						continue;
					}
					continue;
				}

				if (keyboard) {
					const bool typing = TextBoxFocused();
					// cursor mode key (works while hidden too: it brings the widget back)
					if (code == g_keyDik[kKeyCursor] && !typing && code != 0) {
						if (cfg.cursorHold) {
							if (down && !g_wantCursor && CursorAllowed(true)) {
								g_wantCursor = true;
							} else if (up) {
								g_wantCursor = false;
							}
						} else if (down) {
							if (g_wantCursor) {
								g_wantCursor = false;
							} else if (CursorAllowed(true)) {
								g_wantCursor = true;
							}
						}
						continue;
					}
					if (down && !typing && !ConsoleOpen()) {
						if (code == g_keyDik[kKeyToggle] && code != 0 && !g_pausedInput) {
							g_toggleVisible = true;
							continue;
						}
						if (code == g_keyDik[kKeyPlayPause] && code != 0) {
							Media::PlayPause();
							continue;
						}
						if (code == g_keyDik[kKeyNext] && code != 0) {
							Media::Next();
							continue;
						}
						if (code == g_keyDik[kKeyPrevious] && code != 0) {
							Media::Previous();
							continue;
						}
						if (code == g_keyDik[kKeyVolUp] && code != 0) {
							Media::ChangeVolume(cfg.volumeStep / 100.0f);
							continue;
						}
						if (code == g_keyDik[kKeyVolDown] && code != 0) {
							Media::ChangeVolume(-cfg.volumeStep / 100.0f);
							continue;
						}
					}
					if (!g_cursor) {
						continue;
					}
					if (down && code == 0x01 && !typing) {
						g_escRequest = true;  // Esc: close settings, then leave cursor mode
						continue;
					}
					if (down && g_ctrlDown && code == g_keyDik[kKeyUndo] && code != 0 && !typing) {
						g_undoRequest = true;
						continue;
					}
					if (const ImGuiKey key = KeyToImGui(code); key != ImGuiKey_None) {
						InputMsg m{ InputMsg::Kind::Key };
						m.key = key;
						m.down = down;
						PushInput(m);
					}
					const bool altGr = g_ctrlDown && g_altDown;
					if (down && (!g_ctrlDown || altGr)) {
						if (const wchar_t c = KeyToChar(code, g_shiftDown, altGr); c != 0) {
							InputMsg m{ InputMsg::Kind::Char };
							m.button = static_cast<int>(c);
							PushInput(m);
						}
					}
				} else if (e->GetDevice() == RE::INPUT_DEVICE::kMouse && g_cursor) {
					using MouseKey = RE::BSWin32MouseDevice::Key;
					if (code == MouseKey::kWheelUp || code == MouseKey::kWheelDown) {
						if (down) {
							InputMsg m{ InputMsg::Kind::Wheel };
							m.y = code == MouseKey::kWheelUp ? 1.0f : -1.0f;
							PushInput(m);
						}
					} else if (code <= 4) {
						InputMsg m{ InputMsg::Kind::MouseButton };
						m.button = static_cast<int>(code);
						m.down = down;
						PushInput(m);
					}
				}
			}
		}

		void SafeHandleInput(RE::InputEvent* const* a_event)
		{
			__try {
				HandleInput(a_event);
			} __except (SehFilter(GetExceptionInformation(), "HandleInput")) {
				g_inputDisabled = true;
			}
		}

		// ---- input blocker (the same fix as Hearthframe v0.12.1 - v0.12.6)
		// In cursor mode the game and other mods (OAR, MeridianUI, minimaps, Hearthsworn...) must not see the keys
		// and clicks meant for the widget. The game hands each frame's input to every listener in one call; we sit
		// in front of that call, handle the input ourselves and pass on only the key/button releases.
		using DispatchInput = void(RE::BSTEventSource<RE::InputEvent*>*, RE::InputEvent* const*);
		bool           g_inBlockerPass = false;  // the list going on was already handled here: the sink skips it

		void DispatchInputHook(RE::BSTEventSource<RE::InputEvent*>* a_source, RE::InputEvent* const* a_events)
		{
			const auto original = reinterpret_cast<DispatchInput*>(g_dispatchInput);
			if (g_cursor && dbg.blockInput && a_events && *a_events && !g_inputDisabled) {
				SafeHandleInput(a_events);
				// Releases still go on, so nothing (movement, another mod's hold key, Shift...) stays held after the
				// cursor goes away; presses - what hotkeys react to - stay with us. The list must be writable memory:
				// mods further down the chain (SKSE Menu Framework...) write their filtered list back into it.
				static RE::InputEvent* kept[1];
				kept[0] = nullptr;
				RE::InputEvent*              last = nullptr;
				std::vector<RE::InputEvent*> releases;
				for (auto* e = *a_events; e; e = e->next) {
					if (auto* b = e->AsButtonEvent(); b && b->IsUp()) {
						releases.push_back(e);
					}
				}
				for (auto* e : releases) {
					if (last) {
						last->next = e;
					} else {
						kept[0] = e;
					}
					last = e;
				}
				if (last) {
					last->next = nullptr;
				}
				g_inBlockerPass = true;
				original(a_source, kept);
				g_inBlockerPass = false;
				return;
			}
			original(a_source, a_events);
		}

		class InputSink final : public RE::BSTEventSink<RE::InputEvent*>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) override
			{
				if (!g_inputDisabled && !g_inBlockerPass) {
					SafeHandleInput(a_event);
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		// Which module a call target ends up in (follows other mods' trampoline jumps), for the log.
		std::uintptr_t FollowJumps(std::uintptr_t a_addr)  // no C++ objects here: __try needs that
		{
			__try {
				for (int hop = 0; hop < 6; ++hop) {
					const auto* b = reinterpret_cast<const std::uint8_t*>(a_addr);
					if (b[0] == 0xFF && b[1] == 0x25) {  // jmp [rip+disp32]
						const auto disp = *reinterpret_cast<const std::int32_t*>(b + 2);
						a_addr = *reinterpret_cast<const std::uintptr_t*>(a_addr + 6 + disp);
					} else if (b[0] == 0xE9) {  // jmp rel32
						a_addr = a_addr + 5 + *reinterpret_cast<const std::int32_t*>(b + 1);
					} else {
						break;
					}
				}
			} __except (EXCEPTION_EXECUTE_HANDLER) {
			}
			return a_addr;
		}

		std::string ModuleOf(std::uintptr_t a_addr)
		{
			a_addr = FollowJumps(a_addr);
			HMODULE mod = nullptr;
			wchar_t path[MAX_PATH]{};
			if (::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(a_addr), &mod) &&
				::GetModuleFileNameW(mod, path, MAX_PATH)) {
				return ToUtf8(fs::path(path).filename().wstring());
			}
			return fmt::format("unknown ({:X})", a_addr);
		}

		// Ctrl + wheel over the widget: its scale
		bool g_wheelScale = false;

		void DrainInput()
		{
			std::vector<InputMsg> msgs;
			{
				std::lock_guard lk(g_inputMtx);
				msgs.swap(g_input);
			}
			auto& io = ImGui::GetIO();
			for (const auto& m : msgs) {
				switch (m.kind) {
				case InputMsg::Kind::MouseMove:
					g_mouseX = std::clamp(g_mouseX + m.x * cfg.mouseSpeed, 0.0f, io.DisplaySize.x - 1.0f);
					g_mouseY = std::clamp(g_mouseY + m.y * cfg.mouseSpeed, 0.0f, io.DisplaySize.y - 1.0f);
					io.AddMousePosEvent(g_mouseX, g_mouseY);
					break;
				case InputMsg::Kind::MouseButton:
					io.AddMouseButtonEvent(m.button, m.down);
					break;
				case InputMsg::Kind::Wheel:
					if (g_ctrlDown && g_wheelScale) {
						if (!cfg.locked) {
							cfg.scalePct = std::clamp(cfg.scalePct + static_cast<int>(m.y) * 5, 60, 200);
							MarkDirty();
							g_pendingFonts = true;
						}
					} else {
						io.AddMouseWheelEvent(0.0f, m.y);
					}
					break;
				case InputMsg::Kind::Key:
					io.AddKeyEvent(m.key, m.down);
					break;
				case InputMsg::Kind::Char:
					io.AddInputCharacterUTF16(static_cast<ImWchar16>(m.button));
					break;
				}
			}
		}

		// ================================================================ drawing helpers
		float g_k = 1.0f;  // widget pixels per 1080p unit (screen size x widget scale)
		float U(float a_v) { return a_v * g_k; }

		ImU32 Col(int r, int g, int b, int a = 255)
		{
			return IM_COL32(r, g, b, static_cast<int>(a * std::clamp(g_alpha, 0.0f, 1.0f)));
		}
		ImU32 kText() { return Col(238, 234, 226); }
		ImU32 kDim() { return Col(168, 164, 156); }
		ImU32 kFaint() { return Col(138, 134, 127); }
		ImU32 kGold() { return Col(218, 184, 124); }
		ImU32 kGoldDim() { return Col(150, 126, 88); }

		float Ease(ImGuiID a_id, bool a_on, float a_speed = 14.0f)
		{
			float*      v = ImGui::GetStateStorage()->GetFloatRef(a_id, 0.0f);
			const float k = 1.0f - std::exp(-a_speed * ImGui::GetIO().DeltaTime);
			*v += ((a_on ? 1.0f : 0.0f) - *v) * k;
			return *v;
		}

		ImFont* FText() { return g_fText ? g_fText : Theme::Body(); }
		ImFont* FSemi() { return g_fSemi ? g_fSemi : Theme::Label(); }
		ImFont* FTitle() { return g_fTitle ? g_fTitle : Theme::Bold(); }

		float TextW(ImFont* a_f, float a_size, const char* a_t)
		{
			return a_f->CalcTextSizeA(a_size, FLT_MAX, 0.0f, a_t).x;
		}

		// One line of text, shortened with "..." to fit a_maxW. a_align: 0 left, 0.5 centred.
		void Line(ImDrawList* a_dl, ImFont* a_f, float a_size, ImVec2 a_p, ImU32 a_col, const std::string& a_t, float a_maxW, float a_align = 0.0f)
		{
			std::string t = a_t;
			if (a_maxW > 0.0f && TextW(a_f, a_size, t.c_str()) > a_maxW) {
				const float dots = TextW(a_f, a_size, "...");
				while (!t.empty() && TextW(a_f, a_size, t.c_str()) + dots > a_maxW) {
					while (!t.empty()) {  // drop one whole UTF-8 character
						const auto c = static_cast<unsigned char>(t.back());
						t.pop_back();
						if ((c & 0xC0) != 0x80) {
							break;
						}
					}
				}
				while (!t.empty() && t.back() == ' ') {
					t.pop_back();
				}
				t += "...";
			}
			if (a_align > 0.0f) {
				a_p.x += (a_maxW - TextW(a_f, a_size, t.c_str())) * a_align;
			}
			a_dl->AddText(a_f, a_size, a_p, a_col, t.c_str());
		}

		void SoftShadow(ImDrawList* a_dl, ImVec2 a_a, ImVec2 a_b, float a_r, float a_spread, int a_alpha)
		{
			const int steps = 10;
			for (int i = steps; i >= 1; --i) {
				const float t = static_cast<float>(i) / steps;
				const float g = a_spread * t;
				const int   al = static_cast<int>(a_alpha * (1.0f - t) * (1.0f - t) * 0.35f);
				a_dl->AddRectFilled(a_a - ImVec2(g * 0.6f, g * 0.3f), a_b + ImVec2(g * 0.6f, g), Col(0, 0, 0, al), a_r + g);
			}
		}

		// Smoked-glass panel of Hearthframe's Modern look
		void Panel(ImDrawList* a_dl, ImVec2 a_a, ImVec2 a_b, bool a_cursor)
		{
			const float r = U(12.0f);
			const int   op = std::clamp(cfg.opacityPct, 30, 100);
			SoftShadow(a_dl, a_a, a_b, r, U(30.0f), static_cast<int>(160 * op / 100.0f));
			a_dl->AddRectFilled(a_a, a_b, Col(13, 13, 15, static_cast<int>(255 * op / 100.0f)), r);
			a_dl->AddRectFilledMultiColor(ImVec2(a_a.x + r, a_a.y + 1), ImVec2(a_b.x - r, (std::min)(a_b.y - r, a_a.y + U(120.0f))), Col(255, 255, 255, 10),
				Col(255, 255, 255, 10), Col(255, 255, 255, 0), Col(255, 255, 255, 0));
			a_dl->AddRect(a_a, a_b, a_cursor ? Col(218, 184, 124, 120) : Col(255, 255, 255, 26), r, 0, U(1.0f));
			// warm accent line along the top edge, brightest in the middle
			const float mid = (a_a.x + a_b.x) * 0.5f, half = (a_b.x - a_a.x) * 0.32f;
			const ImU32 acc = Col(218, 184, 124, 200), clear = Col(218, 184, 124, 0);
			a_dl->AddRectFilledMultiColor(ImVec2(mid - half, a_a.y), ImVec2(mid, a_a.y + U(1.5f)), clear, acc, acc, clear);
			a_dl->AddRectFilledMultiColor(ImVec2(mid, a_a.y), ImVec2(mid + half, a_a.y + U(1.5f)), acc, clear, clear, acc);
		}

		void HairlineFade(ImDrawList* a_dl, float a_x0, float a_x1, float a_y)
		{
			const float mid = (a_x0 + a_x1) * 0.5f;
			a_dl->AddRectFilledMultiColor(ImVec2(a_x0, a_y), ImVec2(mid, a_y + 1.0f), Col(255, 255, 255, 30), Col(255, 255, 255, 18), Col(255, 255, 255, 18), Col(255, 255, 255, 30));
			a_dl->AddRectFilledMultiColor(ImVec2(mid, a_y), ImVec2(a_x1, a_y + 1.0f), Col(255, 255, 255, 18), Col(255, 255, 255, 4), Col(255, 255, 255, 4), Col(255, 255, 255, 18));
		}

		// ---- icons (centre, size of the icon box, colour)
		using IconFn = void (*)(ImDrawList*, ImVec2, float, ImU32);

		void IconNone(ImDrawList*, ImVec2, float, ImU32) {}
		void IconPlay(ImDrawList* dl, ImVec2 c, float s, ImU32 col)
		{
			dl->AddTriangleFilled(c + ImVec2(-s * 0.32f, -s * 0.42f), c + ImVec2(s * 0.45f, 0), c + ImVec2(-s * 0.32f, s * 0.42f), col);
		}
		void IconPause(ImDrawList* dl, ImVec2 c, float s, ImU32 col)
		{
			dl->AddRectFilled(c + ImVec2(-s * 0.36f, -s * 0.4f), c + ImVec2(-s * 0.1f, s * 0.4f), col, s * 0.06f);
			dl->AddRectFilled(c + ImVec2(s * 0.1f, -s * 0.4f), c + ImVec2(s * 0.36f, s * 0.4f), col, s * 0.06f);
		}
		void IconPrev(ImDrawList* dl, ImVec2 c, float s, ImU32 col)
		{
			const float t = (std::max)(1.0f, s * 0.1f);
			dl->AddLine(c + ImVec2(-s * 0.36f, -s * 0.36f), c + ImVec2(-s * 0.36f, s * 0.36f), col, t);
			dl->AddTriangle(c + ImVec2(s * 0.38f, -s * 0.36f), c + ImVec2(-s * 0.2f, 0), c + ImVec2(s * 0.38f, s * 0.36f), col, t);
		}
		void IconNext(ImDrawList* dl, ImVec2 c, float s, ImU32 col)
		{
			const float t = (std::max)(1.0f, s * 0.1f);
			dl->AddLine(c + ImVec2(s * 0.36f, -s * 0.36f), c + ImVec2(s * 0.36f, s * 0.36f), col, t);
			dl->AddTriangle(c + ImVec2(-s * 0.38f, -s * 0.36f), c + ImVec2(s * 0.2f, 0), c + ImVec2(-s * 0.38f, s * 0.36f), col, t);
		}
		void IconShuffle(ImDrawList* dl, ImVec2 c, float s, ImU32 col)
		{
			const float t = (std::max)(1.0f, s * 0.09f), h = s * 0.5f;
			dl->AddLine(c + ImVec2(-h, -h * 0.5f), c + ImVec2(-h * 0.45f, -h * 0.5f), col, t);
			dl->AddLine(c + ImVec2(-h * 0.45f, -h * 0.5f), c + ImVec2(h * 0.45f, h * 0.5f), col, t);
			dl->AddLine(c + ImVec2(h * 0.45f, h * 0.5f), c + ImVec2(h, h * 0.5f), col, t);
			dl->AddLine(c + ImVec2(-h, h * 0.5f), c + ImVec2(-h * 0.45f, h * 0.5f), col, t);
			dl->AddLine(c + ImVec2(-h * 0.45f, h * 0.5f), c + ImVec2(h * 0.45f, -h * 0.5f), col, t);
			dl->AddLine(c + ImVec2(h * 0.45f, -h * 0.5f), c + ImVec2(h, -h * 0.5f), col, t);
			dl->AddLine(c + ImVec2(h * 0.72f, -h * 0.78f), c + ImVec2(h, -h * 0.5f), col, t);
			dl->AddLine(c + ImVec2(h * 0.72f, -h * 0.22f), c + ImVec2(h, -h * 0.5f), col, t);
			dl->AddLine(c + ImVec2(h * 0.72f, h * 0.22f), c + ImVec2(h, h * 0.5f), col, t);
			dl->AddLine(c + ImVec2(h * 0.72f, h * 0.78f), c + ImVec2(h, h * 0.5f), col, t);
		}
		void IconRepeat(ImDrawList* dl, ImVec2 c, float s, ImU32 col)
		{
			const float t = (std::max)(1.0f, s * 0.09f), h = s * 0.46f;
			dl->AddRect(c + ImVec2(-h, -h * 0.55f), c + ImVec2(h, h * 0.55f), col, h * 0.35f, 0, t);
			dl->AddTriangleFilled(c + ImVec2(h * 0.05f, -h * 0.55f - h * 0.32f), c + ImVec2(h * 0.45f, -h * 0.55f), c + ImVec2(h * 0.05f, -h * 0.55f + h * 0.32f), col);
			dl->AddTriangleFilled(c + ImVec2(-h * 0.05f, h * 0.55f - h * 0.32f), c + ImVec2(-h * 0.45f, h * 0.55f), c + ImVec2(-h * 0.05f, h * 0.55f + h * 0.32f), col);
		}
		void IconGear(ImDrawList* dl, ImVec2 c, float s, ImU32 col)
		{
			const float t = (std::max)(1.0f, s * 0.09f);
			dl->AddCircle(c, s * 0.16f, col, 16, t);
			for (int i = 0; i < 8; ++i) {
				const float a = i * 3.14159265f / 4.0f;
				const ImVec2 d(std::cos(a), std::sin(a));
				dl->AddLine(c + d * (s * 0.27f), c + d * (s * 0.44f), col, t * 1.4f);
			}
			dl->AddCircle(c, s * 0.3f, col, 20, t);
		}
		void IconClose(ImDrawList* dl, ImVec2 c, float s, ImU32 col)
		{
			const float k = s * 0.22f, t = (std::max)(1.0f, s * 0.08f);
			dl->AddLine(c - ImVec2(k, k), c + ImVec2(k, k), col, t);
			dl->AddLine(c + ImVec2(-k, k), c + ImVec2(k, -k), col, t);
		}
		void IconSpeaker(ImDrawList* dl, ImVec2 c, float s, ImU32 col)
		{
			const float t = (std::max)(1.0f, s * 0.08f), h = s * 0.5f;
			dl->AddQuadFilled(c + ImVec2(-h * 0.9f, -h * 0.3f), c + ImVec2(-h * 0.4f, -h * 0.3f), c + ImVec2(-h * 0.4f, h * 0.3f), c + ImVec2(-h * 0.9f, h * 0.3f), col);
			dl->AddTriangleFilled(c + ImVec2(-h * 0.45f, -h * 0.3f), c + ImVec2(h * 0.1f, -h * 0.8f), c + ImVec2(h * 0.1f, h * 0.8f), col);
			dl->AddTriangleFilled(c + ImVec2(-h * 0.45f, -h * 0.3f), c + ImVec2(h * 0.1f, h * 0.8f), c + ImVec2(-h * 0.45f, h * 0.3f), col);
			dl->PathArcTo(c + ImVec2(h * 0.1f, 0), h * 0.55f, -0.9f, 0.9f, 10);
			dl->PathStroke(col, 0, t);
		}
		void IconNote(ImDrawList* dl, ImVec2 c, float s, ImU32 col)
		{
			const float t = (std::max)(1.0f, s * 0.06f), h = s * 0.5f;
			dl->AddLine(c + ImVec2(-h * 0.25f, h * 0.55f), c + ImVec2(-h * 0.25f, -h * 0.65f), col, t);
			dl->AddLine(c + ImVec2(-h * 0.25f, -h * 0.65f), c + ImVec2(h * 0.7f, -h * 0.85f), col, t);
			dl->AddLine(c + ImVec2(h * 0.7f, -h * 0.85f), c + ImVec2(h * 0.7f, h * 0.35f), col, t);
			dl->AddCircle(c + ImVec2(-h * 0.5f, h * 0.55f), h * 0.25f, col, 16, t);
			dl->AddCircle(c + ImVec2(h * 0.45f, h * 0.35f), h * 0.25f, col, 16, t);
		}
		void IconLock(ImDrawList* dl, ImVec2 c, float s, ImU32 col)
		{
			const float t = (std::max)(1.0f, s * 0.08f), h = s * 0.5f;
			dl->AddRect(c + ImVec2(-h * 0.6f, -h * 0.1f), c + ImVec2(h * 0.6f, h * 0.7f), col, h * 0.15f, 0, t);
			dl->PathArcTo(c + ImVec2(0, -h * 0.1f), h * 0.38f, 3.14159265f, 2.0f * 3.14159265f, 12);
			dl->PathStroke(col, 0, t);
		}

		enum class BtnStyle
		{
			Ghost,  // icon only, gold on hover
			Flat,   // hairline outline
			Gold,   // the main button
			Round   // round close button
		};

		// A widget button. Returns true when clicked (only possible in cursor mode).
		bool Button(const char* a_id, ImVec2 a_p, ImVec2 a_size, BtnStyle a_style, IconFn a_icon, float a_iconSize, bool a_enabled = true, bool a_on = false,
			const char* a_tip = nullptr)
		{
			ImGui::SetCursorScreenPos(a_p);
			const bool pressed = ImGui::InvisibleButton(a_id, a_size);
			const bool hov = ImGui::IsItemHovered() && a_enabled;
			const bool held = ImGui::IsItemActive() && a_enabled;
			const float h = Ease(ImGui::GetItemID(), hov);
			auto*       dl = ImGui::GetWindowDrawList();
			const ImVec2 a = a_p, b = a_p + a_size, c = (a + b) * 0.5f;
			const float  r = U(6.0f);
			ImU32        icon = a_enabled ? kText() : Col(255, 255, 255, 60);
			switch (a_style) {
			case BtnStyle::Ghost:
				if (h > 0.01f) {
					dl->AddRectFilled(a, b, Col(255, 255, 255, static_cast<int>(16 * h)), r);
				}
				icon = !a_enabled ? Col(255, 255, 255, 50) : (a_on || hov ? kGold() : kDim());
				break;
			case BtnStyle::Flat:
				dl->AddRectFilled(a, b, Col(255, 255, 255, held ? 28 : static_cast<int>(8 + 10 * h)), r);
				dl->AddRect(a, b, hov ? kGold() : Col(255, 255, 255, 44), r, 0, U(1.0f));
				icon = a_enabled ? Col(232, 226, 216) : Col(255, 255, 255, 60);
				break;
			case BtnStyle::Gold:
				{
					const ImU32 top = held ? Col(194, 160, 104) : Col(226, 196, 142), bot = Col(0, 0, 0, 40);
					if (h > 0.01f) {
						dl->AddRect(a - ImVec2(U(2), U(2)), b + ImVec2(U(2), U(2)), Col(218, 184, 124, static_cast<int>(70 * h)), r + U(2), 0, U(3.0f));
					}
					dl->AddRectFilled(a, b, top, r);
					dl->AddRectFilledMultiColor(ImVec2(a.x + r, (a.y + b.y) * 0.5f), ImVec2(b.x - r, b.y - 1), Col(0, 0, 0, 0), Col(0, 0, 0, 0), bot, bot);
					dl->AddRect(a, b, Col(236, 210, 162), r, 0, U(1.0f));
					icon = a_enabled ? Col(26, 21, 14) : Col(26, 21, 14, 120);
					break;
				}
			case BtnStyle::Round:
				{
					const float rr = (b.x - a.x) * 0.5f;
					dl->AddCircleFilled(c, rr, hov ? Col(255, 255, 255, 26) : Col(255, 255, 255, 8), 32);
					dl->AddCircle(c, rr, hov ? kGold() : Col(255, 255, 255, 50), 32, U(1.2f));
					icon = hov ? kGold() : kText();
					break;
				}
			}
			a_icon(dl, c, a_iconSize, icon);
			if (a_tip && ImGui::IsItemHovered() && g_drag.mode == Grab::None) {
				ImGui::SetTooltip("%s", a_tip);
			}
			return pressed && a_enabled;
		}

		// Switch like Hearthframe's modern toggle
		bool Switch(const char* a_id, ImVec2 a_p, bool* a_v)
		{
			const ImVec2 size(U(40.0f), U(22.0f));
			ImGui::SetCursorScreenPos(a_p);
			const bool pressed = ImGui::InvisibleButton(a_id, size);
			if (pressed) {
				*a_v = !*a_v;
			}
			const bool  hov = ImGui::IsItemHovered();
			const float t = Ease(ImGui::GetItemID(), *a_v, 14.0f);
			auto*       dl = ImGui::GetWindowDrawList();
			const ImVec2 a = a_p, b = a_p + size;
			const float  r = size.y * 0.5f;
			const ImVec4 off = ImGui::ColorConvertU32ToFloat4(Col(255, 255, 255, 28)), on = ImGui::ColorConvertU32ToFloat4(kGold());
			const ImVec4 mix(off.x + (on.x - off.x) * t, off.y + (on.y - off.y) * t, off.z + (on.z - off.z) * t, off.w + (on.w - off.w) * t);
			dl->AddRectFilled(a, b, ImGui::ColorConvertFloat4ToU32(mix), r);
			dl->AddRect(a, b, hov ? kGold() : Col(255, 255, 255, static_cast<int>(40 * (1 - t))), r, 0, U(1.0f));
			const ImVec2 kn(a.x + r + (size.x - 2 * r) * t, (a.y + b.y) * 0.5f);
			dl->AddCircleFilled(kn + ImVec2(0, U(1.5f)), r - U(3.0f), Col(0, 0, 0, 80), 24);
			dl->AddCircleFilled(kn, r - U(3.5f), Col(250, 248, 244), 24);
			return pressed;
		}

		std::string Clock(double a_s)
		{
			const int t = static_cast<int>((std::max)(0.0, a_s));
			return t >= 3600 ? fmt::format("{}:{:02}:{:02}", t / 3600, (t / 60) % 60, t % 60) : fmt::format("{}:{:02}", t / 60, t % 60);
		}

		// Hairline track with glowing gold fill. Seeks when clicked (if the app allows it).
		void Progress(const Media::Snapshot& a_m, float a_x0, float a_x1, float a_y, bool a_times)
		{
			auto*        dl = ImGui::GetWindowDrawList();
			const double pos = Media::CurrentPosition(a_m);
			const float  frac = a_m.duration > 0.0 ? static_cast<float>(pos / a_m.duration) : 0.0f;
			const float  ts = U(11.0f);
			float        bx0 = a_x0, bx1 = a_x1;
			if (a_times) {
				const std::string l = Clock(pos), r = Clock(a_m.duration);
				dl->AddText(FText(), ts, ImVec2(a_x0, a_y - ts * 0.55f), kText(), l.c_str());
				const float rw = TextW(FText(), ts, r.c_str());
				dl->AddText(FText(), ts, ImVec2(a_x1 - rw, a_y - ts * 0.55f), kDim(), r.c_str());
				bx0 = a_x0 + (std::max)(TextW(FText(), ts, "00:00"), TextW(FText(), ts, l.c_str())) + U(10.0f);
				bx1 = a_x1 - (std::max)(TextW(FText(), ts, "00:00"), rw) - U(10.0f);
			}
			ImGui::SetCursorScreenPos(ImVec2(bx0, a_y - U(7.0f)));
			const bool  clicked = ImGui::InvisibleButton("##seek", ImVec2((std::max)(1.0f, bx1 - bx0), U(14.0f)));
			const bool  hov = ImGui::IsItemHovered() && a_m.canSeek;
			const float h = Ease(ImGui::GetItemID(), hov);
			if (clicked && a_m.canSeek && a_m.duration > 0.0) {
				const float f = std::clamp((ImGui::GetIO().MousePos.x - bx0) / (bx1 - bx0), 0.0f, 1.0f);
				Media::Seek(f * a_m.duration);
			}
			const float gx = bx0 + (bx1 - bx0) * std::clamp(frac, 0.0f, 1.0f);
			dl->AddRectFilled(ImVec2(bx0, a_y - U(1.0f)), ImVec2(bx1, a_y + U(1.0f)), Col(255, 255, 255, 34), U(1.0f));
			dl->AddRectFilledMultiColor(ImVec2(bx0, a_y - U(4.0f)), ImVec2(gx, a_y), Col(218, 184, 124, 0), Col(218, 184, 124, static_cast<int>(22 + 30 * h)),
				Col(218, 184, 124, static_cast<int>(22 + 30 * h)), Col(218, 184, 124, 0));
			dl->AddRectFilledMultiColor(ImVec2(bx0, a_y - U(1.0f)), ImVec2(gx, a_y + U(1.0f)), kGoldDim(), kGold(), kGold(), kGoldDim());
			const float hw = U(2.0f + 1.0f * h), hh = U(5.0f + 2.0f * h);
			dl->AddRectFilled(ImVec2(gx - hw, a_y - hh), ImVec2(gx + hw, a_y + hh), kGold(), hw);
			if (hov && g_drag.mode == Grab::None) {
				ImGui::SetTooltip("Click to jump there");
			}
		}

		// Volume slider for the music app's entry in the Windows volume mixer
		void Volume(const Media::Snapshot& a_m, float a_x0, float a_x1, float a_y, bool a_number)
		{
			auto*       dl = ImGui::GetWindowDrawList();
			const bool  known = a_m.volume >= 0.0f;
			const float ts = U(12.0f);
			float       bx1 = a_x1;
			if (a_number) {
				bx1 = a_x1 - U(44.0f);
			}
			ImGui::SetCursorScreenPos(ImVec2(a_x0, a_y - U(9.0f)));
			ImGui::InvisibleButton("##vol", ImVec2((std::max)(1.0f, bx1 - a_x0), U(18.0f)));
			const bool  active = ImGui::IsItemActive() && known;
			const bool  hov = (ImGui::IsItemHovered() || active) && known;
			const float h = Ease(ImGui::GetItemID(), hov);
			if (active) {
				const float f = std::clamp((ImGui::GetIO().MousePos.x - a_x0) / (bx1 - a_x0), 0.0f, 1.0f);
				if (std::fabs(f - g_volDrag) > 0.004f) {
					g_volDrag = f;
					Media::SetVolume(f);
				}
				g_volDragUntil = g_time + 0.8f;
			}
			if (ImGui::IsItemHovered() && known && std::fabs(ImGui::GetIO().MouseWheel) > 0.0f) {
				const float base = g_volDrag >= 0.0f ? g_volDrag : a_m.volume;
				g_volDrag = std::clamp(base + ImGui::GetIO().MouseWheel * cfg.volumeStep / 100.0f, 0.0f, 1.0f);
				Media::SetVolume(g_volDrag);
				g_volDragUntil = g_time + 0.8f;
			}
			if (g_time > g_volDragUntil) {
				g_volDrag = -1.0f;
			}
			const float v = g_volDrag >= 0.0f ? g_volDrag : (known ? (a_m.muted ? 0.0f : a_m.volume) : 0.0f);
			const float gx = a_x0 + (bx1 - a_x0) * v;
			dl->AddRectFilled(ImVec2(a_x0, a_y - U(1.0f)), ImVec2(bx1, a_y + U(1.0f)), Col(255, 255, 255, 34), U(1.0f));
			if (known) {
				dl->AddRectFilledMultiColor(ImVec2(a_x0, a_y - U(1.0f)), ImVec2(gx, a_y + U(1.0f)), kGoldDim(), kGold(), kGold(), kGoldDim());
				const float hw = U(2.0f + 1.0f * h), hh = U(8.0f + 2.0f * h);
				dl->AddRectFilled(ImVec2(gx - hw, a_y - hh), ImVec2(gx + hw, a_y + hh), kGold(), hw);
			}
			if (a_number) {
				const std::string n = known ? fmt::format("{}", static_cast<int>(std::lround(v * 100.0f))) : "--";
				const float       nw = TextW(FText(), ts, n.c_str()), pw = TextW(FText(), ts, " %");
				dl->AddText(FText(), ts, ImVec2(a_x1 - nw - pw, a_y - ts * 0.55f), kText(), n.c_str());
				dl->AddText(FText(), ts, ImVec2(a_x1 - pw, a_y - ts * 0.55f), kFaint(), " %");
			}
			if (ImGui::IsItemHovered() && !known && g_drag.mode == Grab::None) {
				ImGui::SetTooltip("The music app isn't in the Windows volume mixer yet (start playing something)");
			}
		}

		void ArtBox(ImDrawList* a_dl, ImVec2 a_a, float a_size, float a_r, bool a_label)
		{
			const ImVec2 b = a_a + ImVec2(a_size, a_size);
			SoftShadow(a_dl, a_a, b, a_r, U(10.0f), 140);
			if (g_art) {
				a_dl->AddImageRounded(reinterpret_cast<ImTextureID>(g_art), a_a, b, ImVec2(0, 0), ImVec2(1, 1), Col(255, 255, 255), a_r);
				a_dl->AddRect(a_a, b, Col(255, 255, 255, 26), a_r, 0, U(1.0f));
				return;
			}
			a_dl->AddRectFilled(a_a, b, Col(10, 10, 12), a_r);
			a_dl->AddRect(a_a, b, Col(255, 255, 255, 26), a_r, 0, U(1.0f));
			const ImVec2 c = a_a + ImVec2(a_size, a_size) * 0.5f - ImVec2(0, a_label ? U(6.0f) : 0.0f);
			IconNote(a_dl, c, (std::min)(a_size * 0.4f, U(36.0f)), kFaint());
			(void)a_label;
		}

		// ================================================================ the widget
		enum class Layout
		{
			Standard,
			Wide,
			Tall
		};
		Layout LayoutFor(float a_w, float a_h)
		{
			if (a_w / a_h >= 2.2f) {
				return Layout::Wide;
			}
			if (a_h / a_w >= 1.3f) {
				return Layout::Tall;
			}
			return Layout::Standard;
		}
		const char* LayoutName(Layout a_l)
		{
			return a_l == Layout::Wide ? "Horizontal bar" : (a_l == Layout::Tall ? "Portrait" : "Standard");
		}

		struct Zones
		{
			ImVec2 gripA{}, gripB{};  // where the widget can be picked up (title bar or grip strip)
		} g_zones;

		void GripDots(ImDrawList* a_dl, ImVec2 a_c, bool a_vertical)
		{
			for (int i = -1; i <= 1; ++i) {
				for (int j = 0; j < 2; ++j) {
					const ImVec2 o = a_vertical ? ImVec2((j - 0.5f) * U(5.0f), i * U(5.0f)) : ImVec2(i * U(5.0f), (j - 0.5f) * U(5.0f));
					a_dl->AddCircleFilled(a_c + o, U(1.3f), kFaint(), 8);
				}
			}
		}

		void SideButtons(float a_right, float a_y, float a_size, bool a_round)
		{
			// close + settings, right-aligned at a_right
			const float gap = U(6.0f);
			if (a_round) {
				if (Button("##close", ImVec2(a_right - a_size, a_y), ImVec2(a_size, a_size), BtnStyle::Round, IconClose, a_size, true, false, "Hide the widget")) {
					cfg.visible = false;
					g_wantCursor = false;
					MarkDirty();
				}
			} else if (Button("##close", ImVec2(a_right - a_size, a_y), ImVec2(a_size, a_size), BtnStyle::Ghost, IconClose, a_size * 0.9f, true, false, "Hide the widget")) {
				cfg.visible = false;
				g_wantCursor = false;
				MarkDirty();
			}
			const float gs = a_round ? a_size - U(2.0f) : a_size;
			if (Button("##gear", ImVec2(a_right - a_size - gap - gs, a_y + (a_size - gs) * 0.5f), ImVec2(gs, gs), BtnStyle::Ghost, IconGear, gs * 0.62f, true, g_settingsOpen,
					"Widget settings")) {
				g_settingsOpen = !g_settingsOpen;
			}
			if (cfg.locked) {
				IconLock(ImGui::GetWindowDrawList(), ImVec2(a_right - a_size - gap - gs - U(14.0f), a_y + a_size * 0.5f), U(14.0f), kGold());
			}
		}

		void Transport(const Media::Snapshot& a_m, float a_cx, float a_y, float a_avail, float a_unit)
		{
			// a_unit 1 = full size row (stacked layouts)
			const float sh = U(34.0f * a_unit), nb = U(42.0f * a_unit), pb = U(54.0f * a_unit), ph = U(46.0f * a_unit), gap = U(10.0f * a_unit);
			const bool  extras = a_avail >= sh * 2 + nb * 2 + pb + gap * 4;
			const float total = (extras ? sh * 2 + gap * 2 : 0.0f) + nb * 2 + pb + gap * 2;
			float       x = a_cx - total * 0.5f;
			const float cy = a_y + ph * 0.5f;
			if (extras) {
				if (Button("##shuffle", ImVec2(x, cy - sh * 0.5f), ImVec2(sh, sh), BtnStyle::Ghost, IconShuffle, sh * 0.5f, a_m.shuffleKnown, a_m.shuffle,
						a_m.shuffle ? "Shuffle: on" : "Shuffle: off")) {
					Media::ToggleShuffle();
				}
				x += sh + gap;
			}
			if (Button("##prev", ImVec2(x, cy - nb * 0.5f), ImVec2(nb, nb), BtnStyle::Flat, IconPrev, nb * 0.38f, a_m.canPrev, false, "Previous track")) {
				Media::Previous();
			}
			x += nb + gap;
			if (Button("##play", ImVec2(x, cy - ph * 0.5f), ImVec2(pb, ph), BtnStyle::Gold, a_m.playing ? IconPause : IconPlay, ph * 0.36f, a_m.canPlayPause, false,
					a_m.playing ? "Pause" : "Play")) {
				Media::PlayPause();
			}
			x += pb + gap;
			if (Button("##next", ImVec2(x, cy - nb * 0.5f), ImVec2(nb, nb), BtnStyle::Flat, IconNext, nb * 0.38f, a_m.canNext, false, "Next track")) {
				Media::Next();
			}
			x += nb + gap;
			if (extras) {
				const char* tip = a_m.repeat == 1 ? "Repeat: this track" : (a_m.repeat == 2 ? "Repeat: on" : "Repeat: off");
				if (Button("##repeat", ImVec2(x, cy - sh * 0.5f), ImVec2(sh, sh), BtnStyle::Ghost, IconRepeat, sh * 0.5f, a_m.repeatKnown, a_m.repeat != 0, tip)) {
					Media::CycleRepeat();
				}
				if (a_m.repeat == 1) {
					auto* dl = ImGui::GetWindowDrawList();
					const float fs = U(9.0f * a_unit);
					dl->AddText(FSemi(), fs, ImVec2(x + sh * 0.5f - TextW(FSemi(), fs, "1") * 0.5f, cy - fs * 0.55f), kGold(), "1");
				}
			}
		}

		std::string Subtitle(const Media::Snapshot& a_m)
		{
			std::string s = a_m.artist;
			if (!a_m.album.empty() && a_m.album != a_m.title) {
				s += (s.empty() ? "" : "  \xC2\xB7  ") + a_m.album;
			}
			return s;
		}

		const char* EmptyText(const Media::Snapshot& a_m)
		{
			if (a_m.unavailable) {
				return "Windows' media controls aren't available";
			}
			if (!a_m.ready) {
				return "Connecting...";
			}
			if (a_m.appRunning) {
				return "Spotify is open, but Windows can't see it";
			}
			return cfg.anyPlayer ? "Nothing is playing" : "Spotify isn't open";
		}

		// second line under EmptyText
		const char* EmptyHint(const Media::Snapshot& a_m)
		{
			if (a_m.unavailable) {
				return "Needs Windows 10 (1809) or newer";
			}
			if (a_m.appRunning) {
				return "Play a song in Spotify. Still nothing? See the Sound tab in settings";
			}
			return "Start it and play something";
		}

		void DrawWide(const Media::Snapshot& a_m, ImVec2 a, ImVec2 b)
		{
			auto*       dl = ImGui::GetWindowDrawList();
			const float hgt = b.y - a.y;
			// grip strip on the left
			g_zones.gripA = a;
			g_zones.gripB = ImVec2(a.x + U(16.0f), b.y);
			if (g_cursor) {
				GripDots(dl, ImVec2(a.x + U(9.0f), (a.y + b.y) * 0.5f), true);
			}
			float x = a.x + U(20.0f);
			const float right = b.x - U(10.0f);
			// close + gear stacked on the right - only in cursor mode
			const float sb = g_cursor ? (std::min)(U(26.0f), (hgt - U(16.0f)) * 0.5f) : 0.0f;
			if (g_cursor) {
				if (Button("##close", ImVec2(right - sb, a.y + U(8.0f)), ImVec2(sb, sb), BtnStyle::Ghost, IconClose, sb * 0.9f, true, false, "Hide the widget")) {
					cfg.visible = false;
					g_wantCursor = false;
					MarkDirty();
				}
				if (Button("##gear", ImVec2(right - sb, a.y + U(8.0f) + sb + U(2.0f)), ImVec2(sb, sb), BtnStyle::Ghost, IconGear, sb * 0.62f, true, g_settingsOpen,
						"Widget settings")) {
					g_settingsOpen = !g_settingsOpen;
				}
			}
			float x1 = g_cursor ? right - sb - U(10.0f) : right - U(4.0f);
			const bool ok = a_m.hasSession;
			if (cfg.showArt) {
				const float as = std::clamp(hgt - U(24.0f), U(40.0f), U(120.0f));
				ArtBox(dl, ImVec2(x, (a.y + b.y - as) * 0.5f), as, U(6.0f), false);
				x += as + U(14.0f);
			}
			if (!ok) {
				const float ts = U(14.0f);
				Line(dl, FSemi(), ts, ImVec2(x, (a.y + b.y) * 0.5f - ts * 0.6f), kText(), EmptyText(a_m), x1 - x - U(130.0f));
				if (a_m.ready && !a_m.unavailable && !cfg.anyPlayer && !a_m.appRunning) {
					const float bw = U(120.0f), bh = U(36.0f);
					const ImVec2 bp(x1 - bw, (a.y + b.y - bh) * 0.5f);
					if (Button("##openapp", bp, ImVec2(bw, bh), BtnStyle::Flat, IconNone, 0.0f, true, false, "Start Spotify")) {
						Media::OpenApp();
					}
					dl->AddText(FText(), U(12.0f), bp + ImVec2((bw - TextW(FText(), U(12.0f), "Open Spotify")) * 0.5f, bh * 0.5f - U(7.0f)), kText(), "Open Spotify");
				}
				return;
			}
			// transport on the right
			const bool  wideExtras = (b.x - a.x) >= U(560.0f);
			const bool  vol = g_cursor && cfg.showSound && (b.x - a.x) >= U(760.0f);
			const float unit = std::clamp((hgt - U(20.0f)) / U(46.0f), 0.7f, 1.0f);
			const float tw = (wideExtras ? U(34.0f * unit) * 2 + U(10.0f * unit) * 2 : 0.0f) + U(42.0f * unit) * 2 + U(54.0f * unit) + U(10.0f * unit) * 2;
			if (vol) {
				const float vw = U(130.0f);
				IconSpeaker(dl, ImVec2(x1 - vw + U(8.0f), (a.y + b.y) * 0.5f), U(15.0f), kDim());
				Volume(a_m, x1 - vw + U(22.0f), x1, (a.y + b.y) * 0.5f, false);
				x1 -= vw + U(14.0f);
			}
			Transport(a_m, x1 - tw * 0.5f, (a.y + b.y) * 0.5f - U(23.0f * unit), wideExtras ? tw + 1.0f : tw - 1.0f, unit);
			x1 -= tw + U(14.0f);
			// title + artist, progress under it
			const float ts = U(15.0f);
			const bool  prog = cfg.showProgress && hgt >= U(70.0f);
			const float blockH = ts + (prog ? U(18.0f) : 0.0f);
			float       y = (a.y + b.y - blockH) * 0.5f;
			const float maxW = x1 - x;
			const float tw2 = TextW(FSemi(), ts, a_m.title.c_str());
			Line(dl, FSemi(), ts, ImVec2(x, y), kText(), a_m.title, maxW);
			if (tw2 + U(20.0f) < maxW && !a_m.artist.empty()) {
				Line(dl, FText(), U(13.0f), ImVec2(x + tw2 + U(8.0f), y + U(1.5f)), kDim(), "\xC2\xB7  " + a_m.artist, maxW - tw2 - U(8.0f));
			}
			if (prog) {
				Progress(a_m, x, x1, y + ts + U(14.0f), true);
			}
		}

		void DrawStacked(const Media::Snapshot& a_m, ImVec2 a, ImVec2 b, bool a_tall)
		{
			auto*       dl = ImGui::GetWindowDrawList();
			const float x0 = a.x + U(20.0f), x1 = b.x - U(20.0f);
			float       y = a.y + U(14.0f);
			const float yEnd = b.y - U(16.0f);
			if (cfg.showHeader) {
				const float hh = U(46.0f);
				const float ts = U(18.0f);
				dl->AddText(FTitle(), ts, ImVec2(x0 + U(1.0f), y + U(4.0f)), Col(0, 0, 0, 120), "Now Playing");
				dl->AddText(FTitle(), ts, ImVec2(x0, y + U(2.0f)), kText(), "Now Playing");
				const std::string sub = a_m.hasSession ? (a_m.playing ? "Spotify  \xC2\xB7  playing" : "Spotify  \xC2\xB7  paused") : "Spotify";
				dl->AddText(FText(), U(11.0f), ImVec2(x0, y + U(26.0f)), kDim(), cfg.anyPlayer && a_m.hasSession ? (a_m.playing ? "Playing" : "Paused") : sub.c_str());
				if (g_cursor) {
					SideButtons(x1, y + U(6.0f), U(32.0f), true);
				}
				g_zones.gripA = ImVec2(a.x, a.y);
				g_zones.gripB = ImVec2(x1 - U(32.0f) - U(6.0f) - U(30.0f) - U(20.0f), y + hh);
				HairlineFade(dl, x0, x1, y + hh);
				y += hh + U(14.0f);
			} else {
				// no title bar: a slim grip strip along the top, with small settings/close buttons
				const float gh = U(24.0f);
				g_zones.gripA = ImVec2(a.x + U(10.0f), a.y);
				g_zones.gripB = ImVec2(x1 - U(56.0f), a.y + gh);
				if (g_cursor) {
					GripDots(dl, ImVec2((a.x + b.x) * 0.5f, a.y + gh * 0.5f), false);
					SideButtons(x1 + U(8.0f), a.y + U(3.0f), U(22.0f), false);
				}
				y = a.y + gh + U(4.0f);
			}
			if (!a_m.hasSession) {
				const float ts = U(15.0f);
				const float cy = (y + yEnd) * 0.5f;
				Line(dl, FSemi(), ts, ImVec2(x0, cy - U(34.0f)), kText(), EmptyText(a_m), x1 - x0, 0.5f);
				Line(dl, FText(), U(12.0f), ImVec2(x0, cy - U(12.0f)), kDim(), EmptyHint(a_m), x1 - x0, 0.5f);
				if (a_m.ready && !a_m.unavailable && !cfg.anyPlayer && !a_m.appRunning && yEnd - cy > U(30.0f)) {
					const float bw = U(150.0f), bh = U(40.0f);
					const ImVec2 bp((x0 + x1 - bw) * 0.5f, cy + U(10.0f));
					if (Button("##openapp", bp, ImVec2(bw, bh), BtnStyle::Gold, IconNone, 0.0f, true, false, "Start Spotify")) {
						Media::OpenApp();
					}
					dl->AddText(FSemi(), U(13.0f), bp + ImVec2((bw - TextW(FSemi(), U(13.0f), "Open Spotify")) * 0.5f, bh * 0.5f - U(8.0f)), Col(26, 21, 14), "Open Spotify");
				}
				return;
			}

			// what fits (the same rules as the preview)
			const float progH = cfg.showProgress ? U(27.0f) : 0.0f, ctrlH = U(46.0f), soundH = U(118.0f);
			float       left = (yEnd - y) - progH - ctrlH;
			float       bigArt = 0.0f;
			bool        rowArt = false;
			if (cfg.showArt && a_tall) {
				bigArt = (std::min)(x1 - x0, left - U(58.0f) - (cfg.showSound ? soundH : 0.0f) - U(14.0f));
				if (bigArt < U(80.0f)) {
					bigArt = (std::min)(x1 - x0, left - U(58.0f) - U(14.0f));
				}
				if (bigArt >= U(80.0f)) {
					left -= bigArt + U(14.0f) + U(58.0f);
				} else {
					bigArt = 0.0f;
				}
			}
			if (cfg.showArt && bigArt <= 0.0f && left >= U(96.0f)) {
				rowArt = true;
				left -= U(96.0f);
			}
			const bool textOnly = bigArt <= 0.0f && !rowArt;
			if (textOnly) {
				left -= U(30.0f);
			}
			const bool sound = g_cursor && cfg.showSound && left >= soundH;  // volume + music mute: cursor mode only

			if (bigArt > 0.0f) {
				ArtBox(dl, ImVec2((x0 + x1 - bigArt) * 0.5f, y), bigArt, U(8.0f), true);
				y += bigArt + U(14.0f);
				Line(dl, FSemi(), U(17.0f), ImVec2(x0, y), kText(), a_m.title, x1 - x0, 0.5f);
				Line(dl, FText(), U(13.0f), ImVec2(x0, y + U(24.0f)), kDim(), Subtitle(a_m), x1 - x0, 0.5f);
				y += U(58.0f);
			} else if (rowArt) {
				const float as = U(80.0f);
				ArtBox(dl, ImVec2(x0, y), as, U(6.0f), true);
				const float tx = x0 + as + U(14.0f);
				Line(dl, FSemi(), U(16.0f), ImVec2(tx, y + U(18.0f)), kText(), a_m.title, x1 - tx);
				Line(dl, FText(), U(13.0f), ImVec2(tx, y + U(42.0f)), kDim(), Subtitle(a_m), x1 - tx);
				y += U(96.0f);
			} else {
				const float tw = TextW(FSemi(), U(14.0f), a_m.title.c_str());
				const float maxW = x1 - x0;
				if (a_tall || tw + U(30.0f) > maxW || a_m.artist.empty()) {
					Line(dl, FSemi(), U(14.0f), ImVec2(x0, y), kText(), a_m.title, maxW, a_tall ? 0.5f : 0.0f);
				} else {
					Line(dl, FSemi(), U(14.0f), ImVec2(x0, y), kText(), a_m.title, maxW);
					Line(dl, FText(), U(13.0f), ImVec2(x0 + tw + U(8.0f), y + U(1.0f)), kDim(), "\xC2\xB7  " + a_m.artist, maxW - tw - U(8.0f));
				}
				y += U(30.0f);
			}
			if (cfg.showProgress) {
				Progress(a_m, x0, x1, y + U(7.0f), true);
				y += progH;
			}
			Transport(a_m, (x0 + x1) * 0.5f, y, x1 - x0, 1.0f);
			y += ctrlH;
			if (sound) {
				y += U(18.0f);
				dl->AddRectFilled(ImVec2(x0, y), ImVec2(x1, y + 1.0f), Col(255, 255, 255, 18));
				y += U(14.0f);
				dl->AddText(FSemi(), U(11.0f), ImVec2(x0, y), kGold(), "SOUND");
				y += U(26.0f);
				dl->AddText(FText(), U(13.0f), ImVec2(x0, y - U(8.0f)), Col(214, 210, 202), "Volume");
				Volume(a_m, x0 + U(70.0f), x1, y, true);
				y += U(26.0f);
				Line(dl, FText(), U(13.0f), ImVec2(x0, y + U(2.0f)), Col(214, 210, 202), "Mute Skyrim's music while playing", x1 - x0 - U(52.0f));
				if (Switch("##duck", ImVec2(x1 - U(40.0f), y), &cfg.muteGameMusic)) {
					MarkDirty();
				}
			}
		}

		// Clamp the widget to the screen (also after a resolution change)
		void KeepOnScreen()
		{
			const float k = g_k;
			float       w = cfg.w * k, h = cfg.h * k;
			if (w > g_displayW) {
				cfg.w = (std::max)(kMinW, g_displayW / k);
				w = cfg.w * k;
			}
			if (h > g_displayH) {
				cfg.h = (std::max)(kMinH, g_displayH / k);
				h = cfg.h * k;
			}
			const float maxX = (std::max)(0.0f, (g_displayW - w) / g_displayW), maxY = (std::max)(0.0f, (g_displayH - h) / g_displayH);
			cfg.posX = std::clamp(cfg.posX, 0.0f, maxX);
			cfg.posY = std::clamp(cfg.posY, 0.0f, maxY);
		}

		void PushUndo()
		{
			g_undo.push_back({ cfg.posX, cfg.posY, cfg.w, cfg.h });
			if (g_undo.size() > 30) {
				g_undo.erase(g_undo.begin());
			}
		}

		void Undo()
		{
			if (g_undo.empty()) {
				g_status = "Nothing to undo";
				return;
			}
			const Rect r = g_undo.back();
			g_undo.pop_back();
			cfg.posX = r.x;
			cfg.posY = r.y;
			cfg.w = r.w;
			cfg.h = r.h;
			MarkDirty();
			g_status = "Moved back";
		}

		void ApplyPreset(float a_w, float a_h)
		{
			PushUndo();
			cfg.w = a_w;
			cfg.h = a_h;
			MarkDirty();
		}

		Grab HitTest(ImVec2 a_p, ImVec2 a_a, ImVec2 a_b)
		{
			const float e = U(6.0f), c = U(14.0f);
			const bool  inX = a_p.x >= a_a.x - e && a_p.x <= a_b.x + e, inY = a_p.y >= a_a.y - e && a_p.y <= a_b.y + e;
			if (!inX || !inY) {
				return Grab::None;
			}
			if (!cfg.locked) {
				const bool l = a_p.x <= a_a.x + e, r = a_p.x >= a_b.x - e, t = a_p.y <= a_a.y + e, bo = a_p.y >= a_b.y - e;
				const bool lc = a_p.x <= a_a.x + c, rc = a_p.x >= a_b.x - c, tc = a_p.y <= a_a.y + c, bc = a_p.y >= a_b.y - c;
				if ((t && lc) || (l && tc)) return Grab::NW;
				if ((t && rc) || (r && tc)) return Grab::NE;
				if ((bo && lc) || (l && bc)) return Grab::SW;
				if ((bo && rc) || (r && bc)) return Grab::SE;
				if (t) return Grab::N;
				if (bo) return Grab::S;
				if (l) return Grab::W;
				if (r) return Grab::E;
				if (a_p.x >= g_zones.gripA.x && a_p.x <= g_zones.gripB.x && a_p.y >= g_zones.gripA.y && a_p.y <= g_zones.gripB.y) {
					return Grab::Move;
				}
			}
			return Grab::None;
		}

		ImGuiMouseCursor CursorFor(Grab a_g)
		{
			switch (a_g) {
			case Grab::N:
			case Grab::S: return ImGuiMouseCursor_ResizeNS;
			case Grab::E:
			case Grab::W: return ImGuiMouseCursor_ResizeEW;
			case Grab::NW:
			case Grab::SE: return ImGuiMouseCursor_ResizeNWSE;
			case Grab::NE:
			case Grab::SW: return ImGuiMouseCursor_ResizeNESW;
			case Grab::Move: return ImGuiMouseCursor_Hand;
			default: return ImGuiMouseCursor_Arrow;
			}
		}

		// Moving / resizing with the safeguards: title bar only, a short hold and a few pixels before anything moves.
		void HandleDrag(ImVec2 a_a, ImVec2 a_b)
		{
			auto&      io = ImGui::GetIO();
			const auto mouse = io.MousePos;
			if (g_drag.mode == Grab::None) {
				const Grab g = HitTest(mouse, a_a, a_b);
				g_wheelScale = mouse.x >= a_a.x && mouse.x <= a_b.x && mouse.y >= a_a.y && mouse.y <= a_b.y;
				if (g != Grab::None && !ImGui::IsAnyItemHovered()) {
					ImGui::SetMouseCursor(CursorFor(g));
					if (ImGui::IsMouseClicked(0)) {
						g_drag.mode = g;
						g_drag.active = false;
						g_drag.start = mouse;
						g_drag.startTime = g_time;
						g_drag.pos0 = a_a;
						g_drag.size0 = a_b - a_a;
					}
				}
				return;
			}
			ImGui::SetMouseCursor(CursorFor(g_drag.mode));
			if (!io.MouseDown[0]) {
				if (g_drag.active) {
					MarkDirty();
				}
				g_drag = {};
				return;
			}
			const ImVec2 d = mouse - g_drag.start;
			if (!g_drag.active) {
				const bool held = (g_time - g_drag.startTime) * 1000.0f >= static_cast<float>(cfg.dragDelayMs);
				const bool moved = std::sqrt(d.x * d.x + d.y * d.y) >= static_cast<float>(cfg.dragDistance);
				if (!(held && moved)) {
					return;
				}
				g_drag.active = true;
				PushUndo();
			}
			const float k = g_k;
			ImVec2      pos = g_drag.pos0, size = g_drag.size0;
			const auto  m = g_drag.mode;
			if (m == Grab::Move) {
				pos += d;
			} else {
				const bool e = m == Grab::E || m == Grab::NE || m == Grab::SE, w = m == Grab::W || m == Grab::NW || m == Grab::SW;
				const bool s = m == Grab::S || m == Grab::SE || m == Grab::SW, n = m == Grab::N || m == Grab::NE || m == Grab::NW;
				if (e) {
					size.x = std::clamp(g_drag.size0.x + d.x, kMinW * k, (std::min)(kMaxW * k, g_displayW - pos.x));
				}
				if (w) {
					size.x = std::clamp(g_drag.size0.x - d.x, kMinW * k, (std::min)(kMaxW * k, g_drag.pos0.x + g_drag.size0.x));
					pos.x = g_drag.pos0.x + g_drag.size0.x - size.x;
				}
				if (s) {
					size.y = std::clamp(g_drag.size0.y + d.y, kMinH * k, (std::min)(kMaxH * k, g_displayH - pos.y));
				}
				if (n) {
					size.y = std::clamp(g_drag.size0.y - d.y, kMinH * k, (std::min)(kMaxH * k, g_drag.pos0.y + g_drag.size0.y));
					pos.y = g_drag.pos0.y + g_drag.size0.y - size.y;
				}
			}
			cfg.w = size.x / k;
			cfg.h = size.y / k;
			cfg.posX = pos.x / g_displayW;
			cfg.posY = pos.y / g_displayH;
			KeepOnScreen();
		}

		// The sound section's height when it fits in a stacked layout (0 if it doesn't). Outside cursor mode the
		// section is hidden and the widget is that much shorter (its top edge stays put). Same rules as DrawStacked.
		float SoundSectionHeight(bool a_tall)
		{
			constexpr float progH = 27.0f, ctrlH = 46.0f, soundH = 118.0f;
			const float     y = cfg.showHeader ? 14.0f + 46.0f + 14.0f : 24.0f + 4.0f;
			float           left = (cfg.h - 16.0f - y) - (cfg.showProgress ? progH : 0.0f) - ctrlH;
			float           bigArt = 0.0f;
			bool            rowArt = false;
			if (cfg.showArt && a_tall) {
				bigArt = (std::min)(cfg.w - 40.0f, left - 58.0f - (cfg.showSound ? soundH : 0.0f) - 14.0f);
				if (bigArt < 80.0f) {
					bigArt = (std::min)(cfg.w - 40.0f, left - 58.0f - 14.0f);
				}
				if (bigArt >= 80.0f) {
					left -= bigArt + 14.0f + 58.0f;
				} else {
					bigArt = 0.0f;
				}
			}
			if (cfg.showArt && bigArt <= 0.0f && left >= 96.0f) {
				rowArt = true;
				left -= 96.0f;
			}
			if (bigArt <= 0.0f && !rowArt) {
				left -= 30.0f;
			}
			return cfg.showSound && left >= soundH ? soundH : 0.0f;
		}

		void DrawWidget(const Media::Snapshot& a_m)
		{
			KeepOnScreen();
			const float  k = g_k;
			const ImVec2 a(std::round(cfg.posX * g_displayW), std::round(cfg.posY * g_displayH));
			const ImVec2 bFull = a + ImVec2(std::round(cfg.w * k), std::round(cfg.h * k));  // the size the player set
			const Layout layout = LayoutFor(cfg.w, cfg.h);
			const float  cut = (!g_cursor && a_m.hasSession && layout != Layout::Wide) ? SoundSectionHeight(layout == Layout::Tall) : 0.0f;
			const ImVec2 b(bFull.x, std::round(bFull.y - cut * k));  // what's drawn: shorter outside cursor mode
			const float  margin = U(10.0f);
			ImGui::SetNextWindowPos(a - ImVec2(margin, margin), ImGuiCond_Always);
			ImGui::SetNextWindowSize(b - a + ImVec2(margin, margin) * 2.0f, ImGuiCond_Always);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1, 1));
			ImGui::Begin("##hearthsong", nullptr,
				ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
					ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollWithMouse);
			ImGui::PopStyleVar(3);
			auto* dl = ImGui::GetWindowDrawList();
			dl->PushClipRect(a - ImVec2(margin, margin), b + ImVec2(margin, margin), false);
			Panel(dl, a, b, g_cursor);
			dl->PushClipRect(a, b, true);
			if (layout == Layout::Wide) {
				DrawWide(a_m, a, b);
			} else {
				DrawStacked(a_m, a, bFull, layout == Layout::Tall);  // laid out for the full size, so nothing moves when the cursor comes up
			}
			dl->PopClipRect();
			// resize corner hint
			if (g_cursor && !cfg.locked) {
				dl->AddLine(b - ImVec2(U(4.0f), U(12.0f)), b - ImVec2(U(12.0f), U(4.0f)), Col(255, 255, 255, 70), U(1.2f));
				dl->AddLine(b - ImVec2(U(4.0f), U(8.0f)), b - ImVec2(U(8.0f), U(4.0f)), Col(255, 255, 255, 70), U(1.2f));
			}
			if (g_cursor) {
				HandleDrag(a, b);
				if (g_drag.active && g_drag.mode != Grab::Move) {  // size readout above the widget
					const std::string t = fmt::format("{:.0f} x {:.0f}  \xC2\xB7  {}", cfg.w, cfg.h, LayoutName(layout));
					const float       ts = U(12.0f), tw = TextW(FSemi(), ts, t.c_str());
					const ImVec2      p((a.x + b.x - tw) * 0.5f - U(10.0f), a.y - U(34.0f) < 0 ? b.y + U(8.0f) : a.y - U(34.0f));
					auto*             fg = ImGui::GetForegroundDrawList();
					fg->AddRectFilled(p, p + ImVec2(tw + U(20.0f), U(24.0f)), kGold(), U(6.0f));
					fg->AddText(FSemi(), ts, p + ImVec2(U(10.0f), U(5.0f)), Col(26, 21, 14), t.c_str());
				}
			}
			dl->PopClipRect();
			ImGui::End();
		}

		// ================================================================ settings window
		float SS(float a_v) { return Theme::S(a_v); }

		void Row(const char* a_label, const char* a_help)
		{
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(a_label);
			if (a_help && ImGui::IsItemHovered()) {
				ImGui::SetTooltip("%s", a_help);
			}
			ImGui::SameLine(SS(320.0f));
		}

		bool ToggleRow(const char* a_label, const char* a_help, bool* a_v)
		{
			Row(a_label, a_help);
			ImGui::PushID(a_label);
			const bool c = Theme::Toggle("##t", a_v);
			ImGui::PopID();
			if (c) {
				MarkDirty();
			}
			return c;
		}

		void Gap() { ImGui::Dummy(ImVec2(0, SS(8.0f))); }

		void DrawSettings(const Media::Snapshot& a_m)
		{
			auto& io = ImGui::GetIO();
			ImGui::SetNextWindowPos(io.DisplaySize * 0.5f, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2((std::min)(SS(860.0f), io.DisplaySize.x * 0.95f), 0));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(SS(30.0f), SS(26.0f)));
			const bool vis = Theme::BeginWindow("Hearthsong settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
			ImGui::PopStyleVar();
			if (!vis) {
				Theme::EndWindow();
				return;
			}
			if (Theme::Header("Hearthsong", "Changes save to Hearthsong.ini by themselves")) {
				g_settingsOpen = false;
			}
			const char* tabs[] = { "Layout", "Behaviour", "Keys", "Sound" };
			for (int t = 0; t < 4; ++t) {
				if (t) {
					ImGui::SameLine(0.0f, SS(8.0f));
				}
				ImGui::PushID(900 + t);
				if (Theme::Tab(tabs[t], g_settingsTab == t, SS(46.0f), -1)) {
					g_settingsTab = t;
				}
				ImGui::PopID();
			}
			const float h = (std::min)(SS(560.0f), io.DisplaySize.y * 0.62f);
			if (Theme::BeginParchment("##hs_settings", ImVec2(0.0f, h), true)) {
				const float ctl = ImGui::GetContentRegionAvail().x - SS(330.0f);
				if (g_settingsTab == 0) {
					const Layout l = LayoutFor(cfg.w, cfg.h);
					Theme::SectionLabel("Shape");
					Row("Layout", "Drag any edge or corner of the widget to resize it. The layout follows the shape:\nwide and short = horizontal bar, tall and narrow = portrait.");
					if (Theme::Chip("Standard", l == Layout::Standard)) {
						ApplyPreset(kDefW, kDefH);
					}
					ImGui::SameLine(0.0f, SS(6.0f));
					if (Theme::Chip("Horizontal bar", l == Layout::Wide)) {
						ApplyPreset(640.0f, 104.0f);
					}
					ImGui::SameLine(0.0f, SS(6.0f));
					if (Theme::Chip("Portrait", l == Layout::Tall)) {
						ApplyPreset(280.0f, 520.0f);
					}
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SS(320.0f));
					Theme::TextItalic("Now: %s, %.0f x %.0f", LayoutName(l), cfg.w, cfg.h);
					Row("Scale", "Makes everything in the widget bigger or smaller. Ctrl + mouse wheel over the widget works too.");
					if (Theme::SliderInt("##scale", &cfg.scalePct, 60, 200, "%d%%", ctl)) {
						MarkDirty();
						g_pendingFonts = true;  // remade once the slider is let go
					}
					Row("Opacity", "How see-through the widget's background is");
					if (Theme::SliderInt("##opacity", &cfg.opacityPct, 30, 100, "%d%%", ctl)) {
						MarkDirty();
					}
					Gap();
					Theme::SectionLabel("Show");
					ToggleRow("Title bar", "\"Now Playing\" with the settings and close buttons. Without it the widget is picked up by a slim strip along its top.", &cfg.showHeader);
					ToggleRow("Album art", nullptr, &cfg.showArt);
					ToggleRow("Progress bar", "Song position. Click it to jump (if the app allows).", &cfg.showProgress);
					ToggleRow("Volume and music mute", "Shown when there's room for them", &cfg.showSound);
					Gap();
					Theme::SectionLabel("Position");
					ToggleRow("Lock position and size", "Nothing can move or resize the widget, even with the cursor", &cfg.locked);
					Row("Undo", "Puts the widget back where it was before the last move, resize or layout change");
					ImGui::BeginDisabled(g_undo.empty());
					if (Theme::Button(fmt::format("Undo last move  (Ctrl+{})", KeyName(cfg.keyVk[kKeyUndo])).c_str(), ImVec2(SS(320.0f), SS(40.0f)))) {
						Undo();
					}
					ImGui::EndDisabled();
					Row("Reset", "Default size and place");
					if (Theme::Button("Reset position and size", ImVec2(SS(320.0f), SS(40.0f)))) {
						PushUndo();
						cfg.posX = 0.70f;
						cfg.posY = 0.06f;
						cfg.w = kDefW;
						cfg.h = kDefH;
						cfg.scalePct = 100;
						g_pendingFonts = true;
						MarkDirty();
					}
					Gap();
					Theme::SectionLabel("This window");
					Row("Settings window size", nullptr);
					int ui = static_cast<int>(std::lround(cfg.uiScale * 100.0f));
					if (Theme::SliderInt("##ui", &ui, 60, 200, "%d%%", ctl)) {
						cfg.uiScale = ui / 100.0f;
						MarkDirty();
						g_pendingFonts = true;
					}
				} else if (g_settingsTab == 1) {
					Theme::SectionLabel("Combat");
					ToggleRow("Leave cursor mode when combat starts", "If a fight starts while the cursor is up, you get the camera and your weapons back at once", &cfg.leaveCursorInCombat);
					ToggleRow("No cursor mode during combat", "The cursor key does nothing until the fight is over", &cfg.noCursorInCombat);
					ToggleRow("Hide during combat", "The widget fades out when a fight starts and comes back afterwards", &cfg.autoHideInCombat);
					ImGui::BeginDisabled(!cfg.autoHideInCombat);
					Row("Show again after", "Seconds after the fight ends");
					if (Theme::SliderFloat("##again", &cfg.showAgainAfter, 0.0f, 10.0f, "%.1f s", ctl)) {
						MarkDirty();
					}
					ImGui::EndDisabled();
					Gap();
					Theme::SectionLabel("Moving and resizing");
					Row("Hold before it moves", "How long the mouse button must be held on the title bar or an edge before the widget moves");
					if (Theme::SliderInt("##delay", &cfg.dragDelayMs, 0, 600, "%d ms", ctl)) {
						MarkDirty();
					}
					Row("Move before it moves", "How far the mouse must travel before the widget follows (a click never nudges it)");
					if (Theme::SliderInt("##dist", &cfg.dragDistance, 0, 40, "%d px", ctl)) {
						MarkDirty();
					}
					Row("Mouse speed", "Cursor speed in cursor mode");
					if (Theme::SliderFloat("##mouse", &cfg.mouseSpeed, 0.2f, 5.0f, "%.1f x", ctl)) {
						MarkDirty();
					}
					Row("Cursor key", "Toggle: press once for the cursor, again to put it away.\nHold: the cursor is there while you hold the key.");
					if (Theme::Chip("Press to toggle", !cfg.cursorHold)) {
						cfg.cursorHold = false;
						MarkDirty();
					}
					ImGui::SameLine(0.0f, SS(6.0f));
					if (Theme::Chip("Hold", cfg.cursorHold)) {
						cfg.cursorHold = true;
						MarkDirty();
					}
					Gap();
					Theme::SectionLabel("Hiding");
					ToggleRow("Hide in menus", "Hidden while the game is paused (inventory, map, journal...)", &cfg.hideInMenus);
					ToggleRow("Hide when Spotify isn't open", "Otherwise the widget shows an \"Open Spotify\" button", &cfg.hideWhenNoPlayer);
				} else if (g_settingsTab == 2) {
					Theme::SectionLabel("Keys");
					Theme::TextItalic("Click a key, then press the new one. Esc keeps the old key, Backspace removes it.");
					Gap();
					const int capturing = g_captureSlot;
					for (int i = 0; i < kKeyCount; ++i) {
						Row(kKeyLabel[i], kKeyHelp[i]);
						ImGui::PushID(i);
						const bool  waiting = capturing == i;
						std::string label = waiting ? std::string("Press a key...") : (i == kKeyUndo ? "Ctrl + " + KeyName(cfg.keyVk[i]) : KeyName(cfg.keyVk[i]));
						if (Theme::Button(label.c_str(), ImVec2(SS(waiting ? 260.0f : 200.0f), SS(40.0f)), waiting) && !waiting) {
							g_capturedVk = 0;
							g_captureSlot = i;
						}
						ImGui::PopID();
					}
					Gap();
					Theme::TextItalic("Your keyboard's own media keys (play/pause, next...) control Spotify too.");
				} else {
					Theme::SectionLabel("Skyrim's music");
					ToggleRow("Mute Skyrim's music while playing", "Turns the game's music down to silence while Spotify plays, and back up when it stops.\n(Back to normal in the Journal menu, so your audio settings stay as they are.)", &cfg.muteGameMusic);
					if (!g_musicCategory) {
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SS(320.0f));
						Theme::TextColoredWrapped(Theme::Warning(), "Skyrim's music sound category wasn't found - see Hearthsong.log");
					}
					Gap();
					Theme::SectionLabel("Volume");
					Row("Volume step", "How much the volume keys and the mouse wheel over the slider change the volume");
					if (Theme::SliderInt("##step", &cfg.volumeStep, 1, 25, "%d%%", ctl)) {
						MarkDirty();
					}
					Gap();
					Theme::SectionLabel("Music app");
					Row("App name", "Part of the app's name as Windows knows it. \"Spotify\" for Spotify; other players work too\n(e.g. \"foobar\", \"MusicBee\", \"chrome\" for a browser tab).");
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(SS(10.0f), SS(10.0f)));
					ImGui::SetNextItemWidth(ctl);
					if (ImGui::InputText("##app", g_appBuf, sizeof g_appBuf)) {
						cfg.musicApp = g_appBuf;
						Media::SetSettings(MediaSettings());
						MarkDirty();
					}
					ImGui::PopStyleVar();
					if (ToggleRow("Use any player if it isn't found", "Control whatever Windows says is playing when the app above isn't running", &cfg.anyPlayer)) {
						Media::SetSettings(MediaSettings());
					}
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + SS(320.0f));
					Theme::TextItalic("Now controlling: %s", a_m.hasSession ? a_m.app.c_str() : "nothing");
					Row("Apps Windows sees", "Every app that's sharing what it plays with Windows right now. Click one to control it.");
					if (a_m.sessions.empty()) {
						Theme::TextItalic("none");
					} else {
						for (std::size_t i = 0; i < a_m.sessions.size(); ++i) {
							if (i) {
								ImGui::SameLine(0.0f, SS(6.0f));
							}
							ImGui::PushID(static_cast<int>(1000 + i));
							if (Theme::Chip(a_m.sessions[i].c_str(), a_m.hasSession && a_m.sessions[i] == a_m.app)) {
								cfg.musicApp = a_m.sessions[i];
								std::snprintf(g_appBuf, sizeof g_appBuf, "%s", cfg.musicApp.c_str());
								Media::SetSettings(MediaSettings());
								MarkDirty();
							}
							ImGui::PopID();
						}
					}
					if (!a_m.hasSession && a_m.appRunning) {
						Gap();
						Theme::SectionLabel("Spotify is open but not in the list?");
						Theme::TextColoredWrapped(Theme::Cream(),
							"1. Play a song in Spotify - it only shows up in Windows once something has played.\n"
							"2. In Spotify: Settings > Display > turn on \"Show desktop overlay when using media keys\", "
							"then restart Spotify. With that setting off, Spotify can stop sharing what it plays with Windows.\n"
							"3. Use the Spotify desktop app, not Spotify in a browser (a browser tab shows up as the browser).");
					}
				}
			}
			Theme::EndParchment();
			ImGui::Dummy(ImVec2(0, SS(6.0f)));
			if (Theme::Button("Reset everything", ImVec2(SS(220.0f), SS(52.0f)))) {
				PushUndo();
				int keys[kKeyCount];
				std::copy(std::begin(cfg.keyVk), std::end(cfg.keyVk), std::begin(keys));
				cfg = Config{};
				std::copy(std::begin(keys), std::end(keys), std::begin(cfg.keyVk));  // keys stay
				std::snprintf(g_appBuf, sizeof g_appBuf, "%s", cfg.musicApp.c_str());
				Media::SetSettings(MediaSettings());
				g_pendingFonts = true;
				MarkDirty();
				g_status = "Everything but the keys is back to default";
			}
			ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - SS(180.0f));
			if (Theme::Button("Done", ImVec2(SS(180.0f), SS(52.0f)), true)) {
				g_settingsOpen = false;
			}
			if (!g_status.empty()) {
				Theme::TextItalic("%s", g_status.c_str());
			} else {
				Theme::TextItalic("%s", g_iniPath.c_str());
			}
			Theme::EndWindow();
		}

		// ================================================================ fonts / ImGui
		std::string FontDir()
		{
			std::error_code ec;
			return fs::absolute(fs::path(L"Data\\SKSE\\Plugins\\Hearthsong\\Fonts"), ec).string() + "\\";
		}

		const ImWchar* GlyphRanges()
		{
			// Latin (with accents), Greek, Cyrillic and punctuation: song titles in most European languages
			static const ImWchar ranges[] = { 0x0020, 0x024F, 0x0370, 0x03FF, 0x0400, 0x052F, 0x1E00, 0x1EFF, 0x2000, 0x206F, 0x20A0, 0x20CF, 0x2100, 0x214F, 0 };
			return ranges;
		}

		void ApplyFonts()
		{
			char windir[MAX_PATH]{};
			::GetWindowsDirectoryA(windir, MAX_PATH);
			const std::string sys = std::string(windir) + "\\Fonts\\";
			const std::string regular = sys + "georgia.ttf", bold = sys + "georgiab.ttf", italic = sys + "georgiai.ttf";
			if (g_imguiReady) {
				ImGui_ImplDX11_InvalidateDeviceObjects();
			}
			const std::string own = FontDir();
			const std::string display = own + "Cinzel-SemiBold.ttf", uiReg = own + "Inter-Regular.ttf", uiSemi = own + "Inter-SemiBold.ttf", uiItal = own + "Inter-Italic.ttf";
			Theme::FontFiles files{ regular.c_str(), bold.c_str(), italic.c_str() };
			files.display = display.c_str();
			files.uiRegular = uiReg.c_str();
			files.uiSemiBold = uiSemi.c_str();
			files.uiItalic = uiItal.c_str();
			const float screen = (std::max)(1.0f, g_displayH / 1080.0f);
			Theme::Setup(screen * cfg.uiScale, cfg.fontSize, files);
			// the widget's own fonts, made at the widget's size so they stay sharp at any scale
			g_fontK = screen * cfg.scalePct / 100.0f;
			auto& io = ImGui::GetIO();
			auto  load = [&](const std::string& a_path, float a_px) -> ImFont* {
                std::error_code ec;
                if (!fs::exists(fs::path(a_path), ec)) {
                    return nullptr;
                }
                ImFontConfig fc;
                fc.OversampleH = 2;
                fc.OversampleV = 2;
                return io.Fonts->AddFontFromFileTTF(a_path.c_str(), a_px, &fc, GlyphRanges());
			};
			g_fText = load(uiReg, 13.0f * g_fontK);
			g_fSemi = load(uiSemi, 17.0f * g_fontK);
			g_fTitle = load(display, 18.0f * g_fontK);
			spdlog::info("UI: screen scale {:.2f}, widget {}%, fonts {}", screen, cfg.scalePct, g_fText ? "Inter + Cinzel" : "MISSING (Data\\SKSE\\Plugins\\Hearthsong\\Fonts)");
		}

		bool InitImGui()
		{
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			auto& io = ImGui::GetIO();
			io.IniFilename = nullptr;
			io.LogFilename = nullptr;
			io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
			for (int i = 0; i < Theme::StyleCount(); ++i) {
				if (std::strcmp(Theme::StyleName(i), "Modern") == 0) {
					Theme::SetStyle(i);
				}
			}
			ApplyFonts();
			if (!ImGui_ImplDX11_Init(g_dev, g_ctx)) {
				return false;
			}
			io.MouseDrawCursor = g_cursor;
			return true;
		}

		ID3D11ShaderResourceView* MakeSRV(const std::uint8_t* a_px, int a_w, int a_h)
		{
			D3D11_TEXTURE2D_DESC d{};
			d.Width = static_cast<UINT>(a_w);
			d.Height = static_cast<UINT>(a_h);
			d.MipLevels = 1;
			d.ArraySize = 1;
			d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			d.SampleDesc.Count = 1;
			d.Usage = D3D11_USAGE_DEFAULT;
			d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			D3D11_SUBRESOURCE_DATA sd{ a_px, static_cast<UINT>(a_w) * 4, 0 };
			ID3D11Texture2D*       tex = nullptr;
			if (FAILED(g_dev->CreateTexture2D(&d, &sd, &tex)) || !tex) {
				return nullptr;
			}
			ID3D11ShaderResourceView* srv = nullptr;
			g_dev->CreateShaderResourceView(tex, nullptr, &srv);
			tex->Release();
			return srv;
		}

		void UpdateArt()
		{
			std::vector<std::uint8_t> px;
			int                       w = 0, h = 0;
			if (!Media::TakeArt(g_artVersion, px, w, h)) {
				return;
			}
			if (g_art) {
				g_art->Release();
				g_art = nullptr;
			}
			if (!px.empty() && w > 0 && h > 0) {
				g_art = MakeSRV(px.data(), w, h);
			}
		}
	}

	// ==================================================================== public
	void LoadSettings()
	{
		std::error_code ec;
		g_iniPath = fs::absolute(fs::path(L"Data\\SKSE\\Plugins\\Hearthsong.ini"), ec).string();
		for (int i = 0; i < kKeyCount; ++i) {
			cfg.keyVk[i] = ReadInt(kKeySection[i], kKeyIni[i], kKeyDefault[i], 0, 255);
		}
		RefreshKeyCodes();
		cfg.cursorHold = ReadBool("General", "bCursorHold", false);
		cfg.visible = ReadBool("General", "bVisible", true);
		cfg.mouseSpeed = ReadFloat("General", "fMouseSpeed", 1.0f, 0.1f, 10.0f);
		cfg.uiScale = ReadFloat("General", "fUIScale", 1.0f, 0.6f, 2.0f);
		cfg.fontSize = ReadFloat("General", "fFontSize", 18.0f, 10.0f, 40.0f);
		cfg.posX = ReadFloat("Widget", "fPosX", 0.70f, 0.0f, 1.0f);
		cfg.posY = ReadFloat("Widget", "fPosY", 0.06f, 0.0f, 1.0f);
		cfg.w = ReadFloat("Widget", "fWidth", kDefW, kMinW, kMaxW);
		cfg.h = ReadFloat("Widget", "fHeight", kDefH, kMinH, kMaxH);
		cfg.scalePct = ReadInt("Widget", "iScale", 100, 60, 200);
		cfg.opacityPct = ReadInt("Widget", "iOpacity", 95, 30, 100);
		cfg.showHeader = ReadBool("Widget", "bShowTitleBar", true);
		cfg.showArt = ReadBool("Widget", "bShowAlbumArt", true);
		cfg.showProgress = ReadBool("Widget", "bShowProgress", true);
		cfg.showSound = ReadBool("Widget", "bShowSound", true);
		cfg.locked = ReadBool("Widget", "bLocked", false);
		cfg.leaveCursorInCombat = ReadBool("Safety", "bLeaveCursorInCombat", true);
		cfg.noCursorInCombat = ReadBool("Safety", "bNoCursorInCombat", true);
		cfg.dragDelayMs = ReadInt("Safety", "iDragDelayMs", 150, 0, 2000);
		cfg.dragDistance = ReadInt("Safety", "iDragDistance", 6, 0, 100);
		cfg.autoHideInCombat = ReadBool("Hiding", "bAutoHideInCombat", false);
		cfg.showAgainAfter = ReadFloat("Hiding", "fShowAgainAfter", 2.0f, 0.0f, 60.0f);
		cfg.hideInMenus = ReadBool("Hiding", "bHideInMenus", true);
		cfg.hideWhenNoPlayer = ReadBool("Hiding", "bHideWhenNoPlayer", false);
		cfg.muteGameMusic = ReadBool("Sound", "bMuteGameMusic", true);
		cfg.volumeStep = ReadInt("Sound", "iVolumeStep", 5, 1, 50);
		{
			char buf[64]{};
			::GetPrivateProfileStringA("Sound", "sMusicApp", "Spotify", buf, sizeof buf, g_iniPath.c_str());
			cfg.musicApp = buf;
			std::snprintf(g_appBuf, sizeof g_appBuf, "%s", buf);
		}
		cfg.anyPlayer = ReadBool("Sound", "bAnyPlayer", false);
		dbg.hookPresent = ReadBool("Debug", "bHookPresent", true);
		dbg.inputSink = ReadBool("Debug", "bInputSink", true);
		dbg.logInput = ReadBool("Debug", "bLogInput", false);
		dbg.blockInput = ReadBool("Debug", "bBlockInput", true);
		spdlog::info("INI: {} ({})", g_iniPath, fs::exists(g_iniPath, ec) ? "found" : "NOT FOUND - using defaults");
		spdlog::info("Keys: show/hide {} ({}), cursor {} ({}), music app \"{}\"", KeyName(cfg.keyVk[kKeyToggle]), cfg.keyVk[kKeyToggle], KeyName(cfg.keyVk[kKeyCursor]),
			cfg.keyVk[kKeyCursor], cfg.musicApp);
		Media::Start(MediaSettings());
	}

	const DebugSwitches& GetDebugSwitches() { return dbg; }

	bool InstallInputSink()
	{
		static InputSink sink;
		auto*            mgr = RE::BSInputDeviceManager::GetSingleton();
		if (!mgr) {
			return false;
		}
		mgr->AddEventSink<RE::InputEvent*>(&sink);
		return true;
	}

	void InstallInputBlocker()
	{
		if (!dbg.inputSink || !dbg.blockInput) {
			spdlog::info("Input blocker off (bInputSink={} bBlockInput={})", dbg.inputSink, dbg.blockInput);
			return;
		}
		// BSInputDeviceManager::PollInputDevices -> the call that sends the input events to all listeners
		// (same place Hearthframe hooks; with both installed, each one calls the one behind it)
		REL::Relocation<std::uintptr_t> target{ REL::RelocationID(67315, 68617), REL::VariantOffset(0x7B, 0x7B, 0x81) };
		const auto* code = reinterpret_cast<const std::uint8_t*>(target.address());
		if (*code != 0xE8) {  // not a call: different game build or another mod rewrote it
			spdlog::warn("Input blocker: unexpected code at {:X} ({:02X}) - other mods' hotkeys can still fire in cursor mode", target.address(), *code);
			return;
		}
		const std::uintptr_t next = target.address() + 5 + *reinterpret_cast<const std::int32_t*>(code + 1);
		g_dispatchInput = SKSE::GetTrampoline().write_call<5>(target.address(), &DispatchInputHook);
		spdlog::info("Input blocker installed in front of {} - in cursor mode the game and other mods get no keys", ModuleOf(next));
	}

	void OnDataLoaded()
	{
		FindMusicCategory();
	}

	void OnGameLoaded()
	{
		g_gameLoaded = true;
	}

	int SehFilter(EXCEPTION_POINTERS* a_info, const char* a_where)
	{
		const auto* rec = a_info ? a_info->ExceptionRecord : nullptr;
		const auto  addr = rec ? reinterpret_cast<std::uintptr_t>(rec->ExceptionAddress) : 0;
		HMODULE     mod = nullptr;
		wchar_t     modPath[MAX_PATH]{};
		if (addr && ::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(addr), &mod)) {
			::GetModuleFileNameW(mod, modPath, MAX_PATH);
		}
		const auto offset = mod ? addr - reinterpret_cast<std::uintptr_t>(mod) : addr;
		spdlog::critical("CRASH CAUGHT in {}: exception 0x{:08X} at {}+0x{:X} - this part is now switched off so the game keeps running", a_where,
			rec ? static_cast<std::uint32_t>(rec->ExceptionCode) : 0u, ToUtf8(fs::path(modPath).filename().wstring()), offset);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	void OnPresent(IDXGISwapChain* a_sc)
	{
		if (!g_sc) {
			DXGI_SWAP_CHAIN_DESC desc{};
			if (FAILED(a_sc->GetDesc(&desc)) || !desc.OutputWindow) {
				return;
			}
			if (FAILED(a_sc->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&g_dev))) || !g_dev) {
				return;
			}
			g_dev->GetImmediateContext(&g_ctx);
			g_sc = a_sc;
			spdlog::info("Attached to the game's swap chain ({}x{})", desc.BufferDesc.Width, desc.BufferDesc.Height);
		}
		if (a_sc != g_sc) {
			return;
		}

		const auto now = std::chrono::steady_clock::now();
		float      dt = g_lastFrame.time_since_epoch().count() ? std::chrono::duration<float>(now - g_lastFrame).count() : 1.0f / 60.0f;
		g_lastFrame = now;
		dt = std::clamp(dt, 1.0f / 1000.0f, 0.5f);
		g_time += dt;

		// ---- game state
		const bool paused = GamePaused();
		const bool combat = g_gameLoaded && PlayerInCombat();
		g_pausedInput = paused;
		g_inCombatInput = combat;
		if (g_wasInCombat && !combat) {
			g_combatEndedAt = g_time;
		}
		const bool combatStarted = combat && !g_wasInCombat;
		g_wasInCombat = combat;

		if (g_toggleVisible.exchange(false)) {
			cfg.visible = !cfg.visible;
			if (!cfg.visible) {
				g_wantCursor = false;
			}
			MarkDirty();
		}
		if (g_wantCursor && !cfg.visible) {
			cfg.visible = true;  // the cursor key brings a hidden widget back
			MarkDirty();
		}
		if (paused || !g_gameLoaded) {
			g_wantCursor = false;
		}
		if (combatStarted && cfg.leaveCursorInCombat && g_wantCursor) {
			g_wantCursor = false;
			Notify("Hearthsong: combat - cursor mode off");
		}
		if (g_wantCursor != g_cursor) {
			ApplyCursor(g_wantCursor);
		}

		const Media::Snapshot m = Media::Get();

		// Skyrim's music: silent while the app plays (normal again in the Journal menu, where the audio settings are)
		{
			auto*      ui = RE::UI::GetSingleton();
			const bool journal = ui && ui->IsMenuOpen(RE::JournalMenu::MENU_NAME);
			SetGameMusicMuted(cfg.muteGameMusic && g_gameLoaded && m.hasSession && m.playing && !journal);
		}

		// ---- how visible
		bool show = g_gameLoaded && cfg.visible;
		if (cfg.hideInMenus && paused) {
			show = false;
		}
		if (!g_cursor) {
			if (cfg.autoHideInCombat && (combat || g_time - g_combatEndedAt < cfg.showAgainAfter)) {
				show = false;
			}
			if (cfg.hideWhenNoPlayer && !m.hasSession) {
				show = false;
			}
		}
		if (g_cursor) {
			show = true;
		}
		const float target = show ? 1.0f : 0.0f;
		g_alpha += (target - g_alpha) * (1.0f - std::exp(-dt * 10.0f));
		if (std::fabs(target - g_alpha) < 0.01f) {
			g_alpha = target;
		}

		// save settings a moment after the last change (not in the middle of a drag)
		if (g_dirty && g_drag.mode == Grab::None && g_time - g_dirtyAt > 0.5f) {
			g_dirty = false;
			SaveAll();
		}
		// a key chosen in the settings window
		if (const int slot = g_captureSlot; slot >= 0) {
			if (const int vk = g_capturedVk.exchange(0); vk != 0) {
				if (vk > 0) {
					for (int i = 0; i < kKeyCount; ++i) {  // one key, one job
						if (i != slot && cfg.keyVk[i] == vk) {
							cfg.keyVk[i] = 0;
							g_status = fmt::format("{} was used for \"{}\" - that one has no key now", KeyName(vk), kKeyLabel[i]);
						}
					}
					cfg.keyVk[slot] = vk;
				} else if (vk == -2) {
					cfg.keyVk[slot] = 0;
				}
				g_captureSlot = -1;
				RefreshKeyCodes();
				MarkDirty();
			}
		}

		if (g_alpha <= 0.0f && !g_cursor) {
			std::lock_guard lk(g_inputMtx);
			g_input.clear();
			return;
		}
		if (g_imguiFailed) {
			return;
		}

		ID3D11Texture2D* back = nullptr;
		if (FAILED(a_sc->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back))) || !back) {
			return;
		}
		D3D11_TEXTURE2D_DESC bd{};
		back->GetDesc(&bd);
		const bool sizeChanged = std::fabs(g_displayH - static_cast<float>(bd.Height)) > 0.5f || std::fabs(g_displayW - static_cast<float>(bd.Width)) > 0.5f;
		g_displayW = static_cast<float>(bd.Width);
		g_displayH = static_cast<float>(bd.Height);
		if (!g_imguiReady) {
			if (!InitImGui()) {
				g_imguiFailed = true;
				spdlog::error("ImGui failed to initialise");
				back->Release();
				return;
			}
			g_imguiReady = true;
		} else if (sizeChanged) {
			g_pendingFonts = true;
		}
		if (g_pendingFonts && g_drag.mode == Grab::None && !ImGui::IsAnyItemActive()) {
			g_pendingFonts = false;
			ApplyFonts();
		}

		auto& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(g_displayW, g_displayH);
		io.DeltaTime = dt;
		io.MouseDrawCursor = g_cursor;
		g_k = (std::max)(1.0f, g_displayH / 1080.0f) * cfg.scalePct / 100.0f;

		static bool lastCursor = false;
		if (g_cursor && !lastCursor) {  // cursor starts in the middle of the widget
			g_mouseX = std::clamp(cfg.posX * g_displayW + cfg.w * g_k * 0.5f, 0.0f, g_displayW - 1.0f);
			g_mouseY = std::clamp(cfg.posY * g_displayH + cfg.h * g_k * 0.5f, 0.0f, g_displayH - 1.0f);
			io.AddMousePosEvent(g_mouseX, g_mouseY);
		}
		if (!g_cursor && lastCursor) {
			io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
			for (int b = 0; b < 5; ++b) {
				io.AddMouseButtonEvent(b, false);
			}
		}
		lastCursor = g_cursor;
		if (g_cursor) {
			DrainInput();
			if (g_undoRequest.exchange(false)) {
				Undo();
			}
			if (g_escRequest.exchange(false)) {
				if (g_settingsOpen) {
					g_settingsOpen = false;
				} else {
					g_wantCursor = false;
				}
			}
		} else {
			g_undoRequest = false;
			g_escRequest = false;
		}
		UpdateArt();

		ImGui_ImplDX11_NewFrame();
		ImGui::NewFrame();
		DrawWidget(m);
		if (g_settingsOpen && g_cursor) {
			DrawSettings(m);
		}
		if (!g_settingsOpen) {
			g_status.clear();
		}
		ImGui::Render();

		ID3D11RenderTargetView* rtv = nullptr;
		g_dev->CreateRenderTargetView(back, nullptr, &rtv);
		back->Release();
		if (rtv) {
			g_ctx->OMSetRenderTargets(1, &rtv, nullptr);
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
			rtv->Release();
		}
	}
}
