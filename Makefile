main: main.c
	mpicc -o main main.c

run: main
	mpirun -np 8 --map-by :OVERSUBSCRIBE ./main 
