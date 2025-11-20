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
		: handle(initialize_handle(orig)),
		  ptr(orig)
	{
	}

	bool check_request(RequestIDType request) const
	{
		// TODO: find a better way to compare these, as request can change from outside
		if constexpr (std::is_same<Tag, MPI_Tag>::value)
		{
			return ((request != nullptr) && (ptr == request)) || (handle == *request);
		}
		else if constexpr (std::is_same<Tag, IOuring_Tag>::value || std::is_same<Tag, Libc_Tag>::value)
		{
			return ptr == request;
		}
		else
		{
			// Error for other types
			throw std::runtime_error("Unsupported Tag type for AsyncRequest comparison.");
		}
	}
	AsyncRequest(const AsyncRequest &) = default;
	AsyncRequest & operator=(const AsyncRequest &) = default;
	AsyncRequest(AsyncRequest &&other) noexcept = default;
	AsyncRequest & operator=(AsyncRequest &&) = default;

private:
	/**
	 * @brief A private static helper function to correctly initialize the 'handle' member.
	 * @details This function is called from the constructor's initializer list.
	 * It uses `if constexpr` to determine the correct value for 'handle' at compile time.
	 * @param orig The original request ID.
	 * @return The value to initialize 'handle' with.
	 */
	static RequestType initialize_handle(RequestIDType orig)
	{
		if constexpr (std::is_same<Tag, MPI_Tag>::value || std::is_same<Tag, Libc_Tag>::value)
		{
			// For pointer-based types, we dereference to get the value.
			if (orig == nullptr)
			{
				throw std::runtime_error("Cannot construct AsyncRequest from a null pointer for MPI or Libc tags.");
			}
			return *orig;
		}
		else if constexpr (std::is_same<Tag, IOuring_Tag>::value)
		{
			// For value-based types (like __u64), we use the value directly.
			return orig;
		}
		else
		{
			// This will cause a compile-time error if an unsupported tag is used.
			throw std::runtime_error("Unsupported Tag type for AsyncRequest construction.");
		}
	}
	RequestType handle;
	RequestIDType ptr;
};
#endif // IOANALYSIS_H