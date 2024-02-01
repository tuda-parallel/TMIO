#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>

#ifndef _CONVERGENCE_INCLUDE_
#define _CONVERGENCE_INCLUDE_

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

//#define PRINTF(...) printf (__VA_ARGS__)
#define PRINTF(...)

struct dc_value {
    double value;
    double result;
    long int size;
};
typedef struct dc_value dc_value_t;

struct dc_context {
    dc_value_t max;
    dc_value_t min;
    double last_value;
    double goal;
    double max_extend;
    double extend_rate;
    double random_rate;
};

typedef struct dc_context dc_context_t;

void dc_init_context(double goal, dc_context_t *context);

void dc_change_limits(double max_extend, double extend_rate, double random_rate, dc_context_t *context);

void dc_change_goal(double goal, dc_context_t *context);

void dc_first_value(double value, double result, long int size, double *new_value, dc_context_t *context);

double dc_next_value(double value, double result, long int size, dc_context_t *context);

#endif /* _CONVERGENCE_INCLUDE_ */
