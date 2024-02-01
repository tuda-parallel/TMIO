#include "iocollect.h"

collect::collect(void)
{
    data = 0;
    t_start = 0;
    t_end_act = 0;
    t_end_req = 0;
    T_sum = 0;
    T_avr = 0;
    B_sum = 0;
    B_avr = 0;
    n_op = 0;
}

double collect::get(std::string mode) const
{
    if (mode == "t_start")
        return t_start;
    else if (mode == "t_end_act")
        return t_end_act;
    else if (mode == "t_end_req")
        return t_end_req;
    else if (mode == "T_sum")
        return T_sum;
    else if (mode == "T_avr")
        return T_avr;
    else if (mode == "B_sum")
        return B_sum;
    else if (mode == "B_avr")
        return B_avr;
    else
        return 0;
}

void collect::set(std::string mode, double value)
{
    if (mode == "t_start")
        t_start = value;
    else if (mode == "t_end_act")
        t_end_act = value;
    else if (mode == "t_end_req")
        t_end_req = value;
    else if (mode == "T_sum")
        T_sum = value;
    else if (mode == "T_avr")
        T_avr = value;
    else if (mode == "B_sum")
        B_sum = value;
    else if (mode == "B_avr")
        B_avr = value;
    else
        printf("not supported assignment");
}


