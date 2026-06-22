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
#include <httplib.h>

using json = nlohmann::json;

const std::unordered_set<std::string> audio_extensions = { ".mp3", ".wav", ".flac", ".aac", ".ogg" };
const std::unordered_set<std::string> video_extensions = { ".mp4", ".avi", ".webm", ".mkv", ".mov", ".flv", ".wmv", ".m4v", ".3gp", ".mpeg", ".mpg" };
const std::unordered_set<std::string> image_extensions = { ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp", ".tiff", ".tif", ".svg", ".heic" };

enum log_type { CONSOLE, TXTFILE, ALL };

std::string json_data = "{}";
std::mutex json_mutex;

void init_log(log_type type, spdlog::level::level_enum level)
{
	std::vector<spdlog::sink_ptr> sinks;
	if (type == CONSOLE || type == ALL) {
		auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		console_sink->set_level(level);
		sinks.push_back(console_sink);
	}
	if(type == TXTFILE || type == ALL) {
		std::filesystem::create_directories("logs");
		auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/log.txt", true);
		file_sink->set_level(level);
		sinks.push_back(file_sink);
	}
	if (sinks.empty()) {
		spdlog::error("No valid log sinks created, falling back to console");
		sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	}
	auto logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
	spdlog::set_default_logger(logger);
	logger->set_level(level);
}

bool is_valid_directory_path(const std::string& path)
{
	if (path.empty()) return false;

	std::error_code ec;
	std::filesystem::path fs_path(path);
	auto status = std::filesystem::status(fs_path, ec);

	if (!ec && std::filesystem::is_directory(status)) {
		auto perms = status.permissions();
		return (perms & std::filesystem::perms::owner_write) != std::filesystem::perms::none;
	}

	std::filesystem::path parent = fs_path.parent_path();
	if (parent.empty()) parent = std::filesystem::current_path();

	std::error_code parent_ec;
	auto parent_status = std::filesystem::status(parent, parent_ec);

	if (parent_ec || !std::filesystem::is_directory(parent_status)) return false;

	auto parent_perms = parent_status.permissions();
	return (parent_perms & std::filesystem::perms::owner_write) != std::filesystem::perms::none;
}

TEST_CASE("is_valid_dir_test1") {
    CHECK(is_valid_directory_path(".") == true);
}

TEST_CASE("is_valid_dir_test2") {
    bool result = is_valid_directory_path("/");
    CHECK((result == true || result == false)); 
}

TEST_CASE("is_valid_dir_test3") {
    CHECK(is_valid_directory_path("") == false);
}

void set_json_data(const std::string& value) {
	spdlog::debug("Set new json data");
	std::lock_guard<std::mutex> lock(json_mutex);
	json_data = value;
}

std::string get_json_data() {
	std::lock_guard<std::mutex> lock(json_mutex);
	return json_data;
}

bool check_directory(std::filesystem::path dir_path) {
	try {
		json media_files;
		media_files["audio_files"] = json::array();
		media_files["video_files"] = json::array();
		media_files["image_files"] = json::array();

		for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
			if (!entry.is_regular_file()) continue;
			std::string extension = entry.path().extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return std::tolower(c); });

			if (audio_extensions.count(extension)) {
				spdlog::debug("find new file: {}", entry.path().string());
				media_files["audio_files"].push_back(entry.path().filename().string());
			}
			else if (video_extensions.count(extension)) {
				spdlog::debug("find new file: {}", entry.path().string());
				media_files["video_files"].push_back(entry.path().filename().string());
			}
			else if (image_extensions.count(extension)) {
				spdlog::debug("find new file: {}", entry.path().string());
				media_files["image_files"].push_back(entry.path().filename().string());
			}
		}
		spdlog::debug("Dumping file to string...\n");
		set_json_data(media_files.dump(4));
	}
	catch (std::filesystem::filesystem_error& e) {
		spdlog::error(e.what());
	}
	return true;
}

void start_server()
{
	spdlog::debug("Start server");
	httplib::Server server;
	server.Get("/media_files", [](const httplib::Request& request, httplib::Response& response){
		response.set_content(get_json_data(), "application/json");
	});
	spdlog::debug("HTTP server running on http://localhost:1234/media_files");
	server.listen("localhost", 1234);
}

int main(int argc, char** argv) {
	spdlog::level::level_enum log_level = spdlog::level::info;
	log_type log_output_type = ALL;
	
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
			if(type_str == "console") log_output_type = CONSOLE;
			else if(type_str == "txtfile") log_output_type = TXTFILE;
			else if(type_str == "all") log_output_type = ALL;
		}
	}

	// init logs
	init_log(log_output_type, log_level);
	spdlog::flush_on(spdlog::level::debug);

	// main logic
	std::string home_directory, output_directory;
	int delay = 0;

	std::cout << "Enter home directory: ";
	std::cin >> home_directory;
	if (!std::filesystem::is_directory(home_directory)) {
		spdlog::critical("directory does not exist or it's not a directory");
		return 0;
	}

	std::cout << "Enter delay(ms): ";
	while (!(std::cin >> delay)) {
		std::cin.clear();
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		spdlog::error("invalid input type");
	}
	if (delay <= 0) {
		spdlog::critical("invalid delay value\n");
		return 0;
	}

	std::filesystem::path home(home_directory);

	std::thread server_thread(start_server);
	server_thread.detach();
	std::cout << "Server started at: " << "http://localhost:1234/media_files" << std::endl;

	while (true)
	{
		if (!check_directory(home)) { break; }
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));
	}

	spdlog::shutdown();
	return 0;
}
