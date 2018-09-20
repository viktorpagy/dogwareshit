#include "../includes.h"
#include "../UTILS/interfaces.h"
#include "../SDK/IEngine.h"
#include "../SDK/CClientEntityList.h"
#include "../SDK/CInputSystem.h"
#include "../UTILS/render.h"
#include "../SDK/ConVar.h"
#include "Components.h"
#include "..\ImGui\imgui.h"
#include <d3d9.h>
#include "menu_framework.h"
#include "../HOOKS/hooks.h"
#include <winuser.h>

int menutab;

typedef void(*CL_FullUpdate_t) (void);
CL_FullUpdate_t CL_FullUpdate = nullptr;

int AutoCalc(int va)
{
	if (va == 1)
		return va * 35;
	else if (va == 2)
		return va * 34;
	else
		return va * 25 + 7.5;
}

namespace MENU
{
	void InitColors()
	{
		using namespace PPGUI_PP_GUI;


		colors[WINDOW_BODY] = CColor(22, 22, 22);
		colors[WINDOW_TITLE_BAR] = CColor(28, 28, 28, 0); //255, 75, 0
		colors[WINDOW_TEXT] = CColor(137, 180, 48);

		colors[GROUPBOX_BODY] = CColor(40, 40, 40, 0);
		colors[GROUPBOX_OUTLINE] = CColor(60, 60, 60);
		colors[GROUPBOX_TEXT] = WHITE;

		colors[SCROLLBAR_BODY] = SETTINGS::settings.menu_col;

		colors[SEPARATOR_TEXT] = WHITE;
		colors[SEPARATOR_LINE] = CColor(60, 60, 60, 255);

		colors[CHECKBOX_CLICKED] = CColor(162, 204, 47);//HOTPINK
		colors[CHECKBOX_NOT_CLICKED] = CColor(60, 60, 60, 255);
		colors[CHECKBOX_TEXT] = WHITE;

		colors[BUTTON_BODY] = CColor(40, 40, 40, 255);
		colors[BUTTON_TEXT] = WHITE;

		colors[COMBOBOX_TEXT] = LIGHTGREY;
		colors[COMBOBOX_SELECTED] = CColor(40, 40, 40, 255);
		colors[COMBOBOX_SELECTED_TEXT] = WHITE;
		colors[COMBOBOX_ITEM] = CColor(20, 20, 20, 255);
		colors[COMBOBOX_ITEM_TEXT] = LIGHTGREY;

		colors[SLIDER_BODY] = CColor(40, 40, 40, 255);
		colors[SLIDER_VALUE] = CColor(137, 180, 48);//HOTPINK
		colors[SLIDER_TEXT] = WHITE;

		colors[TAB_BODY] = CColor(21, 21, 19);
		colors[TAB_TEXT] = CColor(255, 255, 255, 255);
		colors[TAB_BODY_SELECTED] = CColor(40, 40, 40, 150); //HOTPINK
		colors[TAB_TEXT_SELECTED] = CColor(137, 180, 48);

		colors[VERTICAL_TAB_BODY] = CColor(22, 22, 22, 120);
		colors[VERTICAL_TAB_TEXT] = CColor(255, 255, 255, 255);
		colors[VERTICAL_TAB_OUTLINE] = CColor(22, 22, 22, 150); //HOTPINK
		colors[VERTICAL_TAB_BODY_SELECTED] = CColor(22, 22, 22, 255); //HOTPINK
		colors[VERTICAL_TAB_TEXT_SELECTED] = CColor(137, 180, 48);

		colors[COLOR_PICKER_BODY] = CColor(50, 50, 50, 0);
		colors[COLOR_PICKER_TEXT] = WHITE;
		colors[COLOR_PICKER_OUTLINE] = CColor(0, 0, 0, 0);
	}

