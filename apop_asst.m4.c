/** \file apop_asst.c  The odds and ends bin. 
Copyright (c) 2005--2007, 2010 by Ben Klemens.  Licensed under the modified GNU GPL v2; see COPYING and COPYING2.  */

#include "apop_internal.h"
#include <gsl/gsl_math.h>
#include <gsl/gsl_randist.h>
#include <regex.h>

//more efficient than xprintf, but a little less versatile.
static void apop_tack_on(char **in, char *addme){
    if (!addme) return;
    size_t inlen = *in? strlen(*in): 0;
    size_t total_len= inlen + strlen(addme);
    *in = realloc(*in, total_len+1);
    strcpy(*in+inlen, addme);
}

typedef int (*apop_fn_riip)(apop_data*, int, int, void*);

/** Join together a list or array of strings, with optional separators between the strings.

\param strings  An \ref apop_data set with a grid of text to be combined into a single string
\param between  The text to put in between the rows of the table, such as ", ". (Default is a single space: " ")
\param before   The text to put at the head of the string. For the query example, this would be <tt>.before="select "</tt>. (Default: NULL)
\param after   The text to put at the tail of the string. For the query example, <tt>.after=" from data_table"</tt>. (Default: NULL)
\param between_cols The text to insert between columns of text. See below for an example (Default is set to equal <tt>.between</tt>)
\param prune If you don't want to use the entire text set, you can provide a function to indicate which elements should be pruned out. Some examples:

\code
//Just use column 3
int is_not_col_3(apop_data *indata, int row, int col, void *ignore){
    return col!=3;
}

//Jump over blanks as if they don't exist.
int is_blank(apop_data *indata, int row, int col, void *ignore){
    return strlen(indata->text[row][col])==0;
}
\endcode

\param prune_parameter A void pointer to pass to your \c prune function.

\return A single string with the elements of the \c strings table joined as per your specification. Allocated by the function, to be freed by you if desired.

\li If the table of strings is \c NULL or has no text, I will print only the <tt>.before</tt> and <tt>.after</tt> parts with nothing in between.
\li if <tt> apop_opts.verbose >=3</tt>, then print the pasted text to stderr.
\li This function uses the \ref designated syntax for inputs.

The sample snippet generates the SQL for a query using a list of column names (where
the query begins with <tt>select </tt>, ends with <tt>from datatab</tt>, and has commas
in between each element), re-processes the same list to produce the head of an HTML
table, then produces the body of the table with the query result (pasting the
<tt>tr</tt>s and <tt> td</tt>s into the right places).

\include sql_to_html.c
*/
APOP_VAR_HEAD char *apop_text_paste(apop_data const *strings, char *between, char *before, char *after, char *between_cols, apop_fn_riip prune, void *prune_parameter){
    apop_data const *apop_varad_var(strings, NULL);
    char *apop_varad_var(between, " ");
    char *apop_varad_var(before, NULL);
    char *apop_varad_var(after, NULL);
    char *apop_varad_var(between_cols, between);
    apop_fn_riip apop_varad_var(prune, NULL);
    void *apop_varad_var(prune_parameter, NULL);
APOP_VAR_ENDHEAD
    char *prior_line=NULL, *oneline=NULL, *out = before ? strdup(before) : NULL;
    for (int i=0; i< strings->textsize[0]; i++){
        free(oneline); oneline = NULL;
        for (int j=0; j< strings->textsize[1]; j++){
            if (prune && !prune((apop_data*)strings, i, j, prune_parameter)) continue;
            apop_tack_on(&oneline, strings->text[i][j]);
            if (j <strings->textsize[1]-1)  apop_tack_on(&oneline, between_cols);
        }
        apop_tack_on(&out, prior_line);
        if (prior_line && oneline) apop_tack_on(&out, between);
        free(prior_line);
        prior_line=oneline ? strdup(oneline): NULL;
        //if (i <strings->textsize[0]-1)  apop_tack_on(&out, between);
        //if (oneline)  apop_tack_on(&out, oneline);
    }
    apop_tack_on(&out, oneline); //the final one never got a chance to be prior_line
    apop_tack_on(&out, after);
    Apop_notify(3, "%s", out);
    return out;
}

