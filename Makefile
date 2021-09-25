all: scheduler

clean:
	rm -f ./scheduler

scheduler: scheduler.c
	gcc -o scheduler scheduler.c