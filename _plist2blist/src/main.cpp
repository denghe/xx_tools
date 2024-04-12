#include <pch.h>

int main() {
	SetConsoleOutputCP(CP_UTF8);
	auto&& cp = std::filesystem::current_path();
	std::cout << "tool: *.plist -> *.blist ( key can't contains space or dot ) + res_frames.h & cpp\nworking dir: " << cp.string() << "\npress any key continue...\n";
	std::cin.get();

	std::unordered_map<std::string, std::string> keys;	// cross all plist keys

	int n = 0;
	for (auto&& entry : std::filesystem::/*recursive_*/directory_iterator(cp)) {
		if (!entry.is_regular_file()) continue;
		auto&& p = entry.path();
		if (p.extension() != u8".plist") continue;

		auto plistName = xx::U8AsString(p.filename().u8string());
		auto fullpath = xx::U8AsString(std::filesystem::absolute(p).u8string());
		auto blistPath = fullpath.substr(0, fullpath.size() - 6) + ".blist";

		xx::Data fd;
		if (int r = xx::ReadAllBytes(p, fd)) {
			std::cerr << "ReadAllBytes failed. r = " << r << " fn = " << p << std::endl;
			return -__LINE__;
		}

		xx::TexturePackerReader::Plist tp;
		if (int r = tp.Load(fd)) {
			std::cerr << "tp.Load failed. r = " << r << " fn = " << p << std::endl;
			return -__LINE__;
		}

		std::cout << "handle file: " << p << std::endl;
		xx::Data d;
		d.WriteBuf("blist_1 ");	// custom file head 8 bytes
		d.Write(tp.metadata.realTextureFileName);
		d.Write(tp.metadata.premultiplyAlpha);
		d.Write(tp.frames.size());
		for (auto& f : tp.frames) {
			std::cout << "handle frame: " << f.name << std::endl;

			for (auto const& c : f.name) {
				if (c == '.' || c == ' ') {
					std::cerr << "bad key name( contain space or dot ): " << f.name << std::endl;
					return -__LINE__;
				}
			}

			if (auto [iter, success] = keys.emplace(f.name, plistName); !success) {
				std::cerr << "duplicate res name: " << f.name << " in plist " << iter->second
					<< " and plist " << plistName
					<< std::endl;
				return -__LINE__;
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
		xx::WriteAllBytes((std::u8string&)blistPath, d);
		++n;
	}

	// todo: gen code

	// todo: remove key suffix  .png .jpg ?

	// group by prefix...._number
	//std::map<std::string_view, std::vector<std::string_view>> kvs;
	//for (auto& fn : fns) {
	//	std::string_view sv(fn);
	//	if (auto idx = sv.find_last_of('_'); idx != sv.npos) {
	//		auto k = sv.substr(0, idx);
	//		auto v = sv.substr(idx + 1);
	//		if (v.find_first_not_of("0123456789"sv) != v.npos) continue;
	//		kvs[k].push_back(v);
	//	}
	//}



	std::cout << "finished. handled " << n << " files! \npress any key continue...\n";
	std::cin.get();

	return 0;
}