/** Calculate \f$\sum_{n=1}^N {1\over n^s}\f$

\li There are no doubt efficient shortcuts do doing this, but I use brute force. [Though Knuth's Art of Programming v1 doesn't offer anything, which is strong indication of nonexistence.] To speed things along, I save the results so that they can just be looked up should you request the same calculation. 

\li If \c N is zero or negative, return NaN. Notify the user if <tt>apop_opts.verbosity >=1</tt>

For example: 

\include test_harmonic.c
*/
double apop_generalized_harmonic(int N, double s){
/* 
Each row in the saved-results structure is an \f$s\f$, and each column is \f$1\dots n\f$, up to the largest \f$n\f$ calculated to date.

When reading the code, remember that the zeroth element holds the value for N=1, and so on.
*/
    Apop_stopif(N<=0, return GSL_NAN, 0, "N is %i, but must be greater than 0.", N);
    static double *  eses	= NULL;
    static int * 	 lengths= NULL;
    static int		 count	= 0;
    static double ** precalced=NULL;
    int			     j, old_len, i;
	for (i=0; i< count; i++)
		if (eses == NULL || eses[i] == s) 	
            break;
	if (i == count){	//you need to build the vector from scratch.
		count			++;
        i               = count - 1;
		precalced 		= realloc(precalced, sizeof (double*) * count);
		lengths 		= realloc(lengths, sizeof (int*) * count);
		eses 			= realloc(eses, sizeof (double) * count);
		precalced[i]	= malloc(sizeof(double) * N);
		lengths[i]	    = N;
		eses[i]		    = s;
		precalced[i][0]	= 1;
		old_len			= 1;
	}
	else {	//then you found it.
		old_len		= lengths[i];
	}
	if (N-1 >= old_len){	//It's there, but you need to extend what you have.
		precalced[i]	= realloc(precalced[i], sizeof(double) * N);
		for (j=old_len; j<N; j++)
			precalced[i][j] = precalced[i][j-1] + 1/pow((j+1),s);
	}
	return 	precalced[i][N-1];
}

/** Call \c system(), but with <tt>printf</tt>-style arguments. E.g.,
  
\code
char filenames[] = "apop_asst.c apop_asst.o"
apop_system("ls -l %s", filenames);
\endcode

\return The return value of the \c system() call.
 */
int apop_system(const char *fmt, ...){
    char *q;
    va_list argp;
	va_start(argp, fmt);
	vasprintf(&q, fmt, argp);
	va_end(argp);
    int out = system(q);
    free(q);
    return out;
}


/* \defgroup sorting Sorting functions

 A few functions to sort data. One sorts an \c apop_data set in place, and one returns percentiles for a sorted vector.
  \{ */

#include <gsl/gsl_sort_vector.h>

static int find_min_unsorted(size_t *sorted, size_t height, size_t min){
    while (min<height)
        if (!sorted[min])   return min;
        else                min++;
    return -1;
}

/** This function sorts the whole of a \c apop_data set based on one column. Sorts in place, with little additional memory used.

 Uses the \c gsl_sort_vector_index function internally, and that function just ignores NaNs; therefore this function just leaves NaNs exactly where they lay.

\param data    The input set to be modified. (No default. If \c NULL, return \c NULL and emit a warning if <tt>apop_opts.verbose >= 1</tt>.)
\param sortby  The column of data by which the sorting will take place. As usual, -1 indicates the vector element. (default: column zero of the matrix if there is a matrix; if there's a vector but no matrix, then -1).
\param asc   If 'd' or 'D', sort in descending order; else sort in ascending order. (Default: ascending)
\return A pointer to the data set, so you can do things like \c apop_data_show(apop_data_sort(d, -1)).

\li This function uses the \ref designated syntax for inputs.

The following example sorts the <tt>test_data2</tt> file (which you can copy from the tests/ directory of the Apophenia distribution) three different ways.

\include sorting.c
*/
APOP_VAR_HEAD apop_data * apop_data_sort(apop_data *data, int sortby, char asc){
    apop_data * apop_varad_var(data, NULL);
    Apop_stopif(!data, return NULL, 1, "You gave me NULL data to sort. Returning NULL");
    int apop_varad_var(sortby, 0);
    if (sortby==0 && !data->matrix && data->vector) //you meant sort the vector
        sortby = -1;
    char apop_varad_var(asc, 0);
APOP_VAR_ENDHEAD
    size_t height = (sortby==-1) ? data->vector->size: data->matrix->size1;
    size_t sorted[height];
    size_t i, *perm, start=0;
    gsl_permutation *p = gsl_permutation_alloc(height);
    memset(sorted, 0, sizeof(size_t)*height);
    if (sortby == -1)
        gsl_sort_vector_index (p, data->vector);
    else {
        APOP_COL(data, sortby, v);
        gsl_sort_vector_index (p, v);
    }
    perm = p->data;
    if (asc=='d' || asc=='D') //reverse the perm matrix.
        for (size_t j=0; j< height/2; j++){
            double t         = perm[j];
            perm[j]          = perm[height-1-j];
            perm[height-1-j] = t;
        }
    while (1){
        i     =
        start = find_min_unsorted(sorted, height, start);
        if (i==-1) 
            break;
        Apop_data_row(data, start, firstrow);
        apop_data *first_row_storage = apop_data_copy(firstrow);
        sorted[start]++;
        while (perm[i]!=start){
            //copy from perm[i] to i
            Apop_data_row(data, perm[i], onerow);
            apop_data_set_row(data, onerow, i);
            sorted[perm[i]]++;
            i = perm[i];
        }
        apop_data_set_row(data, first_row_storage, i);
        apop_data_free(first_row_storage);
    }
    gsl_permutation_free(p);
    return data;
}


