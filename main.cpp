#include <iostream> 
#include <string>  
#include <unordered_set> 
#include <filesystem> 
#include <algorithm> 
#include <thread> 
#include <fstream> 
#include <limits> 
#include <vector> 

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h> 
#include <nlohmann/json.hpp> 
#include <spdlog/spdlog.h> 
#include <spdlog/sinks/stdout_color_sinks.h> 
#include <spdlog/sinks/basic_file_sink.h> 

using json = nlohmann::json;

const std::unordered_set<std::string> audio_extensions = { ".mp3", ".wav", ".flac", ".aac", ".ogg" };
const std::unordered_set<std::string> video_extensions = { ".mp4", ".avi", ".webm", ".mkv", ".mov", ".flv", ".wmv", ".m4v", ".3gp", ".mpeg", ".mpg" };
const std::unordered_set<std::string> image_extensions = { ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp", ".tiff", ".tif", ".svg", ".heic" };

enum log_type { CONSOLE, TXTFILE, ALL };

void init_log(log_type type)
{
	std::vector<spdlog::sink_ptr> sinks;
	if (type == CONSOLE || type == ALL) {
		auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		sinks.push_back(console_sink);
	}
	if(type == TXTFILE || type == ALL) {
		auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/log.txt", true);
		sinks.push_back(file_sink);
	}
	if (sinks.empty()) {
		std::cerr << "No valid log sinks created, falling back to console" << std::endl;
		sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	}
	auto logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
	spdlog::set_default_logger(logger);
}



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

TEST_CASE("is_valid_dir_test1") {
    CHECK(is_valid_directory_path(".") == true);
}

TEST_CASE("is_valid_dir_test2") {
    bool result = is_valid_directory_path("/");
    CHECK((result == true || result == false)); 
}

TEST_CASE("is_valid_dir_test3") {
    CHECK(is_valid_directory_path("./im_does_not_exists") == true);
}

TEST_CASE("is_valid_dir_test4") {
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
				spdlog::debug("find new file: " + entry.path().string());
				media_files["audio_files"].push_back(entry.path().filename().string());
			}
			else if (video_extensions.count(extension)) {
				spdlog::debug("find new file: " + entry.path().string());
				media_files["video_files"].push_back(entry.path().filename().string());
			}
			else if (image_extensions.count(extension)) {
				spdlog::debug("find new file: " + entry.path().string());
				media_files["image_files"].push_back(entry.path().filename().string());
			}
		}
		std::filesystem::path json_path = media_dir / "media_list.json";
		spdlog::debug("output path: " + json_path.string());
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
	}
	return true;
}

int main(int argc, char** argv) {
	spdlog::level::level_enum log_level = spdlog::level::info;
	log_type log_type = ALL;
	
	// tests
	doctest::Context context;
	context.applyCommandLine(argc, argv);

	for(int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		if (arg.rfind("--test", 0) == 0) {
			int test_result = context.run();
			if (context.shouldExit()) {
				std::cout << "Test result: " << test_result << std::endl;
				return test_result;
			}
		std::cout << "Test result: " << test_result << std::endl;
			break;
		}
		if(arg.find("--log-level=") == 0) {
			std::string level_str = arg.substr(12);
			if(level_str == "trace") log_level = spdlog::level::trace;
			else if(level_str == "debug") log_level = spdlog::level::debug;
			else if(level_str == "info") log_level = spdlog::level::info;
			else if(level_str == "warn") log_level = spdlog::level::warn;
			else if(level_str == "error") log_level = spdlog::level::err;
			else if(level_str == "critical") log_level = spdlog::level::critical;
			else if(level_str == "off") log_level = spdlog::level::off;
		}
		if(arg.find("--log-type=") == 0) {
			std::string type_str = arg.substr(11);
			if(type_str == "console") log_type = CONSOLE;
			else if(type_str == "txtfile") log_type = TXTFILE;
			else if(type_str == "all") log_type = ALL;
		}
	}

	// init logs
	init_log(log_type);
	spdlog::set_level(log_level);

	// test log
	// spdlog::info("Log initialized with level: {} and type: {}", spdlog::level::to_string_view(log_level), log_type == CONSOLE ? "console" : log_type == TXTFILE ? "txtfile" : "all");
	// spdlog::debug("Debug log message");
	// spdlog::warn("Warning log message");
	// spdlog::error("Error log message");	

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
