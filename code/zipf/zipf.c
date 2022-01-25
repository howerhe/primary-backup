#include "zipf.h"
#include "c_avlt.h"

static struct drand48_data _zipf_drand48_data;
struct c_avlt _zipf_avlt;

void init_zipf(double alpha, int n)
{
	double c = 0;
	unsigned long i;
	double *sum_p;
      	double *pow_table;

	sum_p = calloc(n+1, sizeof(double));
	if (!sum_p) exit(EXIT_FAILURE);
	pow_table = calloc(n+1, sizeof(double));
	if (!pow_table) exit(EXIT_FAILURE);

	avlt_init(&_zipf_avlt);

	for (i=1; i<=n; i++) {
		pow_table[i] = pow((double) i, alpha);
		c = c + (1.0 / pow_table[i]); 
	}
	c = 1.0 / c;
	for(i=0; i<n; i++) {
		sum_p[i+1] = sum_p[i] + c / pow_table[i+1];
		avlt_insert(&_zipf_avlt, sum_p[i+1], (void *)i);
		//fprintf(stderr, "%f ", sum_p[i]);
	}
	free(pow_table);
	free(sum_p);
}

unsigned long zipf_blk(void)
{
	unsigned long block_num;
	double prob;       

	_avlt_node_t *node = NULL;

	assert(_zipf_avlt._size > 0);
	do {
		drand48_r(&_zipf_drand48_data, &prob);
	} while ((prob == 0) || (prob == 1));

	node = avlt_search_closest(&_zipf_avlt, prob);

	assert(node != NULL);

	if (node->_key < prob) block_num = (unsigned long) node->_value;
	else block_num = (unsigned long) node->_value + 1;

	return block_num;
}

void destroy_zipf()
{

}