/** Returns an array of size 101, where \c returned_vector[95] gives the value of the 95th percentile, for example. \c Returned_vector[100] is always the maximum value, and \c returned_vector[0] is always the min (regardless of rounding rule).

\param data	a \c gsl_vector of data. (No default, must not be \c NULL.)
\param rounding This will either be \c 'u', \c 'd', or \c 'a'. Unless your data is exactly a multiple of 101, some percentiles will be ambiguous. If \c 'u', then round up (use the next highest value); if \c 'd' (or anything else), round down to the next lowest value; if \c 'a', take the mean of the two nearest points. If \c 'u' or \c 'a', then you can say "5% or more  of the sample is below \c returned_vector[5]"; if \c 'd' or \c 'a', then you can say "5% or more of the sample is above returned_vector[5]".   (Default = \c 'd'.)

\li You may eventually want to \c free() the array returned by this function.
\li This function uses the \ref designated syntax for inputs.
*/ 
APOP_VAR_HEAD double * apop_vector_percentiles(gsl_vector *data, char rounding){
    gsl_vector *apop_varad_var(data, NULL);
    Apop_stopif(!data, return NULL, 0, "You gave me NULL data.");
    char apop_varad_var(rounding, 'd');
APOP_VAR_ENDHEAD
    gsl_vector *sorted	= gsl_vector_alloc(data->size);
    double     *pctiles = malloc(sizeof(double) * 101);
	gsl_vector_memcpy(sorted,data);
	gsl_sort_vector(sorted);
	for(int i=0; i<101; i++){
		int index = i*(data->size-1)/100.0;
		if (rounding == 'u' && index != i*(data->size-1)/100.0)
			index ++; //index was rounded down, but should be rounded up.
		if (rounding == 'a' && index != i*(data->size-1)/100.0)
            pctiles[i]	= (gsl_vector_get(sorted, index)+gsl_vector_get(sorted, index+1))/2.;
        else pctiles[i]	= gsl_vector_get(sorted, index);
	}
	gsl_vector_free(sorted);
	return pctiles;
}

/** \} */

static int count_parens(const char *string){
    int out = 0;
    int last_was_backslash = 0;
    for(const char *step =string; *step !='\0'; step++){
        if (*step == '\\' && !last_was_backslash){
            last_was_backslash = 1;
            continue;
        }
        if (*step == ')' && !last_was_backslash)
            out++;
        last_was_backslash = 0;
    }
    return out;
}

