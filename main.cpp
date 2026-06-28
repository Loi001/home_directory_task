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


namespace output {
	enum type { ALL, SERVER, FILE, CONSOLE };

	static const std::unordered_map<std::string, output::type> output_types = {
	{"console", output::CONSOLE},
	{"file", output::FILE},
	{"server", output::SERVER},
	{"all", output::ALL},
	};

	std::string json_data = "{}";
	std::mutex json_mutex;

	std::string get_output_data() {
		std::lock_guard<std::mutex> lock(json_mutex);
		return json_data;
	}

	void start_server()
	{
		spdlog::debug("Start server");
		httplib::Server server;
		server.Get("/media-files", [](const httplib::Request& request, httplib::Response& response) {
			response.set_content(get_output_data(), "application/json");
		});
		spdlog::info("HTTP server running on http://localhost:1234/media-files");
		server.listen("localhost", 1234);
	}

	void set_output_data(const json& data, type output_data_type, std::filesystem::path output_dir) {
		spdlog::debug("Set new json data");
		std::lock_guard<std::mutex> lock(json_mutex);
		json_data = data.dump(4);

		if (output_data_type == CONSOLE || output_data_type == ALL) {
			std::cout << json_data;
		}
		if (output_data_type == FILE || output_data_type == ALL) {
			std::filesystem::path json_path = output_dir / "media-list.json";
			spdlog::debug("output path: " + json_path.string());
			std::ofstream output_file(json_path);
			if (!output_file.is_open()) {
				spdlog::error("Error: open file");
			} else {
				output_file << json_data;
				output_file.close();
			}
		}
	}
}


namespace logs {
	enum type { ALL, TXTFILE, CONSOLE };

	static const std::unordered_map<std::string, spdlog::level::level_enum> log_levels = {
	{"trace", spdlog::level::trace},
	{"debug", spdlog::level::debug},
	{"info", spdlog::level::info},
	{"warn", spdlog::level::warn},
	{"error", spdlog::level::err},
	{"critical", spdlog::level::critical},
	{"off", spdlog::level::off},
	};

	static const std::unordered_map<std::string, logs::type> log_types = {
    {"console", logs::CONSOLE},
    {"txtfile", logs::TXTFILE},
    {"all",     logs::ALL},
	};

	void init_log(type log_output_type, spdlog::level::level_enum log_level)
	{
		std::vector<spdlog::sink_ptr> sinks;
		if (log_output_type == CONSOLE || log_output_type == ALL) {
			auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			console_sink->set_level(log_level);
			sinks.push_back(console_sink);
		}
		if (log_output_type == TXTFILE || log_output_type == ALL) {
			std::filesystem::create_directories("logs");
			auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/log.txt", true);
			file_sink->set_level(log_level);
			sinks.push_back(file_sink);
		}
		if (sinks.empty()) {
			spdlog::error("No valid log sinks created, falling back to console");
			sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
		}
		auto logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
		spdlog::set_default_logger(logger);
		logger->set_level(log_level);
	}
}

namespace test{
	enum type { ALL, NONE, VALIDATE_DIRECTORY_PATH, WALK_DIRECTORY };
	type test_value = ALL;
	static const std::unordered_map<std::string, type> test_values = {
		{"all", test::ALL},
		{"none", test::NONE},
		{"validate_directory_path", test::VALIDATE_DIRECTORY_PATH},
		{"walk_directory", test::WALK_DIRECTORY},
	};
	static const std::unordered_map<type, std::string> test_suites = {
		{test::VALIDATE_DIRECTORY_PATH, "validate_directory_path"},
		{test::WALK_DIRECTORY, "walk_directory"},
	};
	void start_test(doctest::Context& context)
	{
		if(test_value == NONE) return;
		spdlog::info("Start test");
		auto suite = test_suites.find(test_value);
		if (suite != test_suites.end()) {
			context.setOption("test-suite", suite->second.c_str());
		}
		int test_result = context.run();
		if (context.shouldExit()) {
			std::cout << "Test result: " << test_result << std::endl;
			exit(test_result);
		}
	}
}

enum class parse_result { PREFIX_MISMATCH, INVALID_VALUE, OK };

template<typename T>
parse_result parse_arg(const std::string& arg, const std::string& prefix, const std::unordered_map<std::string, T>& values_table, T& out)
{
	if(arg.substr(0, prefix.size()) != prefix) return parse_result::PREFIX_MISMATCH;
	auto key = arg.substr(prefix.size());
	auto iter = values_table.find(key);
	if(iter == values_table.end()) return parse_result::INVALID_VALUE;
	out = iter->second;
	return parse_result::OK;
}

