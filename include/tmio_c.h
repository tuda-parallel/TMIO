#ifdef __cplusplus
extern "C" {
#endif

void iotrace_summary(void);
void init_prefetcher(double io_frequency, size_t max_cache_bytes, size_t max_file_bytes);
int Write_Checkpoint(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request, double durations_sec);
void set_io_freqency(double);

#ifdef __cplusplus
}
#endif