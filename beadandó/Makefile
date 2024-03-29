# all:
# 	gcc sequential_main.c src/load_images.c src/color_management.c src/combine_images.c -o img_seq.exe -Iinclude -lOpenCL -g
# 	gcc openCL_main.c src/kernel_loader.c src/load_images.c -o img_openCL.exe -Iinclude -lOpenCL -g

CC = gcc

CFLAGS += -I./include
CFLAGS += -Wall -Wextra -Wpedantic -Wno-unused-parameter
CFLAGS += -g

LDFLAGS += -lOpenCL

SOURCES = openCL_main.c src_openCL/kernel_loader.c src_openCL/load_images.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = img_openCL

all: $(EXECUTABLE)

$(EXECUTABLE) : $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(OBJECTS)