/** A convenience function for regular expression searching 

\li There are three common flavors of regular expression: Basic, Extended,
and Perl-compatible (BRE, ERE, PCRE). I use EREs, as per the specs of
your C library, which should match POSIX's ERE specification. 

For example, "p.val" will match "P value", "p.value", "p values" (and even "tempeval", so be
careful).

If you give a non-\c NULL address in which to place a table of paren-delimited substrings, I'll return them as a row in the text element of the returned \ref apop_data set. I'll return <em>all</em> the matches, filling the first row with substrings from the first application of your regex, then filling the next row with another set of matches (if any), and so on to the end of the string. Useful when parsing a list of items, for example.


\param string        The string to search (no default)
\param regex       The regular expression (no default)
\param substrings   Parens in the regex indicate that I should return matching substrings. Give me the _address_ of an \ref apop_data* set, and I will allocate and fill the text portion with matches. Default= \c NULL, meaning do not return substrings (even if parens exist in the regex). If no match, return an empty \ref apop_data set, so <tt>output->textsize[0]==0</tt>.
\param use_case         Should I be case sensitive, \c 'y' or \c 'n'? (default = \c 'n', which is not the POSIX default.)

\return         Count of matches found. 0 == no match. \c substrings may be allocated and filled if needed.
\ingroup names


\li If <tt>apop_opts.stop_on_warning='n'</tt> returns -1 on error (e.g., regex \c NULL or didn't compile).
\li If <tt>strings==NULL</tt>, I return 0---no match---and if \c substrings is provided, set it to \c NULL.

\li Here is the test function. Notice that the substring-pulling
function call passes \c &subs, not plain \c subs. Also, the non-match
has a zero-length blank in <tt>subs->text[0][1]</tt>.

\include test_regex.c

\li Each set of matches will be one row of the output data. E.g., given the regex <tt>([A-Za-z])([0-9])</tt>, the column zero of <tt>outdata</tt> will hold letters, and column one will hold numbers.
Use \ref apop_data_transpose to reverse this so that the letters are in <tt>outdata->text[0]</tt> and numbers in <tt>outdata->text[1]</tt>.
*/
APOP_VAR_HEAD int  apop_regex(const char *string, const char* regex, apop_data **substrings, const char use_case){
    const char * apop_varad_var(string, NULL);
    apop_data **apop_varad_var(substrings, NULL);
    if (!string) {
        if (substrings) *substrings=NULL;
        return 0;
    }
    const char * apop_varad_var(regex, NULL);
    Apop_stopif(!regex, return -1, 0, "You gave me a NULL regex.");
    const char apop_varad_var(use_case, 'n');
APOP_VAR_ENDHEAD
    regex_t re;
    int matchcount=count_parens(regex);
    int found, found_ct=0;
    regmatch_t result[matchcount];
    int compiled_ok = !regcomp(&re, regex, REG_EXTENDED 
                                            + (use_case=='y' ? 0 : REG_ICASE)
                                            + (substrings ? 0 : REG_NOSUB) );
    Apop_stopif(!compiled_ok, return -1, 0, "This regular expression didn't compile: \"%s\"", regex)

    int matchrow = 0;
    if (substrings) *substrings = apop_data_alloc();
    do {
        found_ct+=
        found    = !regexec(&re, string, matchcount+1, result, matchrow ? REG_NOTBOL : 0);
        if (substrings && found){
            *substrings = apop_text_alloc(*substrings, matchrow+1, matchcount);
            //match zero is the whole string; ignore.
            for (int i=0; i< matchcount; i++){
                if (result[i+1].rm_eo > 0){//GNU peculiarity: match-to-empty marked with -1.
                    int length_of_match = result[i+1].rm_eo - result[i+1].rm_so;
                    free((*substrings)->text[matchrow][i]);
                    (*substrings)->text[matchrow][i] = malloc(strlen(string)+1);
                    memcpy((*substrings)->text[matchrow][i], string + result[i+1].rm_so, length_of_match);
                    (*substrings)->text[matchrow][i][length_of_match] = '\0';
                } //else matches nothing; apop_text_alloc already made this cell this NULL.
            }
            string += result[0].rm_eo; //end of whole match;
            matchrow++;
        }
    } while (substrings && found && string[0]!='\0');
    regfree(&re);
    return found_ct;
}

/** RNG from a Generalized Hypergeometric type B3.

Devroye uses this as the base for many of his distribution-generators, including the Waring.

\li If one of the inputs is <=0, error. Returns \c GSL_NAN if the function doesn't stop.
*/  //Header in stats.h
double apop_rng_GHgB3(gsl_rng * r, double* a){
    Apop_stopif(!((a[0]>0) && (a[1] > 0) && (a[2] > 0)), return GSL_NAN, 0, "all inputs must be positive.");
    double aa = gsl_ran_gamma(r, a[0], 1),
		   b  = gsl_ran_gamma(r, a[1], 1),
		   c  = gsl_ran_gamma(r, a[2], 1);
    int	p = gsl_ran_poisson(r, aa*b/c);
	return p;
}

