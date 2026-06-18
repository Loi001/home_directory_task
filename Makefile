
.PHONY : build install

build: 	
	cmake --build --preset conan-release

install:
	echo hello
	apt install cmake
	apt install cpython3 python3-pip
	pip install conan --break-system-packages
	echo 'export PATH="$$HOME/.local/bin:$$PATH"' >> ~/.bashrc
	source ~/.bashrc
	conan profile detect
	conan install . --output-folder=build --build=missing
	cmake --preset conan-release
	apt-get install clang-tidy