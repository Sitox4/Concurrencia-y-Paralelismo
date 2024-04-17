#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include "pthread.h"
#include "compress.h"
#include "chunk_archive.h"
#include "queue.h"
#include "options.h"

#define CHUNK_SIZE (1024*1024)
#define QUEUE_SIZE 20

#define COMPRESS 1
#define DECOMPRESS 0

struct datas {
    queue in;
    queue out;
};

// Worker para varios hilos
void *worker(void *args) {
    struct datas *thread_data = (struct datas *)args;
    
    // Procesa los chunks de la cola de entrada hasta que esté vacía
    while (q_elements(thread_data->in) > 0) {
        chunk ch = q_remove(thread_data->in);
        chunk res = zcompress(ch);
        free_chunk(ch);
        q_insert(thread_data->out, res);
    }

    pthread_exit(NULL);
}

// Compress file taking chunks of opt.size from the input file,
// inserting them into the in queue, running them using a worker,
// and sending the output from the out queue into the archive file
void comp(struct options opt) {
    int fd, chunks, i, offset;
    char comp_file[256];
    struct stat st;
    archive ar;
    queue in, out;
    pthread_t hilos[opt.num_threads];
    struct datas datas_array[opt.num_threads];

    if ((fd = open(opt.file, O_RDONLY)) == -1) {
        printf("Cannot open %s\n", opt.file);
        exit(0);
    }

    fstat(fd, &st);
    chunks = st.st_size / opt.size + (st.st_size % opt.size ? 1 : 0);

    if (opt.out_file) {
        strncpy(comp_file, opt.out_file, 255);
    } else {
        strncpy(comp_file, opt.file, 255);
        strncat(comp_file, ".ch", 255);
    }

    ar = create_archive_file(comp_file);

    in = q_create(opt.queue_size);
    out = q_create(opt.queue_size);

    // Leer el archivo de entrada y enviar los chunks a la cola de entrada
    for (i = 0; i < chunks; i++) {
        chunk ch = alloc_chunk(opt.size);
        offset = lseek(fd, 0, SEEK_CUR);
        ch->size = read(fd, ch->data, opt.size);
        ch->num = i;
        ch->offset = offset;
        q_insert(in, ch);
    }

    // Inicializar y ejecutar hilos de compresión
    for (i = 0; i < opt.num_threads; i++) {
        datas_array[i].in = in;
        datas_array[i].out = out;
        pthread_create(&hilos[i], NULL, worker, (void *)&datas_array[i]);
    }

    /*// compression of chunks from in to out
    worker(in, out);*/
    for (i = 0; i < opt.num_threads; i++) {
        pthread_join(hilos[i], NULL);
    }

    // send chunks to the output archive file
    for (i = 0; i < chunks; i++) {
        chunk ch = q_remove(out);
        add_chunk(ar, ch);
        free_chunk(ch);
    }

    close_archive_file(ar);
    close(fd);
    q_destroy(in);
    q_destroy(out);
}


// Decompress file taking chunks of size opt.size from the input file

void decomp(struct options opt) {
    int fd, i;
    char uncomp_file[256];
    archive ar;
    chunk ch, res;

    if((ar=open_archive_file(opt.file))==NULL) {
        printf("Cannot open archive file\n");
        exit(0);
    };

    if(opt.out_file) {
        strncpy(uncomp_file, opt.out_file, 255);
    } else {
        strncpy(uncomp_file, opt.file, strlen(opt.file) -3);
        uncomp_file[strlen(opt.file)-3] = '\0';
    }

    if((fd=open(uncomp_file, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))== -1) {
        printf("Cannot create %s: %s\n", uncomp_file, strerror(errno));
        exit(0);
    }

    for(i=0; i<chunks(ar); i++) {
        ch = get_chunk(ar, i);

        res = zdecompress(ch);
        free_chunk(ch);

        lseek(fd, res->offset, SEEK_SET);
        write(fd, res->data, res->size);
        free_chunk(res);
    }

    close_archive_file(ar);
    close(fd);
}

int main(int argc, char *argv[]) {
    struct options opt;

    opt.compress    = COMPRESS;
    opt.num_threads = 3;
    opt.size        = CHUNK_SIZE;
    opt.queue_size  = QUEUE_SIZE;
    opt.out_file    = NULL;

    read_options(argc, argv, &opt);

    if(opt.compress == COMPRESS) comp(opt);
    else decomp(opt);
}