/** The Beta distribution is useful for modeling because it is bounded between zero and one, and can be either unimodal (if the variance is low) or bimodal (if the variance is high), and can have either a slant toward the bottom or top of the range (depending on the mean).

The distribution has two parameters, typically named \f$\alpha\f$ and \f$\beta\f$, which can be difficult to interpret. However, there is a one-to-one mapping between (alpha, beta) pairs and (mean, variance) pairs. Since we have good intuition about the meaning of means and variances, this function takes in a mean and variance, calculates alpha and beta behind the scenes, and returns a random draw from the appropriate Beta distribution.

\param m
The mean the Beta distribution should have. Notice that m
is in [0,1].

\param v
The variance which the Beta distribution should have. It is in (0, 1/12), where (1/12) is the variance of a Uniform(0,1) distribution. Funny things happen with variance near 1/12 and mean far from 1/2.

\return Returns an \c apop_beta model with its parameters appropriately set.
\exception out->error=='r' Range error: mean is not within [0, 1].
*/
apop_model *apop_beta_from_mean_var(double m, double v){
    Apop_stopif(m>=1|| m<=0, apop_model *out = apop_model_copy(apop_beta);
                        out->error='r'; return out,
                       0, "You asked for a beta distribution "
                        "with mean %g, but the mean of the beta will always "
                        "be strictly between zero and one.", m);
    double k     = (m * (1- m)/ v) -1;
    double alpha = m*k;
    double beta  = k * (1-m);
    return apop_model_set_parameters(apop_beta, alpha, beta);
}

/** Make a set of random draws from a model and write them to an \ref apop_data set.

\param model The model from which draws will be made. Must already be prepared and/or estimated.

\param count The number of draws to make. If \c draw_matrix is not \c NULL, then this is ignored and <tt>count=draw_matrix->matrix->size1</tt>. default=1000.

\param rng a \c gsl_rng, already allocated. default: see \ref autorng.

\param draws If not \c NULL, a pre-allocated data set whose \c matrix element will be filled with draws. 

\return An \ref apop_data set with the matrix filled with \c size draws. If <tt>draw_matrix!=NULL</tt>, then return a pointer to it.

\exception out->error=='m' Input model isn't good for making draws: it is \c NULL, or <tt>m->dsize=0</tt>.

\exception out->error=='s' You gave me a \c draws matrix, but its size is less than the size of a single draw from the data, <tt>model->dsize</tt>.

\li Prints a warning if you send in a non-<tt>NULL apop_data</tt> set, but its \c matrix element is \c NULL, when <tt>apop_opts.verbose>=1</tt>.

\li See also \ref apop_draw, which makes a single draw.
 */

APOP_VAR_HEAD apop_data *apop_model_draws(apop_model *model, int count, gsl_rng *rng, apop_data *draws){
    apop_model * apop_varad_var(model, NULL);
    Apop_stopif(!model, apop_return_data_error(n), 0, "Input model is NULL.");
    Apop_stopif(!model->dsize, apop_return_data_error(n), 0, "Input model has dsize==0.");
    apop_data * apop_varad_var(draws, NULL);
    int apop_varad_var(count, 1000);
    if (draws) {
        Apop_stopif(!draws->matrix, return draws, 1, "Input data set's matrix is NULL.");
        Apop_stopif((int)draws->matrix->size2 < model->dsize, draws->error='s'; return draws,
                1, "Input data set's matrix column count is less than model->dsize.");
        count = draws->matrix->size1;
    } else
        Apop_stopif(model->dsize<=0, apop_return_data_error(n), 0, "model->dsize<=0, so I don't know the size of matrix to allocate.");
    static gsl_rng *spare_rng;
    gsl_rng * apop_varad_var(rng, NULL);
    if (!rng && !spare_rng)
        spare_rng = apop_rng_alloc(++apop_opts.rng_seed);
    if (!rng)  rng = spare_rng;
APOP_VAR_ENDHEAD
    apop_data *out = draws ? draws : apop_data_alloc(count, model->dsize);
    for (int i=0; i< count; i++){
        Apop_data_row(out, i, onerow);
        apop_draw(onerow->matrix->data, rng, model);
    }
    return out;
}