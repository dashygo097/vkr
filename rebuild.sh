#!/bin/bash
rm -rf CMakeFiles
rm -rf CMakeCache.txt
rm -rf cmake_install.cmake
rm -rf Makefile
rm -rf CMakeDoxyFiels.cmake
rm -rf CMakeDoxyFile.in
rm -rf CMakeDoxygenDefaults.cmake
rm -rf compile_commands.json
rm -rf ./src/CMakeFiles
rm -rf ./src/CMakeCache.txt
rm -rf ./src/cmake_install.cmake
rm -rf ./src/Makefile

is_rebuild() {
	while true; do
		read -rp "rebuild? [yes(default)/no] " response
		case "${response}" in
		"") 
			cmake CMakeLists.txt
			return 0
			;;
		yes)
			cmake CMakeLists.txt
			return 0
			;;
		no)
			return 1
			;;
		*)
			echo "Invalid input. Please type 'yes' or 'no'."
			;;
		esac
	done
}

is_rebuild
