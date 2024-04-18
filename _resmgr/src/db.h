#pragma once
#include "pch.h"


struct DB {
	struct FileCircle {
		int32_t file_id, idx;
		float x, y, r;
	};
	struct FileHook {
		int32_t file_id, idx;
		float x, y;
	};
	struct FileNail {
		int32_t file_id, idx;
		float x, y, a_from, a_to;
		std::optional<std::string> cfg;
	};
	struct File {
		int32_t id;
		std::string name;
		std::optional<std::string> desc;
		std::optional<float> preview_offset_x;
		std::optional<float> preview_offset_y;
		std::optional<float> preview_scale;
		std::optional<float> angle;
		std::optional<float> pivot_x;
		std::optional<float> pivot_y;

		std::vector<FileCircle> circles;
		std::vector<FileHook> hooks;
		std::vector<FileNail> nails;

		bool HasData() const;
	};

	std::filesystem::path rootDir;
	xx::SQLite::Connection conn;
	operator bool() const;
	void NewOrOpen(std::string path);	// conn == true: success
	void Close();
	void CreateTables();

	// todo: Load Save File xxxxx
};
