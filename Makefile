main: main.c
	mpicc -o main main.c

run: main
	mpirun -np 10 --map-by :OVERSUBSCRIBE ./main 
