/* **********************************************************
 * OpenMP Parallel Implementation: Conway's Game of Life
 * Task + Data Parallelism
 *
 * Author : Modified for parallel execution with OpenMP
 *************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <omp.h>

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

// Shared variables for task parallelism
static int plot_ready = 0;
static int computation_done = 0;
static int current_iteration = 0;

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

    omp_set_num_threads(numthreads);
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

// Parallel region with task parallelism
#pragma omp parallel
    {
        int thread_id = omp_get_thread_num();

        if (thread_id == 0 && !disable_display)
        {
            // Plotter thread (Thread 0)
            for (int t = 1; t <= maxiter; t++)
            {
#pragma omp flush(plot_ready, computation_done)
                while (!plot_ready && !computation_done)
                {
#pragma omp flush(plot_ready, computation_done)
                }

                if (computation_done)
                    break;

                MeshPlot(t - 1, nx, ny, currWorld);

#pragma omp atomic write
                plot_ready = 0;
#pragma omp flush(plot_ready)

                if (s_step)
                {
                    printf("Finished with step %d\n", t - 1);
                    printf("Press enter to continue.\n");
                    getchar();
                }
            }
        }
        else
        {
            // Computation threads (Data parallelism)
            for (int t = 0; t < maxiter && population[w_plot]; t++)
            {
                population[w_update] = 0;

                // Data parallel computation with reduction
                int local_pop = 0;
#pragma omp for schedule(static) nowait
                for (int i = 1; i < nx - 1; i++)
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

#pragma omp atomic
                population[w_update] += local_pop;

#pragma omp barrier

// Single thread performs swap and updates shared state
#pragma omp single
                {
                    tmesh = nextWorld;
                    nextWorld = currWorld;
                    currWorld = tmesh;

                    int tmp = w_update;
                    w_update = w_plot;
                    w_plot = tmp;

                    if (!disable_display)
                    {
                        plot_ready = 1;
#pragma omp flush(plot_ready)
                    }
                }
            }

#pragma omp single
            {
                computation_done = 1;
#pragma omp flush(computation_done)
            }
        }
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

    free(nextWorld);
    free(currWorld);

    return (0);
}