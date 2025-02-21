#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>
#include <time.h>

#define NUM_MAX  100000000
#define SETS_LISTA 3
#define TAMANHO_LISTA 80

// Se usarán los primeros 50 números de cada set
#define SETS  3
#define TAMANHO  50

#define RAND_MAX 2147483647

// Definición de tags para la comunicación
#define TAG_FOUND 100
#define TAG_STOP  101

// Matriz de números (lista original del enunciado)
unsigned int numeros[SETS_LISTA][TAMANHO_LISTA] = {
  {8,66,748,1158,10231,497134,4608707,47820120,2,41,578,1423,52611,770895,2790721,34643109,
   8,10,851,2209,69990,159627,3385804,15821867,5,13,692,1378,20023,820983,5989397,71761937,
   6,62,127,8986,36126,971985,5739524,70900793,3,43,692,6066,60555,844783,3509650,93991716,
   1,56,859,2407,41146,288994,9565675,70779739,9,47,876,2456,20591,920964,4403757,68563587,
   2,21,355,7035,58257,956057,2244822,98602552,2,90,306,2971,95222,814239,7627087,54434251},
  {4,77,886,7564,50085,993399,4874938,74030116,9,23,737,3708,67607,956008,6248183,43669844,
   7,50,684,5893,69898,200014,2136757,91838319,8,75,877,2507,65557,559730,8465332,75839092,
   1,94,144,2057,61882,864629,8394357,80773957,9,74,224,1417,21923,362955,8884601,49938426,
   9,15,573,2464,66176,422314,9860085,88384300,8,55,153,2757,54958,912936,5326961,30264300,
   5,69,299,6618,10463,556147,8711726,95397660,3,29,684,5303,58626,364864,8198828,80756457},
  {1,44,763,6169,67524,919717,1473763,13587296,5,71,569,1163,11550,281387,8255291,93701450,
   5,52,302,4630,61202,218275,7011910,75292541,1,91,505,2139,65462,978520,9179619,24758500,
   9,41,901,5923,31806,660136,6392446,16262444,8,40,543,2759,57699,282161,6890176,39677629,
   8,65,247,6476,88372,558802,7420916,28514707,5,62,103,2025,29201,719487,6806547,97055905,
   1,11,717,6131,75383,936184,4064483,14741509,5,87,136,3076,60375,528071,8505013,67316720}
};

// Funciones de tiempo y generador aleatorio (tal como en el enunciado)
double mygettime(void) {
  struct timeval tv;
  if(gettimeofday(&tv, 0) < 0) {
    perror("oops");
  }
  return (double)tv.tv_sec + (0.000001 * (double)tv.tv_usec);
}

static unsigned int next = 1;
int myrand(void) {
    next = next * 1103515245 + 12345;
    return (int)(next % RAND_MAX);
}
void mysrand(unsigned int seed) {
    next = seed;
}

// Variables globales para conocer el identificador del proceso y quién encontró el número
int my_rank = 0;
int found_by_proc = -1;  // -1 significa “no encontrado” en este proceso

/* 
 * Función de búsqueda (adaptada del enunciado):
 * Se generan números aleatorios hasta que se encuentre el número objetivo o se reciba un mensaje de parada.
 * Además, si se encuentra el número, el proceso (si no es 0) envía un aviso al proceso 0.
 */
