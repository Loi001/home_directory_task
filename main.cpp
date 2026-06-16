#include <iostream>
#include <string>
#include <unordered_set>
#include <filesystem>
#include <algorithm>
#include <thread>
#include <fstream>

#include "single_include/nlohmann/json.hpp"

using json = nlohmann::json;

std::unordered_set<std::string> audio_extensions = {
	".mp3", ".wav", ".mp4", ".avi", ".webm"
};

std::unordered_set<std::string> video_extensions = {
	".mp4", ".avi", ".webm", ".mkv", ".mov", ".flv", ".wmv", ".m4v", ".3gp", ".mpeg", ".mpg"
};

std::unordered_set<std::string> image_extensions = {
	".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp", ".tiff", ".tif", ".svg", ".heic"
};

bool check_directiry(const std::string& dir_path, const std::string& output_path) {
	try {
		std::filesystem::path dir = dir_path;
		std::filesystem::path out = output_path;
		json media_files;
		media_files["audio files: "] = json::array();
		media_files["video files: "] = json::array();
		media_files["image files: "] = json::array();
		if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir) || !std::filesystem::exists(output_path) 
			|| !std::filesystem::is_directory(output_path)) {
			std::cout << "Error: directory does not exists!" << std::endl;
			return false;
		}
		std::filesystem::path media_dir = out / "media_files";
		if (!std::filesystem::exists(media_dir)) {
			std::filesystem::create_directories(media_dir);
		}
		for (const auto& entry : std::filesystem::directory_iterator(dir)) {
			if (!entry.is_regular_file()) continue;
			std::string extensions = entry.path().extension().string();
			std::transform(extensions.begin(), extensions.end(), extensions.begin(), [](unsigned char c) { return std::tolower(c); });
			if (audio_extensions.count(extensions)) {
				media_files["audio files: "].push_back(entry.path().filename().string());
			}
			else if (video_extensions.count(extensions)) {
				media_files["video files: "].push_back(entry.path().filename().string());
			}
			else if (image_extensions.count(extensions)) {
				media_files["image files: "].push_back(entry.path().filename().string());
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
	std::cout << "Enter outpt directory: ";
	std::cin >> output_directory;
	std::cout << "Enter delay(ms): ";
	std::cin >> delay;
	if (delay == 0) {
		std::cout << "Error: delay cannot be 0!";
		return 0;
	}
	while (true) {
		if (!check_directiry(home_directory, output_directory)) break;
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));
	}
	return 0;
}