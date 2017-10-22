/* Compatibility macros for gcc4.2/5

   GCC inverted the sense of inline vs static inline by default in gcc 5.

   Meanwhile, manylinux1 wheels have to be built using gcc4.2. Therefore we
   need to support both, until a future manylinux2.

*/
#if defined __GNUC__ && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 199901L)
#define EXTERN_INLINE inline
#else
#define EXTERN_INLINE extern inline
#endif


#ifdef _MSC_VER
#define __restrict__

/* Under MSVC, the C99 'inline' keyword is not available, use __inline instead.
 *
 * See https://stackoverflow.com/questions/24736304/unable-to-use-inline-in-declaration-get-error-c2054
 */
#define inline __inline
#endif
