#include "pch.h"
#include "db.h"

namespace FS = std::filesystem;

DB::operator bool() const {
	return conn;
}

void DB::NewOrOpen(std::string path) {
	assert(!conn);
	std::string_view sv(path);
	if (sv.size() > 1 && sv.starts_with('"') && sv.ends_with('"')) {
		sv = sv.substr(1, sv.size() - 2);
	}
	conn.Open(sv);
	if (conn) {
		FS::path p((std::u8string_view&)sv);
		rootDir = p.remove_filename();
		CreateTables();
		InitQueries();
	}
}

void DB::Close() {
	assert(conn);
	conn.Close();

	qSelectFile.Clear();
	qSelectFileCircles.Clear();
	qSelectFileHooks.Clear();
	qSelectFileNails.Clear();

	qInsertFile.Clear();

	qUpdateFilePreview.Clear();
	qUpdateFileAngle.Clear();
	qUpdateFilePivot.Clear();

	qDeleteFileCircles.Clear();
	qInsertFileCircles.Clear();
	// todo: more clear
}

void DB::CreateTables() {

	conn.Execute(R"#(
CREATE TABLE IF NOT EXISTS file (
    id               INTEGER PRIMARY KEY AUTOINCREMENT,
    name             TEXT    NOT NULL,
    desc             TEXT,
    preview_offset_x REAL,
    preview_offset_y REAL,
    preview_scale    REAL,
    angle            REAL,
    pivot_x          REAL,
    pivot_y          REAL,
    UNIQUE (
        name COLLATE NOCASE ASC
    )
);
)#");

	conn.Execute(R"#(
CREATE TABLE IF NOT EXISTS file_circle (
    file_id INTEGER REFERENCES file (id),
    idx INTEGER,
    x       REAL    NOT NULL,
    y       REAL    NOT NULL,
    r       REAL    NOT NULL,
    PRIMARY KEY (
        file_id,
        idx
    )
);
)#");
	conn.Execute(R"#(
CREATE TABLE IF NOT EXISTS file_hook (
    file_id INTEGER REFERENCES file (id),
    idx INTEGER,
    x       REAL    NOT NULL,
    y       REAL    NOT NULL,
    PRIMARY KEY (
        file_id,
        idx
    )
);
)#");
	conn.Execute(R"#(
CREATE TABLE IF NOT EXISTS file_nail (
    file_id INTEGER REFERENCES file (id),
    idx INTEGER,
    x       REAL    NOT NULL,
    y       REAL    NOT NULL,
    a_from  REAL,
    a_to    REAL,
    cfg     TEXT,
    PRIMARY KEY (
        file_id,
        idx
    )
);
)#");

}

void DB::InitQueries() {

	qSelectFile.SetQuery(R"(
SELECT id,
       name,
       desc,
       preview_offset_x,
       preview_offset_y,
       preview_scale,
       angle,
       pivot_x,
       pivot_y
  FROM file
 WHERE name = ?
)");

	qSelectFileCircles.SetQuery(R"(
SELECT file_id,
       idx,
       x,
       y,
       r
  FROM file_circle
 WHERE file_id = ?
)");

	qSelectFileHooks.SetQuery(R"(
SELECT file_id,
       idx,
       x,
       y
  FROM file_hook
 WHERE file_id = ?
)");

	qSelectFileNails.SetQuery(R"(
SELECT file_id,
       idx,
       x,
       y,
       a_from,
       a_to
  FROM file_nail
 WHERE file_id = ?
)");

	qInsertFile.SetQuery(R"(
INSERT INTO file (
    name,
    desc,
    preview_offset_x,
    preview_offset_y,
    preview_scale,
    angle,
    pivot_x,
    pivot_y
)
VALUES (
    ?,
    ?,
    ?,
    ?,
    ?,
    ?,
    ?,
    ?
);
)");

	qUpdateFilePreview.SetQuery(R"(
UPDATE file
   SET preview_offset_x = ?,
       preview_offset_y = ?,
       preview_scale = ?
 WHERE id = ?
)");

	qUpdateFileAngle.SetQuery(R"(
UPDATE file
   SET angle = ?
 WHERE id = ?
)");

	qUpdateFilePivot.SetQuery(R"(
UPDATE file
   SET pivot_x = ?,
       pivot_y = ?
 WHERE id = ?
)");

	qDeleteFileCircles.SetQuery(R"(
DELETE FROM file_circle
      WHERE file_id = ?
)");

	qInsertFileCircles.SetQuery(R"(
INSERT INTO file_circle (
    file_id,
    idx,
    x,
    y,
    r
)
VALUES (
    ?,
    ?,
    ?,
    ?,
    ?
)
)");

	// todo: more set query
}

