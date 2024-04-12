#include <pch.h>

// replace . space to _
std::string ToFieldName(std::string_view sv) {
	std::string s(sv);
	for (auto& c : s) {
		if (c == '.' || c == ' ') c = '_';
	}
	return s;
}

int main() {
	SetConsoleOutputCP(CP_UTF8);
	auto&& cp = std::filesystem::current_path();
	std::cout << "tool: *.plist -> *.blist + res_frames.h & cpp\nworking dir: " << cp.string() << "\npress any key continue...\n";
	std::cin.get();

	std::unordered_map<std::string, std::string> keys;	// cross all plist keys

	int n = 0;
	for (auto&& entry : std::filesystem::/*recursive_*/directory_iterator(cp)) {
		if (!entry.is_regular_file()) continue;
		auto&& p = entry.path();
		if (p.extension() != u8".plist") continue;

		auto fullpath = xx::U8AsString(std::filesystem::absolute(p).u8string());
		auto newPath = fullpath.substr(0, fullpath.size() - 6) + ".blist";
		auto newPath2 = fullpath.substr(0, fullpath.size() - 6) + ".txt";

		xx::Data fd;
		if (int r = xx::ReadAllBytes(p, fd)) {
			std::cerr << "ReadAllBytes failed. r = " << r << " fn = " << p << std::endl;
			return -1;
		}

		xx::TexturePackerReader::Plist tp;
		if (int r = tp.Load(fd)) {
			std::cerr << "tp.Load failed. r = " << r << " fn = " << p << std::endl;
			return -2;
		}

		std::cout << "handle file: " << p << std::endl;
		xx::Data d;
		d.WriteBuf("blist_1 ");	// custom file head 8 bytes
		d.Write(tp.metadata.realTextureFileName);
		d.Write(tp.metadata.premultiplyAlpha);
		d.Write(tp.frames.size());
		for (auto& f : tp.frames) {
			std::cout << "handle frame: " << f.name << std::endl;

			if (auto [iter, success] = keys.emplace(f.name, fullpath); !success) {
				std::cerr << "duplicate res name: " << f.name << " in plist " << iter->second
					<< " and plist " << fullpath
					<< std::endl;
				return -2;
			}

			d.Write(f.name);
			d.Write(f.aliases);
			if (f.anchor.has_value()) {
				d.WriteFixed((uint8_t)1);
				d.Write(f.anchor->x, f.anchor->y);
			} else {
				d.WriteFixed((uint8_t)0);
			}
			d.WriteFixed(f.spriteOffset.x);
			d.WriteFixed(f.spriteOffset.y);
			d.WriteFixed(f.spriteSize.width);
			d.WriteFixed(f.spriteSize.height);
			d.WriteFixed(f.spriteSourceSize.width);
			d.WriteFixed(f.spriteSourceSize.height);
			d.WriteFixed(f.textureRect.x);
			d.WriteFixed(f.textureRect.y);
			d.WriteFixed(f.textureRect.width);
			d.WriteFixed(f.textureRect.height);
			d.WriteFixed(f.textureRotated);
		}
		xx::WriteAllBytes((std::u8string&)newPath, d);
		++n;
	}

	// todo: gen code

	std::cout << "finished. handled " << n << " files! \npress any key continue...\n";
	std::cin.get();

	return 0;
}
