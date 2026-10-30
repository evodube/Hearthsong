#include "Theme.h"

#include <imgui_internal.h>

#include <cctype>
#include <cfloat>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

namespace Theme
{
	namespace
	{
		constexpr ImU32 C(int r, int g, int b, int a = 255) { return IM_COL32(r, g, b, a); }

		enum class Chrome
		{
			InsetDiamonds,   // thin inset line with diamond corners
			CornerBrackets,  // L-shaped brackets in the corners
			Rules,           // Skyrim-menu style: fading horizontal rules top and bottom
			DoubleLine,      // two inset lines
			Rounded,         // soft rounded outline
			Rivets,          // double line with rivets
		};

		enum class Title
		{
			SmallCaps,
			Upper,   // all caps, tracked
			Normal,
		};

		// Extra surface treatment (all off = the flat look)
		struct Fx
		{
			bool textured = false;   // parchment fibre + leather grain textures
			bool bevel = false;      // lit/shaded edges on frames, boxes and buttons
			bool shadows = false;    // soft drop shadows under panels and cards
			bool deckle = false;     // torn parchment edges
			int  frame = 0;          // 0 thin line, 1 metallic gold band with corner pieces, 2 stitched leather
			bool medallion = false;  // gold guild medallion before the title
			bool seal = false;       // wax seal on parchment panels
			bool gilt = false;       // small gilt frames around thumbnails
			int  hall = 0;           // "gallery hall" family: 1 Hall of Memories, 2 Royal Archive, 3 Carved Ledger
		};

		struct Look
		{
			const char* name;
			const char* blurb;
			bool        sans;  // font family
			Title       title;
			Chrome      chrome;
			float       rounding;       // buttons, chips, cards
			float       windowRounding;
			// window
			ImU32 winMid, winEdge, winOuter;
			ImU32 accent, accentDim, accentFaint;
			ImU32 text, textDim;
			ImU32 frameBg, frameHov, frameAct, grab, grabAct;
			ImU32 btnBg, btnHov, btnAct, btnText, btnBorder;
			ImU32 priBg, priHov, priAct, priText, priBorder;
			ImU32 boxBg, closeHov, closeX;
			// panel
			ImU32 panel, panelShade, panelBorder, ink, inkSoft, panelRule;
			ImU32 pFrame, pFrameHov, pFrameAct, pGrab, pGrabAct;
			ImU32 pBtnHov, pBtnAct, pSel, pSelHov, pSelText, pBorder;
			ImU32 pPriBg, pPriHov, pPriText;
			// misc
			ImU32 well, cardBorder, shadow, muted, viewerBg, warning;
			Fx    fx{};
		};

		// clang-format off
		const Look kLooks[] = {
			{ "Guild Ledger", "Charcoal and gold with parchment panels",
				false, Title::SmallCaps, Chrome::InsetDiamonds, 2.0f, 0.0f,
				C(40,38,36), C(22,21,20), C(10,10,10),
				C(206,174,116), C(128,104,66), C(92,76,52),
				C(230,218,194), C(168,154,128),
				C(22,21,20), C(52,47,40), C(66,58,46), C(176,146,94), C(206,174,116),
				C(46,41,34), C(76,64,46), C(98,80,54), C(230,218,194), C(128,104,66),
				C(122,96,56), C(150,120,72), C(170,138,84), C(250,240,218), C(206,174,116),
				C(24,23,22), C(70,58,40), C(230,218,194),
				C(224,208,170), C(150,120,74,110), C(150,122,80), C(56,38,22), C(112,88,60), C(150,122,80,160),
				C(205,186,144), C(196,175,130), C(186,164,118), C(92,66,38), C(70,48,26),
				C(92,66,38,45), C(92,66,38,80), C(92,66,38), C(108,78,46), C(244,232,205), C(112,88,60),
				C(92,66,38), C(112,82,48), C(244,232,205),
				C(34,28,22), C(112,88,60), C(90,66,36,70), C(170,150,120), C(16,15,14), C(140,40,30) },

			{ "Nordic Stone", "Cold slate and frost-silver, carved corner brackets",
				false, Title::SmallCaps, Chrome::CornerBrackets, 0.0f, 0.0f,
				C(52,58,64), C(28,31,35), C(12,14,16),
				C(196,214,226), C(118,134,148), C(78,88,98),
				C(226,232,238), C(150,162,172),
				C(24,27,31), C(48,54,61), C(60,68,77), C(150,170,186), C(196,214,226),
				C(42,47,53), C(62,70,79), C(78,88,99), C(226,232,238), C(110,124,138),
				C(78,102,124), C(96,124,150), C(110,140,168), C(240,246,250), C(170,196,216),
				C(30,34,38), C(64,74,84), C(226,232,238),
				C(64,71,79), C(20,24,28,90), C(104,118,132), C(232,238,243), C(160,174,186), C(130,146,160,140),
				C(44,50,56), C(54,61,68), C(62,70,78), C(176,196,212), C(206,222,234),
				C(255,255,255,18), C(255,255,255,34), C(150,176,198), C(170,196,216), C(22,28,34), C(130,146,160),
				C(78,102,124), C(96,124,150), C(240,246,250),
				C(26,29,33), C(110,124,138), C(0,0,0,90), C(150,162,172), C(14,16,18), C(232,128,110) },

			{ "Skyrim Menu", "Like the game's own menus: black, white text, fading rules",
				true, Title::Upper, Chrome::Rules, 0.0f, 0.0f,
				C(10,10,10,232), C(4,4,4,236), C(0,0,0,0),
				C(240,240,240), C(150,150,150), C(90,90,90),
				C(235,235,235), C(150,150,150),
				C(0,0,0,150), C(255,255,255,24), C(255,255,255,40), C(110,110,110), C(160,160,160),
				C(0,0,0,0), C(255,255,255,26), C(255,255,255,46), C(235,235,235), C(120,120,120),
				C(255,255,255,30), C(255,255,255,50), C(255,255,255,70), C(255,255,255), C(220,220,220),
				C(0,0,0,120), C(255,255,255,30), C(235,235,235),
				C(255,255,255,8), C(0,0,0,0), C(255,255,255,40), C(235,235,235), C(150,150,150), C(255,255,255,50),
				C(0,0,0,140), C(255,255,255,20), C(255,255,255,36), C(110,110,110), C(160,160,160),
				C(255,255,255,24), C(255,255,255,44), C(255,255,255,40), C(255,255,255,60), C(255,255,255), C(120,120,120),
				C(255,255,255,30), C(255,255,255,50), C(255,255,255),
				C(0,0,0,170), C(90,90,90), C(0,0,0,0), C(150,150,150), C(0,0,0,200), C(230,110,100) },

			{ "Daedric Ember", "Black iron and smouldering red, double-lined",
				false, Title::SmallCaps, Chrome::DoubleLine, 0.0f, 0.0f,
				C(40,18,16), C(14,7,6), C(0,0,0),
				C(232,112,58), C(140,58,32), C(92,38,24),
				C(238,214,198), C(168,128,112),
				C(18,9,8), C(54,24,18), C(70,30,22), C(200,90,46), C(240,128,70),
				C(40,18,15), C(70,28,20), C(92,36,24), C(238,214,198), C(120,50,30),
				C(150,52,26), C(182,66,32), C(206,80,40), C(255,236,222), C(232,112,58),
				C(22,10,9), C(80,30,20), C(238,214,198),
				C(46,22,19), C(0,0,0,110), C(110,44,28), C(238,214,198), C(176,128,110), C(140,58,32,160),
				C(28,13,11), C(52,22,17), C(66,28,20), C(214,98,50), C(240,128,70),
				C(232,112,58,30), C(232,112,58,56), C(150,52,26), C(182,66,32), C(255,236,222), C(120,50,30),
				C(150,52,26), C(182,66,32), C(255,236,222),
				C(18,8,7), C(110,44,28), C(0,0,0,110), C(170,120,100), C(8,4,4), C(255,140,100) },

			{ "Modern Glass", "Clean modern app look: rounded, translucent, teal accent",
				true, Title::Normal, Chrome::Rounded, 8.0f, 14.0f,
				C(30,36,46,240), C(20,24,32,240), C(0,0,0,0),
				C(94,214,200), C(70,140,136), C(60,72,88),
				C(232,238,244), C(146,158,172),
				C(255,255,255,14), C(255,255,255,26), C(255,255,255,36), C(94,214,200), C(140,236,224),
				C(255,255,255,16), C(255,255,255,30), C(255,255,255,44), C(232,238,244), C(255,255,255,30),
				C(52,170,158), C(68,192,178), C(40,146,136), C(255,255,255), C(52,170,158),
				C(255,255,255,12), C(255,255,255,30), C(232,238,244),
				C(255,255,255,10), C(0,0,0,0), C(255,255,255,22), C(232,238,244), C(146,158,172), C(255,255,255,26),
				C(255,255,255,14), C(255,255,255,24), C(255,255,255,34), C(94,214,200), C(140,236,224),
				C(255,255,255,20), C(255,255,255,34), C(52,170,158), C(68,192,178), C(255,255,255), C(255,255,255,40),
				C(52,170,158), C(68,192,178), C(255,255,255),
				C(10,12,16,200), C(255,255,255,30), C(0,0,0,70), C(146,158,172), C(10,12,16,230), C(255,128,120) },

			{ "Dwemer Brass", "Verdigris metal with brass trim and rivets",
				false, Title::SmallCaps, Chrome::Rivets, 1.0f, 0.0f,
				C(34,50,47), C(16,26,24), C(6,10,9),
				C(212,164,82), C(140,104,52), C(92,72,40),
				C(226,218,196), C(160,160,138),
				C(14,22,21), C(36,52,49), C(46,64,60), C(196,150,74), C(228,180,98),
				C(30,44,41), C(46,64,60), C(60,80,75), C(226,218,196), C(140,104,52),
				C(150,108,48), C(176,128,58), C(196,146,66), C(255,246,226), C(212,164,82),
				C(18,28,26), C(56,72,62), C(226,218,196),
				C(40,58,55), C(0,0,0,80), C(120,96,56), C(230,222,200), C(164,164,140), C(140,104,52,150),
				C(26,38,36), C(36,52,49), C(46,64,60), C(212,164,82), C(232,186,104),
				C(212,164,82,28), C(212,164,82,52), C(150,108,48), C(176,128,58), C(255,246,226), C(140,104,52),
				C(150,108,48), C(176,128,58), C(255,246,226),
				C(14,22,21), C(120,96,56), C(0,0,0,90), C(160,160,138), C(8,12,11), C(240,130,100) },

			{ "Scholar's Desk", "Light theme: warm paper, ink and red-brown accents",
				false, Title::SmallCaps, Chrome::DoubleLine, 2.0f, 0.0f,
				C(238,228,204), C(214,198,162), C(120,96,62),
				C(132,44,32), C(150,110,80), C(176,148,108),
				C(52,36,22), C(118,96,72),
				C(226,212,180), C(216,200,164), C(206,188,150), C(132,44,32), C(160,58,40),
				C(0,0,0,0), C(132,44,32,30), C(132,44,32,56), C(52,36,22), C(118,96,72),
				C(132,44,32), C(156,56,40), C(112,36,26), C(252,244,226), C(112,36,26),
				C(232,220,192), C(214,196,160), C(52,36,22),
				C(248,242,226), C(150,120,74,60), C(176,148,108), C(52,36,22), C(118,96,72), C(176,148,108,160),
				C(236,226,202), C(228,216,188), C(218,204,172), C(132,44,32), C(160,58,40),
				C(132,44,32,26), C(132,44,32,50), C(132,44,32), C(156,56,40), C(252,244,226), C(150,120,90),
				C(132,44,32), C(156,56,40), C(252,244,226),
				C(58,44,30), C(150,120,90), C(90,66,36,60), C(140,118,90), C(40,30,20), C(160,40,30) },
		};
		// clang-format on
		std::vector<Look> BuildLooks()
		{
			std::vector<Look> v(std::begin(kLooks), std::end(kLooks));
			auto              variant = [&](const char* a_name, const char* a_blurb, Fx a_fx, Chrome a_chrome) {
                Look l = kLooks[0];  // Guild Ledger colours
				l.name = a_name;
				l.blurb = a_blurb;
				l.fx = a_fx;
				l.chrome = a_chrome;
				v.push_back(l);
			};
			// A: textured + bevelled + shadows, thin gold line with diamonds
			variant("Ledger: Textured", "Parchment fibre, leather grain, lit gold edges and soft shadows",
				Fx{ true, true, true, false, 0, false, false, false }, Chrome::InsetDiamonds);
			// B: ornate metallic frame band, deckled parchment, medallion
			variant("Ledger: Gilded", "Metallic gold frame band with corner pieces, torn parchment edges, guild medallion",
				Fx{ true, true, true, true, 1, true, false, false }, Chrome::InsetDiamonds);
			// C: guild card - stitched leather, wax seal, medallion
			variant("Ledger: Guild Card", "Stitched leather cover, wax seal and medallion like a guild member card",
				Fx{ true, true, true, true, 2, true, true, false }, Chrome::InsetDiamonds);
			// D: gallery wall - thumbnails as gilt-framed paintings on parchment
			variant("Ledger: Gallery Wall", "Every thumbnail in a little gilt frame, raised cards, embossed buttons",
				Fx{ true, true, true, false, 0, false, false, true }, Chrome::InsetDiamonds);
			// Gallery hall family: near-black stone, gold deco lines, tracked capitals, plaque tabs, switches
			auto hall = [&](const char* a_name, const char* a_blurb, int a_hall) {
				Look l = kLooks[0];
				l.name = a_name;
				l.blurb = a_blurb;
				l.title = Title::Upper;
				l.chrome = Chrome::DoubleLine;
				l.rounding = 0.0f;
				l.winMid = C(34, 31, 28);
				l.winEdge = C(15, 14, 13);
				l.winOuter = C(0, 0, 0);
				l.accent = C(208, 172, 108);
				l.accentDim = C(152, 120, 72);
				l.accentFaint = C(96, 77, 50);
				l.text = C(234, 222, 198);
				l.textDim = C(172, 158, 132);
				l.btnBg = C(30, 27, 24);
				l.btnHov = C(54, 45, 34);
				l.btnAct = C(66, 54, 38);
				l.btnText = C(228, 208, 166);
				l.btnBorder = C(152, 120, 72);
				l.priBg = C(74, 57, 34);
				l.priHov = C(96, 74, 42);
				l.priAct = C(108, 84, 48);
				l.priText = C(252, 238, 208);
				l.priBorder = C(218, 182, 114);
				l.boxBg = C(24, 22, 20);
				l.closeHov = C(62, 50, 36);
				l.panel = C(231, 218, 188);
				l.panelShade = C(150, 118, 70, 80);
				l.panelBorder = C(150, 122, 80);
				l.ink = C(50, 35, 20);
				l.inkSoft = C(112, 90, 62);
				l.well = C(18, 16, 14);
				l.fx = Fx{ true, true, true, false, 0, false, false, false, a_hall };
				v.push_back(l);
			};
			hall("Hall of Memories", "Centred engraved title, every picture matted in a wooden frame with its name on the mat", 1);
			hall("Royal Archive", "Emblem title bar, gold-cornered pictures with name plaques underneath", 2);
			hall("Carved Ledger", "Carved wood border, title on a plaque, gold-lined picture tiles on parchment", 3);
			{  // Modern: smoked-glass panels over the dimmed game, hairlines, one warm accent, big pictures
				Look l = kLooks[0];
				l.name = "Modern";
				l.blurb = "Smoked glass over the dimmed game, Cinzel titles, big pictures with names on them, smooth hover";
				l.sans = true;
				l.title = Title::Upper;
				l.chrome = Chrome::Rounded;
				l.rounding = 6.0f;
				l.windowRounding = 12.0f;
				l.winMid = C(22, 22, 25, 238);
				l.winEdge = C(13, 13, 15, 242);
				l.winOuter = C(0, 0, 0, 0);
				l.accent = C(218, 184, 124);
				l.accentDim = C(150, 126, 88);
				l.accentFaint = C(255, 255, 255, 22);
				l.text = C(238, 234, 226);
				l.textDim = C(150, 146, 138);
				l.frameBg = C(255, 255, 255, 12);
				l.frameHov = C(255, 255, 255, 20);
				l.frameAct = C(255, 255, 255, 28);
				l.grab = C(218, 184, 124);
				l.grabAct = C(236, 206, 150);
				l.btnBg = C(255, 255, 255, 6);
				l.btnHov = C(255, 255, 255, 18);
				l.btnAct = C(255, 255, 255, 28);
				l.btnText = C(232, 226, 216);
				l.btnBorder = C(255, 255, 255, 42);
				l.priBg = C(206, 172, 112);
				l.priHov = C(224, 192, 134);
				l.priAct = C(188, 154, 98);
				l.priText = C(26, 21, 14);
				l.priBorder = C(232, 202, 146);
				l.boxBg = C(255, 255, 255, 6);
				l.closeHov = C(255, 255, 255, 20);
				l.closeX = C(232, 226, 216);
				l.panel = C(0, 0, 0, 70);
				l.panelShade = C(0, 0, 0, 0);
				l.panelBorder = C(255, 255, 255, 16);
				l.ink = C(238, 234, 226);
				l.inkSoft = C(150, 146, 138);
				l.panelRule = C(255, 255, 255, 16);
				l.well = C(10, 10, 12);
				l.cardBorder = C(255, 255, 255, 26);
				l.shadow = C(0, 0, 0, 120);
				l.muted = C(124, 120, 114);
				l.viewerBg = C(6, 6, 7, 245);
				l.fx = Fx{ false, false, false, false, 0, false, false, false, 4 };
				v.push_back(l);
			}
			return v;
		}
		const std::vector<Look>& Looks()
		{
			static const std::vector<Look> looks = BuildLooks();
			return looks;
		}
		int kLookCount = static_cast<int>(Looks().size());

		int         g_look = 0;
		const Look* L = &Looks()[0];

		// ---- generated textures (luminance maps tinted with the palette; tile seamlessly)
		std::function<ImTextureID(int, int, const unsigned char*)> g_texFactory;
		ImTextureID g_texParch = 0, g_texGrain = 0;
		constexpr int kTex = 256;

		float Hash(int x, int y, int seed)
		{
			unsigned h = static_cast<unsigned>(x) * 374761393u + static_cast<unsigned>(y) * 668265263u + static_cast<unsigned>(seed) * 2246822519u;
			h = (h ^ (h >> 13)) * 1274126177u;
			return static_cast<float>((h ^ (h >> 16)) & 0xFFFF) / 65535.0f;
		}

		// Periodic value noise (period = cells) so the texture tiles.
		float Noise(float x, float y, int cells, int seed)
		{
			const int   x0 = static_cast<int>(std::floor(x)), y0 = static_cast<int>(std::floor(y));
			const float fx = x - x0, fy = y - y0;
			auto        at = [&](int ix, int iy) { return Hash(((ix % cells) + cells) % cells, ((iy % cells) + cells) % cells, seed); };
			const float sx = fx * fx * (3 - 2 * fx), sy = fy * fy * (3 - 2 * fy);
			const float a = at(x0, y0) + (at(x0 + 1, y0) - at(x0, y0)) * sx;
			const float b = at(x0, y0 + 1) + (at(x0 + 1, y0 + 1) - at(x0, y0 + 1)) * sx;
			return a + (b - a) * sy;
		}

		float Fbm(float u, float v, int baseCells, int octaves, int seed)
		{
			float sum = 0, amp = 0.5f, norm = 0;
			int   cells = baseCells;
			for (int o = 0; o < octaves; ++o) {
				sum += amp * Noise(u * cells, v * cells, cells, seed + o * 17);
				norm += amp;
				amp *= 0.5f;
				cells *= 2;
			}
			return sum / norm;
		}

