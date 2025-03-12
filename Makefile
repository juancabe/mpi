main: main.c
	mpicc -o main main.c

run: main
	mpirun -np 4 ./main
