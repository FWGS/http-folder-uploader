
unsigned int __udivmodsi4(unsigned int divident, unsigned int divisor, unsigned int *remainder);

// c-based 64 bit dividion wrap (unused)
#if 0
#define du_int unsigned long long int
#define su_int unsigned int
#define di_int long long int
typedef union 
{	unsigned long long all;
	struct{
		unsigned int low;
		int high;
	}s;
} udwords;


du_int __attribute((optimize(0)))
__udivmoddi4(du_int a, du_int b, du_int* rem)
{
    const unsigned n_uword_bits = sizeof(su_int) * CHAR_BIT;
    const unsigned n_udword_bits = sizeof(du_int) * CHAR_BIT;
    udwords n;
    n.all = a;
    udwords d;
    d.all = b;
    udwords q;
    udwords r;	
    unsigned sr;
    /* special cases, X is unknown, K != 0 */
    if (n.s.high == 0)
    {
        if (d.s.high == 0)
        {
            /* 0 X
             * ---
             * 0 X
             */
//            if (rem)
  //              *rem = n.s.low % d.s.low;
            return __udivmodsi4(n.s.low,d.s.low,rem);//((unsigned long long)n.s.low) / ((unsigned long long)d.s.low);
        }
        /* 0 X
         * ---
         * K X
         */
        if (rem)
            *rem = n.s.low;
        return 0;
    }
    /* n.s.high != 0 */
    if (d.s.low == 0)
    {
        if (d.s.high == 0)
        {
            /* K X
             * ---
             * 0 0
             */ 
            //if (rem)
//                *rem = n.s.high % d.s.low;
            //return (unsigned long)n.s.high / (unsigned long)d.s.low;
            return __udivmodsi4(n.s.high,d.s.low,rem);
            
        }
        /* d.s.high != 0 */
        if (n.s.low == 0)
        {
            /* K 0
             * ---
             * K 0
             */
            return  __udivmodsi4(n.s.high,d.s.high,rem);
            /*if (rem)
            {
                r.s.high = n.s.high % d.s.high;
                r.s.low = 0;
                *rem = r.all;
            }
            return n.s.high / d.s.high;*/
        }
        /* K K
         * ---
         * K 0
         */
        if ((d.s.high & (d.s.high - 1)) == 0)     /* if d is a power of 2 */
        {
            if (rem)
            {
                r.s.low = n.s.low;
                r.s.high = n.s.high & (d.s.high - 1);
                *rem = r.all;
            }
            return n.s.high >> __builtin_ctz(d.s.high);
        }
        /* K K
         * ---
         * K 0
         */
        sr = __builtin_clz(d.s.high) - __builtin_clz(n.s.high);
        /* 0 <= sr <= n_uword_bits - 2 or sr large */
        if (sr > n_uword_bits - 2)
        {
           if (rem)
                *rem = n.all;
            return 0;
        }
        ++sr;
        /* 1 <= sr <= n_uword_bits - 1 */
        /* q.all = n.all << (n_udword_bits - sr); */
        q.s.low = 0;
        q.s.high = n.s.low << (n_uword_bits - sr);
        /* r.all = n.all >> sr; */
        r.s.high = n.s.high >> sr;
        r.s.low = (n.s.high << (n_uword_bits - sr)) | (n.s.low >> sr);
    }
    else  /* d.s.low != 0 */
    {
        if (d.s.high == 0)
        {
            /* K X
             * ---
             * 0 K
             */
            if ((d.s.low & (d.s.low - 1)) == 0)     /* if d is a power of 2 */
            {
                if (rem)
                    *rem = n.s.low & (d.s.low - 1);
                if (d.s.low == 1)
                    return n.all;
                sr = __builtin_ctz(d.s.low);
                q.s.high = n.s.high >> sr;
                q.s.low = (n.s.high << (n_uword_bits - sr)) | (n.s.low >> sr);
                return q.all;
            }
            /* K X
             * ---
             * 0 K
             */
            sr = 1 + n_uword_bits + __builtin_clz(d.s.low) - __builtin_clz(n.s.high);
            /* 2 <= sr <= n_udword_bits - 1
             * q.all = n.all << (n_udword_bits - sr);
             * r.all = n.all >> sr;
             */
            if (sr == n_uword_bits)
            {
                q.s.low = 0;
                q.s.high = n.s.low;
                r.s.high = 0;
                r.s.low = n.s.high;
            }
            else if (sr < n_uword_bits)  // 2 <= sr <= n_uword_bits - 1
            {
                q.s.low = 0;
                q.s.high = n.s.low << (n_uword_bits - sr);
                r.s.high = n.s.high >> sr;
                r.s.low = (n.s.high << (n_uword_bits - sr)) | (n.s.low >> sr);
            }
            else              // n_uword_bits + 1 <= sr <= n_udword_bits - 1
            {
                q.s.low = n.s.low << (n_udword_bits - sr);
                q.s.high = (n.s.high << (n_udword_bits - sr)) |
                           (n.s.low >> (sr - n_uword_bits));
                r.s.high = 0;
                r.s.low = n.s.high >> (sr - n_uword_bits);
            }
        }
        else
        {
            /* K X
             * ---
             * K K
             */
            sr = __builtin_clz(d.s.high) - __builtin_clz(n.s.high);
            /* 0 <= sr <= n_uword_bits - 1 or sr large */
            if (sr > n_uword_bits - 1)
            {
                if (rem)
                    *rem = n.all;
                return 0;
            }
            ++sr;
            /* 1 <= sr <= n_uword_bits */
            /*  q.all = n.all << (n_udword_bits - sr); */
            q.s.low = 0;
            if (sr == n_uword_bits)
            {
                q.s.high = n.s.low;
                r.s.high = 0;
                r.s.low = n.s.high;
            }
            else
            {
                q.s.high = n.s.low << (n_uword_bits - sr);
                r.s.high = n.s.high >> sr;
                r.s.low = (n.s.high << (n_uword_bits - sr)) | (n.s.low >> sr);
            }
        }
    }
    /* Not a special case
     * q and r are initialized with:
     * q.all = n.all << (n_udword_bits - sr);
     * r.all = n.all >> sr;
     * 1 <= sr <= n_udword_bits - 1
     */
    su_int carry = 0;
    for (; sr > 0; --sr)
    {
        /* r:q = ((r:q)  << 1) | carry */
        r.s.high = (r.s.high << 1) | (r.s.low  >> (n_uword_bits - 1));
        r.s.low  = (r.s.low  << 1) | (q.s.high >> (n_uword_bits - 1));
        q.s.high = (q.s.high << 1) | (q.s.low  >> (n_uword_bits - 1));
        q.s.low  = (q.s.low  << 1) | carry;
        /* carry = 0;
         * if (r.all >= d.all)
         * {
         *      r.all -= d.all;
         *      carry = 1;
         * }
         */
        const di_int s = (di_int)(d.all - r.all - 1) >> (n_udword_bits - 1);
        carry = s & 1;
        r.all -= d.all & s;
    }
    q.all = (q.all << 1) | carry;
    if (rem)
        *rem = r.all;
    return q.all;
}
#endif
#if 0
asm(R"(
	.global __aeabi_uldivmod
		__aeabi_uldivmod:
        push	{r11, lr}
        sub	sp, sp, #16
        add	r12, sp, #8
        str	r12, [sp]
        bl	__udivmoddi4
        ldr	r2, [sp, #8]
        ldr	r3, [sp, #12]
        add	sp, sp, #16
        pop	{r11, pc}
        )");
#endif

#ifndef __clang__
asm(R"(
    .global __aeabi_uidivmod
	__aeabi_uidivmod:
        push    { lr }
        sub     sp, sp, #4
        mov     r2, sp
        bl      __udivmodsi4
        ldr     r1, [sp]
        add     sp, sp, #4
        pop     { pc }
    .global __aeabi_uidivmod
	__aeabi_idivmod:
        push    { lr }
        sub     sp, sp, #4
        mov     r2, sp
        bl      __udivmodsi4
        ldr     r1, [sp]
        add     sp, sp, #4
        pop     { pc }
        )");
#endif

// arm long division
#if !defined __thumb__ && !defined __clang__
asm(R"(
	
	.global __udivmodsi4
	__udivmodsi4:
	str	r4, [sp, #-8]!

	mov	r4, r0
	adr	ip, div0block

	lsr	r3, r4, #16
	cmp	r3, r1
	movhs	r4, r3
	subhs	ip, ip, #(16 * 12)

	lsr	r3, r4, #8
	cmp	r3, r1
	movhs	r4, r3
	subhs	ip, ip, #(8 * 12)

	lsr	r3, r4, #4
	cmp	r3, r1
	movhs	r4, r3
	subhs	ip, #(4 * 12)

	lsr	r3, r4, #2
	cmp	r3, r1
	movhs	r4, r3
	subhs	ip, ip, #(2 * 12)

	/* Last block, no need to update r3 or r4. */
	cmp	r1, r4, lsr #1
	subls	ip, ip, #(1 * 12)

	ldr	r4, [sp], #8	/* restore r4, we are done with it. */
	mov	r3, #0

	bx ip
	)"

#define	IMM	#

#define block(shift) "cmp	r0, r1, lsl #"#shift "\n\t" \
	"addhs	r3, r3, #(1 <<" #shift ")\n\t"\
	"subhs	r0, r0, r1, lsl #" # shift "\n\t"

	block(31)
	block(30)
	block(29)
	block(28)
	block(27)
	block(26)
	block(25)
	block(24)
	block(23)
	block(22)
	block(21)
	block(20)
	block(19)
	block(18)
	block(17)
	block(16)
	block(15)
	block(14)
	block(13)
	block(12)
	block(11)
	block(10)
	block(9)
	block(8)
	block(7)
	block(6)
	block(5)
	block(4)
	block(3)
	block(2)
	block(1)
"div0block:"
	block(0)
	R"(
	str	r0, [r2]
	mov	r0, r3
	bx lr

quotient0:
	str	r0, [r2]
	mov	r0, #0
	bx lr

divby1:
	mov	r3, #0
	str	r3, [r2]
	bx lr

divby0:
	mov	r0, #0
	bx lr

)");
#endif

// gcc generates compressed thumb switches sometimes
#if !defined __clang__ && defined __thumb__
asm(R"(
.global __gnu_thumb1_case_uhi
.thumb_func
__gnu_thumb1_case_uhi:
	push    {r0, r1}
	mov     r1, lr
	lsr    r1, r1, #1
	lsl    r0, r0, #1
	lsl    r1, r1, #1
	ldrh    r1, [r1, r0]
	lsl    r1, r1, #1
	add     lr, lr, r1
	pop     {r0, r1}
	bx      lr
.global __gnu_thumb1_case_uqi
.thumb_func
__gnu_thumb1_case_uqi:
	mov     r12, r1
	mov     r1, lr
	lsr    r1, r1, #1
	lsl    r1, r1, #1
	ldrb    r1, [r1, r0]
	lsl    r1, r1, #1
	add     lr, lr, r1
	mov     r1, r12
	bx      lr
	)");
#endif
static int divide(int dividend, int divisor, int *rem)
{
#if !defined __thumb__ && !defined __clang__
	return __udivmodsi4(dividend, divisor, rem);
#else// slow division, but compact


   // int sign = ((dividend < 0) ^ (divisor < 0)) ? -1 : 1;
   // dividend = abs(dividend);
   // divisor = abs(divisor);
   if(divisor == 0) return 0xffffffff;
    int quotient = 0;
    while (dividend >= divisor) {
        dividend -= divisor;
        ++quotient;
    }
    if(rem) *rem = dividend;// * sign;
    return quotient;// * sign;
#endif
}
 
#ifdef __thumb__
unsigned __attribute__((used)) __aeabi_uidiv(unsigned numerator, unsigned denominator)
{
	return divide(numerator, denominator, NULL);
}
unsigned __attribute__((used)) int __udivmodsi4(unsigned int divident, unsigned int divisor, unsigned int *remainder)
{
	return divide(divident, divisor, remainder);
}
#endif
// clang does not support raw literals, do optimized version not used
#ifdef __clang__
static void __attribute__((used)) __aeabi_uidivmod(unsigned numerator, unsigned denominator)
{
	int rem1;
	register int rem asm("r1");
	register int ret asm("r0") = divide(numerator, denominator, &rem1);
	rem = rem1;
//	asm("mov r0,%0\n\tmov r1,%1\n\t"::"r"(ret),"r"(rem):"r0", "r1");
}
#endif
#if 0
unsigned __attribute__((used)) long long int __udivmoddi4(unsigned long long divident, unsigned long long divisor, unsigned long long *remainder)
{
	*remainder = 0;
	return divide(divident, divisor, (int*)remainder);
}
#endif
