
all: profiler test

profiler: profiler.c
	gcc profiler.c -o profiler -ldwarf

test: test.c
	gcc test.c -o test -g

debug: profiler-debug test-debug

sandbox: sandbox.c
	gcc sandbox.c -o sandbox

profiler-debug: profiler.c
	gcc -g profiler.c -o profiler-debug -ldwarf

test-debug: test.c
	gcc -g test.c -o test-debug

clean:
	rm profiler test test-debug profiler-debug