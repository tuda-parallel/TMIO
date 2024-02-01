#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>

#include "convergence.h"

#define MIN_VALUE 0.0001


void dc_init_context(double goal, dc_context_t *context)
{
    assert (context != NULL);

    context->max.value=0.0;
    context->max.result=0.0;
    context->max.size=0.0;
    context->min.value=0.0;
    context->min.result=0.0;
    context->min.size=0.0;
    context->last_value=0.0;
    context->goal=goal;
    context->max_extend=1.0;
    context->extend_rate=2.0;
    context->random_rate=2.0;
}

void dc_change_limits(double max_extend, double extend_rate, double random_rate, dc_context_t *context)
{
    assert (context != NULL);
    
    if (max_extend > 0.0) context->max_extend = max_extend;
    if (extend_rate > 0.0) context->extend_rate = extend_rate;
    if (random_rate > 0.0) context->random_rate = random_rate;
}

void dc_change_goal(double goal, dc_context_t *context)
{
    double theoryBW;
    double time_old, time_new, time_fix;
    
    
    assert (context != NULL);

    // get new max result with this goal
    
    // calculate fix time for old goal
    PRINTF ("dc_change_goal: OLD context->max.result = %f, context->max.value = %f\n",context->max.result, context->max.value);

    theoryBW = context->max.value * context->goal;
    time_old = ((double)(context->max.size))/theoryBW;
    time_new = ((double)(context->max.size))/(context->max.result);
    time_fix = time_new - time_old;
    //assert (time_fix >= 0.0);
    //printf ("convergence: time_fix=%.10f\n",time_fix);
    //if (time_fix < 0.0) time_fix = 0.0;
    
    // caculate new BW with the new goal and the same fix time
    theoryBW = context->max.value * goal;
    time_old = ((double)(context->max.size))/theoryBW;
    time_new = time_fix + time_old;
    context->max.result = ((double)(context->max.size))/time_new;
    PRINTF ("dc_change_goal: context->max.result = %f, context->max.value = %f\n",context->max.result, context->max.value);
    
    // get new min result with this goal
    
    // calculate fix time for old goal
    PRINTF ("dc_change_goal: OLD context->min.result = %f, context->min.value = %f\n",context->min.result, context->min.value);

    theoryBW = context->min.value * context->goal;
    time_old = ((double)(context->min.size))/theoryBW;
    time_new = ((double)(context->min.size))/(context->min.result);
    time_fix = time_new - time_old;
    //assert (time_fix >= 0.0);
    //printf ("convergence: time_fix=%.10f\n",time_fix);
    //7if (time_fix < 0.0) time_fix = 0.0;

    // caculate new BW with the new goal and the same fix time
    theoryBW = context->min.value * goal;
    time_old = ((double)(context->min.size))/theoryBW;
    time_new = time_fix + time_old;
    context->min.result = ((double)(context->min.size))/time_new;
    PRINTF ("dc_change_goal: context->min.result = %f, context->min.value = %f\n",context->min.result, context->min.value);

    // change goal
    context->goal=goal;
}

void dc_first_value(double value, double result, long int size, double *new_value, dc_context_t *context)
{
    assert ((context != NULL) && (new_value != NULL));

    (*new_value) = 1; //result / context->goal;

    context->max.value=value;
    context->max.result=result;
    context->max.size=size;
    context->min.value=value;
    context->min.result=result;
    context->min.size=size;
    context->last_value=(*new_value);
    PRINTF ("dc_first_value: new_value=%f\n", (*new_value));

}

