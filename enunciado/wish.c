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

// Función para ejecutar comandos externos con soporte de redirección
void execute_command(char **args) {
    int redirect_index = -1;

    // Buscar el operador de redirección '>'
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            redirect_index = i;
            break;
        }
    }

    // Configurar la redirección si se encuentra '>'
    int saved_stdout = -1;
    if (redirect_index != -1) {
        // Verificar que exista un archivo de destino después de '>'
        if (args[redirect_index + 1] == NULL || args[redirect_index + 2] != NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return;
        }

        // Abrir el archivo para redirigir la salida
        int fd = open(args[redirect_index + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            write(STDERR_FILENO, error_message, strlen(error_message));
            return;
        }

        // Guardar stdout y redirigir la salida
        saved_stdout = dup(STDOUT_FILENO);
        dup2(fd, STDOUT_FILENO);
        close(fd);

        // Terminar la lista de argumentos en el operador '>'
        args[redirect_index] = NULL;
    }

    // Intentar ejecutar el comando en cada directorio de `path`
    int found = 0;
    for (int i = 0; i < path_count; i++) {
        char exec_path[1024];
        snprintf(exec_path, sizeof(exec_path), "%s/%s", path[i], args[0]);

        if (access(exec_path, X_OK) == 0) {
            found = 1;
            if (fork() == 0) {
                execv(exec_path, args);
                exit(0);
            } else {
                wait(NULL);
            }
            break;
        }
    }

    // Si no se encontró el comando en ninguna ruta
    if (!found) {
        write(STDERR_FILENO, error_message, strlen(error_message));
    }

    // Restaurar stdout si fue redirigido
    if (saved_stdout != -1) {
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
    }
}

// Función para procesar y ejecutar un comando
void process_command(char *line) {
    char *args[MAX_ARGS];
    int arg_count = 0;
    char *token = strtok(line, " \t\n");

    // Salta si el único comando es '&'
    if (token != NULL && strcmp(token, "&") == 0 && strtok(NULL, " \t\n") == NULL) {
        return; // Ignorar '&' como único comando
    }

    // Tokenizar la línea de entrada en argumentos
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
        // Libera cualquier memoria previamente asignada en `path`
        for (int i = 0; i < path_count; i++) {
            free(path[i]);
            path[i] = NULL;
        }
        path_count = 0;

        // Asigna nuevas rutas a `path`
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
    execute_command(args);
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
        process_command(line);
    }

    free(line);
}

int main(int argc, char *argv[]) {
    // Inicializa path con "/bin" dinámicamente
    path[0] = strdup("/bin");
    if (path[0] == NULL) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
    path_count = 1;

    if (argc == 1) {
        // Modo interactivo
        wish_shell(stdin, 1);
    } else if (argc == 2) {
        // Modo batch, abre el archivo especificado
        FILE *file = fopen(argv[1], "r");
        if (file == NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
        wish_shell(file, 0); // Modo batch, sin prompt
        fclose(file);
    } else {
        // Error: demasiados argumentos
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    // Libera la memoria asignada a `path`
    for (int i = 0; i < path_count; i++) {
        free(path[i]);
    }
    return 0;
}
