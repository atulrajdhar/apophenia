/** \file apop_smoothing.c	A few functions like moving averages.

 \author Ben Klemens
Copyright (c) 2007 by Ben Klemens.  Licensed under the modified GNU GPL v2; see COPYING and COPYING2.  */


#include "asst.h" //The headers are currently here.
#include "types.h"
#include "model/model.h"
#include "settings.h"
#include "conversions.h"

/** Return a new vector that is the moving average of the input vector.
 \param v The input vector, unsmoothed
 \param bandwidth The number of elements to be smoothed. 
 */
gsl_vector *apop_vector_moving_average(gsl_vector *v, size_t bandwidth){
  apop_assert(v,  NULL, 0, 'c', "You asked me to smooth a NULL vector; returning NULL.\n");
  apop_assert(bandwidth,  NULL, 0, 's', "Bandwidth must be >=1.\n");
  int   halfspan = bandwidth/2;
  int   i,j;
  gsl_vector *vout = gsl_vector_calloc(v->size - halfspan*2);
    for(i=0; i < vout->size; i ++){
        double *item = gsl_vector_ptr(vout, i);
        for (j=-halfspan; j < halfspan+1; j ++)
            *item += gsl_vector_get(v, j+ i+ halfspan);
        *item /= halfspan*2 +1;
    }
    return vout;
}

/** Return a new histogram that is the moving average of the input histogram.
 \param m A histogram, in \c apop_model form.
 \param bandwidth The number of elements to be smoothed. 
 */
apop_model *apop_histogram_moving_average(apop_model *m, size_t bandwidth){
  apop_assert(m && !strcmp(m->name, "Histogram"),  NULL, 0, 'c', "The first argument needs to be an apop_histogram model.\n");
  apop_assert(bandwidth,  NULL, 0, 's', "bandwidth must be >=1.\n");
  int  i;
  apop_model *out = apop_model_copy(*m);
  gsl_histogram *h     = Apop_settings_get(m, apop_histogram, pdf);
  gsl_histogram *hout  = Apop_settings_get(out, apop_histogram, pdf);
  gsl_vector *bins     = apop_array_to_vector(h->bin, h->n);
  gsl_vector *smoothed = apop_vector_moving_average(bins, bandwidth);
    for (i=0; i< h->n; i++)
        if (i < bandwidth/2 || i>= smoothed->size+bandwidth/2)
            hout->bin[i] = 0;
        else
            hout->bin[i] = gsl_vector_get(smoothed, i-bandwidth/2);
    gsl_vector_free(bins);
    gsl_vector_free(smoothed);
    return out;
}
