.PHONY: build run clean

build:
	cmake -B build
	cmake --build build

run:
	@if [ -z "$(FILE)" ]; then \
		echo "Usage: make run FILE=<filename>"; \
	else \
		./build/kompy $(FILE); \
	fi

clean:
	rm -rf build out out.asm out.o