bool validate_directory_path(const std::string& path)	
{
	try {
		std::filesystem::path test_path(path);
		std::filesystem::path dir = test_path;
		if (!dir.empty() && !std::filesystem::exists(dir)) {
			std::filesystem::create_directory(dir);
		}
		std::filesystem::path temp_file = dir / ".temp_file";
		std::ofstream test_file(temp_file, std::ios::out | std::ios::trunc);
		if (!test_file.is_open()) return false;			test_file.close();
		std::filesystem::remove(temp_file);
		return true;
	} catch (const std::filesystem::filesystem_error& e) {
		spdlog::error(e.what());
	}
	return false;
}

json walk_directory(std::filesystem::path dir_path) {
	json media_files;
	try {
		media_files["audio_files"] = json::array();
		media_files["video_files"] = json::array();
		media_files["image_files"] = json::array();

		for (const auto& entry : std::filesystem::recursive_directory_iterator(dir_path)) {
			if (!entry.is_regular_file()) continue;
			std::string extension = entry.path().extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return std::tolower(c); });

			if (audio_extensions.count(extension)) {
				spdlog::debug("find new file: {}", entry.path().string());
				media_files["audio_files"].push_back(entry.path().filename().string());
			} else if (video_extensions.count(extension)) {
				spdlog::debug("find new file: {}", entry.path().string());
				media_files["video_files"].push_back(entry.path().filename().string());
			} else if (image_extensions.count(extension)) {
				spdlog::debug("find new file: {}", entry.path().string());
				media_files["image_files"].push_back(entry.path().filename().string());
			}
		}
	} catch (std::filesystem::filesystem_error& e) {
		spdlog::error(e.what());
	}
	return media_files;
}

TEST_SUITE("validate_directory_path") {
    TEST_CASE("current dir") {
        CHECK(validate_directory_path(".") == true);
    }
    TEST_CASE("root dir") {
        bool result = validate_directory_path("/");
        CHECK((result == true || result == false));
    }
    TEST_CASE("non exist dir") {
        CHECK(validate_directory_path("/nonexistent") == false);
    }
}

TEST_SUITE("walk_directory") {
    TEST_CASE("valid dir") {
        json result = walk_directory(".");
        CHECK(result.contains("audio_files"));
        CHECK(result.contains("video_files"));
        CHECK(result.contains("image_files"));
        CHECK(result["audio_files"].is_array());
        CHECK(result["video_files"].is_array());
        CHECK(result["image_files"].is_array());
    }
    TEST_CASE("nonexistent dir") {
        json result = walk_directory("/nonexistent_path");
        CHECK(result.contains("audio_files"));
        CHECK(result["audio_files"].is_array());
        CHECK(result["audio_files"].empty());
    }
}

int main(int argc, char** argv) {
	doctest::Context context;
	context.applyCommandLine(argc, argv);

	logs::type log_output_type = logs::ALL;
	spdlog::level::level_enum logs_level = spdlog::level::trace;
	test::type test_value = test::ALL;
	output::type output_data_type = output::ALL;

	for (int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		auto test_result = parse_arg(arg, "--test=", test::test_values, test_value);
		if (test_result == parse_result::INVALID_VALUE) {
			std::cout << "Invalid test value: " << arg << std::endl;
			return 1;
		}
		auto log_type = parse_arg(arg, "--log-type=", logs::log_types, log_output_type);
		if (log_type == parse_result::INVALID_VALUE) {
			std::cout << "Invalid log type: " << arg << std::endl;
			return 1;
		}
		auto log_level = parse_arg(arg, "--log-level=", logs::log_levels, logs_level);
		if (log_level == parse_result::INVALID_VALUE) {
			std::cout << "Invalid log level: " << arg << std::endl;
			return 1;
		}
		auto output_type = parse_arg(arg, "--output-type=", output::output_types, output_data_type);
		if (output_type == parse_result::INVALID_VALUE) {
			std::cout << "Invalid output type: " << arg << std::endl;
			return 1;
		}

	}

	logs::init_log(log_output_type, logs_level);
	spdlog::flush_on(spdlog::level::debug);

	test::start_test(context);

	std::string home_directory;
	std::filesystem::path output_dir;

	std::cout << "Enter home directory: ";
	std::cin >> home_directory;
	if (!std::filesystem::is_directory(home_directory)) {
		spdlog::critical("directory does not exist or it's not a directory");
		return 0;
	}

	if (output_data_type == output::FILE || output_data_type == output::ALL) {
		std::string output_path;
		std::cout << "Enter output directory: ";
		std::cin >> output_path;
		if (!validate_directory_path(output_path)) {
			spdlog::critical("invalid or inaccessible output directory");
			return 0;
		}
		output_dir = std::filesystem::absolute(output_path);
	}

	
	if(output_data_type == output::SERVER || output_data_type == output::ALL)
	{
		static std::once_flag server_started;
			std::call_once(server_started, []() {
				std::thread server_thread(output::start_server);
				server_thread.detach();
			});
	}

	int delay = 0;
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

	while (true)
	{
		output::set_output_data(walk_directory(home), output_data_type, output_dir);
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));
	}

	spdlog::shutdown();
	return 0;
}