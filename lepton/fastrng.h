/* Fast RNGs
 *
 * The primary virtue of these generators is speed,
 * but they are also tried-and-true algorithms with
 * solid histories.
 *
 * Use these generators, when speed is paramount over
 * other considerations, such as period
 *
 * $Id$ */

#include <math.h>
#include <stdlib.h>

#ifndef _FASTRNG_H_
#define _FASTRNG_H_

#ifdef _MSC_VER
#define inline
#define __restrict__
#endif

/* Seed the random number generators and initialize tables for rand_norm */
void
rand_seed(unsigned long s);

/*
   Generate a 32-bit random number in with the interval [0,0xffffffff]
*/
inline unsigned long 
rand_int32(void);

/*
	Generate a random number with uniform distribution in the interval (0, 1.0]
*/
inline float
rand_uni(void);

/*
	Generate a random number with normal distribution

	mu is the mean and sigma is the std deviation
*/
inline float
rand_norm(const float mu, const float sigma);

/*
	Generate a random number with exponential distribution

	mu is the desired mean
*/
inline float
rand_expo(const float mu);

#endif
