#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 64
#define MAX_PATHS 64

char *path[MAX_PATHS] = { "/bin", NULL };  // Inicializa la ruta con /bin

void print_exec_error(char *command) {
    fprintf(stderr, "execv failed for %s: %s\n", command, strerror(errno));
}

void execute_command(char **args) {
    pid_t pid = fork();
    
    if (pid < 0) {
        // Error al hacer fork
        perror("fork failed");
        exit(1);
    } else if (pid == 0) {
        // Proceso hijo: ejecuta el comando
        char executable_path[MAX_INPUT_SIZE];
        int found = 0;
        
        for (int i = 0; path[i] != NULL; i++) {
            snprintf(executable_path, sizeof(executable_path), "%s/%s", path[i], args[0]);
            printf("Trying path: %s\n", executable_path);  // Para depuración
            if (access(executable_path, X_OK) == 0) {
                found = 1;
                execv(executable_path, args);
                print_exec_error(executable_path);  // En caso de error
                exit(1);
            }
        }
        
        if (!found) {
            fprintf(stderr, "An error has occurred\n");
        }
        exit(1);
    } else {
        // Proceso padre: espera a que el hijo termine
        wait(NULL);
    }
}

void parse_and_execute(char *input) {
    char *args[MAX_ARGS];
    int i = 0;
    
    // Parsing de la línea de entrada
    char *token = strsep(&input, " \t\n");
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strsep(&input, " \t\n");
    }
    args[i] = NULL;  // Termina la lista de argumentos con NULL

    // Ejecuta el comando si se ha ingresado alguno
    if (args[0] != NULL) {
        execute_command(args);
    }
}

int main(int argc, char *argv[]) {
    FILE *input_stream;

    // Modo batch
    if (argc == 2) {
        input_stream = fopen(argv[1], "r");
        if (!input_stream) {
            perror("fopen failed");
            exit(1);
        }
    }
    // Modo interactivo
    else if (argc == 1) {
        input_stream = stdin;
    } else {
        fprintf(stderr, "An error has occurred\n");
        exit(1);
    }

    char input[MAX_INPUT_SIZE];
    
    // Loop principal del shell
    while (1) {
        if (input_stream == stdin) {
            printf("wish> ");
        }
        
        // Lee una línea de entrada
        if (fgets(input, MAX_INPUT_SIZE, input_stream) == NULL) {
            break;  // EOF
        }
        
        // Procesa y ejecuta el comando ingresado
        parse_and_execute(input);
    }

    // Cierra el archivo si está en modo batch
    if (input_stream != stdin) {
        fclose(input_stream);
    }

    return 0;
}
