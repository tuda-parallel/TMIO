#include "tmio.h" 
#include "tmio_c.h"

extern "C" {

void iotrace_summary(void){
	iotrace.Summary();
}
}