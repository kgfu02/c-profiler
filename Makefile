
all: profiler test

profiler: profiler.c
	gcc profiler.c -o profiler

test: test.c
	gcc test.c -o test

debug: profiler-debug test-debug

profiler-debug: profiler.c
	gcc -g profiler.c -o profiler-debug

test-debug: test.c
	gcc -g test.c -o test-debug

clean:
	rm profiler test test-debug profiler-debug