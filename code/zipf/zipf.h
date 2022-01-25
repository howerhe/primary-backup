#ifndef _ZIPF_H_
#define _ZIPF_H_

#include <assert.h>
#include <math.h>  
#include <stdio.h> 
#include <stdlib.h>
#include <time.h>

void init_zipf(double alpha, int n);
unsigned long zipf_blk();
void destroy_zipf();

#endif // end of #ifndef _ZIPF_H_