		void MakeTextures()
		{
			if (!g_texFactory || g_texParch) {
				return;
			}
			std::vector<unsigned char> px(kTex * kTex * 4);
			// parchment: soft blotches + long fibres + fine grain, mean ~0.94
			for (int y = 0; y < kTex; ++y) {
				for (int x = 0; x < kTex; ++x) {
					const float u = x / static_cast<float>(kTex), v = y / static_cast<float>(kTex);
					const float blot = Fbm(u, v, 3, 4, 11);
					const float fibre = Fbm(u * 1.0f, v * 1.0f + 0.15f * Fbm(u, v, 4, 2, 5), 16, 3, 23);
					const float grain = Hash(x, y, 7);
					float       l = 0.80f + 0.16f * blot + 0.05f * (fibre > 0.62f ? (fibre - 0.62f) * 4.0f : 0.0f) + 0.035f * (grain - 0.5f);
					l = std::fmin(1.0f, std::fmax(0.0f, l));
					auto* o = &px[(y * kTex + x) * 4];
					o[0] = o[1] = o[2] = 255;
					o[3] = static_cast<unsigned char>((1.0f - l) * 255.0f);
				}
			}
			g_texParch = g_texFactory(kTex, kTex, px.data());
			// leather / dark stone grain: pebbled, mean ~0.9
			for (int y = 0; y < kTex; ++y) {
				for (int x = 0; x < kTex; ++x) {
					const float u = x / static_cast<float>(kTex), v = y / static_cast<float>(kTex);
					const float pebble = Fbm(u, v, 24, 2, 31);
					const float broad = Fbm(u, v, 2, 3, 41);
					const float grain = Hash(x, y, 9);
					float       l = 0.74f + 0.20f * pebble + 0.10f * broad + 0.05f * (grain - 0.5f);
					l = std::fmin(1.0f, std::fmax(0.0f, l));
					auto* o = &px[(y * kTex + x) * 4];
					o[0] = o[1] = o[2] = 255;
					o[3] = static_cast<unsigned char>((1.0f - l) * 255.0f);
				}
			}
			g_texGrain = g_texFactory(kTex, kTex, px.data());
		}

		void Tiled(ImDrawList* a_dl, ImTextureID a_tex, const ImVec2& a_a, const ImVec2& a_b, ImU32 a_tint, float a_rounding = 0.0f)
		{
			const float  tile = 256.0f * 1.25f;
			const ImVec2 uv1((a_b.x - a_a.x) / tile, (a_b.y - a_a.y) / tile);
			if (a_rounding > 0.0f) {
				a_dl->AddImageRounded(a_tex, a_a, a_b, ImVec2(0, 0), uv1, a_tint, a_rounding);
			} else {
				a_dl->AddImage(a_tex, a_a, a_b, ImVec2(0, 0), uv1, a_tint);
			}
		}

		float   g_s = 1.0f;
		float   g_base = 18.0f;
		ImFont* g_body = nullptr;
		ImFont* g_bold = nullptr;
		ImFont* g_italic = nullptr;
		ImFont* g_label = nullptr;   // labels / names (Inter SemiBold in the modern look, else = body)
		bool    g_backdrop = false;
		const char* g_hoverLabel = nullptr;
		const char* g_hoverHelp = nullptr;
		int     g_parchmentDepth = 0;
		std::vector<bool> g_parchmentScroll;  // per open parchment: does it scroll (draw our scroll bar)
		std::vector<bool> g_parchmentDark;    // per open parchment: dark gallery wall instead of paper
		bool              g_nextDark = false;
		bool              InDarkPanel() { return !g_parchmentDark.empty() && g_parchmentDark.back(); }

		ImVec4 V(ImU32 c) { return ImGui::ColorConvertU32ToFloat4(c); }

		bool FileExists(const char* a_path)
		{
			if (!a_path || !*a_path) {
				return false;
			}
			FILE* f = std::fopen(a_path, "rb");
			if (!f) {
				return false;
			}
			std::fclose(f);
			return true;
		}

		ImFont* LoadFont(const char* a_path, float a_size)
		{
			auto& io = ImGui::GetIO();
			if (FileExists(a_path)) {
				ImFontConfig fc;
				fc.OversampleH = 2;
				fc.OversampleV = 2;
				if (auto* f = io.Fonts->AddFontFromFileTTF(a_path, a_size, &fc)) {
					return f;
				}
			}
			return nullptr;
		}

		void Diamond(ImDrawList* a_dl, const ImVec2& a_c, float a_r, ImU32 a_col)
		{
			a_dl->AddQuadFilled(a_c + ImVec2(0, -a_r), a_c + ImVec2(a_r, 0), a_c + ImVec2(0, a_r), a_c + ImVec2(-a_r, 0), a_col);
		}

		ImU32 WithAlpha(ImU32 a_col, int a_alpha)
		{
			return (a_col & 0x00FFFFFF) | (static_cast<ImU32>(a_alpha) << 24);
		}

		// Horizontal rule that fades out at both ends, with a small diamond in the middle.
		void FadingRule(ImDrawList* a_dl, float a_x0, float a_x1, float a_y, ImU32 a_col, float a_thick, bool a_diamond)
		{
			const float  mid = (a_x0 + a_x1) * 0.5f;
			const ImU32  clear = WithAlpha(a_col, 0);
			a_dl->AddRectFilledMultiColor(ImVec2(a_x0, a_y), ImVec2(mid, a_y + a_thick), clear, a_col, a_col, clear);
			a_dl->AddRectFilledMultiColor(ImVec2(mid, a_y), ImVec2(a_x1, a_y + a_thick), a_col, clear, clear, a_col);
			if (a_diamond) {
				Diamond(a_dl, ImVec2(mid, a_y + a_thick * 0.5f), a_thick * 3.5f, a_col);
			}
		}
	}

	int         StyleCount() { return kLookCount; }
	const char* StyleName(int a_i) { return Looks()[(a_i % kLookCount + kLookCount) % kLookCount].name; }
	const char* StyleBlurb(int a_i) { return Looks()[(a_i % kLookCount + kLookCount) % kLookCount].blurb; }
	void        SetTextureFactory(std::function<ImTextureID(int, int, const unsigned char*)> a_factory) { g_texFactory = std::move(a_factory); }
	int         CurrentStyle() { return g_look; }

	void SetStyle(int a_i)
	{
		g_look = (a_i % kLookCount + kLookCount) % kLookCount;
		L = &Looks()[g_look];
	}

	void Setup(float a_scale, float a_baseSize, const FontFiles& a_files)
	{
		g_s = a_scale < 0.5f ? 0.5f : a_scale;
		g_base = a_baseSize;
		auto&       io = ImGui::GetIO();
		const float size = g_base * g_s;
		io.Fonts->Clear();  // Setup may run again when the interface size or style changes

		const char* reg = L->sans && a_files.sansRegular ? a_files.sansRegular : a_files.regular;
		const char* bold = L->sans && a_files.sansBold ? a_files.sansBold : a_files.bold;
		const char* ital = L->sans && a_files.sansItalic ? a_files.sansItalic : a_files.italic;
		g_body = LoadFont(reg, size);
		if (!g_body) {
			ImFontConfig fc;
			fc.SizePixels = size;
			g_body = io.Fonts->AddFontDefault(&fc);
		}
		g_bold = LoadFont(bold, size * 1.9f);  // big: used for titles and numbers (drawn smaller when needed)
		if (!g_bold) {
			g_bold = g_body;
		}
		g_italic = LoadFont(ital, size * 0.92f);
		if (!g_italic) {
			g_italic = g_body;
		}
		g_label = g_body;
		if (L->fx.hall == 4) {  // modern: Inter for text, Cinzel for titles (bundled fonts; fall back if missing)
			if (ImFont* f = LoadFont(a_files.uiRegular, size * 0.92f)) {
				g_body = f;
			}
			if (ImFont* f = LoadFont(a_files.display, size * 1.9f)) {
				g_bold = f;
			}
			if (ImFont* f = LoadFont(a_files.uiItalic, size * 0.88f)) {
				g_italic = f;
			}
			if (ImFont* f = LoadFont(a_files.uiSemiBold, size * 0.92f)) {
				g_label = f;
			}
		}
		io.FontDefault = g_body;
		if (L->fx.textured) {
			MakeTextures();
		}

		ImGuiStyle st;
		st.WindowPadding = ImVec2(26, 22);
		st.FramePadding = ImVec2(14, 9);
		st.CellPadding = ImVec2(8, 6);
		st.ItemSpacing = ImVec2(12, 10);
		st.ItemInnerSpacing = ImVec2(10, 6);
		st.IndentSpacing = 20;
		st.ScrollbarSize = 16;
		st.GrabMinSize = 22;
		st.WindowBorderSize = 0;
		st.ChildBorderSize = 0;
		st.PopupBorderSize = 1;
		st.FrameBorderSize = 1;
		st.WindowRounding = L->windowRounding;
		st.ChildRounding = L->rounding;
		st.FrameRounding = L->rounding;
		st.PopupRounding = L->rounding;
		st.ScrollbarRounding = L->rounding;
		st.GrabRounding = L->rounding;
		st.WindowMinSize = ImVec2(200, 120);
		st.ScaleAllSizes(g_s);

		auto* c = st.Colors;
		c[ImGuiCol_Text] = V(L->text);
		c[ImGuiCol_TextDisabled] = V(L->textDim);
		c[ImGuiCol_WindowBg] = V(L->winEdge);
		c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
		c[ImGuiCol_PopupBg] = V(WithAlpha(L->winMid, 250));
		c[ImGuiCol_Border] = V(L->fx.hall == 4 ? IM_COL32(255, 255, 255, 40) : L->accentDim);
		c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
		c[ImGuiCol_FrameBg] = V(L->frameBg);
		c[ImGuiCol_FrameBgHovered] = V(L->frameHov);
		c[ImGuiCol_FrameBgActive] = V(L->frameAct);
		c[ImGuiCol_TitleBg] = V(L->winEdge);
		c[ImGuiCol_TitleBgActive] = V(L->winEdge);
		c[ImGuiCol_TitleBgCollapsed] = V(L->winEdge);
		c[ImGuiCol_MenuBarBg] = V(L->winMid);
		c[ImGuiCol_ScrollbarBg] = V(C(0, 0, 0, 30));
		c[ImGuiCol_ScrollbarGrab] = V(WithAlpha(L->accentDim, 170));
		c[ImGuiCol_ScrollbarGrabHovered] = V(WithAlpha(L->accentDim, 230));
		c[ImGuiCol_ScrollbarGrabActive] = V(L->accent);
		c[ImGuiCol_CheckMark] = V(L->accent);
		c[ImGuiCol_SliderGrab] = V(L->grab);
		c[ImGuiCol_SliderGrabActive] = V(L->grabAct);
		c[ImGuiCol_Button] = V(L->btnBg);
		c[ImGuiCol_ButtonHovered] = V(L->btnHov);
		c[ImGuiCol_ButtonActive] = V(L->btnAct);
		c[ImGuiCol_Header] = V(WithAlpha(L->accent, 50));
		c[ImGuiCol_HeaderHovered] = V(WithAlpha(L->accent, 90));
		c[ImGuiCol_HeaderActive] = V(WithAlpha(L->accent, 130));
		c[ImGuiCol_Separator] = V(L->accentFaint);
		c[ImGuiCol_SeparatorHovered] = V(L->accentDim);
		c[ImGuiCol_SeparatorActive] = V(L->accent);
		c[ImGuiCol_ResizeGrip] = V(WithAlpha(L->accentDim, 90));
		c[ImGuiCol_ResizeGripHovered] = V(WithAlpha(L->accentDim, 180));
		c[ImGuiCol_ResizeGripActive] = V(L->accent);
		c[ImGuiCol_Tab] = V(L->btnBg);
		c[ImGuiCol_TabHovered] = V(L->btnHov);
		c[ImGuiCol_TabActive] = V(L->btnAct);
		c[ImGuiCol_PlotLines] = V(L->accent);
		c[ImGuiCol_PlotHistogram] = V(L->accent);
		c[ImGuiCol_TextSelectedBg] = V(WithAlpha(L->accent, 90));
		c[ImGuiCol_DragDropTarget] = V(L->accent);
		c[ImGuiCol_NavHighlight] = V(L->accent);
		c[ImGuiCol_ModalWindowDimBg] = V(C(0, 0, 0, 150));
		ImGui::GetStyle() = st;
	}

	float   Scale() { return g_s; }
	float   S(float a_px) { return a_px * g_s; }
	ImFont* Body() { return g_body; }
	ImFont* Bold() { return g_bold; }
	ImFont* Italic() { return g_italic; }
	ImFont* Label() { return g_label ? g_label : g_body; }
	ImU32   Gold() { return L->accent; }
	ImU32   GoldDim() { return L->accentDim; }
	ImU32   Ink() { return L->ink; }
	ImU32   InkSoft() { return L->inkSoft; }
	ImU32   Cream() { return L->text; }
	ImU32   Well() { return L->well; }
	ImU32   CardBorder() { return L->cardBorder; }
	ImU32   Parchment() { return L->panel; }
	ImU32   TabIdle() { return L->boxBg; }
	ImU32   TabHover() { return L->closeHov; }
	ImU32   CardShadow() { return L->shadow; }
	ImU32   Muted() { return L->muted; }
	ImU32   ViewerBg() { return L->viewerBg; }
	ImU32   Warning() { return L->warning; }
	float   Rounding() { return S(L->rounding); }

	namespace
	{
		ImU32 Mix(ImU32 a_a, ImU32 a_b, float a_t)
		{
			auto ch = [&](int sh) {
				const float x = ((a_a >> sh) & 0xFF) + (static_cast<float>((a_b >> sh) & 0xFF) - ((a_a >> sh) & 0xFF)) * a_t;
				return static_cast<ImU32>(std::fmin(255.0f, std::fmax(0.0f, x)));
			};
			return ch(0) | (ch(8) << 8) | (ch(16) << 16) | (ch(24) << 24);
		}
		ImU32 Bright(ImU32 a_c, float a_t) { return Mix(a_c, IM_COL32(255, 248, 225, (a_c >> 24) & 0xFF), a_t); }
		ImU32 Dark(ImU32 a_c, float a_t) { return Mix(a_c, IM_COL32(0, 0, 0, (a_c >> 24) & 0xFF), a_t); }

		// Layered soft shadow, offset down-right.
		// Soft drop shadow: 1 px rings around the shape (never under it), fading out smoothly, a little lower than
		// the shape. (Stacked filled shapes showed as light/dark steps and darkened see-through panels.)
		void SoftShadow(ImDrawList* a_dl, const ImVec2& a_a, const ImVec2& a_b, float a_r, float a_size, int a_alpha = 70)
		{
			const int    n = (std::max)(2, static_cast<int>(a_size + 0.5f));
			const ImVec2 drop(0.0f, a_size * 0.18f);
			for (int i = 1; i <= n; ++i) {
				const float t = (i - 0.5f) / n;
				const int   al = static_cast<int>(a_alpha * 0.5f * std::pow(1.0f - t, 2.2f));
				if (al <= 0) {
					continue;
				}
				const float  g = i - 0.5f;
				const ImVec2 o = drop * t;
				a_dl->AddRect(a_a + o - ImVec2(g, g), a_b + o + ImVec2(g, g), IM_COL32(0, 0, 0, al), a_r > 0.0f ? a_r + g : 0.0f, 0, 1.5f);
			}
		}

