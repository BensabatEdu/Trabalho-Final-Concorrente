/*
 * src/image_vertical.c
 * Compilar:
 * gcc src/image_vertical.c -o image_vertical -lpthread -O3 -Wall -Wextra
 * * Uso:
 * ./image_vertical <arquivo.pgm> <n_threads>
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h> 
#include <ctype.h>  

#define THRESHOLD 128

typedef struct {
    int width;
    int height;
    unsigned char *data;
} Image;

static long long g_contador_global = 0;
static pthread_mutex_t g_mutex_global = PTHREAD_MUTEX_INITIALIZER;

static inline int pixel_match(unsigned char p) { return p > THRESHOLD; }

void pgm_skip_comments_and_whitespace(FILE *fp) {
    int ch;
    while ((ch = fgetc(fp)) != EOF) {
        if (isspace(ch)) continue;
        if (ch == '#') {
            while ((ch = fgetc(fp)) != EOF && ch != '\n' && ch != '\r');
            continue;
        }
        ungetc(ch, fp);
        break;
    }
}

int pgm_read_next_int(FILE *fp) {
    int val;
    pgm_skip_comments_and_whitespace(fp);
    if (fscanf(fp, "%d", &val) != 1) {
        fprintf(stderr, "Erro: Falha ao ler valor inteiro do cabeçalho PGM.\n");
        return -1;
    }
    return val;
}

Image *carregar_pgm_real(const char *nome_arquivo) {
    FILE *fp = fopen(nome_arquivo, "rb");
    if (fp == NULL) {
        perror("Erro ao abrir arquivo de imagem");
        return NULL;
    }
    Image *img = malloc(sizeof(Image));
    if (img == NULL) {
        fprintf(stderr, "Erro: Falha ao alocar memória para Image struct.\n");
        fclose(fp);
        return NULL;
    }
    char magic[3];
    if (fgets(magic, 3, fp) == NULL || strncmp(magic, "P5", 2) != 0) {
        fprintf(stderr, "Erro: Arquivo não é do formato PGM P5 (binário).\n");
        free(img);
        fclose(fp);
        return NULL;
    }
    img->width = pgm_read_next_int(fp);
    img->height = pgm_read_next_int(fp);
    int max_val = pgm_read_next_int(fp);
    if (img->width <= 0 || img->height <= 0 || max_val <= 0) {
        fprintf(stderr, "Erro: Cabeçalho PGM inválido.\n");
        free(img);
        fclose(fp);
        return NULL;
    }
    fgetc(fp); 
    size_t total_pixels = (size_t)img->width * img->height;
    img->data = malloc(total_pixels);
    if (img->data == NULL) {
        fprintf(stderr, "Erro: Falha ao alocar memória para %zu pixels.\n", total_pixels);
        free(img);
        fclose(fp);
        return NULL;
    }
    size_t n_read = fread(img->data, 1, total_pixels, fp);
    if (n_read != total_pixels) {
        fprintf(stderr, "Erro: Falha ao ler dados de pixel. Esperado %zu, lido %zu\n",
                total_pixels, n_read);
        free(img->data);
        free(img);
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    return img;
}

typedef struct {
    int id;
    Image *img;
    int x_start;
    int x_end;
} ThreadArgsV;

static void *thread_func_v(void *arg) {
    ThreadArgsV *a = arg;
    Image *img = a->img;
    long long local = 0;
    for (int y = 0; y < img->height; y++) {
        int off = y * img->width;
        for (int x = a->x_start; x < a->x_end; x++) {
            if (pixel_match(img->data[off + x])) local++;
        }
    }
    pthread_mutex_lock(&g_mutex_global);
    g_contador_global += local;
    pthread_mutex_unlock(&g_mutex_global);
    return NULL;
}

static double diff_time_ms(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 +
           (end.tv_usec - start.tv_usec) / 1000.0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <arquivo.pgm> <n_threads>\n", argv[0]);
        return 1;
    }
    
    const char *nome_arquivo = argv[1];
    int threads = atoi(argv[2]); 
    if (threads <= 0) {
        fprintf(stderr, "Erro: Número de threads deve ser positivo.\n");
        return 1;
    }

    struct timeval t_io_start, t_io_end;
    gettimeofday(&t_io_start, NULL);

    Image *img = carregar_pgm_real(nome_arquivo);
    
    gettimeofday(&t_io_end, NULL);
    double ms_io = diff_time_ms(t_io_start, t_io_end);
    
    if (img == NULL) {
        return 1;
    }
    
    g_contador_global = 0;

    printf("=== VERSAO: CONCORRENTE - FATIAS VERTICAIS ===\n");
    printf("Imagem %s (%dx%d) | threads=%d\n", nome_arquivo, img->width, img->height, threads);
    printf("[VERTICAL] Tempo de I/O (leitura): %.3f ms\n", ms_io);

    pthread_t *ths = malloc((size_t)threads * sizeof(pthread_t));
    ThreadArgsV *args = malloc((size_t)threads * sizeof(ThreadArgsV));

    int W = img->width; 
    int cols_base = W / threads;
    int resto = W % threads;
    int x = 0;

    struct timeval t_comp_start, t_comp_end;
    gettimeofday(&t_comp_start, NULL);

    for (int i = 0; i < threads; i++) {
        int ini = x;
        int cols_extra = (i < resto ? 1 : 0);
        int fim = ini + cols_base + cols_extra;
        
        args[i].id = i;
        args[i].img = img;
        args[i].x_start = ini;
        args[i].x_end = fim;
        pthread_create(&ths[i], NULL, thread_func_v, &args[i]);
        x = fim;
    }

    for (int i = 0; i < threads; i++)
        pthread_join(ths[i], NULL);

    gettimeofday(&t_comp_end, NULL);
    double ms_comp = diff_time_ms(t_comp_start, t_comp_end);

    printf("[VERTICAL] Contador final = %lld\n", g_contador_global);
    printf("[VERTICAL] Tempo de Computação: %.3f ms (%.6f s)\n", ms_comp, ms_comp/1000.0);

    free(ths); free(args); free(img->data); free(img);
    return 0;
}