std::optional<DB::File> DB::SelectOrInsertFile(std::string_view const& file_name) {
	assert(conn);
	std::optional<DB::File> rtv;
	rtv.emplace();
	qSelectFile.SetParameters(file_name);
	// try ?
	bool found = qSelectFile.ExecuteTo(
		rtv->id
		, rtv->name
		, rtv->desc
		, rtv->preview_offset_x
		, rtv->preview_offset_y
		, rtv->preview_scale
		, rtv->angle
		, rtv->pivot_x
		, rtv->pivot_y
	);
	if (found) {
		rtv->circles = SelectFileCircles(rtv->id);
		rtv->hooks = SelectFileHooks(rtv->id);
		rtv->nails = SelectFileNails(rtv->id);
	} else {
		rtv->name = file_name;
		InsertFile(*rtv);
	}

	return rtv;
}

std::vector<DB::FileCircle> DB::SelectFileCircles(int32_t file_id) {
	std::vector<DB::FileCircle> rtv;
	qSelectFileCircles.SetParameters(file_id);
	qSelectFileCircles.Execute([&](xx::SQLite::Reader& r)->int {
		auto& d = rtv.emplace_back();
		r.Reads(
			d.file_id
			, d.idx
			, d.x
			, d.y
			, d.r
		);
		return 0;
		});
	return rtv;
}

std::vector<DB::FileHook> DB::SelectFileHooks(int32_t file_id) {
	std::vector<DB::FileHook> rtv;
	qSelectFileHooks.SetParameters(file_id);
	qSelectFileHooks.Execute([&](xx::SQLite::Reader& r)->int {
		auto& d = rtv.emplace_back();
		r.Reads(
			d.file_id
			, d.idx
			, d.x
			, d.y
		);
		return 0;
		});
	return rtv;
}

std::vector<DB::FileNail> DB::SelectFileNails(int32_t file_id) {
	std::vector<DB::FileNail> rtv;
	qSelectFileNails.SetParameters(file_id);
	qSelectFileNails.Execute([&](xx::SQLite::Reader& r)->int {
		auto& d = rtv.emplace_back();
		r.Reads(
			d.file_id
			, d.idx
			, d.x
			, d.y
			, d.a_from
			, d.a_to
			, d.cfg
		);
		return 0;
		});
	return rtv;
}

void DB::InsertFile(File& file) {
	qInsertFile.SetParameters(
		file.name
		, file.desc
		, file.preview_offset_x
		, file.preview_offset_y
		, file.preview_scale
		, file.angle
		, file.pivot_x
		, file.pivot_y
	);
	qInsertFile.Execute();
	file.id = (int32_t)conn.GetLastInsertRowId();
}

bool DB::UpdateFilePreview(File const& file) {
	qUpdateFilePreview.SetParameters(
		file.preview_offset_x
		, file.preview_offset_y
		, file.preview_scale
		, file.id
	);
	qUpdateFilePreview.Execute();
	return conn.GetAffectedRows() == 1;
}

bool DB::UpdateFileAngle(File const& file) {
	qUpdateFileAngle.SetParameters(
		file.angle
		, file.id
	);
	qUpdateFileAngle.Execute();
	return conn.GetAffectedRows() == 1;
}

bool DB::UpdateFilePivot(File const& file) {
	qUpdateFilePivot.SetParameters(
		file.pivot_x
		, file.pivot_y
		, file.id
	);
	qUpdateFilePivot.Execute();
	return conn.GetAffectedRows() == 1;
}

void DB::UpdateFileCircles(File const& file) {
	conn.BeginTransaction();
	qDeleteFileCircles.SetParameters(
		file.id
	);
	qDeleteFileCircles.Execute();
	for (auto& c : file.circles) {
		qInsertFileCircles.SetParameters(
			c.file_id,
			c.idx,
			c.x,
			c.y,
			c.r
		);
		qInsertFileCircles.Execute();
	}
	conn.Commit();
}

bool DB::File::HasPreviewData() const {
	return preview_offset_x.has_value()
		&& preview_offset_y.has_value()
		&& preview_scale.has_value();
}

bool DB::File::HasAngleData() const {
	return angle.has_value();
}

bool DB::File::HasPivotData() const {
	return pivot_x.has_value()
		&& pivot_y.has_value();
}

bool DB::File::HasCirclesData() const {
	return !circles.empty();
}

bool DB::File::HasHooksData() const {
	return !hooks.empty();
}

bool DB::File::HasNailsData() const {
	return !nails.empty();
}
