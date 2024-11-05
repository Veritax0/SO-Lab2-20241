#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_PATHS 10
#define MAX_ARGS 100

char *path[MAX_PATHS] = {"/bin"}; // Inicializa el path en "/bin"
int path_count = 1;                // Cuenta de los directorios en el path
char error_message[] = "An error has occurred\n";

// Función para ejecutar comandos externos
void execute_command(char **args) {
    for (int i = 0; i < path_count; i++) {
        char exec_path[1024];
        snprintf(exec_path, sizeof(exec_path), "%s/%s", path[i], args[0]);

        if (access(exec_path, X_OK) == 0) {
            if (fork() == 0) {
                execv(exec_path, args);
                exit(0);
            } else {
                wait(NULL);
            }
            return;
        }
    }
    write(STDERR_FILENO, error_message, strlen(error_message));
}

// Función para procesar cada línea de comandos
void process_line(char *line) {
    char *args[MAX_ARGS];
    int arg_count = 0;
    char *token = strtok(line, " \t\n");
    while (token != NULL && arg_count < MAX_ARGS - 1) {
        args[arg_count++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[arg_count] = NULL;

    if (arg_count == 0) return;

    if (strcmp(args[0], "exit") == 0) {
        if (args[1] != NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message));
        } else {
            exit(0);
        }
        return;
    }

    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL || args[2] != NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message));
        } else {
            if (chdir(args[1]) != 0) {
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
        }
        return;
    }

    if (strcmp(args[0], "path") == 0) {
        for (int i = 0; i < path_count; i++) {
            free(path[i]);
        }
        path_count = 0;

        for (int i = 1; args[i] != NULL; i++) {
            path[path_count] = strdup(args[i]);
            path_count++;
        }
        return;
    }

    execute_command(args);
}

// Función para ejecutar el shell
void wish_shell(FILE *input) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, input)) != -1) {
        process_line(line);
    }

    free(line);
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        // Modo interactivo
        wish_shell(stdin);
    } else if (argc == 2) {
        // Modo batch: leer de un archivo
        FILE *file = fopen(argv[1], "r");
        if (file == NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
        wish_shell(file);
        fclose(file);
    } else {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    return 0;
}
