#include "pch.h"
#include "looper.h"
#include "scene_main_menu.h"

Looper gLooper;

int32_t main() {
	SetConsoleOutputCP(CP_UTF8);
	xx::SQLite::Init();

	auto&& cp = std::filesystem::current_path();
	std::cout << "tool: resource manager ( preview, config: angle pivot circles aabb hook nail )\n"
		<< "current path :" << cp << std::endl;

	gLooper.showFps = false;
	gLooper.title = "resource manager";
	gLooper.Init();
	gLooper.Run<true>();
}

xx::Task<> Looper::DrawTask() {

	// load first scene
	co_await AsyncSwitchTo<Scene_MainMenu::Scene>();
}
