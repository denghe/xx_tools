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

		bool HasPreviewData() const;
		bool HasAngleData() const;
		bool HasPivotData() const;
		bool HasCirclesData() const;
		bool HasHooksData() const;
		bool HasNailsData() const;
	};

	std::filesystem::path rootDir;

	xx::SQLite::Connection conn;
	operator bool() const;

	void NewOrOpen(std::string path);	// conn == true: success
	void Close();
	void CreateTables();
	void InitQueries();

	xx::SQLite::Query qSelectFile{ conn };
	std::optional<File> SelectOrInsertFile(std::string_view const& file_name);

	xx::SQLite::Query qSelectFileCircles{ conn };
	std::vector<FileCircle> SelectFileCircles(int32_t file_id);

	xx::SQLite::Query qSelectFileHooks{ conn };
	std::vector<FileHook> SelectFileHooks(int32_t file_id);

	xx::SQLite::Query qSelectFileNails{ conn };
	std::vector<FileNail> SelectFileNails(int32_t file_id);

	xx::SQLite::Query qInsertFile{ conn };
	void InsertFile(File& file);	// insert & fill auto increase id

	// todo: update desc ?

	xx::SQLite::Query qUpdateFilePreview{ conn };
	bool UpdateFilePreview(File const& file);

	xx::SQLite::Query qUpdateFileAngle{ conn };
	bool UpdateFileAngle(File const& file);

	xx::SQLite::Query qUpdateFilePivot{ conn };
	bool UpdateFilePivot(File const& file);

	// update circles = delete + inserts
	xx::SQLite::Query qDeleteFileCircles{ conn };
	xx::SQLite::Query qInsertFileCircles{ conn };
	void UpdateFileCircles(File const& file);

	// todo: more updates Hooks Nails need check foreign key depends + disable check + delete + batch insert + enable check
};
