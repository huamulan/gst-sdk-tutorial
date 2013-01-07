# http://www.gnu.org/software/make/manual/make.html

CC:=gcc

CFLAGS:=-Wall -ggdb $(shell pkg-config --cflags gstreamer-0.10 gstreamer-interfaces-0.10 gtk+-2.0 clutter-gst-1.0 gstreamer-pbutils-0.10)
LDFLAGS:=$(shell pkg-config --libs gstreamer-0.10 gstreamer-interfaces-0.10 gtk+-2.0 clutter-gst-1.0 gstreamer-pbutils-0.10)

EXE := basic-tutorial-1.out \
	basic-tutorial-2.out \
	basic-tutorial-3.out \
	basic-tutorial-4.out \
	basic-tutorial-5.out \
	basic-tutorial-6.out \
	basic-tutorial-7.out \
	basic-tutorial-8.out \
	basic-tutorial-9.out \
	basic-tutorial-12.out \
	basic-tutorial-13.out \
	basic-tutorial-15.out \
	playback-tutorial-1.out \
	playback-tutorial-2.out \
	playback-tutorial-3.out \
	playback-tutorial-4.out \
	playback-tutorial-5.out \
	playback-tutorial-6.out \
	playback-tutorial-7.out \

# $< is the first dependency in the dependency list
# $@ is the target name
all: dirs $(addprefix bin/, $(EXE)) tags

dirs:
	mkdir -p obj
	mkdir -p bin

tags: *.c
	ctags *.c

bin/%.out: obj/%.o
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

obj/%.o : %.c
	$(CC) $(CFLAGS) $< -c -o $@

clean:
	rm -rf obj/
	rm -rf bin/
	rm -rf tags