		// Vertical gradients on any shape (rounded corners included): draw the shape, then shade the vertices it added.
		// Alpha: multiplies each vertex's alpha from a_top to a_bottom (0..1) between a_y0 and a_y1.
		void FadeVertsY(ImDrawList* a_dl, int a_first, float a_y0, float a_y1, float a_top, float a_bottom)
		{
			const float h = (std::max)(1.0f, a_y1 - a_y0);
			for (int i = a_first; i < a_dl->VtxBuffer.Size; ++i) {
				ImDrawVert& v = a_dl->VtxBuffer[i];
				const float t = std::fmin(1.0f, std::fmax(0.0f, (v.pos.y - a_y0) / h));
				const float m = a_top + (a_bottom - a_top) * t;
				const ImU32 al = (v.col >> IM_COL32_A_SHIFT) & 0xFF;
				v.col = (v.col & ~IM_COL32_A_MASK) | (static_cast<ImU32>(al * m + 0.5f) << IM_COL32_A_SHIFT);
			}
		}
		// Colour: blends each vertex's colour from a_top to a_bottom, keeping its alpha (anti-aliased edges stay soft).
		void TintVertsY(ImDrawList* a_dl, int a_first, float a_y0, float a_y1, ImU32 a_top, ImU32 a_bottom)
		{
			const float h = (std::max)(1.0f, a_y1 - a_y0);
			for (int i = a_first; i < a_dl->VtxBuffer.Size; ++i) {
				ImDrawVert& v = a_dl->VtxBuffer[i];
				const float t = std::fmin(1.0f, std::fmax(0.0f, (v.pos.y - a_y0) / h));
				ImU32       c = 0;
				for (int sh = 0; sh < 24; sh += 8) {
					const float x = ((a_top >> sh) & 0xFF) + ((((a_bottom >> sh) & 0xFF) - static_cast<float>((a_top >> sh) & 0xFF)) * t);
					c |= static_cast<ImU32>(x + 0.5f) << sh;
				}
				const float ta = static_cast<float>((a_top >> IM_COL32_A_SHIFT) & 0xFF), ba = static_cast<float>((a_bottom >> IM_COL32_A_SHIFT) & 0xFF);
				const float own = static_cast<float>((v.col >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
				c |= static_cast<ImU32>((ta + (ba - ta) * t) * own + 0.5f) << IM_COL32_A_SHIFT;
				v.col = c;
			}
		}
		// A highlight (or shade) across the top of a rounded shape: same rounded corners, fades to nothing by a_h.
		void TopGlow(ImDrawList* a_dl, const ImVec2& a_a, const ImVec2& a_b, float a_r, float a_h, ImU32 a_col)
		{
			const float  h = std::fmin(a_h, a_b.y - a_a.y);
			const int    first = a_dl->VtxBuffer.Size;
			a_dl->AddRectFilled(a_a, ImVec2(a_b.x, a_a.y + h), a_col, a_r, ImDrawFlags_RoundCornersTop);
			FadeVertsY(a_dl, first, a_a.y, a_a.y + h, 1.0f, 0.0f);
		}
		// Pixel-crisp 1 px outline: the line sits on the pixel grid instead of between two pixels.
		void CrispRect(ImDrawList* a_dl, const ImVec2& a_a, const ImVec2& a_b, ImU32 a_col, float a_r, float a_t)
		{
			const ImVec2 a(std::floor(a_a.x) + 0.5f, std::floor(a_a.y) + 0.5f), b(std::ceil(a_b.x) - 0.5f, std::ceil(a_b.y) - 0.5f);
			a_dl->AddRect(a, b, a_col, a_r, 0, a_t);
		}

		// Raised (light top-left, dark bottom-right) or sunken edge.
		void Bevel(ImDrawList* a_dl, const ImVec2& a_a, const ImVec2& a_b, float a_t, bool a_sunken, int a_light = 60, int a_dark = 90)
		{
			const ImU32 hi = IM_COL32(255, 244, 214, a_light), lo = IM_COL32(0, 0, 0, a_dark);
			const ImU32 tl = a_sunken ? lo : hi, br = a_sunken ? hi : lo;
			const float h = a_t * 0.5f;
			a_dl->AddLine(ImVec2(a_a.x + h, a_b.y - h), ImVec2(a_a.x + h, a_a.y + h), tl, a_t);
			a_dl->AddLine(ImVec2(a_a.x + h, a_a.y + h), ImVec2(a_b.x - h, a_a.y + h), tl, a_t);
			a_dl->AddLine(ImVec2(a_b.x - h, a_a.y + h), ImVec2(a_b.x - h, a_b.y - h), br, a_t);
			a_dl->AddLine(ImVec2(a_b.x - h, a_b.y - h), ImVec2(a_a.x + h, a_b.y - h), br, a_t);
		}

		// Metallic gold band: nested lines dark -> bright -> dark, lit from the top-left.
		void GoldBand(ImDrawList* a_dl, const ImVec2& a_a, const ImVec2& a_b, float a_w)
		{
			const int   n = 6;
			const float step = a_w / n;
			const float prof[n] = { 0.10f, 0.55f, 1.00f, 0.75f, 0.35f, 0.05f };
			for (int i = 0; i < n; ++i) {
				const ImVec2 o(step * (i + 0.5f), step * (i + 0.5f));
				const ImU32  base = Mix(Dark(L->accentDim, 0.50f), Mix(L->accent, IM_COL32(255, 226, 150, 255), 0.25f), prof[i]);
				const ImU32  hi = Mix(base, IM_COL32(255, 230, 160, 255), 0.18f), lo = Dark(base, 0.30f);
				const ImVec2 a = a_a + o, b = a_b - o;
				a_dl->AddLine(ImVec2(a.x, b.y), a, hi, step + 0.6f);
				a_dl->AddLine(a, ImVec2(b.x, a.y), hi, step + 0.6f);
				a_dl->AddLine(ImVec2(b.x, a.y), b, lo, step + 0.6f);
				a_dl->AddLine(b, ImVec2(a.x, b.y), lo, step + 0.6f);
			}
		}

		void CornerPiece(ImDrawList* a_dl, const ImVec2& a_c, float a_r)
		{
			const ImVec2 a = a_c - ImVec2(a_r, a_r), b = a_c + ImVec2(a_r, a_r);
			a_dl->AddRectFilled(a, b, Dark(L->winEdge, 0.2f));
			GoldBand(a_dl, a, b, a_r * 0.45f);
			Diamond(a_dl, a_c, a_r * 0.42f, Bright(L->accent, 0.15f));
			Diamond(a_dl, a_c - ImVec2(a_r * 0.06f, a_r * 0.06f), a_r * 0.16f, IM_COL32(255, 246, 220, 200));
		}

		void Stitches(ImDrawList* a_dl, const ImVec2& a_a, const ImVec2& a_b, ImU32 a_col, float a_len, float a_gap, float a_t)
		{
			auto run = [&](ImVec2 p0, ImVec2 p1) {
				const ImVec2 d = p1 - p0;
				const float  len = std::sqrt(d.x * d.x + d.y * d.y);
				const ImVec2 u = d / len;
				for (float t = 0; t + a_len <= len; t += a_len + a_gap) {
					a_dl->AddLine(p0 + u * t + ImVec2(0.8f, 1.0f), p0 + u * (t + a_len) + ImVec2(0.8f, 1.0f), IM_COL32(0, 0, 0, 110), a_t);
					a_dl->AddLine(p0 + u * t, p0 + u * (t + a_len), a_col, a_t);
				}
			};
			run(a_a, ImVec2(a_b.x, a_a.y));
			run(ImVec2(a_b.x, a_a.y), a_b);
			run(a_b, ImVec2(a_a.x, a_b.y));
			run(ImVec2(a_a.x, a_b.y), a_a);
		}

		// Gold coin with a small picture-frame emblem.
		void Medallion(ImDrawList* a_dl, const ImVec2& a_c, float a_r)
		{
			a_dl->AddCircleFilled(a_c + ImVec2(a_r * 0.08f, a_r * 0.14f), a_r * 1.04f, IM_COL32(0, 0, 0, 110), 40);
			const int n = 8;
			for (int i = 0; i < n; ++i) {  // radial gradient, highlight towards top-left
				const float t = static_cast<float>(i) / (n - 1);
				const float r = a_r * (1.0f - t * 0.92f);
				a_dl->AddCircleFilled(a_c - ImVec2(a_r, a_r) * (0.18f * t), r, Mix(Dark(L->accentDim, 0.40f), Mix(L->accent, IM_COL32(255, 228, 155, 255), 0.35f), t), 40);
			}
			a_dl->AddCircle(a_c, a_r * 0.80f, Dark(L->accentDim, 0.3f), 40, a_r * 0.06f);
			a_dl->AddCircle(a_c, a_r * 0.98f, Dark(L->accentDim, 0.45f), 40, a_r * 0.05f);
			const ImVec2 e(a_r * 0.36f, a_r * 0.28f);
			a_dl->AddRect(a_c - e, a_c + e, Dark(L->accentDim, 0.55f), 0.0f, 0, a_r * 0.09f);
			a_dl->AddTriangleFilled(a_c + ImVec2(-e.x * 0.7f, e.y * 0.7f), a_c + ImVec2(-e.x * 0.05f, -e.y * 0.35f), a_c + ImVec2(e.x * 0.6f, e.y * 0.7f), Dark(L->accentDim, 0.45f));
		}

		// Red wax seal with an uneven rim.
		void WaxSeal(ImDrawList* a_dl, const ImVec2& a_c, float a_r)
		{
			const ImU32 wax = IM_COL32(140, 30, 26, 255);
			a_dl->AddCircleFilled(a_c + ImVec2(a_r * 0.1f, a_r * 0.16f), a_r * 1.05f, IM_COL32(40, 10, 5, 90), 32);
			a_dl->PathClear();
			for (int i = 0; i < 28; ++i) {
				const float ang = i / 28.0f * 6.2831853f;
				const float rr = a_r * (0.92f + 0.10f * Hash(i, 3, 77));
				a_dl->PathLineTo(a_c + ImVec2(std::cos(ang), std::sin(ang)) * rr);
			}
			a_dl->PathFillConvex(wax);
			a_dl->AddCircleFilled(a_c, a_r * 0.66f, Dark(wax, 0.18f), 32);
			a_dl->AddCircle(a_c, a_r * 0.66f, Bright(wax, 0.25f), 32, a_r * 0.06f);
			a_dl->AddCircleFilled(a_c - ImVec2(a_r * 0.35f, a_r * 0.38f), a_r * 0.16f, IM_COL32(255, 200, 190, 70), 16);
			const float k = a_r * 0.28f;  // emblem: a little frame
			a_dl->AddRect(a_c - ImVec2(k, k * 0.8f), a_c + ImVec2(k, k * 0.8f), Dark(wax, 0.45f), 0.0f, 0, a_r * 0.08f);
			a_dl->AddRect(a_c - ImVec2(k, k * 0.8f) - ImVec2(0.8f, 0.8f), a_c + ImVec2(k, k * 0.8f) - ImVec2(0.8f, 0.8f), Bright(wax, 0.2f), 0.0f, 0, a_r * 0.04f);
		}

		// Torn paper edge: notches along every side, cut with the colour behind the panel.
		void DeckleEdges(ImDrawList* a_dl, const ImVec2& a_a, const ImVec2& a_b, ImU32 a_behind, float a_depth)
		{
			auto side = [&](ImVec2 p0, ImVec2 p1, ImVec2 inward, int seed) {
				const ImVec2 d = p1 - p0;
				const float  len = std::sqrt(d.x * d.x + d.y * d.y);
				const ImVec2 u = d / len;
				const float  step = a_depth * 1.6f;
				const int    n = static_cast<int>(len / step);
				ImVec2       prev = p0;
				for (int i = 1; i <= n; ++i) {
					const float  t = std::fmin(len, i * step);
					const float  dep = a_depth * (0.25f + 0.75f * Hash(i, seed, 13));
					const ImVec2 q = p0 + u * t + inward * dep;
					const ImVec2 base0 = p0 + u * ((i - 1) * step), base1 = p0 + u * t;
					a_dl->AddQuadFilled(base0 - inward * 2.0f, base1 - inward * 2.0f, q, prev, a_behind);
					prev = q;
				}
			};
			side(a_a, ImVec2(a_b.x, a_a.y), ImVec2(0, 1), 1);
			side(ImVec2(a_b.x, a_a.y), a_b, ImVec2(-1, 0), 2);
			side(a_b, ImVec2(a_a.x, a_b.y), ImVec2(0, -1), 3);
			side(ImVec2(a_a.x, a_b.y), a_a, ImVec2(1, 0), 4);
		}

		// Background fill + light/depth for a filled box (buttons, chips, stat boxes).
		void RaisedFill(ImDrawList* a_dl, const ImVec2& a_a, const ImVec2& a_b, ImU32 a_col, float a_r, bool a_pressed)
		{
			const bool  clear = (a_col >> 24) < 8;
			const ImU32 top = clear ? IM_COL32(255, 245, 220, a_pressed ? 0 : 34) : (a_pressed ? Dark(a_col, 0.15f) : Bright(a_col, 0.24f));
			const ImU32 bot = clear ? IM_COL32(60, 40, 20, a_pressed ? 40 : 28) : (a_pressed ? Bright(a_col, 0.05f) : Dark(a_col, 0.32f));
			if (a_r > S(4.0f)) {
				a_dl->AddRectFilled(a_a, a_b, Mix(top, bot, 0.5f), a_r);
			} else {
				a_dl->AddRectFilledMultiColor(a_a, a_b, top, top, bot, bot);
			}
			if (!a_pressed) {
				a_dl->AddLine(ImVec2(a_a.x + 2, a_a.y + 1.5f), ImVec2(a_b.x - 2, a_a.y + 1.5f), IM_COL32(255, 248, 228, clear ? 60 : 70), 1.0f);
			}
		}

		// ---- gallery hall ornaments
		// Art-deco corner: an outer bracket, a stepped inner bracket and a small square. a_d = (+1/-1, +1/-1) into the window.
		void DecoCorner(ImDrawList* a_dl, const ImVec2& a_c, const ImVec2& a_d, float a_s, ImU32 a_col, float a_t)
		{
			auto P = [&](float x, float y) { return ImVec2(a_c.x + a_d.x * x, a_c.y + a_d.y * y); };
			a_dl->AddLine(P(0, a_s), P(0, 0), a_col, a_t);
			a_dl->AddLine(P(0, 0), P(a_s, 0), a_col, a_t);
			const float k = a_s * 0.22f;
			a_dl->AddLine(P(k, a_s * 0.62f), P(k, k), a_col, a_t);
			a_dl->AddLine(P(k, k), P(a_s * 0.62f, k), a_col, a_t);
			const float q0 = a_s * 0.36f, q1 = a_s * 0.52f;
			const ImVec2 qa = P(q0, q0), qb = P(q1, q1);
			a_dl->AddRect(ImVec2((std::min)(qa.x, qb.x), (std::min)(qa.y, qb.y)), ImVec2((std::max)(qa.x, qb.x), (std::max)(qa.y, qb.y)), a_col, 0.0f, 0, a_t);
			Diamond(a_dl, P(a_s * 1.08f, 0), a_s * 0.07f, a_col);
			Diamond(a_dl, P(0, a_s * 1.08f), a_s * 0.07f, a_col);
		}

		// Carved block for the corners of the wood border: dark wood square, gold double outline, diamond.
		void CarvedCorner(ImDrawList* a_dl, const ImVec2& a_c, float a_r)
		{
			const ImVec2 a = a_c - ImVec2(a_r, a_r), b = a_c + ImVec2(a_r, a_r);
			a_dl->AddRectFilled(a + ImVec2(S(2), S(3)), b + ImVec2(S(2), S(3)), IM_COL32(0, 0, 0, 120));
			a_dl->AddRectFilledMultiColor(a, b, C(74, 54, 36), C(62, 45, 30), C(36, 26, 18), C(48, 35, 24));
			Bevel(a_dl, a, b, S(1.5f), false, 60, 140);
			a_dl->AddRect(a + ImVec2(S(4), S(4)), b - ImVec2(S(4), S(4)), L->accentDim, 0.0f, 0, S(1.2f));
			a_dl->AddRect(a + ImVec2(S(8), S(8)), b - ImVec2(S(8), S(8)), L->accentFaint, 0.0f, 0, S(1.0f));
			Diamond(a_dl, a_c, a_r * 0.30f, L->accent);
			Diamond(a_dl, a_c - ImVec2(a_r * 0.05f, a_r * 0.05f), a_r * 0.12f, IM_COL32(255, 244, 214, 200));
		}

		// Small gold drawing of a framed landscape picture (the Hearthframe emblem).
		void FramedPictureGlyph(ImDrawList* a_dl, const ImVec2& a_c, float a_r, ImU32 a_col)
		{
			const ImVec2 e(a_r, a_r * 0.78f);
			a_dl->AddRect(a_c - e, a_c + e, a_col, 0.0f, 0, a_r * 0.12f);
			a_dl->AddRect(a_c - e * 0.72f, a_c + e * 0.72f, a_col, 0.0f, 0, a_r * 0.06f);
			const ImVec2 i0 = a_c - e * 0.72f, i1 = a_c + e * 0.72f;
			a_dl->AddTriangleFilled(ImVec2(i0.x, i1.y), ImVec2(i0.x + (i1.x - i0.x) * 0.38f, i0.y + (i1.y - i0.y) * 0.35f), ImVec2(i0.x + (i1.x - i0.x) * 0.72f, i1.y), a_col);
			a_dl->AddTriangleFilled(ImVec2(i0.x + (i1.x - i0.x) * 0.45f, i1.y), ImVec2(i0.x + (i1.x - i0.x) * 0.72f, i0.y + (i1.y - i0.y) * 0.55f), ImVec2(i1.x, i1.y), a_col);
			a_dl->AddCircleFilled(ImVec2(i0.x + (i1.x - i0.x) * 0.78f, i0.y + (i1.y - i0.y) * 0.28f), a_r * 0.11f, a_col, 12);
		}

		// Tab icons: 0 grid (all), 1 landscape, 2 square, 3 portrait, 4 picture, 5 frame, 6 bell, -1 none
		void TabIcon(ImDrawList* a_dl, int a_icon, const ImVec2& a_c, float a_r, ImU32 a_col)
		{
			const float t = (std::max)(1.0f, a_r * 0.14f);
			switch (a_icon) {
			case 0:
				for (int y = 0; y < 2; ++y) {
					for (int x = 0; x < 2; ++x) {
						const ImVec2 o(a_c.x - a_r + x * a_r * 1.1f, a_c.y - a_r * 0.8f + y * a_r * 0.9f);
						a_dl->AddRectFilled(o, o + ImVec2(a_r * 0.9f, a_r * 0.7f), a_col);
					}
				}
				break;
			case 1:
				a_dl->AddTriangleFilled(ImVec2(a_c.x - a_r, a_c.y + a_r * 0.6f), ImVec2(a_c.x - a_r * 0.25f, a_c.y - a_r * 0.6f), ImVec2(a_c.x + a_r * 0.45f, a_c.y + a_r * 0.6f), a_col);
				a_dl->AddTriangleFilled(ImVec2(a_c.x - a_r * 0.05f, a_c.y + a_r * 0.6f), ImVec2(a_c.x + a_r * 0.5f, a_c.y - a_r * 0.25f), ImVec2(a_c.x + a_r, a_c.y + a_r * 0.6f), a_col);
				break;
			case 2:
				a_dl->AddRect(a_c - ImVec2(a_r * 0.72f, a_r * 0.72f), a_c + ImVec2(a_r * 0.72f, a_r * 0.72f), a_col, 0.0f, 0, t * 1.3f);
				break;
			case 3:
				a_dl->AddRect(a_c - ImVec2(a_r * 0.55f, a_r * 0.85f), a_c + ImVec2(a_r * 0.55f, a_r * 0.85f), a_col, 0.0f, 0, t * 1.3f);
				break;
			case 4:
				FramedPictureGlyph(a_dl, a_c, a_r * 0.92f, a_col);
				break;
			case 5:
				a_dl->AddRect(a_c - ImVec2(a_r, a_r * 0.75f), a_c + ImVec2(a_r, a_r * 0.75f), a_col, 0.0f, 0, t * 1.6f);
				a_dl->AddLine(ImVec2(a_c.x - a_r * 0.35f, a_c.y - a_r * 0.75f), ImVec2(a_c.x, a_c.y - a_r * 1.15f), a_col, t);
				a_dl->AddLine(ImVec2(a_c.x + a_r * 0.35f, a_c.y - a_r * 0.75f), ImVec2(a_c.x, a_c.y - a_r * 1.15f), a_col, t);
				break;
			case 6:
				a_dl->PathArcTo(ImVec2(a_c.x, a_c.y - a_r * 0.1f), a_r * 0.62f, 3.14159f, 6.28318f, 12);
				a_dl->PathLineTo(ImVec2(a_c.x + a_r * 0.75f, a_c.y + a_r * 0.6f));
				a_dl->PathLineTo(ImVec2(a_c.x - a_r * 0.75f, a_c.y + a_r * 0.6f));
				a_dl->PathFillConvex(a_col);
				a_dl->AddCircleFilled(ImVec2(a_c.x, a_c.y + a_r * 0.8f), a_r * 0.18f, a_col, 10);
				break;
			default:
				break;
			}
		}
	}

	// ------------------------------------------------------------------ text
	float SmallCapsWidth(ImFont* a_font, float a_size, const char* a_text)
	{
		float       w = 0.0f;
		const float smallSize = a_size * 0.78f;
		const float track = a_size * 0.06f;
		char        ch[2] = { 0, 0 };
		for (const char* p = a_text; *p; ++p) {
			const bool lower = std::islower(static_cast<unsigned char>(*p)) != 0;
			ch[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(*p)));
			w += a_font->CalcTextSizeA(lower ? smallSize : a_size, FLT_MAX, 0.0f, ch).x + track;
		}
		return w;
	}

	void SmallCaps(ImDrawList* a_dl, ImFont* a_font, float a_size, const ImVec2& a_pos, ImU32 a_col, const char* a_text)
	{
		const float smallSize = a_size * 0.78f;
		const float track = a_size * 0.06f;
		const float drop = (a_size - smallSize) * 0.80f;  // keep the small letters on the same baseline
		float       x = a_pos.x;
		char        ch[2] = { 0, 0 };
		for (const char* p = a_text; *p; ++p) {
			const bool  lower = std::islower(static_cast<unsigned char>(*p)) != 0;
			const float sz = lower ? smallSize : a_size;
			ch[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(*p)));
			a_dl->AddText(a_font, sz, ImVec2(x, a_pos.y + (lower ? drop : 0.0f)), a_col, ch);
			x += a_font->CalcTextSizeA(sz, FLT_MAX, 0.0f, ch).x + track;
		}
	}

	namespace
	{
		// A heading in the current look's style (small caps / tracked capitals / plain).
		void Heading(ImDrawList* a_dl, ImFont* a_font, float a_size, const ImVec2& a_pos, ImU32 a_col, const char* a_text)
		{
			switch (L->title) {
			case Title::SmallCaps:
				SmallCaps(a_dl, a_font, a_size, a_pos, a_col, a_text);
				break;
			case Title::Upper:
				{
					const float track = a_size * 0.14f;
					float       x = a_pos.x;
					char        ch[2] = { 0, 0 };
					for (const char* p = a_text; *p; ++p) {
						ch[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(*p)));
						a_dl->AddText(a_font, a_size * 0.86f, ImVec2(x, a_pos.y + a_size * 0.08f), a_col, ch);
						x += a_font->CalcTextSizeA(a_size * 0.86f, FLT_MAX, 0.0f, ch).x + track;
					}
					break;
				}
			case Title::Normal:
				a_dl->AddText(a_font, a_size, a_pos, a_col, a_text);
				break;
			}
		}

