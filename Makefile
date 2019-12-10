CFLAGS = -O0 -std=c11
OUTPUT = build

all:: main

main: main.c
		mkdir -p $(OUTPUT)
		cc $(CFLAGS) -c main.c -g -o $(OUTPUT)/main.o -Wno-deprecated-declarations
		cc $(OUTPUT)/main.o -o $(OUTPUT)/main.dll

clean:
	rm -rf $(OUTPUT)
