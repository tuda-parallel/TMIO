#include "iotrace.h"

//! ------------------------------ Async write tracing -------------------------------
//************************************************************************************
//*                               1. Write_Async_Start
//************************************************************************************
/**
 * @brief starts tracing the async write call. Takes timestamp of function call once entered.
 * @param count    [in] counting variable from write operations. number of variables of type datarype to write
 * @param datatype [in] data type of the variables to write
 * @param request  [in] write request
 * @param offset   [in,optional] offset of the I/O operation
 */
void IOtraceMPI::Write_Async_Start(int count, MPI_Datatype datatype, MPI_Request *request, MPI_Offset offset)
{
    double start_time = MPI_Wtime() - t_0;
    MPI_Type_size(datatype, &data_size_write);
    long long total_size = static_cast<long long>(count) * data_size_write;

    IOtraceMPI::LogWithAction<VerbosityLevel::BASIC_LOG>([&]()
                                                         {
        static long int counter = 1;
        IOtraceMPI::Log<VerbosityLevel::BASIC_LOG>(
            "%s > rank %i > #%li will asnyc write %i x %i = %lli bytes \n",
                    caller, rank, counter++, count, data_size_write, total_size); });

    Write_Async_Start_Impl(request, total_size, offset, start_time);
}

//************************************************************************************
//*                               2. Write_Async_End
//************************************************************************************
/**
 * @brief ends tracing the async write call. Takes timestamp of function call as endtime for the I/O operation.
 * @param request [in] request is compared to write request from Write_Async_Start
 * @param write_status [in,optinoal] if set to 1, indicates that the request is over. Used when tracing a MPI_Test call
 */
void IOtraceMPI::Write_Async_End(MPI_Request *request, int write_status)
{
    Write_Async_End_Impl(request, write_status);
}

//************************************************************************************
//*                               3. Write_Async_Required
//************************************************************************************
/**
 * @brief ends tracing the required async write call. Takes timestamp of function call as endtime for the I/O operation.
 * Once this function is called, the required Phase ends
 * @param request [in] request is compared to read request from Read_Async_Start
 */
void IOtraceMPI::Write_Async_Required(MPI_Request *request)
{
    Write_Async_Required_Impl(request);
}

//! ------------------------------ Async read tracing -------------------------------
//************************************************************************************
//*                               1. Read_Async_Start
//************************************************************************************
/**
 * @brief starts tracing the async read call. Takes timestamp of function call.
 * @param count    [in] counting variable from write operations. number of variables of type datarype to write
 * @param datatype [in] data type of the variables to write
 * @param request  [in] write request
 * @param offset   [in,optional] offset of the I/O operation
 */
void IOtraceMPI::Read_Async_Start(int count, MPI_Datatype datatype, MPI_Request *request, MPI_Offset offset)
{
    double start_time = MPI_Wtime() - t_0;
    MPI_Type_size(datatype, &data_size_read);
    long long total_size = static_cast<long long>(count) * data_size_read;

    IOtraceMPI::LogWithAction<VerbosityLevel::BASIC_LOG>([&]()
                                                         {
        static long int counter = 1;
        IOtraceMPI::Log<VerbosityLevel::BASIC_LOG>(
            "%s > rank %i > #%li will asnyc read %i x %i = %lli bytes \n",
                    caller, rank, counter++, count, data_size_read, async_read_size.back()); });

    Read_Async_Start_Impl(request, total_size, offset, start_time);
}

//************************************************************************************
//*                               2. Read_Async_End
//************************************************************************************
/*!
 * @brief ends tracing the async read call. Takes timestamp of function call as endtime for the I/O operation.
 * @param request [in] request is compared to read request from Read_Async_Start
 * @param read_status [in,optinoal] if set to 1, indicates that the request is over. Used when tracing a MPI_Test call
 */
void IOtraceMPI::Read_Async_End(MPI_Request *request, int read_status)
{
    Read_Async_End_Impl(request, read_status);
}

//************************************************************************************
//*                               3. Read_Async_Required
//************************************************************************************
/**
 * @brief ends tracing the required async read call. Takes timestamp of function call as endtime for the I/O operation.
 * Once this function is called, the required Phase ends
 * @param request [in] request is compared to read request from Read_Async_Start
 */
void IOtraceMPI::Read_Async_Required(MPI_Request *request)
{
    Read_Async_Required_Impl(request);
}

//! ------------------------------ Sync write tracing -------------------------------
//************************************************************************************
//*                               1. Write_Sync_Start
//************************************************************************************
/**
 * @brief starts tracing the sync write call. Takes timestamp of function call
 * @param count   : counting variable from write operations. number of variables of type datarype to write
 * @param datatype: data type of the variables to write
 * @param offset   [in,optional] offset of the I/O operation
 */
void IOtraceMPI::Write_Sync_Start(int count, MPI_Datatype datatype, MPI_Offset offset)
{
    // get write timestamp
    double start_time = MPI_Wtime() - t_0;
    t_sync_write_start = Overhead_Start(start_time);

    MPI_Type_size(datatype, &data_size_write);
    size_sync_write = static_cast<long long>(count) * data_size_write;
    Write_Sync_Start_Impl(size_sync_write, offset, start_time);
}

//************************************************************************************
//*                               2. Write_Sync_End
//************************************************************************************
/**
 * @brief ends tracing the write call. Takes timestamp of function call
 */
void IOtraceMPI::Write_Sync_End(void)
{
    Write_Sync_End_Impl();
}

//! ------------------------------ Sync read tracing -------------------------------
//************************************************************************************
//*                               1. Read_Sync_Start
//************************************************************************************
/**
 * @brief starts tracing the sync read call. Takes timestamp of function call
 * @param count    [in] counting variable from read operations. number of variables of type datarype to read
 * @param datatype [in] data type of the variables to read
 * @param offset   [in,optional] offset of the I/O operation
 */
void IOtraceMPI::Read_Sync_Start(int count, MPI_Datatype datatype, MPI_Offset offset)
{
    // get read timestamp
    double start_time = MPI_Wtime() - t_0;
    t_sync_read_start = Overhead_Start(start_time);
    
    MPI_Type_size(datatype, &data_size_read);
    size_sync_read = static_cast<long long>(count) * data_size_read;
    Read_Sync_Start_Impl(size_sync_read, offset, start_time);
}

//************************************************************************************
//*                               2. Read_Sync_End
//************************************************************************************
/**
 * @brief ends tracing the sync read call. Takes timestamp of function call
 */
void IOtraceMPI::Read_Sync_End(void)
{
    Read_Sync_End_Impl();
}
