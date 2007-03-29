#ifndef __apop_mapply__
#define __apop_mapply__

#include <assert.h>
#include <pthread.h>
#include <gsl/gsl_matrix.h>

#undef __BEGIN_DECLS    /* extern "C" stuff cut 'n' pasted from the GSL. */
#undef __END_DECLS
#ifdef __cplusplus
# define __BEGIN_DECLS extern "C" {
# define __END_DECLS }
#else
# define __BEGIN_DECLS /* empty */
# define __END_DECLS /* empty */
#endif

__BEGIN_DECLS

gsl_vector *apop_matrix_map(gsl_matrix *m, double (*fn)(gsl_vector*));
gsl_vector *apop_vector_map(gsl_vector *v, double (*fn)(double));
void apop_matrix_apply(gsl_matrix *m, void (*fn)(gsl_vector*));
void apop_vector_apply(gsl_vector *v, void (*fn)(double*));

__END_DECLS
#endif