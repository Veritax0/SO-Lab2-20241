// Integrantes:
// Juan Manuel Vera Osorio  CC 1000416823
// Valentina Cadena Zapata  CC 1000099120

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_PATHS 10
#define MAX_ARGS 100

char *path[MAX_PATHS]; // Array para almacenar las rutas de búsqueda
int path_count = 0;    // Contador de rutas en path
char error_message[] = "An error has occurred\n";

// Función para ejecutar comandos externos con o sin redirección
void execute_command(char **args, char *output_file) {
    for (int i = 0; i < path_count; i++) {
        char exec_path[1024];
        snprintf(exec_path, sizeof(exec_path), "%s/%s", path[i], args[0]);

        if (access(exec_path, X_OK) == 0) {
            if (fork() == 0) {
                if (output_file) {
                    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
                    if (fd == -1) {
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        exit(1);
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }
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

// Función para procesar y ejecutar un comando individual
void process_command(char *line) {
    char *args[MAX_ARGS];
    int arg_count = 0;
    char *output_file = NULL;

    // Manejo de redirección de salida
    char *redirect_pos = strchr(line, '>');
    if (redirect_pos) {
        if (redirect_pos == line) {
            // Si `>` es el primer carácter, muestra un error y retorna
            write(STDERR_FILENO, error_message, strlen(error_message));
            return;
        }

        *redirect_pos = '\0';
        redirect_pos++;

        // Verifica que sólo hay un archivo de salida especificado
        char *output_token = strtok(redirect_pos, " \t\n");
        if (output_token && strtok(NULL, " \t\n") == NULL) {
            output_file = output_token;
        } else {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return;
        }
    }

    // Tokeniza la línea de comando
    char *token = strtok(line, " \t\n");
    while (token != NULL && arg_count < MAX_ARGS - 1) {
        args[arg_count++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[arg_count] = NULL;

    if (arg_count == 0) return;

    // Comando `exit`
    if (strcmp(args[0], "exit") == 0) {
        if (args[1] != NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message));
        } else {
            exit(0);
        }
        return;
    }

    // Comando `cd`
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

    // Comando `path`
    if (strcmp(args[0], "path") == 0) {
        for (int i = 0; i < path_count; i++) {
            free(path[i]);
            path[i] = NULL;
        }
        path_count = 0;

        for (int i = 1; args[i] != NULL && path_count < MAX_PATHS; i++) {
            path[path_count] = strdup(args[i]);
            if (path[path_count] == NULL) {
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(1);
            }
            path_count++;
        }
        return;
    }

    // Si el comando no es `exit`, `cd` o `path`, intenta ejecutarlo como comando externo
    execute_command(args, output_file);
}

// Función para procesar comandos paralelos
void process_parallel_commands(char *line) {
    char *command;
    char *saveptr;

    command = strtok_r(line, "&", &saveptr);
    int parallel_pids[MAX_ARGS];
    int parallel_count = 0;

    while (command != NULL) {
        while (*command == ' ' || *command == '\t') command++;
        size_t len = strlen(command);
        while (len > 0 && (command[len - 1] == ' ' || command[len - 1] == '\t')) {
            command[--len] = '\0';
        }

        if (strlen(command) > 0) {
            pid_t pid = fork();
            if (pid == 0) {
                process_command(command);
                exit(0);
            } else if (pid > 0) {
                parallel_pids[parallel_count++] = pid;
            } else {
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
        }
        command = strtok_r(NULL, "&", &saveptr);
    }

    for (int i = 0; i < parallel_count; i++) {
        waitpid(parallel_pids[i], NULL, 0);
    }
}

// Función principal del shell
void wish_shell(FILE *input, int is_interactive) {
    char *line = NULL;
    size_t len = 0;

    while (1) {
        if (is_interactive) {
            printf("wish> ");
        }
        if (getline(&line, &len, input) == -1) {
            break;
        }

        if (strchr(line, '&') != NULL) {
            process_parallel_commands(line);
        } else {
            process_command(line);
        }
    }

    free(line);
}

int main(int argc, char *argv[]) {
    path[0] = strdup("/bin");
    if (path[0] == NULL) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
    path_count = 1;

    if (argc == 1) {
        wish_shell(stdin, 1);
    } else if (argc == 2) {
        FILE *file = fopen(argv[1], "r");
        if (file == NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
        wish_shell(file, 0);
        fclose(file);
    } else {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    for (int i = 0; i < path_count; i++) {
        free(path[i]);
    }
    return 0;
}
