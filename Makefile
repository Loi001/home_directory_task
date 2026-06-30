.PHONY: build install lint test

install:
	sudo apt-get update -y
	sudo apt-get install -y cmake python3 python3-pip clang-tidy doctest-dev libspdlog-dev libcpp-httplib-dev
	pip install conan --break-system-packages
	conan profile detect
	conan install . --output-folder=build --build=missing -s build_type=Release

build:
	cmake --preset conan-release
	cmake --build --preset conan-release

lint:
	clang-tidy -p build/compile_commands.json main.cpp

test:
	ctest --preset conan-release
	