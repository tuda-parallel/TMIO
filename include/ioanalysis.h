#ifndef IOANALYSIS_H
#define IOANALYSIS_H
#include "iodata.h"
#include "iotrace_traits.h"
#include <stdexcept>
// #include <complex>
#if ENABLE_IOURING_TRACE == 1
#include <liburing.h>
#endif // ENABLE_IOURING_TRACE

/*!
 * @file ioanalysis.h
 * @brief Contains definitions of methods from the \e Ioanalysis namespace.
 * @details used for performaning analysis on the Input/Output collected
 * @author Ahmad Tarraf
 * @date 05.08.2021
 */

struct n_struct
{
	int aw, ar, sw, sr;
};

namespace ioanalysis
{

	collect *Gather_Collect(IOdata *, int *, int, int, MPI_Comm, bool finilize);
	n_struct *Gather_N_OP(n_struct, int, int, MPI_Comm);
	void Sum_N(n_struct *, n_struct &, int, int);
	int *Get_N_From_ALL_N(IOdata *, n_struct *, int, int);

}
template <typename Tag>
class AsyncRequest
{

public:
	using RequestType = typename IOtraceTraits<Tag>::RequestType;
	using RequestIDType = typename IOtraceTraits<Tag>::RequestIDType;
	explicit AsyncRequest(RequestIDType orig)
	{
		ptr = orig;
		// Only store the handle if it's MPI_Tag or Libc_Tag
		if constexpr (std::is_same<Tag, MPI_Tag>::value || std::is_same<Tag, Libc_Tag>::value)
		{
			handle = *orig;
		}
		else if constexpr (std::is_same<Tag, IOuring_Tag>::value)
		{
			handle = orig; // Default initialize for other types
		}
		else
		{
			// Error for other types
			throw std::runtime_error("Unsupported Tag type for AsyncRequest construction.");
		}
	}

	bool check_request(RequestIDType request)
	{
		// TODO: find a better way to compare these, as request can change from outside
		if constexpr (std::is_same<Tag, MPI_Tag>::value || std::is_same<Tag, Libc_Tag>::value)
		{
			return ((request != nullptr) && (ptr == request)) || (handle == *request);
		}
		else if constexpr (std::is_same<Tag, IOuring_Tag>::value)
		{
			return ptr == request;
		}
		else
		{
			// Error for other types
			throw std::runtime_error("Unsupported Tag type for AsyncRequest comparison.");
		}
	}

private:
	RequestType handle;
	RequestIDType ptr;
};
#endif // IOANALYSIS_H