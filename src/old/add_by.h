/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowcup@maersk-moller.net
 *
 *
 */

#ifndef __ADD_BY_H__
#define __ADD_BY_H__

#define Add2by1(src1, off1, off2, s1, s2, divisor) \
		((((u_int16_t)(*(src1+off1)))<<s1) + \
		 (((u_int16_t)(*(src1+off2)))<<s2) \
		)/divisor;

#define Add2by1mul(src1, off1, off2, m1, m2, divisor) \
		((((u_int16_t)(*(src1+off1)))*((u_int16_t)m1)) + \
		 (((u_int16_t)(*(src1+off2)))*((u_int16_t)m2)) \
		)/((u_int16_t)divisor);

#define Add2by2(src1, src2, off1, off2, s1, s2, s3, s4, divisor) \
		((((u_int16_t)(*(src1+off1)))<<s1) + \
		 (((u_int16_t)(*(src1+off2)))<<s2) + \
		 (((u_int16_t)(*(src2+off1)))<<s3) + \
		 (((u_int16_t)(*(src2+off2)))<<s4) \
		)/divisor;

#define Add3by3(src1, src2, src3, off1, off2, off3, \
	s1, s2, s3, s4, s5, s6, s7, s8, s9, divisor) \
		((((u_int16_t)(*(src1+off1)))<<s1) + \
		 (((u_int16_t)(*(src1+off2)))<<s2) + \
		 (((u_int16_t)(*(src1+off3)))<<s3) + \
		 (((u_int16_t)(*(src2+off1)))<<s4) + \
		 (((u_int16_t)(*(src2+off2)))<<s5) + \
		 (((u_int16_t)(*(src2+off3)))<<s6) + \
		 (((u_int16_t)(*(src3+off1)))<<s7) + \
		 (((u_int16_t)(*(src3+off2)))<<s8) + \
		 (((u_int16_t)(*(src3+off3)))<<s9) \
		)/divisor;

#define Add4by4(src1, src2, src3, src4, off1, off2, off3, off4, \
	s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15, s16, divisor) \
		((((u_int16_t)(*(src1+off1)))<<s1) + \
		 (((u_int16_t)(*(src1+off2)))<<s2) + \
		 (((u_int16_t)(*(src1+off3)))<<s3) + \
		 (((u_int16_t)(*(src1+off4)))<<s4) + \
		 (((u_int16_t)(*(src2+off1)))<<s5) + \
		 (((u_int16_t)(*(src2+off2)))<<s6) + \
		 (((u_int16_t)(*(src2+off3)))<<s7) + \
		 (((u_int16_t)(*(src2+off4)))<<s8) + \
		 (((u_int16_t)(*(src3+off1)))<<s9) + \
		 (((u_int16_t)(*(src3+off2)))<<s10) + \
		 (((u_int16_t)(*(src3+off3)))<<s11) + \
		 (((u_int16_t)(*(src3+off4)))<<s12) + \
		 (((u_int16_t)(*(src4+off1)))<<s13) + \
		 (((u_int16_t)(*(src4+off2)))<<s14) + \
		 (((u_int16_t)(*(src4+off3)))<<s15) + \
		 (((u_int16_t)(*(src4+off4)))<<s16) \
		)/divisor;

#define Add5by5(src1, src2, src3, src4, src5, off1, off2, off3, off4, off5,	\
	s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13,		\
	s14, s15, s16, s17, s18, s19, s20, s21, s22, s23, s24, s25, divisor) \
		((((u_int16_t)(*(src1+off1)))<<s1) + \
		 (((u_int16_t)(*(src1+off2)))<<s2) + \
		 (((u_int16_t)(*(src1+off3)))<<s3) + \
		 (((u_int16_t)(*(src1+off4)))<<s4) + \
		 (((u_int16_t)(*(src1+off5)))<<s5) + \
		 (((u_int16_t)(*(src2+off1)))<<s6) + \
		 (((u_int16_t)(*(src2+off2)))<<s7) + \
		 (((u_int16_t)(*(src2+off3)))<<s8) + \
		 (((u_int16_t)(*(src2+off4)))<<s9) + \
		 (((u_int16_t)(*(src2+off5)))<<s10) + \
		 (((u_int16_t)(*(src3+off1)))<<s11) + \
		 (((u_int16_t)(*(src3+off2)))<<s12) + \
		 (((u_int16_t)(*(src3+off3)))<<s13) + \
		 (((u_int16_t)(*(src3+off4)))<<s14) + \
		 (((u_int16_t)(*(src3+off5)))<<s15) + \
		 (((u_int16_t)(*(src4+off1)))<<s16) + \
		 (((u_int16_t)(*(src4+off2)))<<s17) + \
		 (((u_int16_t)(*(src4+off3)))<<s18) + \
		 (((u_int16_t)(*(src4+off4)))<<s19) + \
		 (((u_int16_t)(*(src4+off5)))<<s20) + \
		 (((u_int16_t)(*(src5+off1)))<<s21) + \
		 (((u_int16_t)(*(src5+off2)))<<s22) + \
		 (((u_int16_t)(*(src5+off3)))<<s23) + \
		 (((u_int16_t)(*(src5+off4)))<<s24) + \
		 (((u_int16_t)(*(src5+off5)))<<s25) \
		)/divisor;

#define Add3by2(src1, src2, src3, off1, off2, s1, s2, s3, s4, s5, s6, divisor) \
		((((u_int16_t)(*(src1+off1)))<<s1) + \
		 (((u_int16_t)(*(src1+off2)))<<s2) + \
		 (((u_int16_t)(*(src2+off1)))<<s3) + \
		 (((u_int16_t)(*(src2+off2)))<<s4) + \
		 (((u_int16_t)(*(src3+off1)))<<s5) + \
		 (((u_int16_t)(*(src3+off2)))<<s6) \
		)/divisor;

#define Add3by2mul(src1, src2, src3, off1, off2, s1, s2, s3, s4, s5, s6, divisor) \
		((((u_int16_t)(*(src1+off1)))*s1) + \
		 (((u_int16_t)(*(src1+off2)))*s2) + \
		 (((u_int16_t)(*(src2+off1)))*s3) + \
		 (((u_int16_t)(*(src2+off2)))*s4) + \
		 (((u_int16_t)(*(src3+off1)))*s5) + \
		 (((u_int16_t)(*(src3+off2)))*s6) \
		)/divisor;

#endif	// ADD_BY
