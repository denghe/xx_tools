﻿#include "pch.h"
#include "looper.h"
#include "scene_main_menu.h"
#include "scene_pivot_editor.h"

/********************************************************************************************************/
/********************************************************************************************************/

namespace Scene_MainMenu {
	void Scene::ImGuiUpdate() {
#if 1
		auto& io = ImGui::GetIO();
		auto mvp = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(mvp->WorkPos, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(mvp->WorkSize, ImGuiCond_FirstUseEver);

		ImGuiWindowFlags wf{};
		wf |= ImGuiWindowFlags_NoTitleBar;
		wf |= ImGuiWindowFlags_NoScrollbar;
		wf |= ImGuiWindowFlags_MenuBar;
		wf |= ImGuiWindowFlags_NoMove;
		wf |= ImGuiWindowFlags_NoResize;
		wf |= ImGuiWindowFlags_NoCollapse;
		wf |= ImGuiWindowFlags_NoNav;
		wf |= ImGuiWindowFlags_NoBackground;
		wf |= ImGuiWindowFlags_NoBringToFrontOnFocus;
		wf |= ImGuiWindowFlags_UnsavedDocument;
		if (ImGui::Begin("Scene_MainMenu", nullptr, wf)) {
			if (ImGui::BeginMainMenuBar()) {
				if (ImGui::BeginMenu("File")) {
					if (ImGui::MenuItem("New Or Open", "Ctrl+N", nullptr, !db)) {
						currentWindow = Windows::File_NewOrOpen;
					}
					if (ImGui::MenuItem("Close", nullptr, nullptr, db && currentWindow == Windows::None)) {
						db.Close();
					}
					if (ImGui::MenuItem("Import Files", "Ctrl+I", nullptr, db && currentWindow == Windows::None)) {
						currentWindow = Windows::File_ImportFiles;
					}
					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}
		}
		ImGui::End();

		// shortcut handlers
		if (io.KeyCtrl) {
			if (!db && ImGui::IsKeyPressed(ImGuiKey_N, false)) {
				currentWindow = Windows::File_NewOrOpen;
			}
			if (!db && ImGui::IsKeyPressed(ImGuiKey_I, false)) {
				currentWindow = Windows::File_ImportFiles;
			}
		}

		Draw_File_NewOrOpen();
		Draw_File_ImportFiles();
		Draw_File_Browse();
		// ...
		Draw_Error();
		lastWindow = currentWindow;

#else
		static bool showDemo{ true};
		if (showDemo) {
			ImGui::ShowDemoWindow(&showDemo);
		}
#endif
	}

	/********************************************************************************************************/
	/********************************************************************************************************/

	void Scene::Draw_File_NewOrOpen() {
		if (currentWindow != Windows::File_NewOrOpen) return;
		auto isFirst = lastWindow != currentWindow;

		auto& io = ImGui::GetIO();
		auto mvp = ImGui::GetMainViewport();
		static constexpr float wndHeight = 170;
		static constexpr float lrMargin = 30;
		const auto tbMargin = (mvp->WorkSize.y - wndHeight) / 2;
		const auto wndWidth = mvp->WorkSize.x - lrMargin * 2;
		ImGui::SetNextWindowPos({ lrMargin, tbMargin });
		ImGui::SetNextWindowSize({ wndWidth, wndHeight });

		ImGuiWindowFlags wf{};
		wf |= ImGuiWindowFlags_NoScrollbar;
		wf |= ImGuiWindowFlags_NoMove;
		wf |= ImGuiWindowFlags_NoResize;
		wf |= ImGuiWindowFlags_NoCollapse;
		wf |= ImGuiWindowFlags_NoNav;
		wf |= ImGuiWindowFlags_UnsavedDocument;
		if (ImGui::Begin("create new or open current", nullptr, wf)) {
			ImGui::Text("please input the db file full path:");
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (isFirst) {
				ImGui::SetKeyboardFocusHere(0);
				auto dbPath = FS::current_path() / "res.sqlite3";
				if (dbFileFullPath.empty() && FS::exists(dbPath)) {
					dbFileFullPath = (std::string&&)dbPath.u8string();
				}
			}
			ImGui::InputText("##dbFileFullPath", &dbFileFullPath);
			ImGui::Dummy({ 0.0f, 5.0f });
			ImGui::Separator();
			ImGui::Dummy({ 0.0f, 5.0f });

			ImGui::Dummy({ 0.0f, 0.0f });
			ImGui::SameLine(ImGui::GetWindowWidth() - (ImGui::GetStyle().ItemSpacing.x + 120 + 10 + 120));
			if (ImGui::Button("Submit", { 120, 35 })) {
				db.NewOrOpen(dbFileFullPath);
				if (!db) {
					errorMessage = "open or create db error! code = " + std::to_string(db.conn.lastErrorCode);
				} else {
					currentWindow = Windows::File_ImportFiles;
				}
			}
			ImGui::SameLine(ImGui::GetWindowWidth() - (ImGui::GetStyle().ItemSpacing.x + 120));
			if (ImGui::Button("Cancel", { 120, 35 })) {
				currentWindow = {};
			}
		}
		ImGui::End();
	}

	/********************************************************************************************************/
	/********************************************************************************************************/

	void Scene::Draw_File_ImportFiles() {
		if (currentWindow != Windows::File_ImportFiles) return;
		auto isFirst = lastWindow != currentWindow;

		auto& io = ImGui::GetIO();
		auto mvp = ImGui::GetMainViewport();
		static constexpr float margin{ 10 };
		ImGui::SetNextWindowPos({ mvp->WorkPos.x + margin, mvp->WorkPos.y + margin });
		ImGui::SetNextWindowSize({ mvp->WorkSize.x - margin * 2, mvp->WorkSize.y - margin * 2 });

		if (isFirst) {
			ImportFilesTask = ImportFilesTask_();
			wndLog.Clear();
			pngs.clear();
		}
		ImportFilesTask();
		wndLog.Draw("import files");
	}

	xx::Task<> Scene::ImportFilesTask_() {
		assert(db);
		std::filesystem::current_path(db.rootDir);
		auto cp = std::filesystem::current_path();
		wndLog.AddLog("current res path: %s\nscaning...\n", cp.u8string().c_str());
		co_yield 0;
		for (auto&& entry : std::filesystem::recursive_directory_iterator(cp)) {
			if (!entry.is_regular_file()) continue;
			auto&& p = entry.path();
			if (p.extension() != u8".png"sv) continue;
			auto&& pn = p.filename().u8string();
			//std::string_view sv(pn.c_str(), pn.size() - 4);	// remove ".png"

			if (auto iter = pngs.find(pn); iter == pngs.end()) {
				pngs[pn] = p;
				wndLog.AddLog("%s\n", pn.c_str());
			} else {
				errorMessage = std::string("dupelicate file name: ") + (std::string&)pn 
					+ "\npath1 = " + (std::string&)iter->second.u8string()
					+ "\npath2 = " + (std::string&)p.u8string()
					+ "\n\nplease fix it and import files again !"
					;
				currentWindow = {};
				co_return;
			}
			co_yield 0;
		}
		wndLog.AddLog("done!\n");
		co_await gLooper.AsyncSleep(3);
		currentWindow = Windows::File_Browse;
	}

	/********************************************************************************************************/
	/********************************************************************************************************/

	void Scene::Draw_File_Browse() {
		if (currentWindow != Windows::File_Browse) return;
		auto isFirst = lastWindow != currentWindow;

		// todo

		auto& io = ImGui::GetIO();
		auto mvp = ImGui::GetMainViewport();
		static constexpr float width{ 450 }, margin{ 10 };
		ImGui::SetNextWindowPos({ mvp->WorkPos.x + margin, mvp->WorkPos.y + margin });
		ImGui::SetNextWindowSize({ width, mvp->WorkSize.y - margin * 2 });

		ImGuiWindowFlags wf{};
		wf |= ImGuiWindowFlags_NoMove;
		wf |= ImGuiWindowFlags_NoResize;
		wf |= ImGuiWindowFlags_NoCollapse;
		if (ImGui::Begin("file browser", nullptr, wf)) {
			for( auto& [k, v] : pngs) {
				if (ImGui::Selectable((char*)k.c_str(), selectedFileName == k)) {
					selectedFileName = k;
					selectedFile = db.SelectOrInsertFile((std::string&)k);
				}
			}
		}
		ImGui::End();

		if (!selectedFileName.empty()) {

			// todo: prepare selected item's context
			static float scale{}, angle{};	// todo

			ImGui::SetNextWindowPos({ mvp->WorkPos.x + margin + width + margin, mvp->WorkPos.y + margin });
			ImGui::SetNextWindowSize({ mvp->WorkSize.x - margin * 3 - width, 105 });

			wf = {};
			wf |= ImGuiWindowFlags_NoScrollbar;
			wf |= ImGuiWindowFlags_NoMove;
			wf |= ImGuiWindowFlags_NoResize;
			wf |= ImGuiWindowFlags_NoCollapse;
			if (ImGui::Begin("selected file details", nullptr, wf)) {
				if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None)) {
					// todo: switch by file type( more types support )

					if (ImGui::BeginTabItem("preview")) {
						ImGui::Text("please input scale: ");
						ImGui::SameLine();
						ImGui::SetNextItemWidth(100);
						ImGui::InputFloat("##scale", &scale, {}, {}, "%.3f", ImGuiInputTextFlags_CharsDecimal);
						ImGui::SameLine();
						if (ImGui::Button("Submit")) {
							// todo: change scale ( store to db )
							xx::CoutN("scale changed to ", scale);
						}
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("angle")) {
						ImGui::Text("please input angle: ");
						ImGui::SameLine();
						ImGui::SetNextItemWidth(100);
						ImGui::InputFloat("##angle", &angle, {}, {}, "%.3f", ImGuiInputTextFlags_CharsDecimal);
						ImGui::SameLine();
						if (ImGui::Button("Submit")) {
							// todo: input angle text ( store to db )
							xx::CoutN("angle changed to ", angle);
						}
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("pivot")) {
						ImGui::Text("MOUSE Left: drag  Whell: zoom  Right: set pivot");
						ImGui::SameLine();
						if (ImGui::Button("Clear")) {
							// todo
							xx::CoutN("pivot clear");
						}
						ImGui::EndTabItem();
					}

					// todo
					if (ImGui::BeginTabItem("circles")) {
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("aabb")) {
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("hooks")) {
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("nails")) {
						ImGui::EndTabItem();
					}

					ImGui::EndTabBar();
				}
			}
			ImGui::End();

			// todo: call logic by tab
		}
	}

