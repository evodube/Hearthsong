#pragma once

// Hearthframe look: dark charcoal window with thin gold lines and diamond corners, gold serif
// small-caps headings, parchment panels with dark-brown text, outlined buttons and chips.
// Depends only on Dear ImGui (no game headers), so it can be previewed outside the game.

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#	define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>

#include <functional>

namespace Theme
{
	struct FontFiles
	{
		const char* regular = nullptr;  // e.g. C:\Windows\Fonts\georgia.ttf
		const char* bold = nullptr;
		const char* italic = nullptr;
		const char* sansRegular = nullptr;  // e.g. C:\Windows\Fonts\segoeui.ttf (used by sans-serif styles)
		const char* sansBold = nullptr;
		const char* sansItalic = nullptr;
		// modern look (bundled with the mod; any missing file falls back to the ones above)
		const char* display = nullptr;     // Cinzel SemiBold: titles
		const char* uiRegular = nullptr;   // Inter Regular: body text
		const char* uiSemiBold = nullptr;  // Inter SemiBold: labels, names
		const char* uiItalic = nullptr;    // Inter Italic
	};

	// ---- styles ("looks"): call SetStyle before Setup
	int         StyleCount();
	const char* StyleName(int a_index);
	const char* StyleBlurb(int a_index);
	int         CurrentStyle();
	void        SetStyle(int a_index);

	// Call once after ImGui::CreateContext(). a_scale ~ screen height / 1080, a_baseSize in px at 1080p.
	void Setup(float a_scale, float a_baseSize, const FontFiles& a_files);
	float Scale();
	float S(float a_px);  // a_px at 1080p -> pixels on this screen

	ImFont* Body();
	ImFont* Bold();
	ImFont* Italic();
	ImFont* Label();  // Inter SemiBold in the modern look

	// ---- colours (ImU32)
	ImU32 Gold();
	ImU32 GoldDim();
	ImU32 Ink();       // dark brown text on parchment
	ImU32 InkSoft();
	ImU32 Cream();     // light text on charcoal
	ImU32 Well();        // background behind a thumbnail
	ImU32 CardBorder();
	ImU32 CardShadow();
	ImU32 Muted();       // "loading..." text
	ImU32 ViewerBg();    // behind the full-size picture
	ImU32 Warning();
	ImU32 Parchment();   // panel colour
	ImU32 TabIdle();     // folder tab, not selected
	ImU32 TabHover();
	float Rounding();    // corner radius for cards (pixels)
	// Thumbnail card: CardBack before the picture, CardFront after it; CardInset = padding around the picture.
	float CardInset();
	void  CardBack(ImDrawList* a_dl, const ImVec2& a_a, const ImVec2& a_b, bool a_hovered);
	void  CardFront(ImDrawList* a_dl, const ImVec2& a_a, const ImVec2& a_b, bool a_hovered);
	// Lets the look generate its surface textures (RGBA8, tiling). Call before Setup. Optional.
	void  SetTextureFactory(std::function<ImTextureID(int, int, const unsigned char*)> a_factory);

	// ---- window chrome
	// Begins a borderless themed window (NoTitleBar). a_open may be null. Returns ImGui::Begin's result.
	bool BeginWindow(const char* a_id, bool* a_open, int a_extraFlags = 0);
	// Window background + gold chrome for the current window (used by BeginWindow and themed popups).
	void DrawChrome();
	void EndWindow();
	struct Stat
	{
		const char* label;
		const char* value;
		const char* tooltip = nullptr;  // set = the box is a button (Header reports it in a_clickedStat)
	};
	// Gold small-caps title + italic subtitle on the left, stat boxes and a close box on the right.
	// Returns true when the close box was clicked.
	// Makes the next Header's subtitle open something: 1 = the subtitle is a link, 2 = an "Open folder" button
	// after it. *a_clicked is set when used. One-shot (applies to the next Header call only).
	void SetSubtitleAction(int a_mode, bool* a_clicked, const char* a_tooltip = nullptr);
	bool Header(const char* a_title, const char* a_subtitle, const Stat* a_stats = nullptr, int a_statCount = 0, bool a_closeBox = true, int* a_clickedStat = nullptr);

	// ---- parchment panel (child window); text inside is dark ink
	// a_scroll: scrolls, with the wood-and-brass scroll bar; a_alwaysBar: the bar shows even when everything fits
	bool BeginParchment(const char* a_id, const ImVec2& a_size, bool a_scroll = false, bool a_autoHeight = false, bool a_alwaysBar = false);
	// The next BeginParchment is a dark gallery wall (light text) - used by looks whose picture cards bring their own mat.
	void NextPanelDark();
	// 0 = classic cards (the gallery draws them), 1 matted in wood with the name on the mat, 2 gold corners + name plaque,
	// 3 gold-lined tile with the name underneath
	int  CardStyle();
	bool PlaqueTabs();  // tabs are plaques with icons (Tab) instead of folder tabs
	// Plaque tab (CardStyle looks). a_icon: 0 grid, 1 landscape, 2 square, 3 portrait, 4 picture, 5 frame, 6 bell, -1 none
	bool Tab(const char* a_label, bool a_selected, float a_h, int a_icon);
	// On/off switch. Returns true when it was flipped.
	bool Toggle(const char* a_id, bool* a_v);
	bool ToggleSwitches();  // the look uses switches instead of On / Off chips
	float ButtonLabelWidth(const char* a_label);  // width of a button/chip label as the look writes it
	// Modern look: dim and vignette the game behind the next window (one-shot; the main gallery uses it).
	void SetBackdrop();
	// Row of key hints ("[Esc] Close") along the bottom of a window; FooterHeight() = room to leave for it (0 = look has none).
	float FooterHeight();
	void  KeyHints(const char* const* a_keys, const char* const* a_labels, int a_count);
	float KeyHintsAt(ImDrawList* a_dl, const ImVec2& a_pos, const char* const* a_keys, const char* const* a_labels, int a_count, bool a_draw = true);
	bool  IsModern();  // the Modern look is active
	// Settings description pane (modern look): rows report the hovered label/help, the pane shows it.
	bool  DescriptionPane();
	void  SetHoveredHelp(const char* a_label, const char* a_help);
	void  DrawDescription(const char* a_fallbackTitle, const char* a_fallbackText);
	// Height of a picture card for a cell a_cell wide (CardStyle 1-3), and the card itself.
	float PictureCardHeight(float a_cell);
	void  PictureCard(ImDrawList* a_dl, const ImVec2& a_pos, float a_cell, ImTextureID a_tex, float a_w, float a_h,
		 const char* a_name, const char* a_placeholder, bool a_hovered, const char* a_detail = nullptr);
	void EndParchment();
	void SectionLabel(const char* a_text);  // small-caps label with a rule under it (inside parchment)

	// ---- controls
	bool Button(const char* a_label, const ImVec2& a_size = ImVec2(0, 0), bool a_primary = false);
	bool Chip(const char* a_label, bool a_selected = false);  // outlined chip for parchment panels
	bool SliderInt(const char* a_label, int* a_v, int a_min, int a_max, const char* a_format, float a_width);
	bool SliderFloat(const char* a_label, float* a_v, float a_min, float a_max, const char* a_format, float a_width);

	// Text helpers
	void SmallCaps(ImDrawList* a_dl, ImFont* a_font, float a_size, const ImVec2& a_pos, ImU32 a_col, const char* a_text);
	float SmallCapsWidth(ImFont* a_font, float a_size, const char* a_text);
	void TextItalic(const char* a_fmt, ...);
	void TextColoredWrapped(ImU32 a_col, const char* a_text);
}
