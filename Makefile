all:
	g++ -I D:/sdl/SDLTEMPLATE/src/include -L D:/sdl/SDLTEMPLATE/src/lib -o main main.c -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lgdi32 -luser32