double dc_next_value(double value, double result, long int size, dc_context_t *context)
{
    double rmax;
    double rmin;
    double rand_rate;

    assert ((context != NULL) && (context->last_value == value));

    // check value must be = last value
    PRINTF ("dc_next_value: context->last_value=%f, value=%f\n", context->last_value, value);
    
    // check where to set first and second value
    if (context->max.value <=  value) {
        PRINTF ("dc_next_value: TOP: change MAX value\n");
        context->max.value=value;
        context->max.result=result;
        context->max.size=size;
    } else if (context->min.value >=  value) {
        PRINTF ("dc_next_value: BOTTOM: change MIN value\n");
        context->min.value=value;
        context->min.result=result;
        context->min.size=size;
    } else if ((context->max.value >=  value) && (context->min.value <=  value)) {
        if (context->goal >= result) {
            PRINTF ("dc_next_value: MIDDLE: change MIN value\n");
            context->min.value=value;
            context->min.result=result;
            context->min.size=size;
        } else if (context->goal <= result) {
            PRINTF ("dc_next_value: MIDDLE: change MAX value\n");
            context->max.value=value;
            context->max.result=result;
            context->max.size=size;
        }
    } else {
        PRINTF ("dc_next_value: ERROR max and min values are not in order\n");
        assert (0);
    }
    
    PRINTF ("dc_next_value: context->max= %.50f/%.50f, context->min= %.50f/%.50f\n", context->max.value, context->max.result, context->min.value, context->min.result);

    // get random rate [0,1]
    rand_rate = ((double)rand())/((double)RAND_MAX);
    PRINTF ("dc_next_value: rand_rate:%.20lf\n",rand_rate);

    if (context->max.value == context->min.value) {
        PRINTF ("dc_next_value: context->max.value == context->min.value,\n");

    }
    double range_size = (context->max.value - context->min.value);
    double result_size = fabs(context->max.result - context->min.result);
    double rate_size = 0.0;
    if (result_size != 0) {
        rate_size = range_size / result_size;
    }
    double max_result = MAX(context->max.result,context->min.result);
    double min_result = MIN(context->max.result,context->min.result);
    if ( (context->goal-max_result) > result_size) {
        //range_size = 2 * rate_size * (context->goal - max_result);
        range_size = context->extend_rate * rate_size * (context->goal - max_result);
        if (range_size > context->max_extend) range_size = context->max_extend;
    } else if ( (min_result - context->goal) > result_size) {
        //range_size = 2 * rate_size * (min_result - context->goal);
        range_size = context->extend_rate * rate_size * (min_result - context->goal);
        if (range_size > context->max_extend) range_size = context->max_extend;
    }
    
    // case all results are lower than goal
    // get random value in the upper half of the
    // simetric range at the top of the current
    if (context->goal >= max_result)  {
        rmax = context->max.value + range_size;
        //rmin = context->max.value + range_size/2.0;
        rmin = context->max.value + range_size/context->random_rate;
        PRINTF ("dc_next_value: TOP RANGE: (rmax,rmin) = (%f,%f)\n", rmax, rmin);

    // case all results are bigger than goal
    // get random value in the lower half of the
    // simetric range at the bottom of the current
    } else if (context->goal <= min_result)  {
        rmin = context->min.value - range_size;
        //rmax = context->min.value - range_size/2.0;
        rmax = context->min.value - range_size/context->random_rate;
        PRINTF ("dc_next_value: BOTTOM RANGE: (rmax,rmin) = (%f,%f)\n", rmax, rmin);

    // case goal is between the range results
    // get random value in the middle range of the current
    } else if ( ((context->goal >= context->min.result) &&
                 (context->goal <= context->max.result)) ||
                ((context->goal <= context->min.result) &&
                 (context->goal >= context->max.result)) ) {
        //rmin = context->min.value + range_size/4.0;
        //rmax = context->max.value - range_size/4.0;
        rmin = context->min.value + ((range_size/context->random_rate)/2.0);
        rmax = context->max.value - ((range_size/context->random_rate)/2.0);
        PRINTF ("dc_next_value: MIDDLE RANGE: (rmax,rmin) = (%f,%f)\n", rmax, rmin);
    } else {
        PRINTF ("dc_next_value: ERROR ((context->goal <= context->min.result) &&                (context->goal >= context->max.result))\n");
        assert (0);
    }
    
    //compute and return next value
    context->last_value = (rmax * rand_rate) + (rmin *(1.0 - rand_rate));
    
    // AVOID GOING FURTHER THAN ZERO
    if (context->last_value < MIN_VALUE) {
        context->last_value = MIN_VALUE;
    }

    return context->last_value;
}
