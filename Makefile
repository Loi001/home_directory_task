
.PHONY : build install lint

build: 	
	cmake --build --preset conan-debug

install:
	echo hello
	apt install cmake
	apt install cpython3 python3-pip
	pip install conan --break-system-packages
	echo 'export PATH="$$HOME/.local/bin:$$PATH"' >> ~/.bashrc
	source ~/.bashrc
	conan profile detect
	conan install . --output-folder=build --build=missing
	cmake --preset conan-debug
	apt-get install clang-tidy
	sudo apt-get install doctest-dev
lint:
	clang-tidy -p build/compile_commands.json main.cpp
