//#include "statistics.h"
#include "iotime.h"

/**
 *  IO print functions
 * @file   ioprint.h
 * @author Ahmad Tarraf
 * @date   14.02.2022
 */

namespace ioprint
{

    void Summary(int, statistics, statistics, statistics, statistics, iotime);
    void Json(int,    statistics, statistics, statistics, statistics, iotime);
    void Jsonl(int,    statistics, statistics, statistics, statistics, iotime);
    void Binary(int,    statistics, statistics, statistics, statistics, iotime);
    std::string Format_Json(statistics, std::string, bool req = false, bool jsonl = false);
    
    template <class T>
    std::string Print_Series(T, int, double, int, std::string, std::string, bool);

    std::string Print_Series(collect*, std::string, int, double, int, std::string, std::string, bool);
    
}