/* \file apop_multivariate_normal.c

   The multivariate Normal distribution.

  (c) 2007, Ben Klemens. Licensed under the GNU GPL v 2.
*/
 
#include <apop.h>

apop_model apop_multivariate_normal;

double apop_multinormal_ll_prob(const apop_data *x, apop_data *v, apop_params * m){
  double    determinant = 0;
  gsl_matrix* inverse   = NULL;
  int       i, dimensions  = x->matrix->size2;
  gsl_vector* x_minus_mu= gsl_vector_alloc(x->matrix->size2);
  double        ll      = 0;
    determinant = apop_det_and_inv(v->matrix, &inverse, 1,1);
    if (determinant == 0) {
        fprintf(stderr, "apop_multivariate_normal.p: the determinant of the given covariance is zero. Returning GSL_NEGINF.  \n"); 
        gsl_vector_free(x_minus_mu);
        return(GSL_NEGINF); //tell maximizers to look elsewhere.
    }
    for (i=0; i< x->matrix->size1; i++){
        APOP_ROW(x,i, vv);
        gsl_vector_memcpy(x_minus_mu, vv);
        gsl_vector_sub(x_minus_mu, v->vector);
       ll  += - apop_x_prime_sigma_x(x_minus_mu, inverse) / 2;
       ll  -= log(2 * M_PI)* dimensions/2. + .5 *  log(determinant);
    }
    gsl_matrix_free(inverse);
    gsl_vector_free(x_minus_mu);
    return ll;
}

double apop_multinormal_prob(const apop_data *x, apop_data *v, apop_params * m){
    return exp(apop_multinormal_ll_prob(x, v, m));
}

static apop_params * multivariate_normal_estimate(apop_data * data, apop_params *p){
    if (!p) p = apop_params_alloc(data, &apop_multivariate_normal, NULL, NULL);
  int   i;
    for (i=0; i< data->matrix->size2; i++){
        APOP_COL(data,i,v);
        gsl_vector_set(p->parameters->vector, i, apop_mean(v));
    }
    p->covariance         =  apop_data_covariance_matrix(data, 0);
    p->parameters->matrix =  p->covariance->matrix;
    /*if (est->ep.uses.log_likelihood)
        est->log_likelihood = normal_log_likelihood(est->parameters->vector, data, NULL);
    if (est->ep.uses.covariance)
        apop_numerical_covariance_matrix(apop_normal, est, data);*/
    return p;
}

/** The nice, easy method from Devroye, p 565 */
static void mvnrng(double *out, gsl_rng *r, apop_params *eps){
  apop_data *params = eps->parameters;
  int i, j;
  gsl_vector *v     = gsl_vector_alloc(params->vector->size);
  gsl_vector *dotted= gsl_vector_calloc(params->vector->size);
    for (i=0; i< params->vector->size; i++)
        gsl_vector_set(v, i, gsl_ran_gaussian(r, 1));
  gsl_matrix *copy  = apop_matrix_copy(params ->matrix);
        gsl_linalg_cholesky_decomp(copy); //returns upper and lower triangle; we want just one.
    for (i=0; i< copy->size1; i++)
        for (j=i+1; j< copy->size2; j++)
            gsl_matrix_set(copy, i, j, 0);
    gsl_blas_dgemv(CblasNoTrans, 1, copy, v, 0, dotted);
    for (i=0; i< params->vector->size; i++)
        out[i]  = gsl_vector_get(dotted, i) + gsl_vector_get(params->vector,i);
    gsl_vector_free(v);
    gsl_vector_free(dotted);
    gsl_matrix_free(copy);
}

/** This is the multivarate generalization of the Normal distribution.
  The probability/log_likelihood methods take in an \c apop_data set whose vector element is the vector of
  means, and whose matrix is the covariances; the estimate method
  returns parameters of that form.

  \todo This needs an RNG and other filling in.
  \ingroup models
 */
apop_model apop_multivariate_normal= {"Multivariate normal distribution", -1,-1,-1,
     .estimate = multivariate_normal_estimate, .p = apop_multinormal_prob, 
     .log_likelihood = apop_multinormal_ll_prob, .draw = mvnrng};