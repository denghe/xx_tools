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
	}
}

void DB::Close() {
	conn.Close();
}

void DB::CreateTables() {
	try {
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
	} catch (std::exception const& ex) {
		xx::CoutN("throw exception at CreateTables. ex = ", ex.what());
	}
}

bool DB::File::HasData() const {
    return false;
}
