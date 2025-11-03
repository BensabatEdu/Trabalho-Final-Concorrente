/*
 * src/image_tiling_dynamic.c
 * Compilar:
 * gcc src/image_tiling_dynamic.c -o image_tiling_dynamic -lpthread -O3 -Wall -Wextra
 * * Uso:
 * ./image_tiling_dynamic <arquivo.pgm> <n_threads> [largura_bloco] [altura_bloco]
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h> 
#include <ctype.h>  

typedef struct {
    int width;
    int height;
    unsigned char *data;
} Image;

long long g_contador_global = 0;
pthread_mutex_t g_mutex_global = PTHREAD_MUTEX_INITIALIZER;

int g_proximo_bloco = 0;
pthread_mutex_t g_mutex_tarefas = PTHREAD_MUTEX_INITIALIZER;

int pixel_match(unsigned char px) { return px > 128; }

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

void processar_bloco(Image *img, int bx, int by, int bw, int bh, int n_blocos_x, long long *local) {
    (void)n_blocos_x;
    int x_ini = bx * bw, y_ini = by * bh;
    int x_fim = x_ini + bw, y_fim = y_ini + bh;
    if (x_fim > img->width)  x_fim = img->width;
    if (y_fim > img->height) y_fim = img->height;
    for (int y = y_ini; y < y_fim; y++) {
        int off = y * img->width;
        for (int x = x_ini; x < x_fim; x++)
            if (pixel_match(img->data[off + x])) (*local)++;
    }
}

typedef struct {
    int thread_id;
    Image *img;
    int total_blocos;
    int bw, bh, n_blocos_x;
} ThreadArgsDyn;

void *thread_func_dynamic(void *arg) {
    ThreadArgsDyn *a = arg;
    long long local = 0;
    while (1) {
        int task_id;
        pthread_mutex_lock(&g_mutex_tarefas);
        task_id = g_proximo_bloco;
        if (task_id >= a->total_blocos) {
            pthread_mutex_unlock(&g_mutex_tarefas);
            break;
        }
        g_proximo_bloco++;
        pthread_mutex_unlock(&g_mutex_tarefas);

        int by = task_id / a->n_blocos_x;
        int bx = task_id % a->n_blocos_x;
        
        processar_bloco(a->img, bx, by, a->bw, a->bh, a->n_blocos_x, &local);
    }
    
    pthread_mutex_lock(&g_mutex_global);
    g_contador_global += local;
    pthread_mutex_unlock(&g_mutex_global);
    return NULL;
}

double diff_time(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 +
           (end.tv_usec - start.tv_usec) / 1000.0;
}


int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <arquivo.pgm> <n_threads> [largura_bloco] [altura_bloco]\n", argv[0]);
        return 1;
    }
    
    const char *nome_arquivo = argv[1];
    int n_threads = atoi(argv[2]);
    int bw = 64, bh = 64; 

    if (argc > 3) bw = atoi(argv[3]);
    if (argc > 4) bh = (argc > 3) ? atoi(argv[4]) : bw; 
    
    if (n_threads <= 0 || bw <= 0 || bh <= 0) {
        fprintf(stderr, "Erro: threads, bw, bh devem ser positivos.\n");
        return 1;
    }

    struct timeval t_io_start, t_io_end;
    gettimeofday(&t_io_start, NULL);

    Image *img = carregar_pgm_real(nome_arquivo);
    
    gettimeofday(&t_io_end, NULL);
    double ms_io = diff_time(t_io_start, t_io_end);
    
    if (img == NULL) {
        return 1;
    }
    
    g_contador_global = 0;
    g_proximo_bloco = 0; // 

    int nbx = (img->width + bw - 1) / bw;
    int nby = (img->height + bh - 1) / bh;
    int total_blocos = nbx * nby;

    printf("=== VERSAO: TILING DINAMICO (POOL DE THREADS) ===\n");
    printf("Imagem %s (%dx%d) | threads=%d | bloco=%dx%d | total_blocos=%d\n",
           nome_arquivo, img->width, img->height, n_threads, bw, bh, total_blocos);
    printf("[TILING DINAMICO] Tempo de I/O (leitura): %.3f ms\n", ms_io);

    pthread_t *ths = malloc((size_t)n_threads * sizeof(pthread_t));
    ThreadArgsDyn *args = malloc((size_t)n_threads * sizeof(ThreadArgsDyn));

    struct timeval t_comp_start, t_comp_end;
    gettimeofday(&t_comp_start, NULL);

    for (int i = 0; i < n_threads; i++) {
        args[i].thread_id = i;
        args[i].img = img;
        args[i].total_blocos = total_blocos;
        args[i].bw = bw;
        args[i].bh = bh;
        args[i].n_blocos_x = nbx;
        pthread_create(&ths[i], NULL, thread_func_dynamic, &args[i]);
    }

    for (int i = 0; i < n_threads; i++)
        pthread_join(ths[i], NULL); 

    gettimeofday(&t_comp_end, NULL);
    double ms_comp = diff_time(t_comp_start, t_comp_end);

    printf("[TILING DINAMICO] Contador final = %lld\n", g_contador_global);
    printf("[TILING DINAMICO] Tempo de Computação: %.3f ms (%.6f s)\n", ms_comp, ms_comp/1000.0);

    free(ths); free(args); free(img->data); free(img);
    return 0;
}