void busca_numero(unsigned int numero, unsigned long long *intentos, double *tpo) {
    short bSalir = 0;
    double t1, t2;
    *intentos = 0;
    t1 = mygettime();
    MPI_Status status;
    while (!bSalir) {
        (*intentos)++;
        int rnd = (myrand() % (NUM_MAX - 1)) + 1;
        if (rnd == numero) {
            bSalir = 1;
            found_by_proc = my_rank;
            // Si no es el proceso 0, notifica al proceso 0 que encontró el número.
            if (my_rank != 0) {
                MPI_Send(&my_rank, 1, MPI_INT, 0, TAG_FOUND, MPI_COMM_WORLD);
            }
        }
        // El proceso 0 comprueba si recibe notificación de que otro proceso encontró el número.
        if (my_rank == 0) {
            int flag_found = 0;
            MPI_Iprobe(MPI_ANY_SOURCE, TAG_FOUND, MPI_COMM_WORLD, &flag_found, &status);
            if (flag_found) {
                int sender;
                MPI_Recv(&sender, 1, MPI_INT, status.MPI_SOURCE, TAG_FOUND, MPI_COMM_WORLD, &status);
                found_by_proc = sender;
                bSalir = 1;
            }
        }
        // Todos los procesos comprueban si han recibido un mensaje de parada (del proceso 0)
        int flag_stop = 0;
        MPI_Iprobe(0, TAG_STOP, MPI_COMM_WORLD, &flag_stop, &status);
        if (flag_stop) {
            int dummy;
            MPI_Recv(&dummy, 1, MPI_INT, 0, TAG_STOP, MPI_COMM_WORLD, &status);
            break;
        }
    }
    t2 = mygettime();
    *tpo = t2 - t1;
}

//////////////////////////////////////////////////////////////////////////
// Programa principal
//////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    int size;
    MPI_Init(&argc, &argv); 
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Inicializar la semilla de forma distinta en cada proceso
    mysrand((unsigned int)(time(NULL) + my_rank * 100));

    unsigned long long total_intentos = 0;
    double total_tpo = 0.0;

    // Bucle sobre cada "set"
    for (int set = 0; set < SETS; set++) {
        if (my_rank == 0)
            printf("SET: %d\n", set);
        // Para cada número (se usan los primeros TAMANHO números de cada set)
        for (int n = 0; n < TAMANHO; n++) {
            int numero;
            // El proceso 0 obtiene el número de la lista
            if (my_rank == 0)
                numero = numeros[set][n];
            // Difunde el número a todos los procesos
            MPI_Bcast(&numero, 1, MPI_INT, 0, MPI_COMM_WORLD);

            // Reiniciar la variable global de control para la búsqueda
            found_by_proc = -1;

            // Cada proceso intenta adivinar el número usando la función proporcionada
            unsigned long long intentos_local = 0;
            double tpo_local = 0.0;
            busca_numero(numero, &intentos_local, &tpo_local);

            // El proceso 0, una vez finalizada la búsqueda (por haber encontrado él o por notificación),
            // envía un mensaje de parada a los otros procesos para asegurarse de que abandonen la búsqueda.
            if (my_rank == 0) {
                for (int proc = 1; proc < size; proc++) {
                    int dummy = 0;
                    MPI_Send(&dummy, 1, MPI_INT, proc, TAG_STOP, MPI_COMM_WORLD);
                }
            }

            // Se recopilan las estadísticas globales: suma de intentos y máximo tiempo (cuello de botella)
            unsigned long long suma_intentos = 0;
            double max_tpo = 0.0;
            MPI_Reduce(&intentos_local, &suma_intentos, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
            MPI_Reduce(&tpo_local, &max_tpo, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

            if (my_rank == 0) {
                total_intentos += suma_intentos;
                total_tpo += max_tpo;
                printf("S:%d/%d, N:%2d/%d) Num:%10d, Int:%10llu, Tpo: %.8f (ENCONTRADO POR %d)\n",
                       set+1, SETS, n+1, TAMANHO, numero, suma_intentos, max_tpo,
                       (found_by_proc == -1 ? 0 : found_by_proc));
            }
            MPI_Barrier(MPI_COMM_WORLD); // Sincronización antes de pasar al siguiente número
        }
        if (my_rank == 0)
            printf("Resumen SET %d: Intentos Acumulados: %llu, Tiempo Acumulado: %.8f\n", set, total_intentos, total_tpo);
        MPI_Barrier(MPI_COMM_WORLD);
    }
    
    // Al finalizar, el proceso 0 muestra los totales
    if (my_rank == 0) {
        printf("FINAL:\n");
        printf("Intentos Acumulados: %llu\n", total_intentos);
        printf("Tiempo Total Prueba: %.8f\n", total_tpo);
    }
    
    MPI_Finalize();
    return 0;
}

