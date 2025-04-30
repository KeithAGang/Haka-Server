compile:
	@echo "Compiling..."
	g++ -std=c++20 src/main.cpp src/users.cpp -I./include -ID:/Devlibs/include -lws2_32 -lmswsock -o build/out/main

run:
	@echo "Running...\n"
	./build/out/main