/** \file apop_linear_algebra.c	Assorted things to do with matrices,
such as take determinants or do singular value decompositions.



	Copyright 2005 by Ben Klemens. Licensed under the GNU GPL.
*/

/** \defgroup linear_algebra 	Singular value decompositions, determinants, et cetera.  

<b>common matrix tricks</b><br>
\li \ref apop_det_and_inv: Calculate the determinant, inverse, or both.
\li \ref apop_x_prime_sigma_x: A very common operation for statistics.
\li \ref apop_sv_decomposition: the singular value decomposition

<b>printing</b><br>
\ref apop_print: Some convenience functions to quickly dump
a matrix or vector to the screen: <tt>apop_print_matrix</tt>,
<tt>apop_print_matrix_int</tt>, <tt>apop_print_vector</tt>, and
<tt>apop_print_vector_int</tt>.

*/

/** \defgroup convenience_fns 	Things to make life easier with the GSL.
 */

/** \defgroup output		Printing to the screen or a text file

Most functions print only to the screen, but the \ref apop_print "matrix
and vector printing functions" will let you print to a text file as
well. The presumption is that statistic estimates are for your own
consumption, while you are printing a matrix for import into another program.

See \ref apop_name_print.
*/
/** \defgroup apop_print 	Asst printing functions		

Many have multiple aliases, because I could never remember which way to write them.
\ingroup output
*/

#include <gsl/gsl_blas.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>	//popen, I think.
#include <apophenia/linear_algebra.h> 
#include <apophenia/stats.h>
#include "math.h" //pow!
#include <apophenia/vasprintf.h>

/** Returns the variance/covariance matrix relating each column with each other.

\param in 	A data matrix: rows are observations, columns are variables.

\param normalize
1= subtract the mean from each column, thus changing the input data but speeding up the computation.<br>
0= don't modify the input data

\return Returns the variance/covariance matrix relating each column with each other. This function allocates the matrix for you.
\ingroup matrix_moments */
gsl_matrix *apop_covariance_matrix(gsl_matrix *in, int normalize){
gsl_matrix	*out;
int		i,j,k;
double		means[in->size2];
gsl_vector_view	v;
	if (normalize){
		out	= gsl_matrix_alloc(in->size2, in->size2);
		apop_normalize_matrix(in);
		gsl_blas_dgemm(CblasTrans,CblasNoTrans, 1, in, in, 0, out);
	}
	else{
		out	= gsl_matrix_calloc(in->size2, in->size2);
		for(i=0; i< in->size2; i++){
			v		= gsl_matrix_column(in, i);
			means[i]	= apop_mean(&(v.vector));
		}
		for(i=0; i< in->size2; i++)
			for(j=i; j< in->size2; j++){
				for(k=0; k< in->size1; k++)
					apop_matrix_increment(out, i, j, 
					   (gsl_matrix_get(in,k,i)-means[i])* (gsl_matrix_get(in,k,j)-means[j]));
				if (i != j)	//set the symmetric element.
					gsl_matrix_set(out, j, i, gsl_matrix_get(out, i, j));
			}
	}
	gsl_matrix_scale(out, 1.0/in->size2);
	return out;
}

/**
Calculate the determinant of a matrix, its inverse, or both. The \c in matrix is not destroyed in the process.

\param in
The matrix to be inverted/determined.

\param out 
If you want an inverse, this is where to place the matrix to be filled with the inverse. If <tt>calc_inv == 0</tt>, then use <tt>NULL</tt>. 

\param calc_det 
0: Do not calculate the determinant.

1: Do.

\param calc_inv
0: Do not calculate the inverse.

1: Do.

\return
If <tt>calc_det == 1</tt>, then return the determinant. Otherwise, just returns zero.
\ingroup linear_algebra
*/
double apop_det_and_inv(gsl_matrix *in, gsl_matrix **out, int calc_det, int calc_inv) {
int 		sign;
double 		the_determinant = 0;
	gsl_matrix *invert_me = gsl_matrix_alloc(in->size1, in->size1);
	gsl_permutation * perm = gsl_permutation_alloc(in->size1);
	invert_me = gsl_matrix_alloc(in->size1, in->size1);
	gsl_matrix_memcpy (invert_me, in);
	gsl_linalg_LU_decomp(invert_me, perm, &sign);
	if (calc_inv){
		*out	= gsl_matrix_alloc(in->size1, in->size1); //square.
		gsl_linalg_LU_invert(invert_me, perm, *out);
		}
	if (calc_det)
		the_determinant	= gsl_linalg_LU_det(invert_me, sign);
	gsl_matrix_free(invert_me);
	gsl_permutation_free(perm);
	return(the_determinant);
}

