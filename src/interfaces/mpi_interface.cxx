#include "tmio.h"
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

/**
 *  iotrace trace class
 * @file	 tmio.cpp
 * @brief Contains definitions of methods from the \e tmio.h class.
 * @author  Ahmad Tarraf
 * @date	05.08.2021
 */

#if ENABLE_MPI_TRACE == 1
//! -----------------------Set up variables ------------------------------
IOtraceMPI mpi_iotrace;
#endif

//! ----------------------- Init and Finilize ------------------------------

//**********************************************************************
//*							 1. MPI_Init
//**********************************************************************
int MPI_Init(int *argc, char ***argv)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int result = PMPI_Init(argc, argv);
	iotrace_init_helper();
	return result;
}

//**********************************************************************
//*							 2. MPI_Init_thread
//**********************************************************************
int MPI_Init_thread(int *argc, char ***argv, int required, int *provided)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int result = PMPI_Init_thread(argc, argv, required, provided);
	iotrace_init_helper();
	return result;
}
//**********************************************************************
//*							 3. MPI_Finalize
//**********************************************************************
int MPI_Finalize()
{
	Function_Debug(__PRETTY_FUNCTION__);
	iotrace_finalize_helper();
	return PMPI_Finalize();
}

//! ----------------------- Open and Close ------------------------------
#ifdef ENABLE_MPI_TRACE

//**********************************************************************
//*							 1. MPI_File_open
//**********************************************************************
int MPI_File_open(MPI_Comm comm, const char *filename, int amode, MPI_Info info, MPI_File *fh)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Open();
	return PMPI_File_open(comm, filename, amode, info, fh);
}

//**********************************************************************
//*							 2. MPI_File_close
//**********************************************************************
int MPI_File_close(MPI_File *fh)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Close();
	return PMPI_File_close(fh);
}

//! ----------------------- Async Write ------------------------------

//**********************************************************************
//*							 1. MPI_File_iwrite
//**********************************************************************
int MPI_File_iwrite(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Write_Async_Start(count, datatype, request);
	return PMPI_File_iwrite(fh, buf, count, datatype, request);
}

//**********************************************************************
//*							 2. MPI_File_iwrite_at
//**********************************************************************
int MPI_File_iwrite_at(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Write_Async_Start(count, datatype, request, offset);
	return PMPI_File_iwrite_at(fh, offset, buf, count, datatype, request);
}

//**********************************************************************
//*							 3. MPI_File_iwrite_all
//**********************************************************************
int MPI_File_iwrite_all(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Write_Async_Start(count / mpi_iotrace.Get_Relevant_Ranks(fh), datatype, request);
	return PMPI_File_iwrite_all(fh, buf, count, datatype, request);
}

//**********************************************************************
//*							 4. MPI_File_iwrite_at_all
//**********************************************************************
int MPI_File_iwrite_at_all(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Write_Async_Start(count / mpi_iotrace.Get_Relevant_Ranks(fh), datatype, request);
	return PMPI_File_iwrite_at_all(fh, offset, buf, count, datatype, request);
}

//**********************************************************************
//*							 5. MPI_File_iwrite_shared
//**********************************************************************
int MPI_File_iwrite_shared(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request)
{
	Function_Debug(__PRETTY_FUNCTION__);

	mpi_iotrace.Write_Async_Start(count, datatype, request);
	return PMPI_File_iwrite_shared(fh, buf, count, datatype, request);
}

//! ----------------------- Sync Write ------------------------------

//**********************************************************************
//*							 1. MPI_File_write
//**********************************************************************
int MPI_File_write(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Write_Sync_Start(count, datatype);
	int result = PMPI_File_write(fh, buf, count, datatype, status);
	mpi_iotrace.Write_Sync_End();
	return result;
}

//**********************************************************************
//*							 2. MPI_File_write_at
//**********************************************************************
int MPI_File_write_at(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Write_Sync_Start(count, datatype, offset);
	int result = PMPI_File_write_at(fh, offset, buf, count, datatype, status);
	mpi_iotrace.Write_Sync_End();
	return result;
}

//**********************************************************************
//*							 3. MPI_File_write_all
//**********************************************************************
int MPI_File_write_all(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Write_Sync_Start(count / mpi_iotrace.Get_Relevant_Ranks(fh), datatype);
	int result = PMPI_File_write_all(fh, buf, count, datatype, status);
	mpi_iotrace.Write_Sync_End();
	return result;
}

//**********************************************************************
//*							 4. MPI_File_write_at_all
//**********************************************************************
int MPI_File_write_at_all(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Write_Sync_Start(count / mpi_iotrace.Get_Relevant_Ranks(fh), datatype, offset);
	int result = PMPI_File_write_at_all(fh, offset, buf, count, datatype, status);
	mpi_iotrace.Write_Sync_End();
	return result;
}

//**********************************************************************
//*							 5. MPI_File_write_at
//**********************************************************************
int MPI_File_write_shared(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Write_Sync_Start(count, datatype);
	int result = PMPI_File_write_shared(fh, buf, count, datatype, status);
	mpi_iotrace.Write_Sync_End();
	return result;
}

//! ----------------------- Async Read ------------------------------

//**********************************************************************
//*							 1. MPI_File_iread
//**********************************************************************
int MPI_File_iread(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Request *request)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Read_Async_Start(count, datatype, request);
	return PMPI_File_iread(fh, buf, count, datatype, request);
}

