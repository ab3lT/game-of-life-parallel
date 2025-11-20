/* **********************************************************
 * Pthreads Parallel Implementation: Conway's Game of Life
 * Task + Data Parallelism
 *
 * Author : Modified for parallel execution with Pthreads
 *************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>

#define MATCH(s) (!strcmp(argv[ac], (s)))

int MeshPlot(int t, int m, int n, char **mesh);

double real_rand();
int seed_rand(long sd);

static char **currWorld = NULL, **nextWorld = NULL, **tmesh = NULL;
static int maxiter = 200;
static int population[2] = {0, 0};

int nx = 100;
int ny = 100;

static int w_update = 0;
static int w_plot = 1;

double getTime();
extern FILE *gnu;

// Pthreads synchronization variables
pthread_mutex_t plot_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pop_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t plot_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t compute_cond = PTHREAD_COND_INITIALIZER;
pthread_barrier_t barrier;

static int plot_ready = 0;
static int computation_done = 0;
static int current_iteration = 0;
static int disable_display_global = 0;
static int s_step_global = 0;
static int numthreads_global = 1;

typedef struct
{
    int thread_id;
    int start_row;
    int end_row;
} thread_data_t;

void *plotter_thread(void *arg)
{
    for (int t = 1; t <= maxiter; t++)
    {
        pthread_mutex_lock(&plot_mutex);
        while (!plot_ready && !computation_done)
        {
            pthread_cond_wait(&plot_cond, &plot_mutex);
        }

        if (computation_done)
        {
            pthread_mutex_unlock(&plot_mutex);
            break;
        }

        MeshPlot(t - 1, nx, ny, currWorld);

        plot_ready = 0;
        pthread_cond_broadcast(&compute_cond);
        pthread_mutex_unlock(&plot_mutex);

        if (s_step_global)
        {
            printf("Finished with step %d\n", t - 1);
            printf("Press enter to continue.\n");
            getchar();
        }
    }

    return NULL;
}

void *compute_thread(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;
    int start_row = data->start_row;
    int end_row = data->end_row;

    for (int t = 0; t < maxiter && population[w_plot]; t++)
    {
        int local_pop = 0;

        // Compute assigned rows
        for (int i = start_row; i < end_row; i++)
        {
            for (int j = 1; j < ny - 1; j++)
            {
                int nn = currWorld[i + 1][j] + currWorld[i - 1][j] +
                         currWorld[i][j + 1] + currWorld[i][j - 1] +
                         currWorld[i + 1][j + 1] + currWorld[i - 1][j - 1] +
                         currWorld[i - 1][j + 1] + currWorld[i + 1][j - 1];

                nextWorld[i][j] = currWorld[i][j] ? (nn == 2 || nn == 3) : (nn == 3);
                local_pop += nextWorld[i][j];
            }
        }

        // Update global population count
        pthread_mutex_lock(&pop_mutex);
        population[w_update] += local_pop;
        pthread_mutex_unlock(&pop_mutex);

        // Wait for all compute threads to finish
        pthread_barrier_wait(&barrier);

        // Thread 1 performs the swap (master compute thread)
        if (data->thread_id == 1)
        {
            pthread_mutex_lock(&plot_mutex);

            tmesh = nextWorld;
            nextWorld = currWorld;
            currWorld = tmesh;

            int tmp = w_update;
            w_update = w_plot;
            w_plot = tmp;

            population[w_update] = 0;

            if (!disable_display_global)
            {
                plot_ready = 1;
                pthread_cond_signal(&plot_cond);

                // Wait for plotter to finish
                while (plot_ready && !computation_done)
                {
                    pthread_cond_wait(&compute_cond, &plot_mutex);
                }
            }

            pthread_mutex_unlock(&plot_mutex);
        }

        pthread_barrier_wait(&barrier);
    }

    // Signal completion
    if (data->thread_id == 1)
    {
        pthread_mutex_lock(&plot_mutex);
        computation_done = 1;
        pthread_cond_signal(&plot_cond);
        pthread_mutex_unlock(&plot_mutex);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    int i, j, ac;

    float prob = 0.5;
    long seedVal = 0;
    int game = 0;
    int s_step = 0;
    int numthreads = 1;
    int disable_display = 0;

    for (ac = 1; ac < argc; ac++)
    {
        if (MATCH("-n"))
        {
            nx = atoi(argv[++ac]);
        }
        else if (MATCH("-i"))
        {
            maxiter = atoi(argv[++ac]);
        }
        else if (MATCH("-t"))
        {
            numthreads = atoi(argv[++ac]);
        }
        else if (MATCH("-p"))
        {
            prob = atof(argv[++ac]);
        }
        else if (MATCH("-s"))
        {
            seedVal = atof(argv[++ac]);
        }
        else if (MATCH("-step"))
        {
            s_step = 1;
        }
        else if (MATCH("-d"))
        {
            disable_display = 1;
        }
        else if (MATCH("-g"))
        {
            game = atoi(argv[++ac]);
        }
        else
        {
            printf("Usage: %s [-n <meshpoints>] [-i <iterations>] [-s seed] [-p prob] [-t numthreads] [-step] [-g <game #>] [-d]\n", argv[0]);
            return (-1);
        }
    }

    disable_display_global = disable_display;
    s_step_global = s_step;
    numthreads_global = numthreads;

    printf("Number of threads: %d\n", numthreads);

    int rs = seed_rand(seedVal);

    nx = nx + 2;
    ny = nx;

    currWorld = (char **)malloc(sizeof(char *) * nx + sizeof(char) * nx * ny);
    for (i = 0; i < nx; i++)
        currWorld[i] = (char *)(currWorld + nx) + i * ny;

    nextWorld = (char **)malloc(sizeof(char *) * nx + sizeof(char) * nx * ny);
    for (i = 0; i < nx; i++)
        nextWorld[i] = (char *)(nextWorld + nx) + i * ny;

    for (i = 0; i < nx; i++)
    {
        currWorld[i][0] = 0;
        currWorld[i][ny - 1] = 0;
        nextWorld[i][0] = 0;
        nextWorld[i][ny - 1] = 0;
    }
    for (i = 0; i < ny; i++)
    {
        currWorld[0][i] = 0;
        currWorld[nx - 1][i] = 0;
        nextWorld[0][i] = 0;
        nextWorld[nx - 1][i] = 0;
    }

    if (game == 0)
    {
        for (i = 1; i < nx - 1; i++)
            for (j = 1; j < ny - 1; j++)
            {
                currWorld[i][j] = (real_rand() < prob);
                population[w_plot] += currWorld[i][j];
            }
    }
    else if (game == 1)
    {
        printf("2x2 Block, still life\n");
        int nx2 = nx / 2;
        int ny2 = ny / 2;
        currWorld[nx2 + 1][ny2 + 1] = currWorld[nx2][ny2 + 1] = currWorld[nx2 + 1][ny2] = currWorld[nx2][ny2] = 1;
        population[w_plot] = 4;
    }
    else if (game == 2)
    {
        printf("Glider (spaceship)\n");
        int nx2 = nx / 2;
        int ny2 = ny / 2;
        currWorld[nx2][ny2 + 1] = 1;
        currWorld[nx2 + 1][ny2 + 2] = 1;
        currWorld[nx2 + 2][ny2] = 1;
        currWorld[nx2 + 2][ny2 + 1] = 1;
        currWorld[nx2 + 2][ny2 + 2] = 1;
        population[w_plot] = 5;
    }
    else
    {
        printf("Unknown game %d\n", game);
        exit(-1);
    }

    printf("probability: %f\n", prob);
    printf("Random # generator seed: %d\n", rs);

    if (!disable_display)
        MeshPlot(0, nx, ny, currWorld);

    double t0 = getTime();

    // Initialize barrier for compute threads
    pthread_barrier_init(&barrier, NULL, numthreads - 1);

    // Create threads
    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * numthreads);
    thread_data_t *thread_data = (thread_data_t *)malloc(sizeof(thread_data_t) * numthreads);

    // Thread 0 is plotter
    if (!disable_display)
    {
        pthread_create(&threads[0], NULL, plotter_thread, NULL);
    }

    // Threads 1 to numthreads-1 are compute threads
    int rows_per_thread = (nx - 2) / (numthreads - 1);
    int remaining_rows = (nx - 2) % (numthreads - 1);

    int current_row = 1;
    for (int t = 1; t < numthreads; t++)
    {
        thread_data[t].thread_id = t;
        thread_data[t].start_row = current_row;
        thread_data[t].end_row = current_row + rows_per_thread + (t - 1 < remaining_rows ? 1 : 0);
        current_row = thread_data[t].end_row;

        pthread_create(&threads[t], NULL, compute_thread, &thread_data[t]);
    }

    // Join all threads
    for (int t = 0; t < numthreads; t++)
    {
        if (t == 0 && disable_display)
            continue;
        pthread_join(threads[t], NULL);
    }

    double t1 = getTime();
    printf("Running time for the iterations: %f sec.\n", t1 - t0);
    printf("Final population: %d\n", population[w_plot]);

    if (!disable_display)
    {
        printf("Press enter to end.\n");
        getchar();
    }

    if (gnu != NULL)
        pclose(gnu);

    // Cleanup
    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&plot_mutex);
    pthread_mutex_destroy(&pop_mutex);
    pthread_cond_destroy(&plot_cond);
    pthread_cond_destroy(&compute_cond);

    free(threads);
    free(thread_data);
    free(nextWorld);
    free(currWorld);

    return (0);
}