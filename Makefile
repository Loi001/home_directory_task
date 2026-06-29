
.PHONY : build install lint

build: 	
	cmake --build --preset conan-release

install:
	apt-get -y install cmake python3 python3-pip clang-tidy doctest-dev libspdlog-dev libcpp-httplib-dev
	pip install conan --break-system-packages
	echo 'export PATH="$$HOME/.local/bin:$$PATH"' >> ~/.bashrc
	source ~/.bashrc || echo "cannot source ~/.bashrc"
	conan profile detect
	conan install . --output-folder=build --build=missing
	cmake --preset conan-release
lint:
	clang-tidy -p build/compile_commands.json main.cpp
