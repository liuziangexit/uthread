CFLAGS = -Ofast -std=c11 -Wall -Wno-deprecated-declarations
OUTPUT = build

all:: main

main: main.c
		mkdir -p $(OUTPUT)
		cc $(CFLAGS) -c main.c -o $(OUTPUT)/main.o -g
		cc $(OUTPUT)/main.o -o $(OUTPUT)/main.dll

clean:
	rm -rf $(OUTPUT)