//**********************************************************************
//*							 2. MPI_File_iread_at
//**********************************************************************
int MPI_File_iread_at(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Request *request)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Read_Async_Start(count, datatype, request);
	return PMPI_File_iread_at(fh, offset, buf, count, datatype, request);
}

//**********************************************************************
//*							 3. MPI_File_iread_all
//**********************************************************************
int MPI_File_iread_all(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Request *request)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Read_Async_Start(count / mpi_iotrace.Get_Relevant_Ranks(fh), datatype, request);
	return PMPI_File_iread_all(fh, buf, count, datatype, request);
}

//**********************************************************************
//*							 4. MPI_File_iread_at_all
//**********************************************************************
int MPI_File_iread_at_all(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Request *request)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Read_Async_Start(count / mpi_iotrace.Get_Relevant_Ranks(fh), datatype, request);
	return PMPI_File_iread_at_all(fh, offset, buf, count, datatype, request);
}

//**********************************************************************
//*							 2. MPI_File_iread_shared
//**********************************************************************
int MPI_File_iread_shared(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Request *request)
{
	Function_Debug(__PRETTY_FUNCTION__);

	mpi_iotrace.Read_Async_Start(count, datatype, request);
	return PMPI_File_iread_shared(fh, buf, count, datatype, request);
}

//! ----------------------- Sync Read ------------------------------

//**********************************************************************
//*							 1. MPI_File_read
//**********************************************************************
int MPI_File_read(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Read_Sync_Start(count, datatype);
	int result = PMPI_File_read(fh, buf, count, datatype, status);
	mpi_iotrace.Read_Sync_End();
	return result;
}

//**********************************************************************
//*							 2. MPI_File_read_at
//**********************************************************************
int MPI_File_read_at(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Read_Sync_Start(count, datatype, offset);
	int result = PMPI_File_read_at(fh, offset, buf, count, datatype, status);
	mpi_iotrace.Read_Sync_End();
	return result;
}

//**********************************************************************
//*							 3. MPI_File_read_all
//**********************************************************************
int MPI_File_read_all(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Read_Sync_Start(count / mpi_iotrace.Get_Relevant_Ranks(fh), datatype);
	int result = PMPI_File_read_all(fh, buf, count, datatype, status);
	mpi_iotrace.Read_Sync_End();
	return result;
}

//**********************************************************************
//*							 4. MPI_File_read_at_all
//**********************************************************************
int MPI_File_read_at_all(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Read_Sync_Start(count / mpi_iotrace.Get_Relevant_Ranks(fh), datatype, offset);
	int result = PMPI_File_read_at_all(fh, offset, buf, count, datatype, status);
	mpi_iotrace.Read_Sync_End();
	return result;
}

//**********************************************************************
//*							 5. MPI_File_read_shared
//**********************************************************************
int MPI_File_read_shared(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Read_Sync_Start(count, datatype);
	int result = PMPI_File_read_shared(fh, buf, count, datatype, status);
	mpi_iotrace.Read_Sync_End();
	return result;
}

//! ----------------------- Wait and Test ------------------------------

//**********************************************************************
//*							 1. MPI_Wait
//**********************************************************************
int MPI_Wait(MPI_Request *request, MPI_Status *status)
{
	Function_Debug(__PRETTY_FUNCTION__);
	mpi_iotrace.Write_Async_Required(request);
	mpi_iotrace.Read_Async_Required(request);
	int result = PMPI_Wait(request, status);
	mpi_iotrace.Write_Async_End(request);
	mpi_iotrace.Read_Async_End(request);
#if defined BW_LIMIT
	mpi_iotrace.Apply_Limit();
#elif defined CUSTOM_MPI
	mpi_iotrace.Replace_Test();
#endif
	return result;
}

//**********************************************************************
//*							 2. MPI_Waitall
//**********************************************************************
int MPI_Waitall(int count, MPI_Request requests[], MPI_Status statuses[])
{
	Function_Debug(__PRETTY_FUNCTION__);
	for (int i = 0; i < count; i++)
	{
		mpi_iotrace.Write_Async_Required(requests + i);
		mpi_iotrace.Read_Async_Required(requests + i);
	}
	int result = PMPI_Waitall(count, requests, statuses);
	for (int i = 0; i < count; i++)
	{
		mpi_iotrace.Write_Async_End(requests + i);
		mpi_iotrace.Read_Async_End(requests + i);
	}
#if defined BW_LIMIT
	mpi_iotrace.Apply_Limit();
#elif defined CUSTOM_MPI
	mpi_iotrace.Replace_Test();
#endif
	return result;
}

//**********************************************************************
//*							 3. MPI_Test
//**********************************************************************
int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status)
{
	Function_Debug(__PRETTY_FUNCTION__, *flag);
	int result = PMPI_Test(request, flag, status);
#if TEST == 1
	mpi_iotrace.Write_Async_End(request, *flag);
	mpi_iotrace.Read_Async_End(request, *flag);
#endif
	return result;
}

//**********************************************************************
//*							 4. MPI_Testall
//**********************************************************************
int MPI_Testall(int count, MPI_Request *requests, int *flag, MPI_Status *statuses)
{
	Function_Debug(__PRETTY_FUNCTION__);
	int result = PMPI_Testall(count, requests, flag, statuses);
#if TEST == 1
	for (int i = 0; i < count; i++)
	{
		mpi_iotrace.Write_Async_End(requests + i, *flag);
		mpi_iotrace.Read_Async_End(requests + i, *flag);
	}
#endif
	return result;
}

#endif // ENABLE_MPI_TRACE