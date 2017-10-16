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

