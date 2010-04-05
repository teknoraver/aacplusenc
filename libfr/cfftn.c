/*
  Complex FFT core for transforms
*/
#include "cfftn.h"

#ifdef _FFTW3

#include <stdlib.h>
#include <fftw3.h>

static fftwf_plan plan4, plan8, plan64, plan512;

void init_plans()
{
	fftwf_complex fft_data;

	plan4 = fftwf_plan_dft_1d(4, &fft_data, &fft_data, FFTW_BACKWARD, FFTW_ESTIMATE);
	plan8 = fftwf_plan_dft_1d(8, &fft_data, &fft_data, FFTW_BACKWARD, FFTW_ESTIMATE);
	plan64 = fftwf_plan_dft_1d(64, &fft_data, &fft_data, FFTW_FORWARD, FFTW_ESTIMATE);
	plan512 = fftwf_plan_dft_1d(512, &fft_data, &fft_data, FFTW_FORWARD, FFTW_ESTIMATE);
}

void destroy_plans()
{
	fftwf_destroy_plan(plan512);
	fftwf_destroy_plan(plan64);
	fftwf_destroy_plan(plan8);
	fftwf_destroy_plan(plan4);
}

int CFFTN(float *afftData, int len, int isign)
{
	switch(len) {
	case 4:
		fftwf_execute_dft(plan4, (fftwf_complex*)afftData, (fftwf_complex*)afftData);
		break;
	case 8:
		fftwf_execute_dft(plan8, (fftwf_complex*)afftData, (fftwf_complex*)afftData);
		break;
	case 64:
		fftwf_execute_dft(plan64, (fftwf_complex*)afftData, (fftwf_complex*)afftData);
		break;
	case 512:
		fftwf_execute_dft(plan512, (fftwf_complex*)afftData, (fftwf_complex*)afftData);
		break;
	default:
		printf("non standard len for FFT: %d\nWill now die", len);
		exit(1);
	}

	return 1;
}

#else

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "counters.h" /* the 3GPP instrumenting tool */

#define MAX_FACTORS  23
#define MAX_PERM    209
#define NFACTOR	     11
#ifndef M_PI
#define M_PI	3.14159265358979323846264338327950288
#endif
#define SIN60	0.86602540378443865
#define COS72	0.30901699437494742
#define SIN72	0.95105651629515357