		float HeadingWidth(ImFont* a_font, float a_size, const char* a_text)
		{
			switch (L->title) {
			case Title::SmallCaps:
				return SmallCapsWidth(a_font, a_size, a_text);
			case Title::Upper:
				{
					float w = 0.0f;
					char  ch[2] = { 0, 0 };
					for (const char* p = a_text; *p; ++p) {
						ch[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(*p)));
						w += a_font->CalcTextSizeA(a_size * 0.86f, FLT_MAX, 0.0f, ch).x + a_size * 0.14f;
					}
					return w;
				}
			default:
				return a_font->CalcTextSizeA(a_size, FLT_MAX, 0.0f, a_text).x;
			}
		}
	}

	namespace
	{
		ImFont* LabelFont() { return g_label ? g_label : g_body; }
		bool    Modern() { return L->fx.hall == 4; }
		// 0..1 that eases towards a_on (hover fades, card lifts). Kept per item in ImGui's state storage.
		float Ease(ImGuiID a_id, bool a_on, float a_speed = 12.0f)
		{
			float*      v = ImGui::GetStateStorage()->GetFloatRef(a_id, 0.0f);
			const float k = 1.0f - std::exp(-a_speed * ImGui::GetIO().DeltaTime);
			*v += ((a_on ? 1.0f : 0.0f) - *v) * k;
			return *v;
		}
	}

	void TextItalic(const char* a_fmt, ...)
	{
		ImGui::PushFont(g_italic);
		va_list args;
		va_start(args, a_fmt);
		ImGui::TextV(a_fmt, args);
		va_end(args);
		ImGui::PopFont();
	}

	void TextColoredWrapped(ImU32 a_col, const char* a_text)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, a_col);
		ImGui::TextWrapped("%s", a_text);
		ImGui::PopStyleColor();
	}

	// ------------------------------------------------------------------ window chrome
	bool BeginWindow(const char* a_id, bool* a_open, int a_extraFlags)
	{
		const bool visible = ImGui::Begin(a_id, a_open,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoScrollWithMouse | a_extraFlags);
		DrawChrome();
		return visible;
	}

	void DrawChrome()
	{
		auto*        dl = ImGui::GetWindowDrawList();
		const ImVec2 a = ImGui::GetWindowPos();
		const ImVec2 b = a + ImGui::GetWindowSize();
		const float  wr = S(L->windowRounding);
		if (Modern()) {
			auto* bg = ImGui::GetBackgroundDrawList();
			if (g_backdrop) {  // dim + vignette the game behind the main window
				g_backdrop = false;
				const ImVec2 ds = ImGui::GetIO().DisplaySize;
				bg->AddRectFilled(ImVec2(0, 0), ds, IM_COL32(4, 4, 6, 150));
				const float e = ds.y * 0.35f;
				const ImU32 dk = IM_COL32(0, 0, 0, 150), no = IM_COL32(0, 0, 0, 0);
				bg->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(ds.x, e), dk, dk, no, no);
				bg->AddRectFilledMultiColor(ImVec2(0, ds.y - e), ds, no, no, dk, dk);
				bg->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(e, ds.y), dk, no, no, dk);
				bg->AddRectFilledMultiColor(ImVec2(ds.x - e, 0), ds, no, dk, dk, no);
			}
			SoftShadow(bg, a, b, wr, S(40.0f), 160);  // deep, soft shadow under the panel
			dl->PushClipRect(a, b, false);
			dl->AddRectFilled(a, b, L->winEdge, wr);
			// smoked-glass sheen: lighter towards the top, across the full width with the window's rounded corners
			// (it used to stop a corner-radius in from each side, leaving two hard vertical edges at the top)
			TopGlow(dl, a + ImVec2(1.0f, 1.0f), b - ImVec2(1.0f, 1.0f), (std::max)(0.0f, wr - 1.0f), S(160.0f), IM_COL32(255, 255, 255, 10));
			CrispRect(dl, a, b, IM_COL32(255, 255, 255, 26), wr, 1.0f);
			// warm accent line along the top edge, brightest in the middle
			const float  mid = (a.x + b.x) * 0.5f, half = (b.x - a.x) * 0.32f;
			const ImU32  acc = WithAlpha(L->accent, 200), clear = WithAlpha(L->accent, 0);
			dl->AddRectFilledMultiColor(ImVec2(mid - half, a.y), ImVec2(mid, a.y + S(1.5f)), clear, acc, acc, clear);
			dl->AddRectFilledMultiColor(ImVec2(mid, a.y), ImVec2(mid + half, a.y + S(1.5f)), acc, clear, clear, acc);
			dl->AddRectFilledMultiColor(ImVec2(mid - half * 0.6f, a.y + S(1.5f)), ImVec2(mid, a.y + S(26.0f)), IM_COL32(0, 0, 0, 0), WithAlpha(L->accent, 0), WithAlpha(L->accent, 0), IM_COL32(0, 0, 0, 0));
			dl->PopClipRect();
			return;
		}
		dl->PushClipRect(a, b, false);  // the window's own clip rect excludes its padding

		// background: vignette (lighter middle) or flat
		if (L->winMid != L->winEdge && wr <= 0.0f) {
			const ImU32  mid = L->winMid, edge = L->winEdge;
			const ImVec2 m = (a + b) * 0.5f;
			dl->AddRectFilledMultiColor(a, m, edge, edge, mid, edge);
			dl->AddRectFilledMultiColor(ImVec2(m.x, a.y), ImVec2(b.x, m.y), edge, edge, edge, mid);
			dl->AddRectFilledMultiColor(ImVec2(a.x, m.y), ImVec2(m.x, b.y), edge, mid, edge, edge);
			dl->AddRectFilledMultiColor(m, b, mid, edge, edge, edge);
		} else {
			dl->AddRectFilled(a, b, L->winMid, wr);
		}
		if (L->fx.textured && g_texGrain) {
			Tiled(dl, g_texGrain, a, b, IM_COL32(0, 0, 0, 255), wr);
		}
		if ((L->winOuter >> 24) != 0) {
			dl->AddRect(a, b, L->winOuter, wr, 0, S(2.0f));
		}
		if (L->fx.bevel) {
			Bevel(dl, a + ImVec2(1, 1), b - ImVec2(1, 1), S(1.5f), false, 40, 120);
		}
		if (L->fx.hall == 3) {  // carved wood border with gold inlay and carved corner blocks
			const float  w = S(15.0f);
			const ImVec2 wa = a + ImVec2(S(3.0f), S(3.0f)), wb = b - ImVec2(S(3.0f), S(3.0f));
			const ImU32  wood = C(58, 42, 28), woodDark = C(34, 25, 17);
			dl->AddRectFilled(wa, ImVec2(wb.x, wa.y + w), wood);
			dl->AddRectFilled(ImVec2(wa.x, wb.y - w), wb, woodDark);
			dl->AddRectFilled(wa, ImVec2(wa.x + w, wb.y), wood);
			dl->AddRectFilled(ImVec2(wb.x - w, wa.y), wb, woodDark);
			if (g_texGrain) {
				Tiled(dl, g_texGrain, wa, ImVec2(wb.x, wa.y + w), IM_COL32(0, 0, 0, 200));
				Tiled(dl, g_texGrain, ImVec2(wa.x, wb.y - w), wb, IM_COL32(0, 0, 0, 200));
				Tiled(dl, g_texGrain, wa, ImVec2(wa.x + w, wb.y), IM_COL32(0, 0, 0, 200));
				Tiled(dl, g_texGrain, ImVec2(wb.x - w, wa.y), wb, IM_COL32(0, 0, 0, 200));
			}
			Bevel(dl, wa, wb, S(1.5f), false, 70, 150);
			Bevel(dl, wa + ImVec2(w, w), wb - ImVec2(w, w), S(1.5f), true, 50, 160);
			dl->AddRect(wa + ImVec2(w * 0.5f, w * 0.5f), wb - ImVec2(w * 0.5f, w * 0.5f), L->accentDim, 0.0f, 0, S(1.2f));
			dl->AddRect(wa + ImVec2(w + S(6.0f), w + S(6.0f)), wb - ImVec2(w + S(6.0f), w + S(6.0f)), L->accentFaint, 0.0f, 0, S(1.0f));
			const float r = S(19.0f), o = w * 0.5f;
			CarvedCorner(dl, wa + ImVec2(o, o), r);
			CarvedCorner(dl, ImVec2(wb.x - o, wa.y + o), r);
			CarvedCorner(dl, ImVec2(wa.x + o, wb.y - o), r);
			CarvedCorner(dl, wb - ImVec2(o, o), r);
			dl->PopClipRect();
			return;
		}
		if (L->fx.hall) {  // gold double line with art-deco corners
			const ImVec2 oa = a + ImVec2(S(7.0f), S(7.0f)), ob = b - ImVec2(S(7.0f), S(7.0f));
			dl->AddRect(oa + ImVec2(1, 1), ob + ImVec2(1, 1), IM_COL32(0, 0, 0, 150), 0.0f, 0, S(1.4f));
			dl->AddRect(oa, ob, L->accentDim, 0.0f, 0, S(1.4f));
			const ImVec2 ia2 = oa + ImVec2(S(6.0f), S(6.0f)), ib2 = ob - ImVec2(S(6.0f), S(6.0f));
			dl->AddRect(ia2, ib2, L->accentFaint, 0.0f, 0, S(1.0f));
			const float s = S(46.0f), t = S(1.6f);
			// knock the plain lines out under the ornaments, then draw them
			for (int i = 0; i < 4; ++i) {
				const ImVec2 c(i & 1 ? ob.x : oa.x, i & 2 ? ob.y : oa.y);
				const ImVec2 d(i & 1 ? -1.0f : 1.0f, i & 2 ? -1.0f : 1.0f);
				DecoCorner(dl, c + ImVec2(d.x * S(3.0f), d.y * S(3.0f)), d, s, L->accent, t);
			}
			dl->PopClipRect();
			return;
		}
		if (L->fx.frame == 1) {  // metallic band with corner pieces
			const ImVec2 ba = a + ImVec2(S(5.0f), S(5.0f)), bb = b - ImVec2(S(5.0f), S(5.0f));
			dl->AddRect(ba - ImVec2(1, 1), bb + ImVec2(1, 1), IM_COL32(0, 0, 0, 160), 0.0f, 0, S(2.0f));
			GoldBand(dl, ba, bb, S(9.0f));
			dl->AddRect(ba + ImVec2(S(9.5f), S(9.5f)), bb - ImVec2(S(9.5f), S(9.5f)), IM_COL32(0, 0, 0, 120), 0.0f, 0, S(2.0f));
			const float r = S(15.0f), o = S(9.5f);
			CornerPiece(dl, ba + ImVec2(o, o), r);
			CornerPiece(dl, ImVec2(bb.x - o, ba.y + o), r);
			CornerPiece(dl, ImVec2(ba.x + o, bb.y - o), r);
			CornerPiece(dl, bb - ImVec2(o, o), r);
			dl->PopClipRect();
			return;
		}
		if (L->fx.frame == 2) {  // stitched leather
			const ImVec2 sa = a + ImVec2(S(11.0f), S(11.0f)), sb = b - ImVec2(S(11.0f), S(11.0f));
			Bevel(dl, a + ImVec2(S(5.0f), S(5.0f)), b - ImVec2(S(5.0f), S(5.0f)), S(1.2f), true, 30, 110);
			Stitches(dl, sa, sb, L->accentDim, S(9.0f), S(6.0f), S(2.0f));
			Bevel(dl, sa + ImVec2(S(7.0f), S(7.0f)), sb - ImVec2(S(7.0f), S(7.0f)), S(1.2f), false, 30, 110);
			dl->PopClipRect();
			return;
		}

		const float  inset = S(9.0f);
		const ImVec2 ia = a + ImVec2(inset, inset), ib = b - ImVec2(inset, inset);
		switch (L->chrome) {
		case Chrome::InsetDiamonds:
			{
				if (L->fx.bevel) {  // engraved line: dark groove + light lip
					dl->AddRect(ia + ImVec2(1, 1), ib + ImVec2(1, 1), IM_COL32(0, 0, 0, 150), 0.0f, 0, S(1.2f));
					dl->AddRect(ia, ib, Bright(L->accentFaint, 0.15f), 0.0f, 0, S(1.2f));
				} else {
					dl->AddRect(ia, ib, L->accentFaint, 0.0f, 0, S(1.2f));
				}
				const float r = S(6.0f);
				if (L->fx.bevel) {
					for (const ImVec2& c : { ia, ImVec2(ib.x, ia.y), ImVec2(ia.x, ib.y), ib }) {
						Diamond(dl, c + ImVec2(1.5f, 2.0f), r, IM_COL32(0, 0, 0, 140));
						Diamond(dl, c, r, L->accentDim);
						Diamond(dl, c - ImVec2(r * 0.15f, r * 0.15f), r * 0.55f, Bright(L->accent, 0.2f));
					}
					break;
				}
				Diamond(dl, ia, r, L->accentDim);
				Diamond(dl, ImVec2(ib.x, ia.y), r, L->accentDim);
				Diamond(dl, ImVec2(ia.x, ib.y), r, L->accentDim);
				Diamond(dl, ib, r, L->accentDim);
				break;
			}
		case Chrome::CornerBrackets:
			{
				dl->AddRect(ia, ib, L->accentFaint, 0.0f, 0, S(1.0f));
				const float k = S(34.0f), t = S(3.0f);
				const ImU32 col = L->accentDim;
				// top-left, top-right, bottom-left, bottom-right
				dl->AddRectFilled(ia, ia + ImVec2(k, t), col);
				dl->AddRectFilled(ia, ia + ImVec2(t, k), col);
				dl->AddRectFilled(ImVec2(ib.x - k, ia.y), ImVec2(ib.x, ia.y + t), col);
				dl->AddRectFilled(ImVec2(ib.x - t, ia.y), ImVec2(ib.x, ia.y + k), col);
				dl->AddRectFilled(ImVec2(ia.x, ib.y - t), ImVec2(ia.x + k, ib.y), col);
				dl->AddRectFilled(ImVec2(ia.x, ib.y - k), ImVec2(ia.x + t, ib.y), col);
				dl->AddRectFilled(ib - ImVec2(k, t), ib, col);
				dl->AddRectFilled(ib - ImVec2(t, k), ib, col);
				break;
			}
		case Chrome::Rules:
			FadingRule(dl, a.x + S(20.0f), b.x - S(20.0f), a.y + S(8.0f), L->accentDim, S(1.5f), true);
			FadingRule(dl, a.x + S(20.0f), b.x - S(20.0f), b.y - S(10.0f), L->accentDim, S(1.5f), true);
			break;
		case Chrome::DoubleLine:
			dl->AddRect(ia, ib, L->accentDim, 0.0f, 0, S(1.4f));
			dl->AddRect(ia + ImVec2(S(5.0f), S(5.0f)), ib - ImVec2(S(5.0f), S(5.0f)), L->accentFaint, 0.0f, 0, S(1.0f));
			break;
		case Chrome::Rounded:
			dl->AddRect(a + ImVec2(S(0.5f), S(0.5f)), b - ImVec2(S(0.5f), S(0.5f)), L->accentFaint, wr, 0, S(1.2f));
			break;
		case Chrome::Rivets:
			{
				dl->AddRect(ia, ib, L->accentDim, 0.0f, 0, S(2.0f));
				dl->AddRect(ia + ImVec2(S(6.0f), S(6.0f)), ib - ImVec2(S(6.0f), S(6.0f)), L->accentFaint, 0.0f, 0, S(1.0f));
				const float  rr = S(4.5f);
				const ImVec2 pts[] = { ia, ImVec2(ib.x, ia.y), ImVec2(ia.x, ib.y), ib, ImVec2((ia.x + ib.x) * 0.5f, ia.y),
					ImVec2((ia.x + ib.x) * 0.5f, ib.y), ImVec2(ia.x, (ia.y + ib.y) * 0.5f), ImVec2(ib.x, (ia.y + ib.y) * 0.5f) };
				for (const auto& p : pts) {
					dl->AddCircleFilled(p, rr, L->accentDim);
					dl->AddCircleFilled(p - ImVec2(rr * 0.3f, rr * 0.3f), rr * 0.45f, L->accent);
				}
				break;
			}
		}
		dl->PopClipRect();
	}

	void SetBackdrop() { g_backdrop = true; }

	void EndWindow()
	{
		ImGui::End();
	}

	namespace
	{
		int         g_subMode = 0;
		bool*       g_subClicked = nullptr;
		const char* g_subTooltip = nullptr;

		void FolderIcon(ImDrawList* a_dl, const ImVec2& a_pos, float a_size, ImU32 a_back, ImU32 a_front)
		{
			const ImVec2 f0 = a_pos, f1 = a_pos + ImVec2(a_size, a_size * 0.78f);
			a_dl->AddRectFilled(f0, ImVec2(f0.x + a_size * 0.45f, f0.y + a_size * 0.22f), a_back, a_size * 0.08f, ImDrawFlags_RoundCornersTop);
			a_dl->AddRectFilled(ImVec2(f0.x, f0.y + a_size * 0.14f), f1, a_back, a_size * 0.1f);
			a_dl->AddRectFilled(ImVec2(f0.x + a_size * 0.08f, f0.y + a_size * 0.32f), f1 - ImVec2(a_size * 0.08f, a_size * 0.08f), a_front, a_size * 0.08f);
		}
	}

	void SetSubtitleAction(int a_mode, bool* a_clicked, const char* a_tooltip)
	{
		g_subMode = a_mode;
		g_subClicked = a_clicked;
		g_subTooltip = a_tooltip;
	}

	bool Header(const char* a_title, const char* a_subtitle, const Stat* a_stats, int a_statCount, bool a_closeBox, int* a_clickedStat)
	{
		const int   subMode = g_subMode;
		bool*       subClicked = g_subClicked;
		const char* subTip = g_subTooltip;
		g_subMode = 0;
		g_subClicked = nullptr;
		g_subTooltip = nullptr;
		auto*        dl = ImGui::GetWindowDrawList();
		const ImVec2 p = ImGui::GetCursorScreenPos();
		const float  w = ImGui::GetContentRegionAvail().x;
		float        h = S(76.0f);
		const float  r = S(L->rounding);
		// room on the right for the stat boxes and the close box
		const float rightW = (a_closeBox ? S(54.0f) + S(18.0f) : 0.0f) + a_statCount * (S(130.0f) + S(12.0f));
		const float regionW = (std::max)(S(200.0f), w - rightW);
		const float cx = p.x + regionW * 0.5f;  // centre of the title area
		auto engraved = [&](float a_size, const ImVec2& a_pos) {
			Heading(dl, g_bold, a_size, a_pos + ImVec2(S(1.0f), S(2.0f)), IM_COL32(0, 0, 0, 180), a_title);
			Heading(dl, g_bold, a_size, a_pos, L->accent, a_title);
		};

		// title + subtitle
		float  tx = 0.0f;
		ImVec2 sp = p + ImVec2(S(6.0f), S(46.0f));  // subtitle position
		bool   centerSub = false;
		if (L->fx.hall == 1) {  // centred engraved title between long gold rules
			const float ts = S(44.0f);
			const float tw = HeadingWidth(g_bold, ts, a_title);
			const float ty = p.y + S(2.0f);
			engraved(ts, ImVec2(cx - tw * 0.5f, ty));
			const float ly = ty + ts * 0.52f;
			const float l0 = p.x + S(24.0f), l1 = cx - tw * 0.5f - S(26.0f), r0 = cx + tw * 0.5f + S(26.0f), r1 = p.x + regionW - S(24.0f);
			if (l1 - l0 > S(40.0f)) {
				dl->AddLine(ImVec2(l0, ly), ImVec2(l1, ly), L->accentDim, S(1.2f));
				dl->AddLine(ImVec2(r0, ly), ImVec2(r1, ly), L->accentDim, S(1.2f));
				Diamond(dl, ImVec2(l1 + S(8.0f), ly), S(5.0f), L->accent);
				Diamond(dl, ImVec2(r0 - S(8.0f), ly), S(5.0f), L->accent);
				Diamond(dl, ImVec2(l0, ly), S(3.0f), L->accentDim);
				Diamond(dl, ImVec2(r1, ly), S(3.0f), L->accentDim);
			}
			FadingRule(dl, cx - tw * 0.45f, cx + tw * 0.45f, ty + ts + S(4.0f), L->accentDim, S(1.2f), true);
			sp = ImVec2(cx, ty + ts + S(16.0f));
			centerSub = true;
			h = S(108.0f);
		} else if (L->fx.hall == 2) {  // emblem box, title, long rule with diamonds
			const float  eb = S(70.0f);
			const ImVec2 ea = p + ImVec2(0, S(2.0f)), ebb = ea + ImVec2(eb, eb);
			SoftShadow(dl, ea, ebb, 0.0f, S(8.0f), 90);
			dl->AddRectFilledMultiColor(ea, ebb, C(44, 38, 32), C(36, 31, 26), C(18, 16, 14), C(24, 21, 18));
			Bevel(dl, ea, ebb, S(1.5f), false, 50, 150);
			dl->AddRect(ea + ImVec2(S(3), S(3)), ebb - ImVec2(S(3), S(3)), L->accentDim, 0.0f, 0, S(1.4f));
			for (const ImVec2& c : { ea, ImVec2(ebb.x, ea.y), ImVec2(ea.x, ebb.y), ebb }) {
				Diamond(dl, c, S(4.0f), L->accent);
			}
			FramedPictureGlyph(dl, (ea + ebb) * 0.5f, S(20.0f), L->accent);
			const float ts = S(40.0f);
			tx = eb + S(18.0f);
			engraved(ts, p + ImVec2(tx, S(4.0f)));
			const float tw = HeadingWidth(g_bold, ts, a_title);
			const float ly = p.y + S(4.0f) + ts * 0.5f;
			const float l0 = p.x + tx + tw + S(22.0f), l1 = p.x + regionW - S(20.0f);
			if (l1 - l0 > S(30.0f)) {
				dl->AddLine(ImVec2(l0, ly), ImVec2(l1, ly), L->accentDim, S(1.2f));
				Diamond(dl, ImVec2(l0, ly), S(4.5f), L->accent);
				Diamond(dl, ImVec2(l1, ly), S(4.5f), L->accent);
			}
			sp = p + ImVec2(tx + S(2.0f), S(52.0f));
			h = S(86.0f);
		} else if (L->fx.hall == 3) {  // title on a carved plaque
			const float  ts = S(36.0f);
			const float  tw = HeadingWidth(g_bold, ts, a_title);
			const float  pw = tw + S(96.0f), ph = S(58.0f);
			const ImVec2 pa(cx - pw * 0.5f, p.y + S(2.0f)), pb = pa + ImVec2(pw, ph);
			SoftShadow(dl, pa, pb, 0.0f, S(10.0f), 110);
			dl->AddRectFilledMultiColor(pa, pb, C(70, 51, 34), C(62, 45, 30), C(34, 25, 17), C(42, 31, 21));
			if (g_texGrain) {
				Tiled(dl, g_texGrain, pa, pb, IM_COL32(0, 0, 0, 170));
			}
			Bevel(dl, pa, pb, S(1.5f), false, 70, 160);
			dl->AddRect(pa + ImVec2(S(4), S(4)), pb - ImVec2(S(4), S(4)), L->accentDim, 0.0f, 0, S(1.4f));
			dl->AddRect(pa + ImVec2(S(8), S(8)), pb - ImVec2(S(8), S(8)), L->accentFaint, 0.0f, 0, S(1.0f));
			const float my = (pa.y + pb.y) * 0.5f;
			for (int sdir = -1; sdir <= 1; sdir += 2) {
				const float ex = sdir < 0 ? pa.x : pb.x;
				Diamond(dl, ImVec2(ex, my), S(9.0f), Dark(L->accentDim, 0.2f));
				Diamond(dl, ImVec2(ex, my), S(6.0f), L->accent);
				dl->AddLine(ImVec2(ex + sdir * S(12.0f), my), ImVec2(ex + sdir * S(70.0f), my), L->accentDim, S(1.2f));
				Diamond(dl, ImVec2(ex + sdir * S(74.0f), my), S(3.0f), L->accentDim);
			}
			engraved(ts, ImVec2(cx - tw * 0.5f, my - ts * 0.5f));
			sp = ImVec2(cx, pb.y + S(8.0f));
			centerSub = true;
			h = S(100.0f);
		} else if (Modern()) {  // big Cinzel title in warm white, quiet subtitle, hairline under the header
			const float ts = S(38.0f);
			Heading(dl, g_bold, ts, p + ImVec2(S(1.0f), S(3.0f)), IM_COL32(0, 0, 0, 120), a_title);
			Heading(dl, g_bold, ts, p + ImVec2(0, S(1.0f)), L->text, a_title);
			sp = p + ImVec2(S(2.0f), S(50.0f));
			h = S(92.0f);
			dl->AddRectFilledMultiColor(ImVec2(p.x, p.y + h - S(10.0f)), ImVec2(p.x + w * 0.5f, p.y + h - S(9.0f)), IM_COL32(255, 255, 255, 30), IM_COL32(255, 255, 255, 18), IM_COL32(255, 255, 255, 18), IM_COL32(255, 255, 255, 30));
			dl->AddRectFilledMultiColor(ImVec2(p.x + w * 0.5f, p.y + h - S(10.0f)), ImVec2(p.x + w, p.y + h - S(9.0f)), IM_COL32(255, 255, 255, 18), IM_COL32(255, 255, 255, 4), IM_COL32(255, 255, 255, 4), IM_COL32(255, 255, 255, 18));
		} else {
			if (L->fx.medallion) {
				Medallion(dl, p + ImVec2(S(34.0f), S(34.0f)), S(32.0f));
				tx = S(80.0f);
			}
			if (L->fx.bevel) {  // engraved: dark copy below, faint highlight above
				Heading(dl, g_bold, S(34.0f), p + ImVec2(tx + S(4.0f), S(2.0f)) + ImVec2(S(1.0f), S(2.0f)), IM_COL32(0, 0, 0, 170), a_title);
				Heading(dl, g_bold, S(34.0f), p + ImVec2(tx + S(4.0f), S(2.0f)), L->accent, a_title);
			} else {
				Heading(dl, g_bold, S(34.0f), p + ImVec2(tx + S(4.0f), S(2.0f)), L->accent, a_title);
			}
			sp = p + ImVec2(tx + S(6.0f), S(46.0f));
		}
		if (a_subtitle && *a_subtitle) {
			const float  ss = S(18.0f);
			const ImVec2 sz = g_italic->CalcTextSizeA(ss, FLT_MAX, 0.0f, a_subtitle);
			if (centerSub) {  // centre the subtitle (and the Open folder button after it) under the title
				float total = sz.x;
				if (subMode == 2) {
					total += S(14.0f) + S(10.0f) * 2 + S(16.0f) + S(7.0f) + g_body->CalcTextSizeA(S(15.0f), FLT_MAX, 0.0f, "Open folder").x;
				}
				sp.x = (std::max)(p.x + S(6.0f), cx - total * 0.5f);
			}
			if (subMode == 1) {  // the path is a link
				ImGui::SetCursorScreenPos(sp - ImVec2(S(2.0f), S(2.0f)));
				const bool clicked = ImGui::InvisibleButton("##sublink", sz + ImVec2(S(4.0f), S(6.0f)));
				const bool hov = ImGui::IsItemHovered();
				const ImU32 col = hov ? L->accent : L->textDim;
				dl->AddText(g_italic, ss, sp, col, a_subtitle);
				dl->AddLine(ImVec2(sp.x, sp.y + sz.y + S(1.0f)), ImVec2(sp.x + sz.x, sp.y + sz.y + S(1.0f)), hov ? L->accent : WithAlpha(L->textDim, 90), S(1.0f));
				if (hov && subTip) {
					ImGui::SetTooltip("%s", subTip);
				}
				if (clicked && subClicked) {
					*subClicked = true;
				}
			} else {
				dl->AddText(g_italic, ss, sp, L->textDim, a_subtitle);
			}
			if (subMode == 2) {  // small "Open folder" button after the path
				const char*  label = "Open folder";
				const float  ls = S(15.0f);
				const ImVec2 lsz = g_body->CalcTextSizeA(ls, FLT_MAX, 0.0f, label);
				const float  icon = S(16.0f), padX = S(10.0f), bh = S(28.0f);
				const ImVec2 ba(sp.x + sz.x + S(14.0f), sp.y + (sz.y - bh) * 0.5f);
				const ImVec2 bsz(padX * 2 + icon + S(7.0f) + lsz.x, bh);
				ImGui::SetCursorScreenPos(ba);
				const bool clicked = ImGui::InvisibleButton("##subbtn", bsz);
				const bool hov = ImGui::IsItemHovered();
				const ImVec2 bb = ba + bsz;
				dl->AddRectFilled(ba, bb, hov ? L->closeHov : L->boxBg, S(4.0f));
				if (L->fx.bevel) {
					Bevel(dl, ba, bb, S(1.0f), false, 40, 110);
				}
				dl->AddRect(ba, bb, hov ? L->accent : L->accentDim, S(4.0f), 0, S(1.0f));
				FolderIcon(dl, ImVec2(ba.x + padX, ba.y + (bh - icon * 0.78f) * 0.5f), icon, hov ? L->accent : L->accentDim, L->boxBg);
				dl->AddText(g_body, ls, ImVec2(ba.x + padX + icon + S(7.0f), ba.y + (bh - lsz.y) * 0.5f), hov ? L->text : L->textDim, label);
				if (hov && subTip) {
					ImGui::SetTooltip("%s", subTip);
				}
				if (clicked && subClicked) {
					*subClicked = true;
				}
			}
		}
		if (L->fx.bevel && L->fx.hall != 1 && L->fx.hall != 3) {  // gold rule under the header
			FadingRule(dl, p.x, p.x + w, p.y + h - S(4.0f), L->accentDim, S(1.2f), true);
		}

		// close box
		bool        closed = false;
		float       right = p.x + w;
		const float box = S(54.0f);
		if (a_closeBox && Modern()) {  // round close button
			const float  d = S(46.0f);
			const ImVec2 ca(right - d, p.y + S(6.0f)), cb(right, p.y + S(6.0f) + d);
			ImGui::SetCursorScreenPos(ca);
			closed = ImGui::InvisibleButton("##close", ImVec2(d, d));
			const bool   hov = ImGui::IsItemHovered();
			const ImVec2 cc = (ca + cb) * 0.5f;
			dl->AddCircleFilled(cc, d * 0.5f, hov ? IM_COL32(255, 255, 255, 26) : IM_COL32(255, 255, 255, 8), 40);
			dl->AddCircle(cc, d * 0.5f, hov ? L->accent : IM_COL32(255, 255, 255, 50), 40, S(1.2f));
			const float k = d * 0.17f;
			dl->AddLine(cc - ImVec2(k, k), cc + ImVec2(k, k), hov ? L->accent : L->text, S(1.6f));
			dl->AddLine(cc + ImVec2(-k, k), cc + ImVec2(k, -k), hov ? L->accent : L->text, S(1.6f));
			right -= d + S(26.0f);
		} else if (a_closeBox) {
			const ImVec2 ca(right - box, p.y + S(4.0f)), cb(right, p.y + S(4.0f) + box);
			ImGui::SetCursorScreenPos(ca);
			closed = ImGui::InvisibleButton("##close", ImVec2(box, box));
			const bool hov = ImGui::IsItemHovered();
			if (L->fx.shadows) {
				SoftShadow(dl, ca, cb, r, S(8.0f), 80);
			}
			dl->AddRectFilled(ca, cb, hov ? L->closeHov : L->boxBg, r);
			if (L->fx.bevel) {
				RaisedFill(dl, ca, cb, C(0, 0, 0, 0), r, ImGui::IsItemActive());
				Bevel(dl, ca, cb, S(1.5f), false, 45, 130);
			}
			dl->AddRect(ca, cb, hov ? L->accent : L->accentDim, r, 0, S(1.2f));
			const float  k = box * 0.26f;
			const ImVec2 cc = (ca + cb) * 0.5f;
			dl->AddLine(cc - ImVec2(k, k), cc + ImVec2(k, k), L->closeX, S(2.4f));
			dl->AddLine(cc + ImVec2(-k, k), cc + ImVec2(k, -k), L->closeX, S(2.4f));
			right -= box + S(18.0f);
		}

		// stat boxes, right to left
		for (int i = a_statCount - 1; i >= 0 && Modern(); --i) {  // modern: number over a small label, hairline dividers
			const float  vs = S(28.0f), ls = S(12.0f);
			const float  vw = LabelFont()->CalcTextSizeA(vs, FLT_MAX, 0.0f, a_stats[i].value).x;
			const float  lw = HeadingWidth(LabelFont(), ls, a_stats[i].label);
			const float  bw = (std::max)(vw, lw) + S(8.0f);
			const ImVec2 sa(right - bw, p.y + S(6.0f)), sb(right, p.y + S(56.0f));
			bool         hov = false;
			if (a_stats[i].tooltip) {
				ImGui::SetCursorScreenPos(sa);
				ImGui::PushID(i);
				if (ImGui::InvisibleButton("##stat", sb - sa) && a_clickedStat) {
					*a_clickedStat = i;
				}
				ImGui::PopID();
				hov = ImGui::IsItemHovered();
				if (hov) {
					ImGui::SetTooltip("%s", a_stats[i].tooltip);
				}
			}
			dl->AddText(LabelFont(), vs, ImVec2(sb.x - vw, sa.y), hov ? L->accent : L->text, a_stats[i].value);
			Heading(dl, LabelFont(), ls, ImVec2(sb.x - lw, sa.y + vs + S(4.0f)), hov ? L->accent : L->textDim, a_stats[i].label);
			if (a_stats[i].tooltip) {
				dl->AddLine(ImVec2(sb.x - lw, sb.y + S(4.0f)), ImVec2(sb.x, sb.y + S(4.0f)), hov ? L->accent : WithAlpha(L->accent, 90), S(1.0f));
			}
			right -= bw + S(20.0f);
			if (i > 0) {
				dl->AddLine(ImVec2(right + S(10.0f), sa.y + S(6.0f)), ImVec2(right + S(10.0f), sb.y - S(2.0f)), IM_COL32(255, 255, 255, 26), S(1.0f));
			}
		}
		for (int i = a_statCount - 1; i >= 0 && !Modern(); --i) {
			const float  bw = S(130.0f), bh = box;
			const ImVec2 sa(right - bw, p.y + S(4.0f)), sb(right, p.y + S(4.0f) + bh);
			bool         hov = false;
			if (a_stats[i].tooltip) {
				ImGui::SetCursorScreenPos(sa);
				ImGui::PushID(i);
				if (ImGui::InvisibleButton("##stat", sb - sa) && a_clickedStat) {
					*a_clickedStat = i;
				}
				ImGui::PopID();
				hov = ImGui::IsItemHovered();
				if (hov) {
					ImGui::SetTooltip("%s", a_stats[i].tooltip);
				}
			}
			dl->AddRectFilled(sa, sb, hov ? L->closeHov : (L->fx.bevel ? Dark(L->boxBg, 0.25f) : L->boxBg), r);
			if (L->fx.bevel) {  // sunken counter
				dl->AddRectFilledMultiColor(sa, ImVec2(sb.x, sa.y + S(12.0f)), IM_COL32(0, 0, 0, 90), IM_COL32(0, 0, 0, 90), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));
				Bevel(dl, sa, sb, S(1.5f), true, 40, 140);
			}
			dl->AddRect(sa, sb, hov ? L->accent : L->accentDim, r, 0, S(1.2f));
			if (a_stats[i].tooltip) {  // small arrow: this box opens something
				const ImVec2 c(sb.x - S(10.0f), sb.y - S(10.0f));
				dl->AddTriangleFilled(c + ImVec2(-S(4), -S(4)), c + ImVec2(S(4), -S(4)), c + ImVec2(0, S(3)), hov ? L->accent : L->accentDim);
			}
			const float lw = HeadingWidth(g_body, S(13.0f), a_stats[i].label);
			Heading(dl, g_body, S(13.0f), ImVec2((sa.x + sb.x - lw) * 0.5f, sa.y + S(6.0f)), L->textDim, a_stats[i].label);
			const float vs = S(26.0f);
			const float vw = g_body->CalcTextSizeA(vs, FLT_MAX, 0.0f, a_stats[i].value).x;
			dl->AddText(g_body, vs, ImVec2((sa.x + sb.x - vw) * 0.5f, sa.y + S(22.0f)), L->text, a_stats[i].value);
			right -= bw + S(12.0f);
		}

		ImGui::SetCursorScreenPos(p);
		ImGui::Dummy(ImVec2(w, h));
		return closed;
	}

	// ------------------------------------------------------------------ content panel
	void NextPanelDark() { g_nextDark = true; }
	int  CardStyle() { return L->fx.hall; }
	bool PlaqueTabs() { return L->fx.hall != 0; }

	bool BeginParchment(const char* a_id, const ImVec2& a_size, bool a_scroll, bool a_autoHeight, bool a_alwaysBar)
	{
		const bool dark = g_nextDark || Modern();
		g_nextDark = false;
		const bool fancy = L->fx.shadows || L->fx.textured || L->fx.deckle || dark;
		ImGui::PushStyleColor(ImGuiCol_ChildBg, fancy ? C(0, 0, 0, 0) : L->panel);
		// ImGui's own scroll bar stays (it does the dragging) but is invisible: EndParchment draws ours over it
		ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, C(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, C(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, C(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, C(0, 0, 0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(S(22.0f), S(18.0f)));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, S(L->windowRounding > 0.0f ? L->rounding + 2.0f : L->rounding));
		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, S(Modern() ? 14.0f : 30.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, S(48.0f));
		const bool visible = ImGui::BeginChild(a_id, a_size,
			ImGuiChildFlags_AlwaysUseWindowPadding | (a_autoHeight ? ImGuiChildFlags_AutoResizeY : 0),
			a_scroll ? (a_alwaysBar ? ImGuiWindowFlags_AlwaysVerticalScrollbar : 0) : (ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse));
		ImGui::PopStyleVar(4);
		ImGui::PopStyleColor(5);
		g_parchmentScroll.push_back(a_scroll);
		g_parchmentDark.push_back(dark);

		auto*        dl = ImGui::GetWindowDrawList();
		const ImVec2 a = ImGui::GetWindowPos();
		const ImVec2 b = a + ImGui::GetWindowSize();
		dl->PushClipRect(a, b, false);
		const float pr = S(L->windowRounding > 0.0f ? L->rounding + 2.0f : L->rounding);
		if (dark && Modern()) {  // recessed smoked glass
			const float mr = S(10.0f);
			dl->AddRectFilled(a, b, IM_COL32(0, 0, 0, 72), mr);
			TopGlow(dl, a, b, mr, S(14.0f), IM_COL32(0, 0, 0, 60));  // inner shade along the top, following the rounded corners
			CrispRect(dl, a, b, IM_COL32(255, 255, 255, 16), mr, 1.0f);
		} else if (dark) {  // the gallery wall: dark recessed stone, gold inset line
			dl->AddRectFilled(a, b, C(19, 17, 15));
			if (g_texGrain) {
				Tiled(dl, g_texGrain, a, b, IM_COL32(0, 0, 0, 255));
			}
			dl->AddRectFilledMultiColor(a, ImVec2(b.x, a.y + S(18.0f)), IM_COL32(0, 0, 0, 150), IM_COL32(0, 0, 0, 150), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));
			dl->AddRectFilledMultiColor(a, ImVec2(a.x + S(14.0f), b.y), IM_COL32(0, 0, 0, 110), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 110));
			Bevel(dl, a, b, S(1.5f), true, 40, 170);
			dl->AddRect(a + ImVec2(S(5), S(5)), b - ImVec2(S(5), S(5)), L->accentFaint, 0.0f, 0, S(1.0f));
		}
		if (dark) {
			dl->PopClipRect();
			++g_parchmentDepth;
			ImGui::PushStyleColor(ImGuiCol_Text, L->text);
			ImGui::PushStyleColor(ImGuiCol_TextDisabled, L->textDim);
			ImGui::PushStyleColor(ImGuiCol_FrameBg, L->frameBg);
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, L->frameHov);
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, L->frameAct);
			ImGui::PushStyleColor(ImGuiCol_Border, L->accentFaint);
			ImGui::PushStyleColor(ImGuiCol_SliderGrab, L->grab);
			ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, L->grabAct);
			ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, C(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, WithAlpha(L->accentDim, 140));
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, WithAlpha(L->accentDim, 200));
			ImGui::PushStyleColor(ImGuiCol_PopupBg, WithAlpha(L->winMid, 250));
			ImGui::PushStyleColor(ImGuiCol_Header, L->btnHov);
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, L->btnAct);
			return visible;
		}
		if (fancy) {
			dl->AddRectFilled(a, b, L->panel, pr);
			if (L->fx.textured && g_texParch) {
				Tiled(dl, g_texParch, a, b, Dark(L->panelBorder, 0.35f), pr);
			}
		}
		if ((L->panelShade >> 24) != 0) {  // aged / shaded edges
			const float e = S(26.0f);
			const ImU32 dark = L->panelShade, none = WithAlpha(L->panelShade, 0);
			dl->AddRectFilledMultiColor(a, ImVec2(b.x, a.y + e), dark, dark, none, none);
			dl->AddRectFilledMultiColor(ImVec2(a.x, b.y - e), b, none, none, dark, dark);
			dl->AddRectFilledMultiColor(a, ImVec2(a.x + e, b.y), dark, none, none, dark);
			dl->AddRectFilledMultiColor(ImVec2(b.x - e, a.y), b, none, dark, dark, none);
		}
		if (L->fx.bevel) {  // paper lit from above: bright top edge, darker bottom edge
			dl->AddRectFilledMultiColor(a, ImVec2(b.x, a.y + S(5.0f)), IM_COL32(255, 250, 230, 70), IM_COL32(255, 250, 230, 70), IM_COL32(255, 250, 230, 0), IM_COL32(255, 250, 230, 0));
			dl->AddRectFilledMultiColor(ImVec2(a.x, b.y - S(8.0f)), b, IM_COL32(60, 35, 15, 0), IM_COL32(60, 35, 15, 0), IM_COL32(60, 35, 15, 60), IM_COL32(60, 35, 15, 60));
		}
		if (L->fx.deckle) {
			DeckleEdges(dl, a, b, L->winMid, S(4.0f));
			dl->AddRect(a + ImVec2(S(6.0f), S(6.0f)), b - ImVec2(S(6.0f), S(6.0f)), WithAlpha(L->panelBorder, 110), 0.0f, 0, S(1.0f));
		} else {
			dl->AddRect(a, b, L->panelBorder, pr, 0, S(1.5f));
		}
		if (L->fx.hall) {  // gold inset line with small corner squares
			const ImVec2 ia = a + ImVec2(S(6.0f), S(6.0f)), ib = b - ImVec2(S(6.0f), S(6.0f));
			dl->AddRect(ia, ib, WithAlpha(L->accentDim, 200), 0.0f, 0, S(1.2f));
			if ((b.y - a.y) > S(120.0f)) {
				const float k = S(7.0f);
				for (const ImVec2& c : { ia, ImVec2(ib.x, ia.y), ImVec2(ia.x, ib.y), ib }) {
					dl->AddRectFilled(c - ImVec2(k, k), c + ImVec2(k, k), L->panel);
					dl->AddRect(c - ImVec2(k, k), c + ImVec2(k, k), L->accentDim, 0.0f, 0, S(1.2f));
					dl->AddRect(c - ImVec2(k * 0.45f, k * 0.45f), c + ImVec2(k * 0.45f, k * 0.45f), L->accentDim, 0.0f, 0, S(1.0f));
				}
			}
		}
		if (L->fx.seal && (b.y - a.y) > S(260.0f) && (b.x - a.x) > S(500.0f)) {
			WaxSeal(dl, ImVec2(b.x - S(36.0f) - (ImGui::GetCurrentWindow()->ScrollbarY ? S(30.0f) : 0.0f), a.y + S(28.0f)), S(21.0f));  // clear of the scroll bar
		}
		dl->PopClipRect();

		++g_parchmentDepth;
		ImGui::PushStyleColor(ImGuiCol_Text, L->ink);
		ImGui::PushStyleColor(ImGuiCol_TextDisabled, L->inkSoft);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, L->pFrame);
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, L->pFrameHov);
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, L->pFrameAct);
		ImGui::PushStyleColor(ImGuiCol_Border, L->panelBorder);
		ImGui::PushStyleColor(ImGuiCol_SliderGrab, L->pGrab);
		ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, L->pGrabAct);
		ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, C(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, WithAlpha(L->inkSoft, 140));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, WithAlpha(L->inkSoft, 200));
		ImGui::PushStyleColor(ImGuiCol_PopupBg, (L->panel >> 24) >= 200 ? L->panel : WithAlpha(L->winMid, 250));  // tooltips
		ImGui::PushStyleColor(ImGuiCol_Header, L->pBtnHov);
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, L->pBtnAct);
		return visible;
	}

	// Wood-and-brass scroll bar, drawn over ImGui's invisible one (same size and place, so dragging,
	// clicking the track and the wheel all work as usual).
	void DrawScrollbar(ImGuiWindow* a_win)
	{
		if (!a_win || !a_win->ScrollbarY) {
			return;
		}
		ImGuiContext& g = *GImGui;
		ImRect        bb = ImGui::GetWindowScrollbarRect(a_win, ImGuiAxis_Y);
		bb.Expand(ImVec2(-3.0f, -3.0f));  // the same inset ImGui uses for its grab
		const float trackH = bb.GetHeight();
		if (trackH <= 1.0f) {
			return;
		}
		const float avail = a_win->InnerRect.Max.y - a_win->InnerRect.Min.y;
		const float content = a_win->ContentSize.y + a_win->WindowPadding.y * 2.0f;
		const float winSize = ImMax(ImMax(content, avail), 1.0f);
		const float grabH = ImClamp(trackH * (avail / winSize), S(48.0f), trackH);
		const float ratio = a_win->ScrollMax.y > 0.0f ? ImSaturate(a_win->Scroll.y / a_win->ScrollMax.y) : 0.0f;
		const float grabY = bb.Min.y + (trackH - grabH) * ratio;

		const ImGuiID id = ImGui::GetWindowScrollbarID(a_win, ImGuiAxis_Y);
		const bool    active = g.ActiveId == id;
		const bool    hovered = active || (g.HoveredWindow == a_win && ImGui::IsMouseHoveringRect(bb.Min, bb.Max, false));

		auto* dl = a_win->DrawList;
		dl->PushClipRect(a_win->Rect().Min, a_win->Rect().Max, false);
		const float r = bb.GetWidth() * 0.5f;
		if (Modern()) {  // slim pill
			const float  cxm = (bb.Min.x + bb.Max.x) * 0.5f, hw = hovered ? S(3.5f) : S(2.5f);
			dl->AddRectFilled(ImVec2(cxm - S(1.0f), bb.Min.y), ImVec2(cxm + S(1.0f), bb.Max.y), IM_COL32(255, 255, 255, 16), S(1.0f));
			dl->AddRectFilled(ImVec2(cxm - hw, grabY), ImVec2(cxm + hw, grabY + grabH), active ? L->accent : (hovered ? IM_COL32(255, 255, 255, 120) : IM_COL32(255, 255, 255, 60)), hw);
			dl->PopClipRect();
			return;
		}
		// track: a dark groove in the parchment
		const bool darkWall = InDarkPanel();
		dl->AddRectFilled(bb.Min, bb.Max, darkWall ? IM_COL32(0, 0, 0, 120) : WithAlpha(L->ink, 55), r);
		dl->AddRectFilled(bb.Min, ImVec2(bb.Max.x, bb.Min.y + S(3.0f)), WithAlpha(L->ink, 30), r, ImDrawFlags_RoundCornersTop);
		dl->AddRect(bb.Min, bb.Max, darkWall ? L->accentFaint : WithAlpha(L->ink, 110), r, 0, S(1.0f));
		// grab: wood with a brass rim and grip lines
		const ImVec2 a(bb.Min.x + S(2.0f), grabY + S(2.0f)), b(bb.Max.x - S(2.0f), grabY + grabH - S(2.0f));
		const float  gr = (b.x - a.x) * 0.5f;
		dl->AddRectFilled(ImVec2(a.x + S(1.0f), a.y + S(2.0f)), ImVec2(b.x + S(1.0f), b.y + S(2.0f)), C(0, 0, 0, 70), gr);  // shadow
		dl->AddRectFilled(a, b, active ? L->pGrabAct : (hovered ? L->pGrabAct : L->pGrab), gr);
		dl->AddRectFilled(ImVec2(a.x + S(3.0f), a.y + S(4.0f)), ImVec2(a.x + S(5.0f), b.y - S(4.0f)), C(255, 255, 255, 28), S(1.0f));  // sheen
		dl->AddRect(a, b, hovered ? L->accent : L->accentDim, gr, 0, S(hovered ? 2.0f : 1.5f));
		if (b.y - a.y > S(36.0f)) {
			const float cx = (a.x + b.x) * 0.5f, cy = (a.y + b.y) * 0.5f, w = (b.x - a.x) * 0.28f;
			for (int k = -1; k <= 1; ++k) {
				const float y = cy + k * S(5.0f);
				dl->AddLine(ImVec2(cx - w, y), ImVec2(cx + w, y), WithAlpha(L->accent, hovered ? 255 : 210), S(2.0f));
			}
		}
		dl->PopClipRect();
	}

	void EndParchment()
	{
		ImGui::PopStyleColor(14);
		--g_parchmentDepth;
		if (!g_parchmentDark.empty()) {
			g_parchmentDark.pop_back();
		}
		const bool scroll = !g_parchmentScroll.empty() && g_parchmentScroll.back();
		if (!g_parchmentScroll.empty()) {
			g_parchmentScroll.pop_back();
		}
		if (scroll) {
			DrawScrollbar(ImGui::GetCurrentWindow());
		}
		ImGui::EndChild();
		if (L->fx.shadows) {  // on the parent's draw list, which renders under the child window
			SoftShadow(ImGui::GetWindowDrawList(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), S(L->rounding), S(14.0f), 130);
		}
	}

	void SectionLabel(const char* a_text)
	{
		if (Modern()) {  // small accent capitals, faint hairline
			auto*        dl = ImGui::GetWindowDrawList();
			const ImVec2 p = ImGui::GetCursorScreenPos();
			const float  w = ImGui::GetContentRegionAvail().x;
			const float  size = S(13.0f);
			Heading(dl, LabelFont(), size, p + ImVec2(0, S(4.0f)), L->accent, a_text);
			const float tw = HeadingWidth(LabelFont(), size, a_text);
			dl->AddLine(ImVec2(p.x + tw + S(14.0f), p.y + S(4.0f) + size * 0.55f), ImVec2(p.x + w, p.y + S(4.0f) + size * 0.55f), IM_COL32(255, 255, 255, 18), S(1.0f));
			ImGui::Dummy(ImVec2(w, size + S(16.0f)));
			return;
		}
		auto*        dl = ImGui::GetWindowDrawList();
		const ImVec2 p = ImGui::GetCursorScreenPos();
		const float  w = ImGui::GetContentRegionAvail().x;
		const float  size = S(16.0f);
		const bool   parch = g_parchmentDepth > 0;
		Heading(dl, g_body, size, p, parch ? L->inkSoft : L->textDim, a_text);
		dl->AddLine(ImVec2(p.x, p.y + size + S(7.0f)), ImVec2(p.x + w, p.y + size + S(7.0f)), parch ? L->panelRule : L->accentFaint, S(1.0f));
		ImGui::Dummy(ImVec2(w, size + S(12.0f)));
	}

	// ------------------------------------------------------------------ controls
	namespace
	{
		// ImGui::Button with the current Button colours, but drawn with light and depth when the look asks for it.
		bool DepthButton(const char* a_label, const ImVec2& a_size, float a_r, bool a_parch)
		{
			if (!L->fx.bevel && !Modern()) {
				return ImGui::Button(a_label, a_size);
			}
			auto&       st = ImGui::GetStyle();
			const ImU32 base = ImGui::GetColorU32(ImGuiCol_Button), hov = ImGui::GetColorU32(ImGuiCol_ButtonHovered),
						act = ImGui::GetColorU32(ImGuiCol_ButtonActive), border = ImGui::GetColorU32(ImGuiCol_Border);
			auto* dl = ImGui::GetWindowDrawList();
			dl->ChannelsSplit(2);
			dl->ChannelsSetCurrent(1);
			ImGui::PushStyleColor(ImGuiCol_Button, C(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, C(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, C(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_Border, C(0, 0, 0, 0));
			const ImU32 textCol = ImGui::GetColorU32(ImGuiCol_Text);
			const char* labelEnd = ImGui::FindRenderedTextEnd(a_label);
			const std::string shown(a_label, labelEnd);
			ImVec2      size = a_size;
			const float capsSize = S(Modern() ? 14.5f : 17.0f);
			if (L->fx.hall) {
				ImGui::PushStyleColor(ImGuiCol_Text, C(0, 0, 0, 0));  // we write the label ourselves, in capitals
				if (size.x <= 0.0f) {
					size.x = HeadingWidth(LabelFont(), capsSize, shown.c_str()) + st.FramePadding.x * 2.0f + (a_parch ? 0.0f : 0.0f);
				}
			}
			const bool pressed = ImGui::Button(a_label, size);
			if (L->fx.hall) {
				ImGui::PopStyleColor();
				const ImVec2 ia = ImGui::GetItemRectMin(), ib = ImGui::GetItemRectMax();
				const float  tw = HeadingWidth(LabelFont(), capsSize, shown.c_str());
				const ImVec2 tp((ia.x + ib.x - tw) * 0.5f, (ia.y + ib.y - capsSize) * 0.5f);
				Heading(dl, LabelFont(), capsSize, tp, textCol, shown.c_str());
			}
			ImGui::PopStyleColor(4);
			const bool   down = ImGui::IsItemActive() && ImGui::IsItemHovered();
			const ImU32  fill = down ? act : (ImGui::IsItemHovered() ? hov : base);
			const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
			dl->ChannelsSetCurrent(0);
			if (Modern()) {  // flat: fill, hairline (accent on hover), gentle glow on the gold one
				const bool  hovd = ImGui::IsItemHovered();
				const bool  gold = (base >> 24) > 200;
				if (gold) {
					const ImU32 top = down ? Dark(fill, 0.08f) : Bright(fill, 0.10f), bot = down ? fill : Dark(fill, 0.12f);
					if (hovd) {
						dl->AddRect(a - ImVec2(S(2), S(2)), b + ImVec2(S(2), S(2)), WithAlpha(L->accent, 70), a_r + S(2), 0, S(3.0f));
					}
					const int first = dl->VtxBuffer.Size;
					dl->AddRectFilled(a, b, top, a_r);
					TintVertsY(dl, first, a.y, b.y, top, bot);  // top-to-bottom gradient that follows the rounded corners
					TopGlow(dl, a + ImVec2(1.0f, 1.0f), ImVec2(b.x - 1.0f, b.y), (std::max)(0.0f, a_r - 1.0f), (b.y - a.y) * 0.5f, IM_COL32(255, 255, 255, 40));
				} else {
					dl->AddRectFilled(a, b, fill, a_r);
				}
				CrispRect(dl, a, b, gold ? Bright(fill, 0.25f) : (hovd ? L->accent : border), a_r, 1.0f);
				dl->ChannelsMerge();
				return pressed;
			}
			if (L->fx.shadows && !down) {
				SoftShadow(dl, a, b, a_r, S(5.0f), a_parch ? 45 : 90);
			}
			if ((fill >> 24) >= 8) {
				dl->AddRectFilled(a, b, fill, a_r);
			}
			RaisedFill(dl, a, b, fill, a_r, down);
			Bevel(dl, a, b, S(1.2f), down, a_parch ? 90 : 50, a_parch ? 50 : 120);
			dl->AddRect(a, b, border, a_r, 0, st.FrameBorderSize);
			dl->ChannelsMerge();
			return pressed;
		}
	}

	bool Button(const char* a_label, const ImVec2& a_size, bool a_primary)
	{
		const bool parch = g_parchmentDepth > 0 && !InDarkPanel();
		ImVec2     size = a_size;
		if (size.y <= 0.0f) {
			size.y = S(46.0f);
		}
		int  colors = 0;
		auto push = [&](ImGuiCol a_idx, ImU32 a_col) { ImGui::PushStyleColor(a_idx, a_col); ++colors; };
		if (parch) {
			push(ImGuiCol_Button, a_primary ? L->pPriBg : C(0, 0, 0, 0));
			push(ImGuiCol_ButtonHovered, a_primary ? L->pPriHov : L->pBtnHov);
			push(ImGuiCol_ButtonActive, a_primary ? L->pPriBg : L->pBtnAct);
			push(ImGuiCol_Text, a_primary ? L->pPriText : L->ink);
			push(ImGuiCol_Border, a_primary ? L->pPriBg : L->pBorder);
		} else {
			push(ImGuiCol_Button, a_primary ? L->priBg : L->btnBg);
			push(ImGuiCol_ButtonHovered, a_primary ? L->priHov : L->btnHov);
			push(ImGuiCol_ButtonActive, a_primary ? L->priAct : L->btnAct);
			push(ImGuiCol_Text, a_primary ? L->priText : L->btnText);
			push(ImGuiCol_Border, a_primary ? L->priBorder : L->btnBorder);
		}
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(S(20.0f), S(10.0f)));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, S(1.2f));
		const bool pressed = DepthButton(a_label, size, S(L->rounding), parch);
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(colors);
		return pressed;
	}

	bool Chip(const char* a_label, bool a_selected)
	{
		const bool parch = g_parchmentDepth > 0 && !InDarkPanel();
		int        colors = 0;
		auto push = [&](ImGuiCol a_idx, ImU32 a_col) { ImGui::PushStyleColor(a_idx, a_col); ++colors; };
		if (parch) {
			push(ImGuiCol_Button, a_selected ? L->pSel : C(0, 0, 0, 0));
			push(ImGuiCol_ButtonHovered, a_selected ? L->pSelHov : L->pBtnHov);
			push(ImGuiCol_ButtonActive, a_selected ? L->pSel : L->pBtnAct);
			push(ImGuiCol_Text, a_selected ? L->pSelText : L->ink);
			push(ImGuiCol_Border, a_selected ? L->pSel : L->pBorder);
		} else {
			push(ImGuiCol_Button, a_selected ? L->priBg : C(0, 0, 0, 0));
			push(ImGuiCol_ButtonHovered, a_selected ? L->priHov : L->btnHov);  // a selected chip stays gold when hovered
			push(ImGuiCol_ButtonActive, a_selected ? L->priAct : L->btnAct);
			push(ImGuiCol_Text, a_selected ? L->priText : L->text);
			push(ImGuiCol_Border, a_selected ? L->priBorder : L->btnBorder);
		}
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(S(14.0f), S(8.0f)));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, S(1.2f));
		const float chipR = Modern() ? S(6.0f) : S(L->rounding > 4.0f ? 16.0f : L->rounding);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, chipR);
		const bool pressed = DepthButton(a_label, ImVec2(0, S(40.0f)), chipR, parch);
		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor(colors);
		return pressed;
	}

	namespace
	{
		// Hall sliders: ImGui's slider does the dragging (drawn invisible); we draw a thin gold track, a diamond
		// handle and the value beside it.
		void HallSliderTrack(const ImVec2& a_a, const ImVec2& a_b, float a_t, bool a_hov, bool a_parch)
		{
			auto* dl = ImGui::GetWindowDrawList();
			if (Modern()) {  // hairline track, glowing gold fill, slim pill handle that grows on hover
				const float y = (a_a.y + a_b.y) * 0.5f, x0 = a_a.x + S(4.0f), x1 = a_b.x - S(4.0f);
				const float t = std::fmin(1.0f, std::fmax(0.0f, a_t));
				const float gx = x0 + (x1 - x0) * t;
				const float h = Ease(ImGui::GetItemID(), a_hov, 16.0f);
				dl->AddRectFilled(ImVec2(x0, y - S(1.0f)), ImVec2(x1, y + S(1.0f)), IM_COL32(255, 255, 255, 34), S(1.0f));
				// soft glow under the filled part
				{  // 1 px lines fading out above and below the track (no hard band edge)
					const int peak = static_cast<int>(26 + 30 * h);
					const int reach = (std::max)(2, static_cast<int>(S(5.0f) + 0.5f));
					for (int d = 1; d <= reach; ++d) {
						const int al = static_cast<int>(peak * std::pow(1.0f - static_cast<float>(d) / (reach + 1), 2.0f));
						for (const float yy : { y - S(1.0f) - d, y + S(1.0f) + d - 1.0f }) {
							dl->AddRectFilledMultiColor(ImVec2(x0, yy), ImVec2(gx, yy + 1.0f), WithAlpha(L->accent, 0), WithAlpha(L->accent, al), WithAlpha(L->accent, al), WithAlpha(L->accent, 0));
						}
					}
				}
				dl->AddRectFilledMultiColor(ImVec2(x0, y - S(1.0f)), ImVec2(gx, y + S(1.0f)), L->accentDim, L->accent, L->accent, L->accentDim);
				const float hw = S(2.0f + 1.0f * h), hh = S(9.0f + 3.0f * h);
				if (h > 0.01f) {
					dl->AddRectFilled(ImVec2(gx - hw - S(5.0f), y - hh - S(5.0f)), ImVec2(gx + hw + S(5.0f), y + hh + S(5.0f)), WithAlpha(L->accent, static_cast<int>(40 * h)), hw + S(5.0f));
				}
				dl->AddRectFilled(ImVec2(gx - hw, y - hh + S(1.5f)), ImVec2(gx + hw, y + hh + S(1.5f)), IM_COL32(0, 0, 0, 110), hw);
				dl->AddRectFilled(ImVec2(gx - hw, y - hh), ImVec2(gx + hw, y + hh), Mix(IM_COL32(248, 244, 236, 255), Bright(L->accent, 0.35f), h), hw);
				return;
			}
			const float y = (a_a.y + a_b.y) * 0.5f, x0 = a_a.x + S(13.0f), x1 = a_b.x - S(13.0f);
			const float t = std::fmin(1.0f, std::fmax(0.0f, a_t));
			const float gx = x0 + (x1 - x0) * t;
			dl->AddRectFilled(ImVec2(x0, y - S(3.0f)), ImVec2(x1, y + S(3.0f)), a_parch ? WithAlpha(L->ink, 70) : IM_COL32(0, 0, 0, 160), S(3.0f));
			dl->AddRectFilledMultiColor(ImVec2(x0, y - S(3.0f)), ImVec2(gx, y + S(3.0f)), Bright(L->accent, 0.25f), Bright(L->accent, 0.25f), L->accentDim, L->accentDim);
			dl->AddRect(ImVec2(x0, y - S(3.0f)), ImVec2(x1, y + S(3.0f)), a_parch ? WithAlpha(L->ink, 110) : L->accentFaint, S(3.0f), 0, S(1.0f));
			Diamond(dl, ImVec2(gx + S(1.0f), y + S(2.0f)), S(12.0f), IM_COL32(0, 0, 0, 110));
			Diamond(dl, ImVec2(gx, y), S(12.0f), Dark(L->accentDim, 0.35f));
			Diamond(dl, ImVec2(gx, y), S(9.5f), a_hov ? Bright(L->accent, 0.3f) : L->accent);
			Diamond(dl, ImVec2(gx - S(1.5f), y - S(1.5f)), S(3.5f), IM_COL32(255, 246, 220, 190));
		}

		void HallSliderValue(const char* a_text, const ImVec2& a_a, const ImVec2& a_b, float a_valueW, bool a_parch)
		{
			if (Modern()) {  // number bright, unit dim ("200" "px")
				const float vs = S(16.0f);
				std::string num = a_text, unit;
				size_t      i = 0;
				while (i < num.size() && (std::isdigit(static_cast<unsigned char>(num[i])) || num[i] == '.' || num[i] == '-' || num[i] == ',')) {
					++i;
				}
				if (i > 0 && i < num.size()) {
					unit = num.substr(i);
					num.resize(i);
				}
				auto*        dl = ImGui::GetWindowDrawList();
				const ImVec2 tp(a_b.x + S(16.0f), (a_a.y + a_b.y - vs) * 0.5f);
				dl->AddText(LabelFont(), vs, tp, L->text, num.c_str());
				if (!unit.empty()) {
					dl->AddText(g_body, vs * 0.88f, ImVec2(tp.x + LabelFont()->CalcTextSizeA(vs, FLT_MAX, 0.0f, num.c_str()).x, tp.y + vs * 0.08f), L->textDim, unit.c_str());
				}
				ImGui::SameLine(0.0f, 0.0f);
				ImGui::ItemSize(ImVec2(a_valueW, a_b.y - a_a.y));  // room for the value without a new item: IsItemDeactivatedAfterEdit etc. still see the slider
				return;
			}
			const float vs = S(16.0f);
			Heading(ImGui::GetWindowDrawList(), g_body, vs, ImVec2(a_b.x + S(12.0f), (a_a.y + a_b.y - vs) * 0.5f), a_parch ? L->ink : L->text, a_text);
			ImGui::SameLine(0.0f, 0.0f);
			ImGui::ItemSize(ImVec2(a_valueW, a_b.y - a_a.y));  // room for the value without a new item: IsItemDeactivatedAfterEdit etc. still see the slider
		}

		// The invisible ImGui slider for the hall look; a_draw does the actual ImGui call.
		template <class F>
		bool HallSlider(float a_width, float& a_valueW, ImVec2& a_a, ImVec2& a_b, bool& a_hov, F a_draw)
		{
			a_valueW = (std::min)(S(150.0f), a_width * 0.36f);
			const float trackW = (std::max)(S(80.0f), a_width - a_valueW);
			for (ImGuiCol c : { ImGuiCol_FrameBg, ImGuiCol_FrameBgHovered, ImGuiCol_FrameBgActive, ImGuiCol_SliderGrab, ImGuiCol_SliderGrabActive }) {
				ImGui::PushStyleColor(c, C(0, 0, 0, 0));
			}
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(S(12.0f), S(10.0f)));
			ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, S(26.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
			ImGui::SetNextItemWidth(trackW);
			const bool changed = a_draw();
			ImGui::PopStyleVar(3);
			ImGui::PopStyleColor(5);
			a_a = ImGui::GetItemRectMin();
			a_b = ImGui::GetItemRectMax();
			a_hov = ImGui::IsItemHovered() || ImGui::IsItemActive();
			return changed;
		}
	}

	bool SliderInt(const char* a_label, int* a_v, int a_min, int a_max, const char* a_format, float a_width)
	{
		if (L->fx.hall) {
			const bool parch = g_parchmentDepth > 0 && !InDarkPanel();
			float      valueW;
			ImVec2     a, b;
			bool       hov;
			const bool changed = HallSlider(a_width, valueW, a, b, hov, [&] { return ImGui::SliderInt(a_label, a_v, a_min, a_max, ""); });
			HallSliderTrack(a, b, a_max > a_min ? static_cast<float>(*a_v - a_min) / (a_max - a_min) : 0.0f, hov, parch);
			char buf[96];
			std::snprintf(buf, sizeof buf, a_format, *a_v);
			HallSliderValue(buf, a, b, valueW, parch);
			return changed;
		}
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(S(12.0f), S(10.0f)));
		ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, S(26.0f));
		ImGui::SetNextItemWidth(a_width);
		const bool changed = ImGui::SliderInt(a_label, a_v, a_min, a_max, a_format);
		ImGui::PopStyleVar(2);
		return changed;
	}

	bool SliderFloat(const char* a_label, float* a_v, float a_min, float a_max, const char* a_format, float a_width)
	{
		if (L->fx.hall) {
			const bool parch = g_parchmentDepth > 0 && !InDarkPanel();
			float      valueW;
			ImVec2     a, b;
			bool       hov;
			const bool changed = HallSlider(a_width, valueW, a, b, hov, [&] { return ImGui::SliderFloat(a_label, a_v, a_min, a_max, ""); });
			HallSliderTrack(a, b, a_max > a_min ? (*a_v - a_min) / (a_max - a_min) : 0.0f, hov, parch);
			char buf[96];
			std::snprintf(buf, sizeof buf, a_format, *a_v);
			HallSliderValue(buf, a, b, valueW, parch);
			return changed;
		}
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(S(12.0f), S(10.0f)));
		ImGui::SetNextItemWidth(a_width);
		const bool changed = ImGui::SliderFloat(a_label, a_v, a_min, a_max, a_format);
		ImGui::PopStyleVar();
		return changed;
	}

	// ------------------------------------------------------------------ gallery hall controls
	bool Tab(const char* a_label, bool a_selected, float a_h, int a_icon)
	{
		if (Modern()) {  // text tab: capitals, dim count, gold underline that grows in
			auto*             dl = ImGui::GetWindowDrawList();
			const float       ts = S(14.5f);
			std::string       name = a_label, count;
			if (const auto sp2 = name.rfind("  "); sp2 != std::string::npos) {
				count = name.substr(sp2 + 2);
				name.resize(sp2);
			}
			const float  nw = HeadingWidth(LabelFont(), ts, name.c_str());
			const float  cw = count.empty() ? 0.0f : LabelFont()->CalcTextSizeA(ts, FLT_MAX, 0.0f, count.c_str()).x + S(10.0f);
			const float  iconW = a_icon >= 0 ? S(26.0f) : 0.0f;
			const ImVec2 size(iconW + nw + cw + S(8.0f), a_h);
			const ImVec2 p = ImGui::GetCursorScreenPos();
			const bool   pressed = ImGui::InvisibleButton("##tab", size);
			const bool   hov = ImGui::IsItemHovered();
			const float  t = Ease(ImGui::GetItemID(), a_selected, 14.0f);
			const float  hv = Ease(ImGui::GetItemID() + 1, hov, 16.0f);
			const ImU32  col = Mix(Mix(L->textDim, L->text, hv * 0.7f), L->text, t);
			const float  ty = p.y + (a_h - ts) * 0.5f - S(2.0f);
			if (a_icon >= 0) {
				TabIcon(dl, a_icon, ImVec2(p.x + S(9.0f), ty + ts * 0.5f), S(7.5f), Mix(WithAlpha(L->textDim, 200), L->accent, t));
			}
			Heading(dl, LabelFont(), ts, ImVec2(p.x + iconW, ty), col, name.c_str());
			if (!count.empty()) {
				dl->AddText(LabelFont(), ts, ImVec2(p.x + iconW + nw + S(10.0f), ty), Mix(L->textDim, L->accent, t), count.c_str());
			}
			const float uw = size.x * (0.25f + 0.75f * t), ux = p.x + (size.x - uw) * 0.5f, uy = p.y + a_h - S(3.0f);
			if (t > 0.01f) {
				dl->AddRectFilled(ImVec2(ux, uy - S(4.0f)), ImVec2(ux + uw, uy + S(4.0f)), WithAlpha(L->accent, static_cast<int>(30 * t)), S(4.0f));
				dl->AddRectFilled(ImVec2(ux, uy - S(1.0f)), ImVec2(ux + uw, uy + S(1.0f)), WithAlpha(L->accent, static_cast<int>(255 * t)), S(1.0f));
			} else if (hv > 0.01f) {
				dl->AddRectFilled(ImVec2(p.x, uy - S(0.5f)), ImVec2(p.x + size.x, uy + S(0.5f)), IM_COL32(255, 255, 255, static_cast<int>(50 * hv)));
			}
			return pressed;
		}
		auto*        dl = ImGui::GetWindowDrawList();
		const float  ts = S(16.0f);
		const float  tw = HeadingWidth(LabelFont(), ts, a_label);
		const float  iconW = a_icon >= 0 ? S(28.0f) : 0.0f, padX = S(18.0f);
		const ImVec2 size(padX * 2 + iconW + tw, a_h);
		const ImVec2 p = ImGui::GetCursorScreenPos();
		const bool   pressed = ImGui::InvisibleButton("##tab", size);
		const bool   hov = ImGui::IsItemHovered();
		const ImVec2 a = p, b = p + size;
		SoftShadow(dl, a, b, 0.0f, S(6.0f), 90);
		if (a_selected) {
			dl->AddRectFilledMultiColor(a, b, C(242, 228, 192), C(242, 228, 192), C(206, 182, 134), C(206, 182, 134));
			dl->AddRect(a - ImVec2(S(2), S(2)), b + ImVec2(S(2), S(2)), WithAlpha(L->accent, 70), 0.0f, 0, S(3.0f));
			dl->AddRect(a, b, L->accent, 0.0f, 0, S(1.6f));
			dl->AddRect(a + ImVec2(S(3), S(3)), b - ImVec2(S(3), S(3)), WithAlpha(L->accentDim, 200), 0.0f, 0, S(1.0f));
		} else {
			dl->AddRectFilledMultiColor(a, b, hov ? C(58, 50, 40) : C(42, 38, 33), hov ? C(58, 50, 40) : C(42, 38, 33), C(22, 20, 18), C(22, 20, 18));
			Bevel(dl, a, b, S(1.2f), false, 40, 140);
			dl->AddRect(a, b, hov ? L->accent : L->accentDim, 0.0f, 0, S(1.2f));
		}
		const ImU32 col = a_selected ? L->ink : (hov ? L->text : L->textDim);
		if (a_icon >= 0) {
			TabIcon(dl, a_icon, ImVec2(a.x + padX + S(10.0f), (a.y + b.y) * 0.5f), S(9.0f), a_selected ? Dark(L->accentDim, 0.35f) : (hov ? L->accent : L->accentDim));
		}
		Heading(dl, LabelFont(), ts, ImVec2(a.x + padX + iconW, (a.y + b.y - ts) * 0.5f), col, a_label);
		return pressed;
	}

	bool ToggleSwitches() { return L->fx.hall != 0; }

	float ButtonLabelWidth(const char* a_label)
	{
		const std::string shown(a_label, ImGui::FindRenderedTextEnd(a_label));
		return L->fx.hall ? HeadingWidth(LabelFont(), S(17.0f), shown.c_str()) : ImGui::CalcTextSize(shown.c_str()).x;
	}

	bool Toggle(const char* a_id, bool* a_v)
	{
		ImGui::PushID(a_id);
		const ImVec2 size(S(60.0f), S(30.0f));
		const ImVec2 p = ImGui::GetCursorScreenPos() + ImVec2(0, S(5.0f));
		ImGui::SetCursorScreenPos(p);
		const bool pressed = ImGui::InvisibleButton("##switch", size);
		const bool hov = ImGui::IsItemHovered();
		if (pressed) {
			*a_v = !*a_v;
		}
		auto*        dl = ImGui::GetWindowDrawList();
		const ImVec2 a = p, b = p + size;
		const float  r = size.y * 0.5f;
		if (Modern()) {  // iOS-like switch that slides
			const float t = Ease(ImGui::GetItemID(), *a_v, 14.0f);
			dl->AddRectFilled(a, b, Mix(IM_COL32(255, 255, 255, 28), L->accent, t), r);
			dl->AddRect(a, b, hov ? WithAlpha(L->accent, 220) : IM_COL32(255, 255, 255, static_cast<int>(40 * (1 - t))), r, 0, S(1.0f));
			const ImVec2 k(a.x + r + (size.x - 2 * r) * t, (a.y + b.y) * 0.5f);
			dl->AddCircleFilled(k + ImVec2(0, S(1.5f)), r - S(3.0f), IM_COL32(0, 0, 0, 80), 24);
			dl->AddCircleFilled(k, r - S(3.5f), IM_COL32(250, 248, 244, 255), 24);
			ImGui::SetCursorScreenPos(ImVec2(p.x, b.y));
			ImGui::Dummy(ImVec2(size.x, S(5.0f)));
			ImGui::PopID();
			return pressed;
		}
		if (*a_v) {
			dl->AddRectFilledMultiColor(a + ImVec2(r * 0.4f, 0), b - ImVec2(r * 0.4f, 0), Bright(L->accent, 0.15f), Bright(L->accent, 0.15f), L->accentDim, L->accentDim);
			dl->AddRectFilled(a, b, WithAlpha(L->accentDim, 160), r);
			dl->AddRectFilled(a + ImVec2(S(1), S(1)), ImVec2(b.x - S(1), (a.y + b.y) * 0.5f), IM_COL32(255, 240, 200, 50), r, ImDrawFlags_RoundCornersTop);
		} else {
			dl->AddRectFilled(a, b, C(74, 62, 48), r);
			dl->AddRectFilledMultiColor(a + ImVec2(r, 0), ImVec2(b.x - r, a.y + S(8.0f)), IM_COL32(0, 0, 0, 90), IM_COL32(0, 0, 0, 90), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));
		}
		dl->AddRect(a, b, hov ? L->accent : Dark(L->accentDim, 0.2f), r, 0, S(1.4f));
		const ImVec2 k(*a_v ? b.x - r : a.x + r, (a.y + b.y) * 0.5f);
		dl->AddCircleFilled(k + ImVec2(S(1.0f), S(2.0f)), r - S(3.0f), IM_COL32(0, 0, 0, 90), 24);
		dl->AddCircleFilled(k, r - S(3.5f), C(246, 238, 216), 24);
		dl->AddCircle(k, r - S(3.5f), Dark(L->accentDim, 0.1f), 24, S(1.2f));
		dl->AddCircleFilled(k - ImVec2(S(3.0f), S(3.0f)), S(3.5f), IM_COL32(255, 255, 255, 140), 10);
		ImGui::SetCursorScreenPos(ImVec2(p.x, b.y));
		ImGui::Dummy(ImVec2(size.x, S(5.0f)));
		ImGui::PopID();
		return pressed;
	}

	namespace
	{
		// "dragonsreach_hall.png" -> "dragonsreach hall", shortened with "..." to fit a_maxW
		std::string Caption(const char* a_name, float a_size, float a_maxW)
		{
			std::string s = a_name ? a_name : "";
			if (const auto dot = s.find_last_of('.'); dot != std::string::npos && dot > 0) {
				s.resize(dot);
			}
			for (auto& ch : s) {
				if (ch == '_') {
					ch = ' ';
				}
			}
			if (HeadingWidth(LabelFont(), a_size, s.c_str()) <= a_maxW) {
				return s;
			}
			while (!s.empty() && HeadingWidth(LabelFont(), a_size, (s + "...").c_str()) > a_maxW) {
				s.pop_back();
			}
			return s + "...";
		}

		// UVs that crop the picture to fill a box of aspect a_boxW:a_boxH (centre crop)
		void CoverUV(float a_w, float a_h, float a_boxW, float a_boxH, ImVec2& a_uv0, ImVec2& a_uv1)
		{
			a_uv0 = ImVec2(0, 0);
			a_uv1 = ImVec2(1, 1);
			if (a_w <= 0 || a_h <= 0 || a_boxW <= 0 || a_boxH <= 0) {
				return;
			}
			const float pic = a_w / a_h, box = a_boxW / a_boxH;
			if (pic > box) {
				const float k = box / pic;
				a_uv0.x = (1 - k) * 0.5f;
				a_uv1.x = 1 - a_uv0.x;
			} else {
				const float k = pic / box;
				a_uv0.y = (1 - k) * 0.5f;
				a_uv1.y = 1 - a_uv0.y;
			}
		}

		ImVec2 FitIn(float a_w, float a_h, const ImVec2& a_a, const ImVec2& a_b, ImVec2& a_out1)
		{
			const ImVec2 box = a_b - a_a;
			if (a_w <= 0 || a_h <= 0) {
				a_out1 = a_b;
				return a_a;
			}
			const float  s = (std::min)(box.x / a_w, box.y / a_h);
			const ImVec2 sz(a_w * s, a_h * s);
			const ImVec2 o = a_a + (box - sz) * 0.5f;
			a_out1 = o + sz;
			return o;
		}
	}

	float PictureCardHeight(float a_cell)
	{
		switch (L->fx.hall) {
		case 1: return a_cell * 0.80f + S(34.0f);
		case 2: return a_cell * 0.76f + S(22.0f);
		case 3: return a_cell * 0.74f + S(34.0f);
		case 4: return a_cell;  // square slot; the card inside has the picture's shape
		default: return a_cell;
		}
	}

	void PictureCard(ImDrawList* a_dl, const ImVec2& a_pos, float a_cell, ImTextureID a_tex, float a_w, float a_h,
		const char* a_name, const char* a_placeholder, bool a_hovered, const char* a_detail)
	{
		const float H = PictureCardHeight(a_cell);
		if (Modern()) {  // the picture is the card: rounded, name on a fade at the bottom, lifts and glows on hover
			const float t = Ease(ImGui::GetID("##lift"), a_hovered, 14.0f);
			const float r = S(8.0f);
			// the card takes the picture's shape inside the square slot, sitting on the bottom so rows line up
			const float aspect = (a_w > 0 && a_h > 0) ? a_w / a_h : 4.0f / 3.0f;
			const float cw = aspect >= 1.0f ? a_cell : a_cell * aspect, ch = aspect >= 1.0f ? a_cell / aspect : H;
			const ImVec2 a = a_pos + ImVec2((a_cell - cw) * 0.5f, H - ch) - ImVec2(0, S(5.0f) * t), b = a + ImVec2(cw, ch);
			SoftShadow(a_dl, a, b, r, S(12.0f + 14.0f * t), static_cast<int>(120 + 80 * t));
			if (t > 0.01f) {
				a_dl->AddRect(a - ImVec2(S(3), S(3)), b + ImVec2(S(3), S(3)), WithAlpha(L->accent, static_cast<int>(55 * t)), r + S(3), 0, S(4.0f));
			}
			if (a_tex) {
				ImVec2 uv0, uv1;
				CoverUV(a_w, a_h, b.x - a.x, b.y - a.y, uv0, uv1);
				const int v = static_cast<int>(212 + 43 * t);
				a_dl->AddImageRounded(a_tex, a, b, uv0, uv1, IM_COL32(v, v, v, 255), r);
			} else {  // skeleton
				a_dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 12), r);
				const float sx = a.x + (b.x - a.x) * std::fmod(static_cast<float>(ImGui::GetTime()) * 0.6f, 1.4f) - (b.x - a.x) * 0.2f;
				a_dl->PushClipRect(a, b, true);
				a_dl->AddRectFilledMultiColor(ImVec2(sx - S(60), a.y), ImVec2(sx, b.y), IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 14), IM_COL32(255, 255, 255, 14), IM_COL32(255, 255, 255, 0));
				a_dl->AddRectFilledMultiColor(ImVec2(sx, a.y), ImVec2(sx + S(60), b.y), IM_COL32(255, 255, 255, 14), IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 14));
				a_dl->PopClipRect();
			}
			// fade to black under the name (bands, the last one rounded to the card's corners)
			const float fadeTop = b.y - (b.y - a.y) * 0.48f;
			const int   n = 10;
			for (int i = 0; i < n; ++i) {
				const float y0 = fadeTop + (b.y - fadeTop) * i / n, y1 = (i == n - 1) ? b.y : fadeTop + (b.y - fadeTop) * (i + 1) / n;
				const float k = (i + 1.0f) / n;
				a_dl->AddRectFilled(ImVec2(a.x, y0), ImVec2(b.x, y1 + 0.5f), IM_COL32(6, 6, 8, static_cast<int>(210 * k * k)), i == n - 1 ? r : 0.0f, ImDrawFlags_RoundCornersBottom);
			}
			// name + size
			std::string name = a_name ? a_name : "";
			if (const auto dot = name.find_last_of('.'); dot != std::string::npos && dot > 0) {
				name.resize(dot);
			}
			for (auto& ch : name) {
				if (ch == '_' || ch == '-') {
					ch = ' ';
				}
			}
			if (!name.empty()) {
				name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
			}
			const float ns = S(16.0f), ds = S(12.5f), maxW = (b.x - a.x) - S(28.0f);
			auto width = [&](const std::string& s) { return LabelFont()->CalcTextSizeA(ns, FLT_MAX, 0.0f, s.c_str()).x; };
			if (width(name) > maxW) {
				while (name.size() > 1 && width(name + "...") > maxW) {
					name.pop_back();
				}
				name += "...";
			}
			const bool  detail = a_detail && *a_detail;
			const float ny = b.y - S(14.0f) - ns - (detail ? ds + S(5.0f) : 0.0f);
			a_dl->AddText(LabelFont(), ns, ImVec2(a.x + S(14.0f), ny + S(1.0f)), IM_COL32(0, 0, 0, 160), name.c_str());
			a_dl->AddText(LabelFont(), ns, ImVec2(a.x + S(14.0f), ny), IM_COL32(248, 244, 236, 255), name.c_str());
			if (detail) {
				a_dl->AddText(g_body, ds, ImVec2(a.x + S(14.0f), ny + ns + S(5.0f)), Mix(IM_COL32(200, 194, 184, 200), L->accent, t), a_detail);
			}
			// border: hairline, warm accent when hovered
			a_dl->AddRect(a, b, Mix(IM_COL32(255, 255, 255, 26), L->accent, t), r, 0, S(1.0f + 0.5f * t));
			return;
		}
		auto placeholder = [&](const ImVec2& a, const ImVec2& b, ImU32 col) {
			const char*  txt = a_placeholder ? a_placeholder : "";
			const ImVec2 ts = g_italic->CalcTextSizeA(g_italic->FontSize, FLT_MAX, 0.0f, txt);
			a_dl->AddText(g_italic, g_italic->FontSize, (a + b - ts) * 0.5f, col, txt);
		};
		if (L->fx.hall == 1) {  // wooden frame, parchment mat, name on the mat above the picture
			const ImVec2 a = a_pos, b = a_pos + ImVec2(a_cell, H);
			SoftShadow(a_dl, a, b, 0.0f, S(a_hovered ? 16.0f : 11.0f), a_hovered ? 150 : 120);
			const float fw = S(9.0f);
			a_dl->AddRectFilledMultiColor(a, b, C(84, 61, 40), C(70, 51, 34), C(34, 25, 17), C(46, 34, 23));
			if (g_texGrain) {
				Tiled(a_dl, g_texGrain, a, b, IM_COL32(0, 0, 0, 190));
			}
			Bevel(a_dl, a, b, S(1.5f), false, 70, 160);
			const ImVec2 m0 = a + ImVec2(fw, fw), m1 = b - ImVec2(fw, fw);
			Bevel(a_dl, m0 - ImVec2(S(1.5f), S(1.5f)), m1 + ImVec2(S(1.5f), S(1.5f)), S(1.5f), true, 50, 170);
			a_dl->AddRectFilled(m0, m1, L->panel);
			if (g_texParch) {
				Tiled(a_dl, g_texParch, m0, m1, Dark(L->panelBorder, 0.35f));
			}
			a_dl->AddRectFilledMultiColor(m0, ImVec2(m1.x, m0.y + S(8.0f)), IM_COL32(60, 35, 15, 70), IM_COL32(60, 35, 15, 70), IM_COL32(60, 35, 15, 0), IM_COL32(60, 35, 15, 0));
			const float       cs = S(14.5f);
			const std::string cap = Caption(a_name, cs, (m1.x - m0.x) - S(16.0f));
			const float       cw = HeadingWidth(LabelFont(), cs, cap.c_str());
			Heading(a_dl, LabelFont(), cs, ImVec2((m0.x + m1.x - cw) * 0.5f, m0.y + S(8.0f)), L->ink, cap.c_str());
			const ImVec2 pa = m0 + ImVec2(S(10.0f), S(32.0f)), pb = m1 - ImVec2(S(10.0f), S(10.0f));
			if (a_tex) {
				ImVec2       i1;
				const ImVec2 i0 = FitIn(a_w, a_h, pa, pb, i1);
				a_dl->AddRectFilled(i0 - ImVec2(S(3), S(3)), i1 + ImVec2(S(3), S(3)), C(40, 30, 20));
				a_dl->AddImage(a_tex, i0, i1);
				a_dl->AddRect(i0 - ImVec2(S(3), S(3)), i1 + ImVec2(S(3), S(3)), Dark(L->panelBorder, 0.3f), 0.0f, 0, S(1.0f));
				Bevel(a_dl, i0, i1, S(1.5f), true, 40, 120);
			} else {
				a_dl->AddRectFilled(pa, pb, WithAlpha(L->ink, 40));
				placeholder(pa, pb, L->inkSoft);
			}
			if (a_hovered) {
				a_dl->AddRect(a - ImVec2(S(2), S(2)), b + ImVec2(S(2), S(2)), Bright(L->accent, 0.2f), 0.0f, 0, S(2.5f));
				a_dl->AddRect(a - ImVec2(S(5), S(5)), b + ImVec2(S(5), S(5)), WithAlpha(L->accent, 70), 0.0f, 0, S(3.0f));
			}
			return;
		}
		if (L->fx.hall == 2) {  // picture filling a gold-cornered frame, name plaque across the bottom edge
			const float  ph = a_cell * 0.76f;
			const ImVec2 a = a_pos, b = a_pos + ImVec2(a_cell, ph);
			SoftShadow(a_dl, a, b, 0.0f, S(a_hovered ? 16.0f : 11.0f), a_hovered ? 150 : 120);
			a_dl->AddRectFilled(a, b, C(14, 13, 12));
			const ImVec2 i0 = a + ImVec2(S(6.0f), S(6.0f)), i1 = b - ImVec2(S(6.0f), S(6.0f));
			if (a_tex) {
				ImVec2 uv0, uv1;
				CoverUV(a_w, a_h, i1.x - i0.x, i1.y - i0.y, uv0, uv1);
				a_dl->AddImage(a_tex, i0, i1, uv0, uv1);
			} else {
				placeholder(i0, i1, L->muted);
			}
			Bevel(a_dl, i0, i1, S(1.5f), true, 30, 140);
			a_dl->AddRect(a, b, Dark(L->accentDim, 0.3f), 0.0f, 0, S(2.0f));
			a_dl->AddRect(a + ImVec2(S(3), S(3)), b - ImVec2(S(3), S(3)), a_hovered ? L->accent : L->accentDim, 0.0f, 0, S(1.4f));
			const float  k = S(20.0f), t = S(3.5f);
			const ImU32  cc = a_hovered ? Bright(L->accent, 0.25f) : L->accent;
			for (int i = 0; i < 4; ++i) {
				const ImVec2 c(i & 1 ? b.x : a.x, i & 2 ? b.y : a.y);
				const ImVec2 d(i & 1 ? -1.0f : 1.0f, i & 2 ? -1.0f : 1.0f);
				a_dl->AddRectFilled(ImVec2((std::min)(c.x, c.x + d.x * k), (std::min)(c.y, c.y + d.y * t)), ImVec2((std::max)(c.x, c.x + d.x * k), (std::max)(c.y, c.y + d.y * t)), cc);
				a_dl->AddRectFilled(ImVec2((std::min)(c.x, c.x + d.x * t), (std::min)(c.y, c.y + d.y * k)), ImVec2((std::max)(c.x, c.x + d.x * t), (std::max)(c.y, c.y + d.y * k)), cc);
				Diamond(a_dl, c + ImVec2(d.x * S(9.0f), d.y * S(9.0f)), S(3.0f), cc);
			}
			// name plaque with angled ends, centred on the bottom edge
			const float       cs = S(14.0f);
			const std::string cap = Caption(a_name, cs, a_cell - S(40.0f));
			const float       cw = HeadingWidth(LabelFont(), cs, cap.c_str());
			const float       pw = (std::min)(a_cell + S(8.0f), cw + S(40.0f)), pht = S(32.0f), notch = S(12.0f);
			const float       pcx = (a.x + b.x) * 0.5f, pcy = b.y;
			const ImVec2      q0(pcx - pw * 0.5f, pcy - pht * 0.5f), q1(pcx + pw * 0.5f, pcy + pht * 0.5f);
			a_dl->AddRectFilled(q0 + ImVec2(S(2), S(4)), q1 + ImVec2(S(2), S(4)), IM_COL32(0, 0, 0, 110));
			const ImVec2 poly[] = { ImVec2(q0.x, pcy), ImVec2(q0.x + notch, q0.y), ImVec2(q1.x - notch, q0.y), ImVec2(q1.x, pcy), ImVec2(q1.x - notch, q1.y), ImVec2(q0.x + notch, q1.y) };
			a_dl->AddConvexPolyFilled(poly, 6, C(36, 31, 26));
			a_dl->AddRectFilledMultiColor(ImVec2(q0.x + notch, q0.y + S(1)), ImVec2(q1.x - notch, pcy), IM_COL32(255, 240, 200, 18), IM_COL32(255, 240, 200, 18), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));
			a_dl->AddPolyline(poly, 6, a_hovered ? L->accent : L->accentDim, ImDrawFlags_Closed, S(1.5f));
			Heading(a_dl, LabelFont(), cs, ImVec2(pcx - cw * 0.5f, pcy - cs * 0.55f), a_hovered ? Bright(L->accent, 0.3f) : L->accent, cap.c_str());
			return;
		}
		// 3: dark tile with gold double lines and notched corners, name underneath on the parchment
		const float  ph = a_cell * 0.74f;
		const ImVec2 a = a_pos, b = a_pos + ImVec2(a_cell, ph);
		SoftShadow(a_dl, a, b, 0.0f, S(a_hovered ? 14.0f : 9.0f), a_hovered ? 120 : 90);
		if (a_hovered) {  // warm glow
			for (int i = 3; i >= 1; --i) {
				a_dl->AddRect(a - ImVec2(S(2.0f * i), S(2.0f * i)), b + ImVec2(S(2.0f * i), S(2.0f * i)), WithAlpha(L->accent, 40 * (4 - i)), 0.0f, 0, S(2.5f));
			}
		}
		a_dl->AddRectFilled(a, b, C(26, 22, 18));
		const ImVec2 i0 = a + ImVec2(S(12.0f), S(12.0f)), i1 = b - ImVec2(S(12.0f), S(12.0f));
		if (a_tex) {
			ImVec2       f1;
			const ImVec2 f0 = FitIn(a_w, a_h, i0, i1, f1);
			a_dl->AddImage(a_tex, f0, f1);
		} else {
			placeholder(i0, i1, L->muted);
		}
		const ImU32 gl = a_hovered ? Bright(L->accent, 0.2f) : L->accentDim;
		a_dl->AddRect(a + ImVec2(S(4), S(4)), b - ImVec2(S(4), S(4)), gl, 0.0f, 0, S(1.3f));
		a_dl->AddRect(a + ImVec2(S(8), S(8)), b - ImVec2(S(8), S(8)), WithAlpha(gl, 150), 0.0f, 0, S(1.0f));
		const float n = S(10.0f);
		for (int i = 0; i < 4; ++i) {
			const ImVec2 c(i & 1 ? b.x - S(4) : a.x + S(4), i & 2 ? b.y - S(4) : a.y + S(4));
			const ImVec2 d(i & 1 ? -1.0f : 1.0f, i & 2 ? -1.0f : 1.0f);
			a_dl->AddLine(c + ImVec2(d.x * n, 0), c + ImVec2(0, d.y * n), gl, S(1.3f));
		}
		a_dl->AddRect(a, b, a_hovered ? L->accent : Dark(L->accentDim, 0.3f), 0.0f, 0, a_hovered ? S(2.0f) : S(1.4f));
		const float       cs = S(15.0f);
		const std::string cap = Caption(a_name, cs, a_cell);
		const float       cw = HeadingWidth(LabelFont(), cs, cap.c_str());
		Heading(a_dl, LabelFont(), cs, ImVec2((a.x + b.x - cw) * 0.5f, b.y + S(10.0f)), a_hovered ? Dark(L->accentDim, 0.2f) : L->ink, cap.c_str());
	}

	// ------------------------------------------------------------------ modern extras
	float FooterHeight() { return Modern() ? S(52.0f) : 0.0f; }

	// Key caps: "Ctrl+Wheel" = two caps joined by +, "Wheel/Left/Right" = alternatives split by /,
	// Up/Down/Left/Right draw an arrow. Returns the width; a_draw false only measures.
	float KeyHintsAt(ImDrawList* a_dl, const ImVec2& a_pos, const char* const* a_keys, const char* const* a_labels, int a_count, bool a_draw)
	{
		const float ks = S(12.5f), ls = S(14.0f), kh = S(26.0f);
		float       x = a_pos.x;
		for (int i = 0; i < a_count; ++i) {
			const std::string keys = a_keys[i];
			size_t            start = 0;
			while (true) {
				const size_t      sep = keys.find_first_of("+/", start);
				const std::string k = keys.substr(start, sep == std::string::npos ? std::string::npos : sep - start);
				const int         arrow = k == "Up" ? 0 : k == "Down" ? 1 : k == "Left" ? 2 : k == "Right" ? 3 : -1;
				const ImVec2      kt = LabelFont()->CalcTextSizeA(ks, FLT_MAX, 0.0f, k.c_str());
				const float       kw = arrow >= 0 ? kh : (std::max)(kh, kt.x + S(16.0f));
				if (a_draw) {
					const ImVec2 ka(x, a_pos.y), kb(x + kw, a_pos.y + kh);
					a_dl->AddRectFilled(ka + ImVec2(0, S(2)), kb + ImVec2(0, S(2)), IM_COL32(0, 0, 0, 90), S(5.0f));
					a_dl->AddRectFilled(ka, kb, IM_COL32(255, 255, 255, 18), S(5.0f));
					a_dl->AddRect(ka, kb, IM_COL32(255, 255, 255, 60), S(5.0f), 0, S(1.0f));
					if (arrow >= 0) {
						const ImVec2 c = (ka + kb) * 0.5f;
						const float  r = S(5.0f);
						const ImVec2 d[4] = { ImVec2(0, -1), ImVec2(0, 1), ImVec2(-1, 0), ImVec2(1, 0) };
						const ImVec2 n(-d[arrow].y, d[arrow].x);
						a_dl->AddTriangleFilled(c + d[arrow] * r, c - d[arrow] * r * 0.6f + n * r * 0.9f, c - d[arrow] * r * 0.6f - n * r * 0.9f, L->text);
					} else {
						a_dl->AddText(LabelFont(), ks, ImVec2(x + (kw - kt.x) * 0.5f, a_pos.y + (kh - kt.y) * 0.5f), L->text, k.c_str());
					}
				}
				x += kw;
				if (sep == std::string::npos) {
					break;
				}
				const char sepText[2] = { keys[sep], 0 };
				if (a_draw) {
					a_dl->AddText(g_body, ls, ImVec2(x + S(4.0f), a_pos.y + (kh - ls) * 0.5f - S(1.0f)), L->textDim, sepText);
				}
				x += S(16.0f);
				start = sep + 1;
			}
			const ImVec2 lt = g_body->CalcTextSizeA(ls, FLT_MAX, 0.0f, a_labels[i]);
			if (a_draw) {
				a_dl->AddText(g_body, ls, ImVec2(x + S(10.0f), a_pos.y + (kh - lt.y) * 0.5f), L->textDim, a_labels[i]);
			}
			x += S(10.0f) + lt.x + (i + 1 < a_count ? S(30.0f) : 0.0f);
		}
		return x - a_pos.x;
	}

	void KeyHints(const char* const* a_keys, const char* const* a_labels, int a_count)
	{
		if (!Modern()) {
			return;
		}
		const ImVec2 p = ImGui::GetCursorScreenPos() + ImVec2(0, S(14.0f));
		const float  w = KeyHintsAt(ImGui::GetWindowDrawList(), p, a_keys, a_labels, a_count, true);
		ImGui::Dummy(ImVec2(w, S(26.0f) + S(14.0f)));
	}

	bool IsModern() { return Modern(); }

	bool DescriptionPane() { return Modern(); }
	void SetHoveredHelp(const char* a_label, const char* a_help)
	{
		g_hoverLabel = a_label;
		g_hoverHelp = a_help;
	}
	void DrawDescription(const char* a_fallbackTitle, const char* a_fallbackText)
	{
		const char*  title = g_hoverLabel ? g_hoverLabel : a_fallbackTitle;
		const char*  text = g_hoverLabel ? (g_hoverHelp ? g_hoverHelp : "") : a_fallbackText;
		auto*        dl = ImGui::GetWindowDrawList();
		const ImVec2 p = ImGui::GetCursorScreenPos();
		const float  w = ImGui::GetContentRegionAvail().x;
		dl->AddRectFilled(p + ImVec2(0, S(4.0f)), p + ImVec2(S(28.0f), S(6.0f)), L->accent);
		ImGui::Dummy(ImVec2(w, S(16.0f)));
		ImGui::PushFont(g_bold);
		ImGui::SetWindowFontScale(0.66f);
		ImGui::PushStyleColor(ImGuiCol_Text, L->text);
		ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + w);
		ImGui::TextUnformatted(title);
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopFont();
		ImGui::Dummy(ImVec2(0, S(6.0f)));
		ImGui::PushStyleColor(ImGuiCol_Text, L->textDim);
		ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + w);
		ImGui::TextUnformatted(text);
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();
		g_hoverLabel = nullptr;
		g_hoverHelp = nullptr;
	}

	float CardInset() { return S(L->fx.gilt ? 13.0f : 4.0f); }
	void CardBack(ImDrawList* a_dl, const ImVec2& a_a, const ImVec2& a_b, bool a_hovered)
	{
		const float r = S(L->rounding);
		if (L->fx.shadows) {
			SoftShadow(a_dl, a_a, a_b, r, S(a_hovered ? 16.0f : 10.0f), a_hovered ? 120 : 90);
		} else {
			a_dl->AddRectFilled(a_a + ImVec2(S(3), S(4)), a_b + ImVec2(S(3), S(4)), L->shadow, r);
		}
		if (L->fx.gilt) {  // gilt frame + dark mat
			a_dl->AddRectFilled(a_a, a_b, Dark(L->accentDim, 0.3f));
			GoldBand(a_dl, a_a, a_b, S(8.0f));
			const ImVec2 ma = a_a + ImVec2(S(8.0f), S(8.0f)), mb = a_b - ImVec2(S(8.0f), S(8.0f));
			a_dl->AddRectFilled(ma, mb, L->well);
			Bevel(a_dl, ma, mb, S(2.0f), true, 30, 140);
			return;
		}
		a_dl->AddRectFilled(a_a, a_b, L->well, r);
		if (L->fx.bevel) {
			Bevel(a_dl, a_a, a_b, S(2.0f), true, 50, 120);
		}
	}
	void CardFront(ImDrawList* a_dl, const ImVec2& a_a, const ImVec2& a_b, bool a_hovered)
	{
		const float r = S(L->rounding);
		if (L->fx.gilt) {
			if (a_hovered) {
				a_dl->AddRect(a_a - ImVec2(S(2), S(2)), a_b + ImVec2(S(2), S(2)), Bright(L->accent, 0.3f), 0.0f, 0, S(2.5f));
				a_dl->AddRectFilled(a_a + ImVec2(S(8), S(8)), a_b - ImVec2(S(8), S(8)), IM_COL32(255, 240, 200, 18));
			}
			return;
		}
		if (L->fx.bevel && !a_hovered) {
			a_dl->AddRect(a_a, a_b, L->cardBorder, r, 0, S(1.2f));
			a_dl->AddLine(a_a + ImVec2(r, 1), ImVec2(a_b.x - r, a_a.y + 1), IM_COL32(255, 248, 225, 50), 1.0f);
			return;
		}
		a_dl->AddRect(a_a, a_b, a_hovered ? L->accent : L->cardBorder, r, 0, a_hovered ? S(3.0f) : S(1.2f));
	}
}
