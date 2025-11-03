/*
 * src/image_sequencial.c
 * Compilar:
 * gcc src/image_sequencial.c -o image_sequencial -O3 -Wall -Wextra
 * * Uso:
 * ./image_sequencial <caminho_para_imagem.pgm>
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h> 
#include <ctype.h> 

typedef struct {
    int width;
    int height;
    unsigned char *data;
} Image;

long long g_contador_global = 0;

int pixel_match(unsigned char px) {
    return px > 128; 
}

void versao_sequencial(Image *img) {
    long long local = 0;
    for (int y = 0; y < img->height; y++) {
        int off = y * img->width;
        for (int x = 0; x < img->width; x++) {
            if (pixel_match(img->data[off + x])) local++;
        }
    }
    g_contador_global = local;
}

double diff_time_ms(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 +
           (end.tv_usec - start.tv_usec) / 1000.0;
}

void pgm_skip_comments_and_whitespace(FILE *fp) {
    int ch;
    while ((ch = fgetc(fp)) != EOF) {
        if (isspace(ch)) { 
            continue;
        }
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
        fprintf(stderr, "Erro: Cabeçalho PGM inválido (dimensões ou max_val).\n");
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

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo.pgm>\n", argv[0]);
        return 1;
    }
    const char *nome_arquivo = argv[1];

    struct timeval t_io_start, t_io_end;
    gettimeofday(&t_io_start, NULL);
    
    Image *img = carregar_pgm_real(nome_arquivo); 
    
    gettimeofday(&t_io_end, NULL);
    double ms_io = diff_time_ms(t_io_start, t_io_end);

    if (img == NULL) {
        return 1;
    }

    g_contador_global = 0;

    printf("=== VERSAO: SEQUENCIAL (ARQUIVO REAL) ===\n");
    printf("Imagem %s (%dx%d)\n", nome_arquivo, img->width, img->height);
    printf("[SEQUENCIAL] Tempo de I/O (leitura): %.3f ms\n", ms_io);

    struct timeval t1, t2;
    gettimeofday(&t1, NULL);
    
    versao_sequencial(img); 
    
    gettimeofday(&t2, NULL);

    double ms_comp = diff_time_ms(t1, t2);
    printf("[SEQUENCIAL] Contador final = %lld\n", g_contador_global);
    printf("[SEQUENCIAL] Tempo de Computação: %.3f ms (%.6f s)\n", ms_comp, ms_comp/1000.0);

    free(img->data); 
    free(img);
    return 0;
}