int cfftn(float Re[],
	  float Im[],
   int  nTotal,
   int  nPass,
   int  nSpan,
   int  iSign)
{

	int ii, mfactor, kspan, ispan, inc;
	int j, jc, jf, jj, k, k1, k2, k3=0, k4, kk, kt, nn, ns, nt;
  
	double radf;
	double c1, c2=0.0, c3=0.0, cd;
	double s1, s2=0.0, s3=0.0, sd;
	float  ak,bk,akp,bkp,ajp,bjp,ajm,bjm,akm,bkm,aj,bj,aa,bb;

	float Rtmp[MAX_FACTORS];
	float Itmp[MAX_FACTORS];
	double Cos[MAX_FACTORS];
	double Sin[MAX_FACTORS];
  
	int Perm[MAX_PERM];
	int factor [NFACTOR];
  
	double s60 = SIN60;
	double c72 = COS72;
	double s72 = SIN72;
	double pi2 = M_PI;
  
	COUNT_sub_start("cfftn");

	PTR_INIT(6); /* pointer for Rtmp[],
	Itmp[],
	Cos[],
	Sin[],
	Perm[],
	factor[] */

	ADD(2);
	Re--;
	Im--;
  
	ADD(1); BRANCH(1);
	if (nPass < 2) {
		COUNT_sub_end();
		return 0;
	}
  
	MOVE(1);
	inc = iSign;
	BRANCH(1);
	if( iSign < 0 ) {
		MULT(4);
		s72 = -s72;
		s60 = -s60;
		pi2 = -pi2;
		inc = -inc;
	}
  
	MULT(2);
	nt = inc * nTotal;
	ns = inc * nSpan;
	MOVE(1);
	kspan = ns;
  
	ADD(1);
	nn = nt - inc;

	DIV(1);
	jc = ns / nPass;

	MULT(2);
	radf = pi2 * (double) jc;
	pi2 *= 2.0;
  
	MOVE(2);
	ii = 0;
	jf = 0;

	MOVE(2);
	mfactor = 0;
	k = nPass;

	PTR_INIT(1); /* pointer for factor[] */
	LOOP(1);
	while (k % 16 == 0) {
		ADD(1);
		mfactor++;

		MOVE(1);
		factor [mfactor - 1] = 4;

		MULT(1);
		k /= 16;
	}

	MOVE(2);
	j = 3;
	jj = 9;

	PTR_INIT(1); /* pointer for factor[] */
	LOOP(1);
	do {
		LOOP(1);
		while (k % jj == 0) {
			ADD(1);
			mfactor++;

			MOVE(1);
			factor [mfactor - 1] = j;

			DIV(1);
			k /= jj;
		}

		ADD(1);
		j += 2;

		MULT(1);
		jj = j * j;
	} while (jj <= k);

	ADD(1); BRANCH(1);
	if (k <= 4) {

		MOVE(2);
		kt = mfactor;
		factor [mfactor] = k;

		ADD(1); BRANCH(1);
		if (k != 1) {
			ADD(1);
			mfactor++;
		}
	} else {

		SHIFT(1); ADD(1); BRANCH(1);
		if (k - (k / 4 << 2) == 0) {
			ADD(1);
			mfactor++;
			MOVE(1);
			factor [mfactor - 1] = 2;
			MULT(1);
			k /= 4;
		}

		MOVE(2);
		kt = mfactor;
		j = 2;

		LOOP(1);
		do {

			BRANCH(1);
			if (k % j == 0) {
				ADD(1);
				mfactor++;

				MOVE(1);
				factor [mfactor - 1] = j;

				MULT(1);
				k /= j;
			}
			ADD(2); SHIFT(1);
			j = ((j + 1) / 2 << 1) + 1;
		} while (j <= k);
	}

	BRANCH(1);
	if (kt) {

		MOVE(1);
		j = kt;

		PTR_INIT(1); /* pointer for factor[] */
		LOOP(1);
		do {

			ADD(1);
			mfactor++;

			MOVE(1);
			factor [mfactor - 1] = factor [j - 1];

			ADD(1);
			j--;
		} while (j);
	}
  
	ADD(1); BRANCH(1);
	if (mfactor > NFACTOR) {
		COUNT_sub_end();
		return(0);
	}
  
	LOOP(1);
	for (;;) {
		DIV(1);
		sd = radf / (double) kspan;

		TRANS(1);
		cd = sin(sd);

		MULT(2);
		cd = 2.0 * cd * cd;

		TRANS(1);
		sd = sin(sd + sd);

		MOVE(1);
		kk = 1;

		ADD(1);
		ii++;
    
		BRANCH(2);
		switch (factor [ii - 1]) {
			case 2:
				MULT(1);
				kspan /= 2;

				ADD(1);
				k1 = kspan + 2;

				LOOP(1);
				do {

					PTR_INIT(4); /* pointer for Re[k2], Re[kk],
					Im[k2], Im[kk]  */
					LOOP(1);
					do {
						ADD(1);
						k2 = kk + kspan;

						MOVE(2);
						ak = Re [k2];
						bk = Im [k2];

						ADD(4); STORE(4);
						Re [k2] = Re [kk] - ak;
						Im [k2] = Im [kk] - bk;
						Re [kk] += ak;
						Im [kk] += bk;

						ADD(1);
						kk = k2 + kspan;

					} while (kk <= nn);

					ADD(1);
					kk -= nn;
				} while (kk <= jc);

				ADD(1); BRANCH(1);
				if (kk > kspan)
					goto Permute_Results_Label;


				PTR_INIT(4); /* pointers for Re[k2], Re[kk],
				Im[k2], Im[kk]  */
				LOOP(1);
				do {

					ADD(1);
					c1 = 1.0 - cd;

					MOVE(1);
					s1 = sd;

					LOOP(1);
					do {

						LOOP(1);
						do {

							LOOP(1);
							do {
								ADD(3);
								k2 = kk + kspan;
								ak = Re [kk] - Re [k2];
								bk = Im [kk] - Im [k2];

								ADD(2); STORE(2);
								Re [kk] += Re [k2];
								Im [kk] += Im [k2];

								MULT(2); ADD(1); STORE(1);
								Re [k2] = (float)(c1 * ak - s1 * bk);

								MULT(1); MAC(1); STORE(1);
								Im [k2] = (float)(s1 * ak + c1 * bk);

								ADD(1);
								kk = k2 + kspan;
							} while (kk < nt);

							ADD(1);
							k2 = kk - nt;

							MULT(1);
							c1 = -c1;

							ADD(1);
							kk = k1 - k2;

						} while (kk > k2);

						MULT(2); MAC(1); ADD(1);
						ak = (float)(c1 - (cd * c1 + sd * s1));

						MULT(2); ADD(2);
						s1 = sd * c1 - cd * s1 + s1;

						MULT(2); MAC(1); ADD(1);
						c1 = 2.0 - (ak * ak + s1 * s1);

						MULT(2);
						s1 *= c1;
						c1 *= ak;

						ADD(1);
						kk += jc;
					} while (kk < k2);

					ADD(2);
					k1 += inc + inc;

					ADD(2); MULT(1);
					kk = (k1 - kspan) / 2 + jc;
				} while (kk <= jc + jc);
				break;
      
			case 4:
				MOVE(1);
				ispan = kspan;

				MULT(1);
				kspan /= 4;
      

				PTR_INIT(4); /* pointer for Re[k2], Re[kk],
				Im[k2], Im[kk]  */
				LOOP(1);
				do {

					MOVE(2);
					c1 = 1.0;
					s1 = 0.0;

					LOOP(1);
					do {
						LOOP(1);
						do {
							ADD(3);
							k1 = kk + kspan;
							k2 = k1 + kspan;
							k3 = k2 + kspan;

							ADD(8);
							akp = Re [kk] + Re [k2];
							akm = Re [kk] - Re [k2];
							ajp = Re [k1] + Re [k3];
							ajm = Re [k1] - Re [k3];
							bkp = Im [kk] + Im [k2];
							bkm = Im [kk] - Im [k2];
							bjp = Im [k1] + Im [k3];
							bjm = Im [k1] - Im [k3];

							ADD(2); STORE(2);
							Re [kk] = akp + ajp;
							Im [kk] = bkp + bjp;

							ADD(2);
							ajp = akp - ajp;
							bjp = bkp - bjp;

							BRANCH(1);
							if (iSign < 0) {

								ADD(4);
								akp = akm + bjm;
								bkp = bkm - ajm;
								akm -= bjm;
								bkm += ajm;

							} else {

								ADD(4);
								akp = akm - bjm;
								bkp = bkm + ajm;
								akm += bjm;
								bkm -= ajm;
							}

							BRANCH(1);
							if (s1 != 0.0) {

								MULT(6); ADD(3); STORE(3);
								Re [k1] = (float)(akp * c1 - bkp * s1);
								Re [k2] = (float)(ajp * c2 - bjp * s2);
								Re [k3] = (float)(akm * c3 - bkm * s3);

								MULT(3); MAC(3); STORE(3);
								Im [k1] = (float)(akp * s1 + bkp * c1);
								Im [k2] = (float)(ajp * s2 + bjp * c2);
								Im [k3] = (float)(akm * s3 + bkm * c3);
							} else {

								MOVE(6);
								Re [k1] = akp;
								Re [k2] = ajp;
								Re [k3] = akm;
								Im [k1] = bkp;
								Im [k2] = bjp;
								Im [k3] = bkm;
							}
							ADD(1);
							kk = k3 + kspan;
						} while (kk <= nt);
          
						MULT(1); MAC(1); ADD(1);
						c2 = c1 - (cd * c1 + sd * s1);

						MULT(2); ADD(2);
						s1 = sd * c1 - cd * s1 + s1;

						MULT(1); MAC(1); ADD(1);
						c1 = 2.0 - (c2 * c2 + s1 * s1);

						MULT(2);
						s1 *= c1;
						c1 *= c2;

						MULT(2); ADD(1);
						c2 = c1 * c1 - s1 * s1;

						MULT(2);
						s2 = 2.0 * c1 * s1;

						MULT(2); ADD(1);
						c3 = c2 * c1 - s2 * s1;

						MULT(1); MAC(1);
						s3 = c2 * s1 + s2 * c1;

						ADD(2);
						kk = kk - nt + jc;
					} while (kk <= kspan);

					ADD(2);
					kk = kk - kspan + inc;

				} while (kk <= jc);

				ADD(1); BRANCH(1);
				if (kspan == jc)
					goto Permute_Results_Label;
				break;
      
			default:
				MOVE(1);
				k = factor [ii - 1];
				MOVE(1);
				ispan = kspan;
				DIV(1);
				kspan /= k;
      
				BRANCH(2);
				switch (k) {
					case 3:


						PTR_INIT(4); /* pointer for Re[k1], Re[k2],
						Im[k1], Im[k2]  */
						LOOP(1);
						do {
							LOOP(1);
							do {

								ADD(2);
								k1 = kk + kspan;
								k2 = k1 + kspan;

								MOVE(2);
								ak = Re [kk];
								bk = Im [kk];

								ADD(2);
								aj = Re [k1] + Re [k2];
								bj = Im [k1] + Im [k2];

								ADD(2); STORE(2);
								Re [kk] = ak + aj;
								Im [kk] = bk + bj;

								MULT(2); ADD(2);
								ak -= 0.5f * aj;
								bk -= 0.5f * bj;

								ADD(2); MULT(2);
								aj = (float)((Re [k1] - Re [k2]) * s60);
								bj = (float)((Im [k1] - Im [k2]) * s60);

								ADD(4); STORE(4);
								Re [k1] = ak - bj;
								Re [k2] = ak + bj;
								Im [k1] = bk + aj;
								Im [k2] = bk - aj;

								ADD(2);
								kk = k2 + kspan;
							} while (kk < nn);

							ADD(1);
							kk -= nn;
						} while (kk <= kspan);
						break;
        
					case 5:

						MULT(2); ADD(1);
						c2 = c72 * c72 - s72 * s72;

						MULT(3);
						s2 = 2.0 * c72 * s72;


						PTR_INIT(10); /* pointer for Re[k1], Re[k2], Re[k3], Re[k4], Re[k4],
						Im[k1], Im[k2], Im[k3], Im[k4], Im[kk] */
						LOOP(1);
						do {

							LOOP(1);
							do {

								ADD(4);
								k1 = kk + kspan;
								k2 = k1 + kspan;
								k3 = k2 + kspan;
								k4 = k3 + kspan;

								ADD(8);
								akp = Re [k1] + Re [k4];
								akm = Re [k1] - Re [k4];
								bkp = Im [k1] + Im [k4];
								bkm = Im [k1] - Im [k4];
								ajp = Re [k2] + Re [k3];
								ajm = Re [k2] - Re [k3];
								bjp = Im [k2] + Im [k3];
								bjm = Im [k2] - Im [k3];

								MOVE(2);
								aa = Re [kk];
								bb = Im [kk];

								ADD(4); STORE(2);
								Re [kk] = aa + akp + ajp;
								Im [kk] = bb + bkp + bjp;

								MULT(2); MAC(2); ADD(2);
								ak = (float)(akp * c72 + ajp * c2 + aa);
								bk = (float)(bkp * c72 + bjp * c2 + bb);

								MULT(2); MAC(2);
								aj = (float)(akm * s72 + ajm * s2);
								bj = (float)(bkm * s72 + bjm * s2);

								ADD(4); STORE(4);
								Re [k1] = ak - bj;
								Re [k4] = ak + bj;
								Im [k1] = bk + aj;
								Im [k4] = bk - aj;

								MULT(2); MAC(2); ADD(2);
								ak = (float)(akp * c2 + ajp * c72 + aa);
								bk = (float)(bkp * c2 + bjp * c72 + bb);

								MULT(2); ADD(2);
								aj = (float)(akm * s2 - ajm * s72);
								bj = (float)(bkm * s2 - bjm * s72);

								ADD(4); STORE(4);
								Re [k2] = ak - bj;
								Re [k3] = ak + bj;
								Im [k2] = bk + aj;
								Im [k3] = bk - aj;

								ADD(1);
								kk = k4 + kspan;
							} while (kk < nn);

							ADD(1);
							kk -= nn;

						} while (kk <= kspan);
						break;
        
					default:

						ADD(1); BRANCH(1);
						if (k != jf) {

							MOVE(1);
							jf = k;

							DIV(1);
							s1 = pi2 / (double) k;

							TRANS(2);
							c1 = cos(s1);
							s1 = sin(s1);

							ADD(1); BRANCH(1);
							if (jf > MAX_FACTORS ) {
								COUNT_sub_end();
								return(0);
							}
          
							MOVE(2);
							Cos [jf - 1] = 1.0;
							Sin [jf - 1] = 0.0;

							MOVE(1);
							j = 1;

							PTR_INIT(4); /* pointers for Cos[j], Cos[k],
							Sin[j], Sin[k] */
							LOOP(1);
							do {
								MULT(1); MAC(1); STORE(1);
								Cos [j - 1] = Cos [k - 1] * c1 + Sin [k - 1] * s1;

								MULT(2); ADD(1); STORE(1);
								Sin [j - 1] = Cos [k - 1] * s1 - Sin [k - 1] * c1;

								ADD(1);
								k--;

								MOVE(1);
								Cos [k - 1] = Cos [j - 1];

								MULT(1); STORE(1);
								Sin [k - 1] = -Sin [j - 1];

								ADD(1);
								j++;
							} while (j < k);
						}

						PTR_INIT(10); /* pointers for Re[k1], Re[k2], Re[kk], Rtmp[], Cos[]
						Im[k1], Im[k2], Im[kk], Itmp[], Sin[]  */
						LOOP(1);
						do {
							LOOP(1);
							do {

								MOVE(1);
								k1 = kk;

								ADD(1);
								k2 = kk + ispan;

								MOVE(4);
								ak = aa = Re [kk];
								bk = bb = Im [kk];

								MOVE(1);
								j = 1;

								ADD(1);
								k1 += kspan;

								LOOP(1);
								do {

									ADD(1);
									k2 -= kspan;

									ADD(1);
									j++;

									ADD(1); STORE(1);
									Rtmp [j - 1] = Re [k1] + Re [k2];

									ADD(1);
									ak += Rtmp [j - 1];

									ADD(1); STORE(1);
									Itmp [j - 1] = Im [k1] + Im [k2];

									ADD(1);
									bk += Itmp [j - 1];

									ADD(1);
									j++;

									ADD(2); STORE(2);
									Rtmp [j - 1] = Re [k1] - Re [k2];
									Itmp [j - 1] = Im [k1] - Im [k2];

									ADD(1);
									k1 += kspan;

								} while (k1 < k2);

								MOVE(2);
								Re [kk] = ak;
								Im [kk] = bk;

								MOVE(1);
								k1 = kk;

								ADD(1);
								k2 = kk + ispan;

								MOVE(1);
								j = 1;

								LOOP(1);
								do {

									ADD(2);
									k1 += kspan;
									k2 -= kspan;

									MOVE(6);
									jj = j;
									ak = aa;
									bk = bb;
									aj = 0.0;
									bj = 0.0;
									k = 1;

									LOOP(1);
									do {

										ADD(1);
										k++;

										MAC(2);
										ak += (float)(Rtmp [k - 1] * Cos [jj - 1]);
										bk += (float)(Itmp [k - 1] * Cos [jj - 1]);

										ADD(1);
										k++;

										MAC(2);
										aj += (float)(Rtmp [k - 1] * Sin [jj - 1]);
										bj += (float)(Itmp [k - 1] * Sin [jj - 1]);

										ADD(1);
										jj += j;

										ADD(1); BRANCH(1);
										if (jj > jf) {
										ADD(1);
										jj -= jf;
										}
									} while (k < jf);

									ADD(1);
									k = jf - j;

									ADD(4); STORE(4);
									Re [k1] = ak - bj;
									Im [k1] = bk + aj;
									Re [k2] = ak + bj;
									Im [k2] = bk - aj;

									ADD(1);
									j++;
								} while (j < k);

								ADD(1);
								kk += ispan;

							} while (kk <= nn);

							ADD(1);
							kk -= nn;

						} while (kk <= kspan);
						break;
				}

				ADD(1); BRANCH(1);
				if (ii == mfactor)
					goto Permute_Results_Label;

				ADD(1);
				kk = jc + 1;

				PTR_INIT(2); /* pointers for Re[kk],
				Im[kk]  */
				LOOP(1);
				do {

					ADD(1);
					c2 = 1.0 - cd;

					MOVE(1);
					s1 = sd;

					LOOP(1);
					do {

						MOVE(2);
						c1 = c2;
						s2 = s1;

						ADD(1);
						kk += kspan;

						LOOP(1);
						do {

							LOOP(1);
							do {

								MOVE(1);
								ak = Re [kk];

								MULT(2); ADD(1); STORE(1);
								Re [kk] = (float)(c2 * ak - s2 * Im [kk]);

								MULT(1); MAC(1); STORE(1);
								Im [kk] = (float)(s2 * ak + c2 * Im [kk]);

								ADD(1);
								kk += ispan;

							} while (kk <= nt);

							MULT(1);
							ak = (float)(s1 * s2);

							MULT(1); MAC(1);
							s2 = s1 * c2 + c1 * s2;

							MULT(1); ADD(1);
							c2 = c1 * c2 - ak;

							ADD(2);
							kk = kk - nt + kspan;

						} while (kk <= ispan);

						MULT(1); MAC(1); ADD(1);
						c2 = c1 - (cd * c1 + sd * s1);

						MULT(1); MAC(2);
						s1 += sd * c1 - cd * s1;

						MULT(1); MAC(1); ADD(1);
						c1 = 2.0 - (c2 * c2 + s1 * s1);

						MULT(2);
						s1 *= c1;
						c2 *= c1;

						ADD(2);
						kk = kk - ispan + jc;

					} while (kk <= kspan);

					ADD(3);
					kk = kk - kspan + jc + inc;

				} while (kk <= jc + jc);
				break;
		}
	}
  
Permute_Results_Label:

		MOVE(1);
  Perm [0] = ns;

  BRANCH(1);
  if (kt) {

	  ADD(2);
	  k = kt + kt + 1;

	  ADD(1); BRANCH(1);
	  if (mfactor < k) {
		  ADD(1);
		  k--;
	  }

	  MOVE(1);
	  j = 1;

	  MOVE(1);
	  Perm [k] = jc;

	  PTR_INIT(2); /* pointers for Perm[j], Perm[k]  */
	  LOOP(1);
	  do {
		  DIV(1); STORE(1);
		  Perm [j] = Perm [j - 1] / factor [j - 1];

		  MULT(1); STORE(1);
		  Perm [k - 1] = Perm [k] * factor [j - 1];

		  ADD(2);
		  j++;
		  k--;
	  } while (j < k);

	  MOVE(2);
	  k3 = Perm [k];
	  kspan = Perm [1];

	  ADD(2);
	  kk = jc + 1;
	  k2 = kspan + 1;

	  MOVE(1);
	  j = 1;

	  ADD(1); BRANCH(1);
	  if (nPass != nTotal) {
Permute_Multi_Label:

		PTR_INIT(5); /* pointers for Re[kk], Re[k2],
                                 Im[kk], Im[k2],
                                 Perm[]          */
    LOOP(1);
    do {

	    LOOP(1);
	    do {

		    ADD(1);
		    k = kk + jc;

		    LOOP(1);
		    do {
			    MOVE(6);
			    ak = Re [kk]; Re [kk] = Re [k2]; Re [k2] = ak;
			    bk = Im [kk]; Im [kk] = Im [k2]; Im [k2] = bk;

			    ADD(2);
			    kk += inc;
			    k2 += inc;

		    } while (kk < k);

		    ADD(4);
		    kk += ns - jc;
		    k2 += ns - jc;

	    } while (kk < nt);

	    ADD(4);
	    k2 = k2 - nt + kspan;
	    kk = kk - nt + jc;

    } while (k2 < ns);

    LOOP(1);
    do {

	    LOOP(1);
	    do {

		    ADD(1);
		    k2 -= Perm [j - 1];

		    ADD(1);
		    j++;

		    ADD(1);
		    k2 = Perm [j] + k2;


	    } while (k2 > Perm [j - 1]);

	    MOVE(1);
	    j = 1;

	    LOOP(1);
	    do {

		    ADD(1); BRANCH(1);
		    if (kk < k2)
			    goto Permute_Multi_Label;

		    ADD(2);
		    kk += jc;
		    k2 += kspan;

	    } while (k2 < ns);
    } while (kk < ns);
	  } else {
Permute_Single_Label:

		PTR_INIT(5); /* pointers for Re[kk], Re[k2],
                                 Im[kk], Im[k2],
                                 Perm[]          */
    LOOP(1);
    do {
	    MOVE(6);
	    ak = Re [kk]; Re [kk] = Re [k2]; Re [k2] = ak;
	    bk = Im [kk]; Im [kk] = Im [k2]; Im [k2] = bk;

	    ADD(2);
	    kk += inc;
	    k2 += kspan;

    } while (k2 < ns);

    LOOP(1);
    do {

	    LOOP(1);
	    do {

		    ADD(1);
		    k2 -= Perm [j - 1];

		    ADD(1);
		    j++;

		    ADD(1);
		    k2 = Perm [j] + k2;

	    } while (k2 > Perm [j - 1]);

	    MOVE(1);
	    j = 1;

	    LOOP(1);
	    do {

		    ADD(1); BRANCH(1);
		    if (kk < k2)
			    goto Permute_Single_Label;

		    ADD(2);
		    kk += inc;
		    k2 += kspan;

	    } while (k2 < ns);
    } while (kk < ns);
	  }

	  MOVE(1);
	  jc = k3;
  }
  
  SHIFT(1); ADD(2); BRANCH(1);
  if ((kt << 1) + 1 >= mfactor) {
	  COUNT_sub_end();
	  return 1;
  }

  MOVE(1);
  ispan = Perm [kt];

  ADD(1);
  j = mfactor - kt;

  MOVE(1);
  factor [j] = 1;

  PTR_INIT(1); /* pointers for factor[] */
  LOOP(1);
  do {

	  MULT(1); STORE(1);
	  factor [j - 1] *= factor [j];

	  ADD(1);
	  j--;
  } while (j != kt);

  ADD(1);
  kt++;

  ADD(1);
  nn = factor [kt - 1] - 1;

  ADD(1); BRANCH(1);
  if (nn > MAX_PERM) {
	  COUNT_sub_end();
	  return(0);
  }
  
  MOVE(2);
  j = jj = 0;

  PTR_INIT(3); /* pointers for factor[kt], factor[k],
  Perm[]                  */
  LOOP(1);
  for (;;) {

	  ADD(1);
	  k = kt + 1;

	  MOVE(2);
	  k2 = factor [kt - 1];
	  kk = factor [k - 1];

	  ADD(1);
	  j++;

	  ADD(1); BRANCH(1);
	  if (j > nn)
		  break;

	  ADD(1);
	  jj += kk;

	  LOOP(1);
	  while (jj >= k2) {

		  ADD(1);
		  jj -= k2;

		  MOVE(1);
		  k2 = kk;

		  ADD(1);
		  k++;

		  MOVE(1);
		  kk = factor [k - 1];

		  ADD(1);
		  jj += kk;
	  }

	  MOVE(1);
	  Perm [j - 1] = jj;
  }

  MOVE(1);
  j = 0;

  PTR_INIT(2); /* pointers for Perm[j], Perm[k] */
  LOOP(1);
  for (;;) {

	  LOOP(1);
	  do {

		  ADD(1);
		  j++;

		  MOVE(1);
		  kk = Perm [j - 1];

	  } while (kk < 0);

	  ADD(1); BRANCH(1);
	  if (kk != j) {

		  LOOP(1);
		  do {

			  MOVE(2);
			  k = kk;
			  kk = Perm [k - 1];

			  MULT(1); STORE(1);
			  Perm [k - 1] = -kk;
		  } while (kk != j);

		  MOVE(1);
		  k3 = kk;

	  } else {

		  MULT(1); STORE(1);
		  Perm [j - 1] = -j;

		  ADD(1); BRANCH(1);
		  if (j == nn)
			  break;
	  }
  }
  
  PTR_INIT(7); /* pointers for Re[k1], Re[k2], Rtmp[],
  Im[k1], Im[k2], Itmp[],
  Perm[]                  */
  LOOP(1);
  for (;;){

	  ADD(1);
	  j = k3 + 1;

	  ADD(1);
	  nt -= ispan;

	  ADD(2);
	  ii = nt - inc + 1;

	  BRANCH(1);
	  if (nt < 0)
		  break;

	  LOOP(1);
	  do {

		  LOOP(1);
		  do {

			  ADD(1);
			  j--;
		  } while (Perm [j - 1] < 0);

		  MOVE(1);
		  jj = jc;

		  LOOP(1);
		  do {

			  MOVE(1);
			  kspan = jj;

			  ADD(1); BRANCH(1); 
			  if (jj > MAX_FACTORS * inc) {

				  MULT(1);
				  kspan = MAX_FACTORS * inc;
			  }

			  ADD(1);
			  jj -= kspan;

			  MOVE(1);
			  k = Perm [j - 1];

			  MULT(1); ADD(2);
			  kk = jc * k + ii + jj;

			  ADD(1);
			  k1 = kk + kspan;

			  MOVE(1);
			  k2 = 0;

			  LOOP(1);
			  do {

				  ADD(1);
				  k2++;

				  MOVE(2);
				  Rtmp [k2 - 1] = Re [k1];
				  Itmp [k2 - 1] = Im [k1];

				  ADD(1);
				  k1 -= inc;

			  } while (k1 != kk);

			  LOOP(1);
			  do {

				  ADD(1);
				  k1 = kk + kspan;

				  ADD(2); MULT(1);
				  k2 = k1 - jc * (k + Perm [k - 1]);

				  MULT(1);
				  k = -Perm [k - 1];

				  LOOP(1);
				  do {

					  MOVE(2);
					  Re [k1] = Re [k2];
					  Im [k1] = Im [k2];

					  ADD(2);
					  k1 -= inc;
					  k2 -= inc;

				  } while (k1 != kk);

				  MOVE(1);
				  kk = k2;

			  } while (k != j);

			  ADD(1);
			  k1 = kk + kspan;

			  MOVE(1);
			  k2 = 0;

			  LOOP(1);
			  do {

				  ADD(1);
				  k2++;

				  MOVE(2);
				  Re [k1] = Rtmp [k2 - 1];
				  Im [k1] = Itmp [k2 - 1];

				  ADD(1);
				  k1 -= inc;

			  } while (k1 != kk);
		  } while (jj);
	  } while (j != 1);
  }

  COUNT_sub_end();
  return 1;
}

/*
   computes complex fourier transform of length len
   returns status
*/
int CFFTN(float *afftData,int len, int isign)
{
	return(cfftn(afftData,afftData+1,len,len,len,2*isign));
}

#endif
