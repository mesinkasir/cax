@echo off
echo Building Windows binary...
mingw32-make clean
mingw32-make

echo Building Linux binary...
gcc -Wall -Wextra -std=c11 -O3 -Iinclude -o cax-linux src/main.c src/utils/*.c src/data/*.c src/frontmatter/*.c src/markdown/*.c src/template/*.c src/sitemap/*.c src/collection/*.c src/pagination/*.c src/tags/*.c src/build/*.c

echo Done!