/** This comes up often enough that it deserves its own convenience function.
\param x	A vector.
\param sigma 	A symmetric matrix.
\return 	The scalar X'SX
\ingroup linear_algebra
*/
double apop_x_prime_sigma_x(gsl_vector *x, gsl_matrix *sigma){
gsl_vector * 	sigma_dot_x	= gsl_vector_calloc(x->size);
double		the_result;
	gsl_blas_dsymv(CblasUpper, 1, sigma, x, 0, sigma_dot_x); //sigma should be symmetric
	gsl_blas_ddot(x, sigma_dot_x, &the_result);
	gsl_vector_free(sigma_dot_x);
	return(the_result);
}

void apop_normalize_for_svd(gsl_matrix *in){
//Greene (2nd ed, p 271) recommends pre- and post-multiplying by sqrt(diag(X'X)) so that X'X = I.
gsl_vector_view	v;
gsl_vector	*diagonal = gsl_vector_alloc(in->size1);
int 		i;
	//Get the diagonal, take the square root
	v	= gsl_matrix_diagonal(in);
	gsl_vector_memcpy(diagonal, &(v.vector));
	for (i=0; i<diagonal->size; i++)
		gsl_vector_set(diagonal, i, pow(gsl_vector_get(diagonal,i), .5));
	//mulitply each row and column by the diagonal vector.
	for (i=0; i<diagonal->size; i++){
		v	= gsl_matrix_column(in, i);
		gsl_vector_mul(&(v.vector), diagonal);
		v	= gsl_matrix_row(in, i);
		gsl_vector_mul(&(v.vector), diagonal);
	}
	gsl_vector_free(diagonal);
}

/**
Singular value decomposition, aka principal component analysis, aka factor analysis.

\param data 
The input matrix.

\param dimensions_we_want 
The singular value decomposition will return this many of the eigenvectors with the largest eigenvalues.

\param pc_space 
This will be the principal component space. Each column of the returned matrix will be another eigenvector; the columns will be ordered by the eigenvalues. Input the address of an un-allocated {{{gsl_matrix}}}.

\param total_explained
This will return the largest eigenvalues, scaled by the total of all eigenvalues (including those that were thrown out). The sum of these returned values will give you the percentage of variance explained by the factor analysis.
\ingroup linear_algebra */
void apop_sv_decomposition(gsl_matrix *data, int dimensions_we_want, gsl_matrix ** pc_space, gsl_vector **total_explained) {
//Get X'X
gsl_matrix * 	eigenvectors 	= gsl_matrix_alloc(data->size2, data->size2);
gsl_vector * 	dummy_v 	= gsl_vector_alloc(data->size2);
gsl_vector * 	all_evalues 	= gsl_vector_alloc(data->size2);
gsl_matrix * 	square  	= gsl_matrix_calloc(data->size2, data->size2);
gsl_vector_view v;
int 		i;
double		eigentotals	= 0;
	*pc_space	= gsl_matrix_alloc(data->size2, dimensions_we_want);
	*total_explained= gsl_vector_alloc(dimensions_we_want);
	gsl_blas_dgemm(CblasTrans,CblasNoTrans, 1, data, data, 0, square);
	apop_normalize_for_svd(square);	
	gsl_linalg_SV_decomp(square, eigenvectors, all_evalues, dummy_v);
	for (i=0; i< all_evalues->size; i++)
		eigentotals	+= gsl_vector_get(all_evalues, i);
	for (i=0; i<dimensions_we_want; i++){
		v	= gsl_matrix_column(eigenvectors, i);
		gsl_matrix_set_col(*pc_space, i, &(v.vector));
		gsl_vector_set(*total_explained, i, gsl_vector_get(all_evalues, i)/eigentotals);
	}
	gsl_vector_free(dummy_v); 	gsl_vector_free(all_evalues);
	gsl_matrix_free(square); 	gsl_matrix_free(eigenvectors);
}


/** Just add <tt>amt</tt> to a \c gsl_vector element. Equivalent to <tt>gsl_vector_set(gsl_vector_get(v, i) + amt, i)</tt>, but more readable (and potentially faster).

\param v The \c gsl_vector in question
\param i The location in the vector to be incremented.
\param amt The amount by which to increment. Of course, one can decrement by specifying a negative amt.
\ingroup convenience_fns
 */
inline void apop_vector_increment(gsl_vector * v, int i, double amt){
	v->data[i * v->stride]	+= amt;
}

/** Just add <tt>amt</tt> to a \c gsl_matrix element. Equivalent to <tt>gsl_matrix_set(gsl_matrix_get(m, i, j) + amt, i, j)</tt>, but more readable (and potentially faster).

\param m The \c gsl_matrix in question
\param i The row of the element to be incremented.
\param j The column of the element to be incremented.
\param amt The amount by which to increment. Of course, one can decrement by specifying a negative amt.
\ingroup convenience_fns
 */
