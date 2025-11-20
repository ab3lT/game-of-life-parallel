include ./arch

# Object files for OpenMP version
OBJ_OPENMP = life_openmp.o real_rand.o plot.o timer.o

# Object files for Pthreads version
OBJ_PTHREADS = life_pthreads.o real_rand.o plot.o timer.o

LDLIBS = -lpthread -lm

CFLAGS = -g -fopenmp

# Default target builds both versions
all: life_openmp life_pthreads

# OpenMP version
life_openmp: $(OBJ_OPENMP)
	$(CC) $(LDFLAGS) -o $@ $(OBJ_OPENMP) $(LDLIBS)

life_openmp.o: life_openmp.c
	$(CC) $(CFLAGS) -c life_openmp.c

# Pthreads version
life_pthreads: $(OBJ_PTHREADS)
	$(CC) $(LDFLAGS) -o $@ $(OBJ_PTHREADS) $(LDLIBS)

life_pthreads.o: life_pthreads.c
	$(CC) -g -c life_pthreads.c

# Common object files
real_rand.o: real_rand.c
	$(CC) $(CFLAGS) -c real_rand.c

plot.o: plot.c
	$(CC) $(CFLAGS) -c plot.c

timer.o: timer.c
	$(CC) $(CFLAGS) -c timer.c

clean:
	$(RM) *.o life_openmp life_pthreads *~
	$(RM) core
	$(RM) PI*