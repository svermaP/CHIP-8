.PHONY: build run clean

build:
	cmake -S . -B build
	cmake --build build

ibm: build
	./build/chip8 ./examples/ibm_logo.ch8

breakout: build
	./build/chip8 ./examples/br8kout.ch8

flight: build
	./build/chip8 ./examples/flightrunner.ch8

snake: build
	./build/chip8 ./examples/snake.ch8

tictactoe: build
	./build/chip8 ./examples/tictactoe.ch8

custom: build  # Usage: make custom ARG=filename (no .ch8)
ifndef ARG
	$(error ARG is required, e.g. make custom ARG=snake)
endif
	./build/chip8 ./examples/$(ARG).ch8

clean:
	rm -rf build bin out dist

help:
	@echo "Makefile targets:"
	@echo "  build         - Build the CHIP-8 emulator"
	@echo "  ibm           - Run the IBM Logo ROM"
	@echo "  breakout      - Run the Breakout ROM"
	@echo "  flight        - Run the Flight Runner ROM"
	@echo "  snake         - Run the Snake ROM"
	@echo "  tictactoe     - Run the Tic Tac Toe ROM"
	@echo "  custom ARG=   - Run a custom ROM from the examples directory (without .ch8 extension)"
	@echo "  clean         - Clean up build artifacts"