#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command>\n", argv[0]);
        return 1;
    }

    struct timeval start, end;
    pid_t pid;
    int status;

    // Obtener tiempo de inicio
    if (gettimeofday(&start, NULL) == -1) {
        perror("gettimeofday");
        return 1;
    }

    // Crear proceso hijo
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    } else if (pid == 0) {
        // Proceso hijo: ejecuta el comando especificado
        execvp(argv[1], &argv[1]);
        // Si execvp falla, imprime el error y termina
        perror("execvp");
        exit(1);
    } else {
        // Proceso padre: espera a que el proceso hijo termine
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            return 1;
        }

        // Obtener tiempo de finalización
        if (gettimeofday(&end, NULL) == -1) {
            perror("gettimeofday");
            return 1;
        }

        // Calcular tiempo transcurrido
        double elapsed_time = (end.tv_sec - start.tv_sec) + 
                              (end.tv_usec - start.tv_usec) / 1000000.0;
        printf("Elapsed time: %.5f seconds\n", elapsed_time);
    }

    return 0;
}