	/********************************************************************************************************/
	/********************************************************************************************************/

	void Scene::Draw_Error() {
		if (errorMessage.empty()) return;

		auto& io = ImGui::GetIO();
		auto mvp = ImGui::GetMainViewport();
		static constexpr float wndHeight = 300;
		static constexpr float lrMargin = 100;
		const auto tbMargin = (mvp->WorkSize.y - wndHeight) / 2;
		const auto wndWidth = mvp->WorkSize.x - lrMargin * 2;
		ImGui::SetNextWindowPos({ lrMargin, tbMargin });
		ImGui::SetNextWindowSize({ wndWidth, wndHeight });

		ImGuiWindowFlags wf{};
		wf |= ImGuiWindowFlags_NoScrollbar;
		wf |= ImGuiWindowFlags_NoMove;
		wf |= ImGuiWindowFlags_NoResize;
		wf |= ImGuiWindowFlags_NoCollapse;
		wf |= ImGuiWindowFlags_NoNav;
		wf |= ImGuiWindowFlags_UnsavedDocument;
		if (ImGui::Begin("error message:", nullptr, wf)) {
			ImGui::InputTextMultiline("##errorMessage", &errorMessage, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 9), ImGuiInputTextFlags_ReadOnly);
			ImGui::Dummy({ 0.0f, 0.0f });
			ImGui::SameLine(ImGui::GetWindowWidth() - (ImGui::GetStyle().ItemSpacing.x + 90));
			if (ImGui::Button("OK", { 90, 35 })) {
				errorMessage.clear();
			}
		}
		ImGui::End();
	}
}
