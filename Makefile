CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O3 -Iinclude -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -D_GNU_SOURCE

SRC = src/main.c \
      src/utils/file_utils.c \
      src/utils/string_utils.c \
      src/data/data_manager.c \
      src/data/yaml_parser.c \
      src/data/json_parser.c \
      src/frontmatter/frontmatter.c \
      src/markdown/markdown.c \
      src/template/template_engine.c \
      src/template/filter.c \
      src/sitemap/sitemap.c \
      src/collection/collection.c \
      src/pagination/pagination.c \
      src/tags/tags.c \
      src/build/builder.c

OBJ = src/main.o \
      src/utils/file_utils.o \
      src/utils/string_utils.o \
      src/data/data_manager.o \
      src/data/yaml_parser.o \
      src/data/json_parser.o \
      src/frontmatter/frontmatter.o \
      src/markdown/markdown.o \
      src/template/template_engine.o \
      src/template/filter.o \
      src/sitemap/sitemap.o \
      src/collection/collection.o \
      src/pagination/pagination.o \
      src/tags/tags.o \
      src/build/builder.o

TARGET = cax.exe

all: banner $(TARGET)

banner:
	@echo ""
	@echo " CAX SSG - C STATIC SITE GENERATOR"
	@echo " BY AXCORA TECHNOLOGY"
	@echo " ------------------------------------------"

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^
	@echo "[OK] Built $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo ""
	@echo " CAX SSG - C STATIC SITE GENERATOR"
	@echo " BY AXCORA TECHNOLOGY"
	@echo " ------------------------------------------"
ifeq ($(OS),Windows_NT)
	-del src\main.o 2>nul
	-del src\utils\*.o 2>nul
	-del src\data\*.o 2>nul
	-del src\frontmatter\*.o 2>nul
	-del src\markdown\*.o 2>nul
	-del src\template\*.o 2>nul
	-del src\sitemap\*.o 2>nul
	-del src\collection\*.o 2>nul
	-del src\pagination\*.o 2>nul
	-del src\tags\*.o 2>nul
	-del src\build\*.o 2>nul
	-del $(TARGET) 2>nul
	-del cax 2>nul
else
	rm -f $(OBJ) $(TARGET) cax
endif
	@echo "[CLEAN] Done"

linux: banner
	$(CC) $(CFLAGS) -o cax $(SRC)
	@echo "[OK] Built cax for Linux"

windows: banner
	$(CC) $(CFLAGS) -o cax.exe $(SRC)
	@echo "[OK] Built cax.exe for Windows"

.PHONY: all banner clean linux windows