#ifndef INDICE_H
#define INDICE_H

typedef struct {
    int id;
    long offset;
    int removido; // 0 = ativo, 1 = removido
} Indice;

#endif
