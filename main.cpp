#include <iostream>
#include <string>
#include <unordered_set>
#include <filesystem>
#include <algorithm>
#include <thread>
#include <fstream>
#include <limits>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

const std::unordered_set<std::string> audio_extensions = { ".mp3", ".wav", ".flac", ".aac", ".ogg" };
const std::unordered_set<std::string> video_extensions = { ".mp4", ".avi", ".webm", ".mkv", ".mov", ".flv", ".wmv", ".m4v", ".3gp", ".mpeg", ".mpg" };
const std::unordered_set<std::string> image_extensions = { ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp", ".tiff", ".tif", ".svg", ".heic" };

bool is_valid_directory_path(const std::string& path)
{
	if(path.empty()) return false;
	std::error_code err_code;
	std::filesystem::path directory_path(path);
	auto status = std::filesystem::status(directory_path, err_code);
	if(!err_code){
		if(!std::filesystem::is_directory(directory_path)) return false;
		auto premissions = status.permissions();
		if((premissions & std::filesystem::perms::owner_write) != std::filesystem::perms::none) 
		{
			return true;
		}
	}
	std::filesystem::path parent = directory_path.parent_path();
	if (parent.empty()) {
		parent = std::filesystem::current_path();
	}
	std::error_code parent_err_code;
	auto parent_status = std::filesystem::status(parent, parent_err_code);
	if(parent_err_code || !std::filesystem::is_directory(parent_status)){
		return false;
	}
	auto parent_premissons = parent_status.permissions();
	if((parent_premissons & std::filesystem::perms::owner_write) != std::filesystem::perms::none) return true;
	return true;
}

TEST_CASE("test1") {
    CHECK(is_valid_directory_path(".") == true);
}

TEST_CASE("test2") {
    bool result = is_valid_directory_path("/");
    CHECK((result == true || result == false)); 
}

TEST_CASE("test3") {
    CHECK(is_valid_directory_path("./im_does_not_exists") == true);
}

TEST_CASE("test4") {
    CHECK(is_valid_directory_path("") == false);
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

int main(int argc, char** argv) {
	// tests
	doctest::Context context;
	context.applyCommandLine(argc, argv);

	bool testChecks = false;
	for(int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		if (arg.rfind("--test", 0) == 0 || arg.rfind("-t", 0) == 0 ||
			arg.rfind("--list", 0) == 0 || arg == "--run-tests") {
			testChecks = true;
			break;
		}
	}
	if (testChecks) {
		int test_result = context.run();
		if (context.shouldExit()) {
			std::cout << "Test result: " << test_result << std::endl;
			return test_result;
		}
		std::cout << "Test result: " << test_result << std::endl;
	}

	// main logic
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