inline void apop_matrix_increment(gsl_matrix * m, int i, int j, double amt){
	m->data[i * m->tda +j]	+= amt;
}


/** Put the first matrix either on top of or to the right of the second matrix.
  The fn returns a new matrix, meaning that at the end of this function, until you gsl_matrix_free() the original matrices, you will be taking up twice as much memory. Plan accordingly.

\param  m1  the upper/rightmost matrix
\param  m2  the second matrix
\param  posn    if 't', stack along first dimension, else, e.g. 'r' stack along second
\return     a new matrix with the stacked data.
\ingroup linear_algebra

For example, here is a little function to merge four matrices into a single two-part-by-two-part matrix:
\code
gsl_matrix *apop_stack_two_by_two(gsl_matrix *ul, gsl_matrix *ur, gsl_matrix *dl, gsl_matrix *dr){
gsl_matrix  *t1, *t2, *output;
    t1   = apop_matrix_stack(ul, ur, 'r');
    gsl_matrix_free(ul);
    gsl_matrix_free(ur);
    t2   = apop_matrix_stack(dl, dr, 'r');
    gsl_matrix_free(dl);
    gsl_matrix_free(dr);
    output  = apop_matrix_stack(t1, t2, 't');
    gsl_matrix_free(t1);
    gsl_matrix_free(t2);
    return output;
}
\endcode
*/
gsl_matrix *apop_matrix_stack(gsl_matrix *m1, gsl_matrix * m2, char posn){
gsl_matrix *out;
gsl_vector *tmp_vector;
int         i;
    if (posn == 't'){
        if (m1->size2 != m2->size2){
            printf("When stacking matrices on top of each other, they have to have the same number of columns (m1->size2==m2->size2). Returning NULL.\n");
            return NULL;
        }
        out         = gsl_matrix_alloc(m1->size1 + m2->size1, m1->size2);
        tmp_vector  = gsl_vector_alloc(m1->size2);
        for (i=0; i< m1->size1; i++){
            gsl_matrix_get_row(tmp_vector, m1, i);
            gsl_matrix_set_row(out, i, tmp_vector);
        }
        for ( ; i< m1->size1+m2->size1; i++){   //i is not reinitialized.
            gsl_matrix_get_row(tmp_vector, m2, i- m1->size1);
            gsl_matrix_set_row(out, i, tmp_vector);
        }
        gsl_vector_free(tmp_vector);
        return out;
    } else {
        if (m1->size1 != m2->size1){
            printf("When stacking matrices side by side, they have to have the same number of rows (m1->size1==m2->size1). Returning NULL.\n");
            return NULL;
        }
        out         = gsl_matrix_alloc(m1->size1, m1->size2 + m2->size2);
        tmp_vector  = gsl_vector_alloc(m1->size1);
        for (i=0; i< m1->size2; i++){
            gsl_matrix_get_col(tmp_vector, m1, i);
            gsl_matrix_set_col(out, i, tmp_vector);
        }
        for ( ; i< m1->size2+m2->size2; i++){   //i is not reinitialized.
            gsl_matrix_get_col(tmp_vector, m2, i- m1->size2);
            gsl_matrix_set_col(out, i, tmp_vector);
        }
        gsl_vector_free(tmp_vector);
        return out;
    } 
}

/** Delete columns from a matrix. 

  This is done via copying, so if you have an exceptionally large
  data set, you're better off producing the matrix in the perfect form
  directly.

\param in   the \c gsl_matrix to be subsetted
\param  use an array of ints. If use[7]==0, then column seven will be cut from the output. A reminder: <tt>memset(use, 1, in->size2 * sizeof(int))</tt> will quickly fill an array of ints with nonzero values; you can switch the 1 to a 0 to fill with zeros, or just use calloc(in->size2 * sizeof(int)) to fill it with zeros on allocation.
\return     a \c gsl_matrix with the specified columns removed.
*/
gsl_matrix *apop_matrix_rm_columns(gsl_matrix *in, int *use){
gsl_matrix      *out;
int             i, 
                ct  = 0, 
                j   = 0;
gsl_vector_view v;
    for (i=0; i < in->size2; i++)
        if (use[i])
            ct++;
    out = gsl_matrix_alloc(in->size1, ct);
    for (i=0; i < in->size2; i++){
        if (use[i]){
            v   = gsl_matrix_column(in, i);
            gsl_matrix_set_col(in, j, &(v.vector));
            j   ++;
        }
    }
    return out;
}
