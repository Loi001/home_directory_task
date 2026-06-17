#include <iostream>
#include <string>
#include <unordered_set>
#include <filesystem>
#include <algorithm>
#include <thread>
#include <fstream>
#include <limits>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::unordered_set<std::string> audio_extensions = {
	".mp3", ".wav", ".flac", ".aac", ".ogg"
};

std::unordered_set<std::string> video_extensions = {
	".mp4", ".avi", ".webm", ".mkv", ".mov", ".flv", ".wmv", ".m4v", ".3gp", ".mpeg", ".mpg"
};

std::unordered_set<std::string> image_extensions = {
	".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp", ".tiff", ".tif", ".svg", ".heic"
};

bool is_valid_directory_path(const std::string& path)
{
	try {
		if (std::filesystem::create_directory(path)) {
			std::filesystem::remove(path);
			return true;
		}
	}
	catch (...) {
		return false;
	}
	return false;
}

bool check_directory(std::filesystem::path dir_path, std::filesystem::path output_path) {
	try {
		json media_files;
		media_files["audio_files"] = json::array();
		media_files["video_files"] = json::array();
		media_files["image_files"] = json::array();

		std::filesystem::path media_dir = output_path / "media_files";
		std::filesystem::create_directories(media_dir);

		for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
			if (!entry.is_regular_file()) continue;
			std::string extension = entry.path().extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return std::tolower(c); });

			if (audio_extensions.count(extension)) {
				media_files["audio_files"].push_back(entry.path().filename().string());
			}
			else if (video_extensions.count(extension)) {
				media_files["video_files"].push_back(entry.path().filename().string());
			}
			else if (image_extensions.count(extension)) {
				media_files["image_files"].push_back(entry.path().filename().string());
			}
		}

		std::filesystem::path json_path = media_dir / "media_list.json";
		std::ofstream output_file(json_path);
		if (output_file.is_open()) {
			output_file << media_files.dump(4);
			output_file.close();
		}
		else {
			std::cout << "Error: open json file" << std::endl;
			return false;
		}
	}
	catch (std::filesystem::filesystem_error& e) {
		std::cout << "Error: " << e.what() << std::endl;
		return false;
	}
	return true;
}

int main() {
	std::string home_directory, output_directory;
	int delay = 0;

	std::cout << "Enter home directory: ";
	std::cin >> home_directory;
	if (!std::filesystem::is_directory(home_directory)) {
		std::cout << "Error: directory does not exist or it's not a directory\n";
		return 0;
	}

	std::cout << "Enter output directory: ";
	std::cin >> output_directory;
	if (!is_valid_directory_path(output_directory)) {
		std::cout << "Error: invalid directory name\n";
		return 0;
	}

	std::cout << "Enter delay(ms): ";
	while (!(std::cin >> delay)) {
		std::cin.clear();
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		std::cout << "Error: invalid input type\n";
	}
	if (delay <= 0) {
		std::cout << "Error: invalid delay value\n";
		return 0;
	}

	std::filesystem::path home(home_directory);
	std::filesystem::path out(output_directory);

	while (true)
	{
		if (!check_directory(home, out)) { return 0; }
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));
	}

	return 0;
}