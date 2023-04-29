INC=-I./includes/
PROVIDED_LIBRARIES:=$(shell find libs/ -type f -name '*.a' 2>/dev/null)
PROVIDED_LIBRARIES:=$(PROVIDED_LIBRARIES:libs/lib%.a=%)
LDFLAGS = -Llibs/ $(foreach lib,$(PROVIDED_LIBRARIES),-l$(lib)) -lm

all: profiler test test2

profiler: profiler.c
	gcc profiler.c -o profiler -ldwarf $(LDFLAGS) $(INC)

test: test.c
	gcc test.c -o test -g

test2: test2.c
	gcc test2.c -o test2 -g

debug: profiler-debug test-debug

sandbox: sandbox.c
	gcc sandbox.c -o sandbox

profiler-debug: profiler.c
	gcc -g profiler.c -o profiler-debug -ldwarf $(LDFLAGS) $(INC)

test-debug: test.c
	gcc -g test.c -o test-debug

clean:
	rm profiler test test-debug profiler-debug test2