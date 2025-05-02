#ifndef MPI_INTERFACE_H
#define MPI_INTERFACE_H


extern IOtrace mpi_iotrace; //allows to access iotrace in application code 

//! -----------------------
//! Traced MPI Function 
//! -----------------------

//! start and end routines 
int MPI_Init (int *, char ***);
int MPI_Init_thread( int *, char ***, int , int *);
int MPI_Finalize(void);

//! File modifications
int MPI_File_open(MPI_Comm comm, const char *filename, int amode,MPI_Info info, MPI_File *fh);
int MPI_File_close(MPI_File *fh);

//! Async write functions
int MPI_File_iwrite_at(MPI_File fh, MPI_Offset offset, const void *buf,int count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iwrite_all(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iwrite(MPI_File fh, const void *buf, int count,MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iwrite_at_all(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iwrite_shared(MPI_File fh, const void *buf, int count,  MPI_Datatype datatype,  MPI_Request *request);

//! read functions
int MPI_File_iread(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iread_at(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iread_all(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iread_at_all(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iread_shared(MPI_File fh, const void *buf, int count,  MPI_Datatype datatype,  MPI_Request *request);

//! Sync write functions
int MPI_File_write(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_write_at(MPI_File fh, MPI_Offset offset, const void *buf,int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_write_all(MPI_File fh, const void *buf, int count,MPI_Datatype datatype, MPI_Status *status);
int MPI_File_write_at_all(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status);

//! read functions
int MPI_File_read(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_read_at(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_read_all(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_read_at_all(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_read_shared(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);

//! monitoring functions
int MPI_Wait(MPI_Request *request, MPI_Status *status);
int MPI_Waitall(int count, MPI_Request requests[], MPI_Status statuses[]);
int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status);
int MPI_Testall(int count, MPI_Request* requests, int* flag, MPI_Status* statuses);

#endif // MPI_INTERFACE_H

