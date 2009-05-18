

#define M_BITS	8

#define is_good_m(x) ((x) < (1 << M_BITS))

#define best_m1(x)	(x)
#define best_m2(x)	(is_good_m(x) ? x : best_m1(x>>2))
#define best_m3(x)	(is_good_m(x) ? x : best_m2(x>>2))
#define best_m4(x)	(is_good_m(x) ? x : best_m3(x>>2))
#define best_m5(x)	(is_good_m(x) ? x : best_m4(x>>2))
#define best_m6(x)	(is_good_m(x) ? x : best_m5(x>>2))
#define best_m7(x)	(is_good_m(x) ? x : best_m6(x>>2))
#define best_m8(x)	(is_good_m(x) ? x : best_m7(x>>2))
#define best_m9(x)	(is_good_m(x) ? x : best_m8(x>>2))
#define best_m10(x)	(is_good_m(x) ? x : best_m9(x>>2))
#define best_m11(x)	(is_good_m(x) ? x : best_m10(x>>2))
#define best_m12(x)	(is_good_m(x) ? x : best_m11(x>>2))
#define best_m13(x)	(is_good_m(x) ? x : best_m12(x>>2))
#define best_m14(x)	(is_good_m(x) ? x : best_m13(x>>2))
#define best_mant(x)	(is_good_m(x) ? x : best_m14(x>>2))

#define best_e1(x)	(1)
#define best_e2(x)	(is_good_m(x) ?  2 : best_e1(x>>2))
#define best_e3(x)	(is_good_m(x) ?  3 : best_e2(x>>2))
#define best_e4(x)	(is_good_m(x) ?  4 : best_e3(x>>2))
#define best_e5(x)	(is_good_m(x) ?  5 : best_e4(x>>2))
#define best_e6(x)	(is_good_m(x) ?  6 : best_e5(x>>2))
#define best_e7(x)	(is_good_m(x) ?  7 : best_e6(x>>2))
#define best_e8(x)	(is_good_m(x) ?  8 : best_e7(x>>2))
#define best_e9(x)	(is_good_m(x) ?  9 : best_e8(x>>2))
#define best_e10(x)	(is_good_m(x) ? 10 : best_e9(x>>2))
#define best_e11(x)	(is_good_m(x) ? 11 : best_e10(x>>2))
#define best_e12(x)	(is_good_m(x) ? 12 : best_e11(x>>2))
#define best_e13(x)	(is_good_m(x) ? 13 : best_e12(x>>2))
#define best_e14(x)	(is_good_m(x) ? 14 : best_e13(x>>2))
#define best_exp(x)	(is_good_m(x) ? 15 : best_e14(x>>2))


#define l4_time(ms) {best_mant(ms), best_exp(ms)}


#define mus(x)		(x)
#define mills(x)	(1000*mus(x))
#define secs(x)		(1000*mills(x))
#define mins(x)		(60*secs(x))
#define hours(x)	(60*mins(x))

#include <stdio.h>

int main()
{

#if 0	/* very long timeouts , long long */
#define ms 68451041280LL
    struct {
	int m;
	int e;
    } to = l4_time(ms);
    

    printf("ms=%Ld, m=%d, e=%d   t=%Ld\n",
	   ms, to.m, to.e,
	   (((long long)to.m)* (1<<(2*(15-(long long)to.e)))));
#else

#define ms secs(1)+mills(250)
    struct {
	int m;
	int e;
    } to = l4_time(ms);
    

    printf("ms=%d, m=%d, e=%d   t=%d\n",
	   ms, to.m, to.e,
	   (to.m* (1<<(2*(15-to.e)))));

#endif

}