	void Do()
	{
		static bool menu_open = false;

		if (UTILS::INPUT::input_handler.GetKeyState(VK_INSERT) & 1)
		{
			menu_open = !menu_open;
			static const auto cvar = INTERFACES::cvar->FindVar("cl_mouseenable");
			if (menu_open)
				cvar->SetValue(0);
			else
				cvar->SetValue(1);

			INTERFACES::InputSystem->EnableInput(!menu_open);
		}

		if (GetAsyncKeyState(UTILS::INPUT::input_handler.keyBindings(SETTINGS::settings.flip_int)) & 1)
			SETTINGS::settings.flip_bool = !SETTINGS::settings.flip_bool;

		if (GetAsyncKeyState(UTILS::INPUT::input_handler.keyBindings(SETTINGS::settings.quickstopkey)) & 1)
			SETTINGS::settings.stop_flip = !SETTINGS::settings.stop_flip;

		if (GetKeyState(UTILS::INPUT::input_handler.keyBindings(SETTINGS::settings.overridekey)) & 1)
			SETTINGS::settings.overridething = !SETTINGS::settings.overridething;


		InitColors();
		if (menu_hide)
		{

		}
		else
		{
			if (menu_open)
			{
				using namespace PPGUI_PP_GUI;
				if (!menu_open) return;

				DrawMouse();

				SetFont(FONTS::menu_window_font);

				//SKIN STUFF BEGIN//
				std::vector<std::string> KnifeModel = { "bayonet",
					"flip",
					"gut",
					"karam",
					"m9",
					"huntsman",
					"butterfly",
					"falchion",
					"shadow daggers",
					"bowie",
					"navaja",
					"stilleto",
					"ursus",
					"talon" };
				std::vector<std::string> KnifeSkins = { "None",
					"Crimson Web",
					"Bone Mask",
					"Fade",
					"Night",
					"Blue Steel",
					"Stained",
					"Case Hardened",
					"Slaughter",
					"Safari Mesh",
					"Boreal Forest",
					"Ultraviolet",
					"Urban Masked",
					"Scorched",
					"Rust Coat",
					"Tiger Tooth",
					"Damascus Steel",
					"Damascus Steel",
					"Marble Fade",
					"Rust Coat",
					"Doppler Ruby",
					"Doppler Sapphire",
					"Doppler Blackpearl",
					"Doppler Phase 1",
					"Doppler Phase 2",
					"Doppler Phase 3",
					"Doppler Phase 4",
					"Gamma Doppler Phase 1",
					"Gamma Doppler Phase 2",
					"Gamma Doppler Phase 3",
					"Gamma Doppler Phase 4",
					"Gamma Doppler Emerald",
					"Lore",
					"Black Laminate",
					"Autotronic",
					"Freehand" };
				//SKIN STUFF END//

				std::vector<std::string> pistol = { "disabled", "dealge | revolver", "elites", "p250" };
				std::vector<std::string> snipers = { "disabled", "scar20 | g3sg1", "ssg08", "awp" };
				std::vector<std::string> armor = { "disabled", "kevlar", "Helmet & kevlar" };

				if (SETTINGS::settings.buybot_enabled) {
					WindowBegin("buybot", Vector2D(200, 200), Vector2D(450, 250));

					Combobox("pistols", pistol, SETTINGS::settings.buybot_pistol);
					Combobox("snipers", snipers, SETTINGS::settings.buybot_rifle);
					Combobox("armor", armor, SETTINGS::settings.buybot_armor);
					Checkbox("zeus", SETTINGS::settings.buybot_zeus);
					Checkbox("nades", SETTINGS::settings.buybot_grenade);

					WindowEnd();
				}

				if (SETTINGS::settings.skinchanger_enable) {
					WindowBegin("knifechanger", Vector2D(15, 15), Vector2D(400, 200));

					Combobox("knife model", KnifeModel, SETTINGS::settings.skinchanger_knifemodel);
					Combobox("knife skin", KnifeSkins, SETTINGS::settings.skinchanger_knifeskin);

					auto FullUpdate = []() -> void
					{
						//this is where the skinchanger problem is probably, i'm too lazy to fix before release so fuck you.
						static auto CL_FullUpdate = reinterpret_cast<CL_FullUpdate_t>(UTILS::FindPattern("engine.dll", reinterpret_cast<PBYTE>("\xA1\x00\x00\x00\x00\xB9\x00\x00\x00\x00\x56\xFF\x50\x14\x8B\x34\x85"), "x????x????xxxxxxx"));
						CL_FullUpdate();
					};

					if (Button("apply")) {
						FullUpdate();
					}

					WindowEnd();
				}

				WindowBegin("OOF", Vector2D(200, 200), Vector2D(600, 400));

				std::vector<std::string> tabs = { "aimbot", "visuals", "misc", "antiaim", "config","colors" };
				std::vector<std::string> aim_mode = { "rage", "legit" };
				std::vector<std::string> acc_mode = { "head", "body aim", "hitscan" };
				std::vector<std::string> chams_mode = { "none", "visible", "invisible" };
				std::vector<std::string> aa_pitch = { "none", "emotion", "fake down", "fake up", "fake zero" };
				std::vector<std::string> aa_mode = { "none", "backwards", "sideways", "backjitter", "lowerbody", "legit troll", "rotational", "autodirectional" };
				std::vector<std::string> aa_fake = { "none", "backjitter", "random", "local view", "opposite", "rotational" };
				std::vector<std::string> configs = { "default", "legit", "autos", "scouts", "pistols", "awps", "nospread" };
				std::vector<std::string> box_style = { "none", "full", "debug" };
				std::vector<std::string> media_style = { "perfect", "random" };
				std::vector<std::string> delay_shot = { "off", "lag compensation" };
				std::vector<std::string> fakelag_mode = { "factor", "adaptive" };
				std::vector<std::string> key_binds = { "none", "mouse1", "mouse2", "mouse3", "mouse4", "mouse5", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12" };
				std::vector<std::string> hitmarker = { "none", "gamesense", "custom" };
				std::vector<std::string> antiaimmode = { "standing", "moving", "jumping" };
				std::vector<std::string> glow_styles = { "regular", "pulsing", "outline" };
				std::vector<std::string> local_chams = { "none","sim fakelag: normal", "non-sim fakelag", "sim fakelag: color" };
				std::vector<std::string> chams_type = { "metallic", "texture" };
				std::vector<std::string> team_select = { "enemy", "team" };
				std::vector<std::string> crosshair_select = { "none", "static", "recoil" };
				std::vector<std::string> autostop_method = { "head", "hitscan" };
				std::vector<std::string> override_method = { "set", "key-press" };
				std::vector<std::string> resolver_method = { "Stackhack", "Bameware","Custom" };
				std::string config;


				SplitWindow(5);
				SetLine(1);
				GroupboxBegin2("", 100, 350, 0);

				switch (VerticalTab("OOF", tabs, OBJECT_FLAGS::FLAG_NONE))
				{
				case 0:
				{
					menutab = 0;
					break;
				}
				case 1:
				{
					menutab = 1;
					break;
				}
				case 2:
				{
					menutab = 2;
					break;
				}
				case 3:
				{
					menutab = 3;
					break;
				}
				case 4:
				{
					menutab = 4;
					break;
				}
				case 5:
				{
					menutab = 5;
					break;
				}
				}
				GroupboxEnd();

				SetLine(2);

				GroupboxBegin2("", 450, 350, 0);
				if (menutab == 0)// RAGE
				{
				     	Checkbox("enable aimbot", SETTINGS::settings.aim_bool);
						Separator();
						Combobox("aimbot type", aim_mode, SETTINGS::settings.aim_type);
						Combobox("aimbot mode", acc_mode, SETTINGS::settings.acc_type);
						Separator();
						Checkbox("resolver enable", SETTINGS::settings.resolve_bool);
						Combobox("resolver type", resolver_method, SETTINGS::settings.resolver_type);
						Separator();
						Checkbox("override enable", SETTINGS::settings.overrideenable);
						Combobox("override key", key_binds, SETTINGS::settings.overridekey);
						Combobox("override method", override_method, SETTINGS::settings.overridemethod);
						Separator();
						Slider("minimum hit-chance", 0, 100, SETTINGS::settings.chance_val);
						Slider("minimum damage", 1, 100, SETTINGS::settings.damage_val);
						Separator();
						Checkbox("more aimpoints", SETTINGS::settings.multi_bool);
						Slider("head scale", 0, 1, SETTINGS::settings.point_val);
						Slider("body scale", 0, 1, SETTINGS::settings.body_val);
						Separator();
						Checkbox("auto revolver", SETTINGS::settings.auto_revolver_enabled);
						Checkbox("velocity-prediction", SETTINGS::settings.vecvelocityprediction);
						Checkbox("fakelag-fix", SETTINGS::settings.fakefix_bool);
						Checkbox("prediciton", SETTINGS::settings.prediction);
						Checkbox("hs when in air", SETTINGS::settings.hsinair);
						Combobox("delay shot", delay_shot, SETTINGS::settings.delay_shot);
				}

				else if (menutab == 1)//VISUALS
				{
									
					Checkbox("enable visuals", SETTINGS::settings.esp_bool);
						Checkbox("draw enemy box", SETTINGS::settings.box_bool);
						Checkbox("draw enemy name", SETTINGS::settings.name_bool);
						Checkbox("draw enemy weapon", SETTINGS::settings.weap_bool);
						Checkbox("draw enemy flags", SETTINGS::settings.info_bool);
						Checkbox("draw enemy health", SETTINGS::settings.health_bool);
						Checkbox("draw enemy ammo", SETTINGS::settings.ammo_bool); 
						Checkbox("draw enemy fov arrows", SETTINGS::settings.fov_bool);
						Separator();
						Checkbox("remove smoke", SETTINGS::settings.smoke_bool);
						Checkbox("remove scope", SETTINGS::settings.scope_bool);
						Checkbox("remove zoom", SETTINGS::settings.removescoping);
						Separator();
						Checkbox("render spread", SETTINGS::settings.spread_bool);
						Checkbox("remove visual recoil", SETTINGS::settings.norecoil);
						Checkbox("lby indicator", SETTINGS::settings.lbyenable);
						Checkbox("fix zoom sensitivity", SETTINGS::settings.fixscopesens);
						Checkbox("bullet tracers", SETTINGS::settings.beam_bool);
						Checkbox("bullet impacts", SETTINGS::settings.impacts);
						Separator();
						Combobox("glow team selection", team_select, SETTINGS::settings.glowteamselection);
						if (SETTINGS::settings.glowteamselection == 0)
							Checkbox("enemy glow enable", SETTINGS::settings.glowenable);
						else if (SETTINGS::settings.glowteamselection == 1)
							Checkbox("team glow enable", SETTINGS::settings.glowteam);
						Combobox("glow style", glow_styles, SETTINGS::settings.glowstyle);
						Checkbox("local glow", SETTINGS::settings.glowlocal);
						Combobox("local glow style", glow_styles, SETTINGS::settings.glowstylelocal);
						Separator();
						Checkbox("thirdperson", SETTINGS::settings.tp_bool);
						Combobox("thirdperson key", key_binds, SETTINGS::settings.thirdperson_int);
						Combobox("model team selection", team_select, SETTINGS::settings.chamsteamselection);
						Combobox("model type", chams_type, SETTINGS::settings.chamstype);
						Combobox("enemy coloured models", chams_mode, SETTINGS::settings.chams_type);
						Checkbox("force crosshair", SETTINGS::settings.forcehair);
						Combobox("crosshair", crosshair_select, SETTINGS::settings.xhair_type);
						Separator();
						Checkbox("local chams", SETTINGS::settings.localchams);
						Separator();
						if (!SETTINGS::settings.matpostprocessenable)
							Checkbox("enable post", SETTINGS::settings.matpostprocessenable);
						else
							Checkbox("disable post", SETTINGS::settings.matpostprocessenable);
						Combobox("hitmarker sound", hitmarker, SETTINGS::settings.hitmarker_val);		
						Checkbox("player hitmarkers", SETTINGS::settings.hitmarker);
						Checkbox("screen hitmarkers", SETTINGS::settings.hitmarker_screen);
						Slider("viewmodel fov", 0, 179, SETTINGS::settings.viewfov_val, 68);
						Slider("render fov", 0, 179, SETTINGS::settings.fov_val, 90);
						Checkbox("sky color changer", SETTINGS::settings.sky_enabled);
						Checkbox("world color changer", SETTINGS::settings.wolrd_enabled);
						Checkbox("night mode", SETTINGS::settings.night_bool);
						Slider("night value", 0, 100, SETTINGS::settings.daytimevalue, 50);
					
				}

				else if (menutab == 2)//MISC
				{

					Checkbox("enable misc", SETTINGS::settings.misc_bool);
						Checkbox("auto bunnyhop", SETTINGS::settings.bhop_bool);
						Checkbox("auto strafer", SETTINGS::settings.strafe_bool);
						Checkbox("meme walk", SETTINGS::settings.astro);
						Checkbox("fakewalk", SETTINGS::settings.fakewalk); 
						Checkbox("knife changer", SETTINGS::settings.skinchanger_enable);
						Checkbox("logs", SETTINGS::settings.logs);
						Checkbox("clan tag", SETTINGS::settings.Clantag);
						Checkbox("buybot", SETTINGS::settings.buybot_enabled);
						Checkbox("say 1 if dmg > 100", SETTINGS::settings.trashtalk);
						Checkbox("trashtalk if hit head", SETTINGS::settings.trashtalk2);
						Checkbox("enable fakelag", SETTINGS::settings.lag_bool);
						Combobox("fakelag type", fakelag_mode, SETTINGS::settings.lag_type);
						Slider("moving lag", 1, 14, SETTINGS::settings.move_lag);
						Slider("jumping lag", 1, 14, SETTINGS::settings.jump_lag);

				}

				else if (menutab == 3)//ANTIAIM
				{
					Checkbox("enable anti aim", SETTINGS::settings.aa_bool);
						Combobox("antiaim mode", antiaimmode, SETTINGS::settings.aa_mode);
						switch (SETTINGS::settings.aa_mode)
						{
						case 0:
							Combobox("antiaim pitch - standing", aa_pitch, SETTINGS::settings.aa_pitch_type);
							Combobox("antiaim real - standing", aa_mode, SETTINGS::settings.aa_real_type);
							Combobox("antiaim fake - standing", aa_fake, SETTINGS::settings.aa_fake_type);
							break;
						case 1:
							Combobox("antiaim pitch - moving", aa_pitch, SETTINGS::settings.aa_pitch1_type);
							Combobox("antiaim real - moving", aa_mode, SETTINGS::settings.aa_real1_type);
							Combobox("antiaim fake - moving", aa_fake, SETTINGS::settings.aa_fake1_type);
							break;
						case 2:
							Combobox("antiaim pitch - jumping", aa_pitch, SETTINGS::settings.aa_pitch2_type);
							Combobox("antiaim real - jumping", aa_mode, SETTINGS::settings.aa_real2_type);
							Combobox("antiaim fake - jumping", aa_fake, SETTINGS::settings.aa_fake2_type);
							break;
						}



						Combobox("flip key", key_binds, SETTINGS::settings.flip_int);
						//Checkbox("fake angle chams", SETTINGS::settings.aa_fakeangchams_bool);
						Checkbox("anti-aim arrows", SETTINGS::settings.rifk_arrow);
						switch (SETTINGS::settings.aa_mode)
						{
						case 0:
							Slider("real additive", -180, 180, SETTINGS::settings.aa_realadditive_val);
							Slider("fake additive", -180, 180, SETTINGS::settings.aa_fakeadditive_val);
							Slider("lowerbodyyaw delta", -180, 180, SETTINGS::settings.delta_val);
							break;
						case 1:
							Slider("real additive ", -180, 180, SETTINGS::settings.aa_realadditive1_val);
							Slider("fake additive", -180, 180, SETTINGS::settings.aa_fakeadditive1_val);
							Slider("lowerbodyyaw delta", -180, 180, SETTINGS::settings.delta1_val);
							break;
						case 2:
							Slider("real additive", -180, 180, SETTINGS::settings.aa_realadditive2_val);
							Slider("fake additive", -180, 180, SETTINGS::settings.aa_fakeadditive2_val);
							Slider("lowerbodyyaw delta", -180, 180, SETTINGS::settings.delta2_val);
							break;
						}



						Slider("rotate fake °", 0, 180, SETTINGS::settings.spinanglefake);
						Slider("rotate fake %", 0, 100, SETTINGS::settings.spinspeedfake);

						switch (SETTINGS::settings.aa_mode)
						{
						case 0:
							Slider("rotate standing °", 0, 180, SETTINGS::settings.spinangle);
							Slider("rotate standing %", 0, 100, SETTINGS::settings.spinspeed);
							break;
						case 1:
							Slider("rotate moving °", 0, 180, SETTINGS::settings.spinangle1);
							Slider("rotate moving %", 0, 100, SETTINGS::settings.spinspeed1);
							break;
						case 2:
							Slider("rotate jumping °", 0, 180, SETTINGS::settings.spinangle2);
							Slider("rotate jumping %", 0, 100, SETTINGS::settings.spinspeed2);
							break;
						}

				}
				else if (menutab == 4)//CONFIG
				{
					switch (Combobox("config", configs, SETTINGS::settings.config_sel))
					{
					case 0: config = "default"; break;
					case 1: config = "legit"; break;
					case 2: config = "auto_hvh"; break;
					case 3: config = "scout_hvh"; break;
					case 4: config = "pistol_hvh"; break;
					case 5: config = "awp_hvh"; break;
					case 6: config = "nospread_hvh"; break;
					}

					if (Button("Load Config"))
					{
						SETTINGS::settings.Load(config);

						INTERFACES::cvar->ConsoleColorPrintf(CColor(200, 255, 0, 255), "[DogWare] ");
						GLOBAL::Msg("Loaded CFG.    \n");
					}

					if (Button("Save Config"))
					{
						SETTINGS::settings.Save(config);

						INTERFACES::cvar->ConsoleColorPrintf(CColor(200, 255, 0, 255), "[DogWare] ");
						GLOBAL::Msg("Saved CFG.    \n");
					}

				}
				else if (menutab == 5)// Colors
				{
					Combobox("ESP Colour Selection", team_select, SETTINGS::settings.espteamcolourselection);
					if (SETTINGS::settings.espteamcolourselection == 0)
					{
						ColorPicker("Enemy Box", SETTINGS::settings.box_col);
						ColorPicker("Enemy Name", SETTINGS::settings.name_col);
						ColorPicker("Enemy Weapon", SETTINGS::settings.weapon_col);
						ColorPicker("Enemy Fov Arrows", SETTINGS::settings.fov_col);
						ColorPicker("Enemy Visible", SETTINGS::settings.vmodel_col);
						ColorPicker("Enemy Invisible", SETTINGS::settings.imodel_col);
						ColorPicker("Glow", SETTINGS::settings.glow_col);
						ColorPicker("Bullet Tracer", SETTINGS::settings.bulletenemy_col);
					}
					else if (SETTINGS::settings.espteamcolourselection == 1)
					{
						ColorPicker("Team Box", SETTINGS::settings.boxteam_col);
						ColorPicker("Team Name", SETTINGS::settings.nameteam_col);
						ColorPicker("Team Weapon", SETTINGS::settings.weaponteam_col);
						ColorPicker("Team Fov Arrows", SETTINGS::settings.arrowteam_col);
						ColorPicker("Team Visible", SETTINGS::settings.teamvis_color);
						ColorPicker("Team Invisible", SETTINGS::settings.teaminvis_color);
						ColorPicker("Glow", SETTINGS::settings.teamglow_color);
						ColorPicker("Bullet Tracer", SETTINGS::settings.bulletteam_col);
					}
					ColorPicker("chams - local", SETTINGS::settings.localchams_col);
					ColorPicker("glow - local", SETTINGS::settings.glowlocal_col);
					ColorPicker("bullet tracer - local", SETTINGS::settings.bulletlocal_col);
					ColorPicker("nade pred", SETTINGS::settings.grenadepredline_col);
					ColorPicker("spread crosshair", SETTINGS::settings.spread_Col);
					ColorPicker("sky color", SETTINGS::settings.skycolor);
					ColorPicker("world color", SETTINGS::settings.night_col);
					ColorPicker("hitmarkers", SETTINGS::settings.awcolor);
				}
				GroupboxEnd();

				WindowEnd();

				int w, h;
				static int x, y;

				INTERFACES::Engine->GetScreenSize(w, h);
				static bool init = false;
				if (init == false) {
					x = w / 2 - (400 / 2);
					y = h / 2 - (200 / 2);
					init = true;
				}
			}
		}
	}
}