.PHONY: build install lint test

install:
	sudo apt-get update -y
	sudo apt-get install -y cmake python3 python3-pip clang-tidy doctest-dev libspdlog-dev libcpp-httplib-dev nlohmann-json3-dev
	pip install conan --break-system-packages --user
	conan profile detect --force
	conan install . --output-folder=build --build=missing -s build_type=Debug

build:
	cmake --preset conan-debug
	cmake --build --preset conan-debug

lint:
	clang-tidy -p build/compile_commands.json main.cpp

test:
	ctest --preset conan-debug --output-on-failure
