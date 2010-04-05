/*
 Huffman Bitcounter & coder
*/
#include <stdlib.h>
#include "bit_cnt.h"
#include "minmax.h"
#include "aac_rom.h"

#include "counters.h" /* the 3GPP instrumenting tool */


#define HI_LTAB(a) (a>>8)
#define LO_LTAB(a) (a & 0xff)

/*
  needed for fast bit counter, since
  huffman cw length tables are packed into 16 
  bit now
*/
#define HI_EXPLTAB(a) (a>>16)
#define LO_EXPLTAB(a) (a & 0xffff)
#define EXPAND(a)  ((((int)(a&0xff00))<<8)|(int)(a&0xff)) 


/*****************************************************************************


    functionname: count1_2_3_4_5_6_7_8_9_10_11
    description:  counts tables 1-11 
    returns:      
    input:        quantized spectrum
    output:       bitCount for tables 1-11

*****************************************************************************/

static void count1_2_3_4_5_6_7_8_9_10_11(const short *values,
                                         const int  width,
                                         int       *bitCount)
{

  int i;
  int bc1_2,bc3_4,bc5_6,bc7_8,bc9_10,bc11,sc;
  int t0,t1,t2,t3;

  COUNT_sub_start("count1_2_3_4_5_6_7_8_9_10_11");

  MOVE(7);
  bc1_2=0;
  bc3_4=0;
  bc5_6=0;
  bc7_8=0;
  bc9_10=0;
  bc11=0;
  sc=0;

  PTR_INIT(1); /* pointer for values[] */
  LOOP(1);
  for(i=0;i<width;i+=4){
    
    MOVE(4);
    t0= values[i+0];
    t1= values[i+1];
    t2= values[i+2];
    t3= values[i+3];
  
    /* 1,2 */

    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc1_2+=EXPAND(huff_ltab1_2[t0+1][t1+1][t2+1][t3+1]);

    /* 5,6 */
    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc5_6+=EXPAND(huff_ltab5_6[t0+4][t1+4]);

    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc5_6+=EXPAND(huff_ltab5_6[t2+4][t3+4]);

    MISC(4);
    t0=abs(t0);
    t1=abs(t1);
    t2=abs(t2);
    t3=abs(t3);

    
    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc3_4+= EXPAND(huff_ltab3_4[t0][t1][t2][t3]);
    
    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc7_8+=EXPAND(huff_ltab7_8[t0][t1]);

    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc7_8+=EXPAND(huff_ltab7_8[t2][t3]);
    
    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc9_10+=EXPAND(huff_ltab9_10[t0][t1]);

    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc9_10+=EXPAND(huff_ltab9_10[t2][t3]);
    
    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc11+=EXPAND(huff_ltab11[t0][t1]);

    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc11+=EXPAND(huff_ltab11[t2][t3]);
   
    LOGIC(4); ADD(4);
    sc+=(t0>0)+(t1>0)+(t2>0)+(t3>0);
  }
  
  SHIFT(1); /* HI_EXPLTAB() */ STORE(1);
  bitCount[1]=HI_EXPLTAB(bc1_2);

  LOGIC(1); /* LO_EXPLTAB() */ STORE(1);
  bitCount[2]=LO_EXPLTAB(bc1_2);

  SHIFT(1); /* HI_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[3]=HI_EXPLTAB(bc3_4)+sc;

  LOGIC(1); /* LO_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[4]=LO_EXPLTAB(bc3_4)+sc;

  SHIFT(1); /* HI_EXPLTAB() */ STORE(1);
  bitCount[5]=HI_EXPLTAB(bc5_6);

  LOGIC(1); /* LO_EXPLTAB() */ STORE(1);
  bitCount[6]=LO_EXPLTAB(bc5_6);

  SHIFT(1); /* HI_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[7]=HI_EXPLTAB(bc7_8)+sc;

  LOGIC(1); /* LO_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[8]=LO_EXPLTAB(bc7_8)+sc;

  SHIFT(1); /* HI_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[9]=HI_EXPLTAB(bc9_10)+sc;

  LOGIC(1); /* LO_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[10]=LO_EXPLTAB(bc9_10)+sc;

  ADD(1); STORE(1);
  bitCount[11]=bc11+sc;
  
  COUNT_sub_end();
}


/*****************************************************************************

    functionname: count3_4_5_6_7_8_9_10_11
    description:  counts tables 3-11 
    returns:      
    input:        quantized spectrum
    output:       bitCount for tables 3-11

*****************************************************************************/

static void count3_4_5_6_7_8_9_10_11(const short *values,
                                     const int  width,
                                     int       *bitCount)
{

  int i;
  int bc3_4,bc5_6,bc7_8,bc9_10,bc11,sc;
  int t0,t1,t2,t3;
  
  COUNT_sub_start("count3_4_5_6_7_8_9_10_11");

  MOVE(6);
  bc3_4=0;
  bc5_6=0;
  bc7_8=0;
  bc9_10=0;
  bc11=0;
  sc=0;

  PTR_INIT(1); /* pointer for values[] */
  LOOP(1);
  for(i=0;i<width;i+=4){

    MOVE(4);
    t0= values[i+0];
    t1= values[i+1];
    t2= values[i+2];
    t3= values[i+3];
    
    /*
      5,6
    */
    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc5_6+=EXPAND(huff_ltab5_6[t0+4][t1+4]);

    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc5_6+=EXPAND(huff_ltab5_6[t2+4][t3+4]);

    MISC(4);
    t0=abs(t0);
    t1=abs(t1);
    t2=abs(t2);
    t3=abs(t3);


    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc3_4+= EXPAND(huff_ltab3_4[t0][t1][t2][t3]);
    
    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc7_8+=EXPAND(huff_ltab7_8[t0][t1]);

    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc7_8+=EXPAND(huff_ltab7_8[t2][t3]);
    
    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc9_10+=EXPAND(huff_ltab9_10[t0][t1]);

    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc9_10+=EXPAND(huff_ltab9_10[t2][t3]);
    
    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc11+=EXPAND(huff_ltab11[t0][t1]);

    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc11+=EXPAND(huff_ltab11[t2][t3]);

    LOGIC(4); ADD(4);
    sc+=(t0>0)+(t1>0)+(t2>0)+(t3>0);
      
   
  }

  MOVE(2);
  bitCount[1]=INVALID_BITCOUNT;
  bitCount[2]=INVALID_BITCOUNT;

  SHIFT(1); /* HI_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[3]=HI_EXPLTAB(bc3_4)+sc;

  LOGIC(1); /* LO_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[4]=LO_EXPLTAB(bc3_4)+sc;

  SHIFT(1); /* HI_EXPLTAB() */ STORE(1);
  bitCount[5]=HI_EXPLTAB(bc5_6);

  LOGIC(1); /* LO_EXPLTAB() */ STORE(1);
  bitCount[6]=LO_EXPLTAB(bc5_6);

  SHIFT(1); /* HI_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[7]=HI_EXPLTAB(bc7_8)+sc;

  LOGIC(1); /* LO_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[8]=LO_EXPLTAB(bc7_8)+sc;

  SHIFT(1); /* HI_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[9]=HI_EXPLTAB(bc9_10)+sc;

  LOGIC(1); /* LO_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[10]=LO_EXPLTAB(bc9_10)+sc;

  ADD(1); STORE(1);
  bitCount[11]=bc11+sc;
  
  COUNT_sub_end();
}



/*****************************************************************************

    functionname: count5_6_7_8_9_10_11
    description:  counts tables 5-11 
    returns:      
    input:        quantized spectrum
    output:       bitCount for tables 5-11

*****************************************************************************/


static void count5_6_7_8_9_10_11(const short *values,
                                 const int  width,
                                 int       *bitCount)
{

  int i;
  int bc5_6,bc7_8,bc9_10,bc11,sc;
  int t0,t1;

  COUNT_sub_start("count5_6_7_8_9_10_11");

  MOVE(5);
  bc5_6=0;
  bc7_8=0;
  bc9_10=0;
  bc11=0;
  sc=0;

  PTR_INIT(1); /* pointer for values[] */
  LOOP(1);
  for(i=0;i<width;i+=2){

    MOVE(2);
    t0 = values[i+0];
    t1 = values[i+1];

    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc5_6+=EXPAND(huff_ltab5_6[t0+4][t1+4]);

    MISC(2);
    t0=abs(t0);
    t1=abs(t1);
     
    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc7_8+=EXPAND(huff_ltab7_8[t0][t1]);

    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc9_10+=EXPAND(huff_ltab9_10[t0][t1]);

    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc11+=EXPAND(huff_ltab11[t0][t1]);
    
    LOGIC(2); ADD(2);
    sc+=(t0>0)+(t1>0);
  }

  MOVE(4);
  bitCount[1]=INVALID_BITCOUNT;
  bitCount[2]=INVALID_BITCOUNT;
  bitCount[3]=INVALID_BITCOUNT;
  bitCount[4]=INVALID_BITCOUNT;

  SHIFT(1); /* HI_EXPLTAB() */ STORE(1);
  bitCount[5]=HI_EXPLTAB(bc5_6);

  LOGIC(1); /* LO_EXPLTAB() */ STORE(1);
  bitCount[6]=LO_EXPLTAB(bc5_6);

  SHIFT(1); /* HI_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[7]=HI_EXPLTAB(bc7_8)+sc;

  LOGIC(1); /* LO_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[8]=LO_EXPLTAB(bc7_8)+sc;

  SHIFT(1); /* HI_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[9]=HI_EXPLTAB(bc9_10)+sc;

  LOGIC(1); /* LO_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[10]=LO_EXPLTAB(bc9_10)+sc;

  ADD(1); STORE(1);
  bitCount[11]=bc11+sc;
  
  COUNT_sub_end();
}


/*****************************************************************************

    functionname: count7_8_9_10_11
    description:  counts tables 7-11 
    returns:      
    input:        quantized spectrum
    output:       bitCount for tables 7-11

*****************************************************************************/

static void count7_8_9_10_11(const short *values,
                             const int  width,
                             int       *bitCount)
{

  int i;
  int bc7_8,bc9_10,bc11,sc;
  int t0,t1;

  COUNT_sub_start("count7_8_9_10_11");
  
  MOVE(4);
  bc7_8=0;
  bc9_10=0;
  bc11=0;
  sc=0;

  PTR_INIT(1); /* pointer for values[] */
  LOOP(1);
  for(i=0;i<width;i+=2){

    
    MISC(2);
    t0=abs(values[i+0]);
    t1=abs(values[i+1]);

    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc7_8+=EXPAND(huff_ltab7_8[t0][t1]);

    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc9_10+=EXPAND(huff_ltab9_10[t0][t1]);

    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc11+=EXPAND(huff_ltab11[t0][t1]);

    LOGIC(2); ADD(2);
    sc+=(t0>0)+(t1>0);
   
  }

  MOVE(6);
  bitCount[1]=INVALID_BITCOUNT;
  bitCount[2]=INVALID_BITCOUNT;
  bitCount[3]=INVALID_BITCOUNT;
  bitCount[4]=INVALID_BITCOUNT;
  bitCount[5]=INVALID_BITCOUNT;
  bitCount[6]=INVALID_BITCOUNT;

  SHIFT(1); /* HI_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[7]=HI_EXPLTAB(bc7_8)+sc;

  LOGIC(1); /* LO_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[8]=LO_EXPLTAB(bc7_8)+sc;

  SHIFT(1); /* HI_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[9]=HI_EXPLTAB(bc9_10)+sc;

  LOGIC(1); /* LO_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[10]=LO_EXPLTAB(bc9_10)+sc;

  ADD(1); STORE(1);
  bitCount[11]=bc11+sc;
  
  COUNT_sub_end();
}

/*****************************************************************************

    functionname: count9_10_11
    description:  counts tables 9-11 
    returns:      
    input:        quantized spectrum
    output:       bitCount for tables 9-11

*****************************************************************************/



static void count9_10_11(const short *values,
                         const int  width,
                         int       *bitCount)
{

  int i;
  int bc9_10,bc11,sc;
  int t0,t1;

  COUNT_sub_start("count9_10_11");

  MOVE(3);
  bc9_10=0;
  bc11=0;
  sc=0;

  PTR_INIT(1); /* pointer for values[] */
  LOOP(1);
  for(i=0;i<width;i+=2){


    MISC(2);
    t0=abs(values[i+0]);
    t1=abs(values[i+1]);
    

    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc9_10+=EXPAND(huff_ltab9_10[t0][t1]);

    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc11+=EXPAND(huff_ltab11[t0][t1]);

    LOGIC(2); ADD(2);
    sc+=(t0>0)+(t1>0);
   
  }

  MOVE(8);
  bitCount[1]=INVALID_BITCOUNT;
  bitCount[2]=INVALID_BITCOUNT;
  bitCount[3]=INVALID_BITCOUNT;
  bitCount[4]=INVALID_BITCOUNT;
  bitCount[5]=INVALID_BITCOUNT;
  bitCount[6]=INVALID_BITCOUNT;
  bitCount[7]=INVALID_BITCOUNT;
  bitCount[8]=INVALID_BITCOUNT;

  SHIFT(1); /* HI_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[9]=HI_EXPLTAB(bc9_10)+sc;

  LOGIC(1); /* LO_EXPLTAB() */ ADD(1); STORE(1);
  bitCount[10]=LO_EXPLTAB(bc9_10)+sc;

  ADD(1); STORE(1);
  bitCount[11]=bc11+sc;
  
  COUNT_sub_end();
}
 
/*****************************************************************************

    functionname: count11
    description:  counts table 11 
    returns:      
    input:        quantized spectrum
    output:       bitCount for table 11

*****************************************************************************/
 
static void count11(const short *values,
                    const int  width,
                    int        *bitCount)
{

  int i;
  int bc11,sc;
  int t0,t1;

  COUNT_sub_start("count11");

  MOVE(2);
  bc11=0;
  sc=0;

  PTR_INIT(1); /* pointer for values[] */
  LOOP(1); 
  for(i=0;i<width;i+=2){

    MISC(2);
    t0=abs(values[i+0]);
    t1=abs(values[i+1]);

    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc11+=EXPAND(huff_ltab11[t0][t1]);

    LOGIC(2); ADD(2);
    sc+=(t0>0)+(t1>0);
  }

  MOVE(10);
  bitCount[1]=INVALID_BITCOUNT;
  bitCount[2]=INVALID_BITCOUNT;
  bitCount[3]=INVALID_BITCOUNT;
  bitCount[4]=INVALID_BITCOUNT;
  bitCount[5]=INVALID_BITCOUNT;
  bitCount[6]=INVALID_BITCOUNT;
  bitCount[7]=INVALID_BITCOUNT;
  bitCount[8]=INVALID_BITCOUNT;
  bitCount[9]=INVALID_BITCOUNT;
  bitCount[10]=INVALID_BITCOUNT;

  ADD(1); STORE(1);
  bitCount[11]=bc11+sc;

  COUNT_sub_end();
}

/*****************************************************************************

    functionname: countEsc
    description:  counts table 11 (with Esc) 
    returns:      
    input:        quantized spectrum
    output:       bitCount for tables 11 (with Esc)

*****************************************************************************/

static void countEsc(const short *values,
                     const int  width,
                     int       *bitCount)
{

  int i;
  int bc11,ec,sc;
  int t0,t1,t00,t01;

  COUNT_sub_start("countEsc");

  MOVE(3);
  bc11=0;
  sc=0;
  ec=0;

  PTR_INIT(1); /* pointer for values[] */
  LOOP(1); 
  for(i=0;i<width;i+=2){

    MISC(2);
    t0=abs(values[i+0]);
    t1=abs(values[i+1]);
    
    LOGIC(2); ADD(2);
    sc+=(t0>0)+(t1>0);

    ADD(2); BRANCH(2); MOVE(2);
    t00 = min(t0,16);
    t01 = min(t1,16);

    SHIFT(1); LOGIC(3); /* EXPAND() */ INDIRECT(1); ADD(1);
    bc11+=EXPAND(huff_ltab11[t00][t01]);
    
    ADD(1); BRANCH(1);
    if(t0>=16){

      ADD(1);
      ec+=5;

      SHIFT(1); ADD(1); LOOP(1); /* while() condition */
      while((t0>>=1) >= 16)
      {
        ADD(1);
        ec+=2;
      }
    }
    
    ADD(1); BRANCH(1);
    if(t1>=16){

      ADD(1);
      ec+=5;

      SHIFT(1); ADD(1); LOOP(1); /* while() condition */
      while((t1>>=1) >= 16)
      {

        ADD(1);
        ec+=2;
      }
    }
  }

  MOVE(10);
  bitCount[1]=INVALID_BITCOUNT;
  bitCount[2]=INVALID_BITCOUNT;
  bitCount[3]=INVALID_BITCOUNT;
  bitCount[4]=INVALID_BITCOUNT;
  bitCount[5]=INVALID_BITCOUNT;
  bitCount[6]=INVALID_BITCOUNT;
  bitCount[7]=INVALID_BITCOUNT;
  bitCount[8]=INVALID_BITCOUNT;
  bitCount[9]=INVALID_BITCOUNT;
  bitCount[10]=INVALID_BITCOUNT;

  ADD(2); STORE(1);
  bitCount[11]=bc11+sc+ec;

  COUNT_sub_end();
}


typedef void (*COUNT_FUNCTION)(const short *values,
                               const int  width,
                               int       *bitCount);

static COUNT_FUNCTION countFuncTable[CODE_BOOK_ESC_LAV+1] =
{

 count1_2_3_4_5_6_7_8_9_10_11,  /* 0  */
 count1_2_3_4_5_6_7_8_9_10_11,  /* 1  */
 count3_4_5_6_7_8_9_10_11,      /* 2  */
 count5_6_7_8_9_10_11,          /* 3  */
 count5_6_7_8_9_10_11,          /* 4  */
 count7_8_9_10_11,              /* 5  */
 count7_8_9_10_11,              /* 6  */
 count7_8_9_10_11,              /* 7  */
 count9_10_11,                  /* 8  */
 count9_10_11,                  /* 9  */
 count9_10_11,                  /* 10 */
 count9_10_11,                  /* 11 */
 count9_10_11,                  /* 12 */
 count11,                       /* 13 */
 count11,                       /* 14 */
 count11,                       /* 15 */
 countEsc                       /* 16 */
};



int    bitCount(const short      *values,
                const int         width,
                      int         maxVal,
                int              *bitCount)
{

  /*
    check if we can use codebook 0
  */

  COUNT_sub_start("bitCount");

  BRANCH(1); MOVE(1);
  if(maxVal == 0)
    bitCount[0] = 0;
  else
    bitCount[0] = INVALID_BITCOUNT;

  ADD(1); BRANCH(1); MOVE(1);
  maxVal = min(maxVal,CODE_BOOK_ESC_LAV);

  INDIRECT(1); FUNC(3);
  countFuncTable[maxVal](values,width,bitCount);

  COUNT_sub_end();
  return(0);
}







int codeValues(short *values, int width, int codeBook,  HANDLE_BIT_BUF hBitstream)
{
  int i,t0,t1,t2,t3,t00,t01;
  int codeWord,codeLength;
  int sign,signLength;

  COUNT_sub_start("codeValues");


  BRANCH(2);
  switch(codeBook){

  case CODE_BOOK_ZERO_NO:
    break;

  case CODE_BOOK_1_NO:

    PTR_INIT(1); /* pointer for values[] */
    LOOP(1);
    for(i=0; i<width; i+=4) {

      MOVE(4);
      t0         = values[i+0];
      t1         = values[i+1];
      t2         = values[i+2];
      t3         = values[i+3];

      INDIRECT(1);
      codeWord   = huff_ctab1[t0+1][t1+1][t2+1][t3+1];

      SHIFT(1); /* HI_LTAB() */ INDIRECT(1);
      codeLength = HI_LTAB(huff_ltab1_2[t0+1][t1+1][t2+1][t3+1]);

      FUNC(3);
      WriteBits(hBitstream,codeWord,codeLength);        
    }
    break;

  case CODE_BOOK_2_NO:

    PTR_INIT(1); /* pointer for values[] */
    LOOP(1);
    for(i=0; i<width; i+=4) {

      MOVE(4);
      t0         = values[i+0];
      t1         = values[i+1];
      t2         = values[i+2];
      t3         = values[i+3];

      INDIRECT(1);
      codeWord   = huff_ctab2[t0+1][t1+1][t2+1][t3+1];

      LOGIC(1); /* LO_TAB() */ INDIRECT(1);
      codeLength = LO_LTAB(huff_ltab1_2[t0+1][t1+1][t2+1][t3+1]);

      FUNC(3);
      WriteBits(hBitstream,codeWord,codeLength);
      
    }
    break;

  case CODE_BOOK_3_NO:

    PTR_INIT(1); /* pointer for values[] */
    LOOP(1);
    for(i=0; i<width; i+=4) {

      MOVE(3);
      sign=0;
      signLength=0;
      t0 = values[i+0];

      BRANCH(1);
      if(t0 != 0){

        ADD(1);
        signLength++;

        SHIFT(1);
        sign<<=1;

        BRANCH(1);
        if(t0 < 0){

          LOGIC(1);
          sign|=1;

          MISC(1);
          t0=abs(t0);
        }
      }

      MOVE(1);
      t1 = values[i+1];

      BRANCH(1);
      if(t1 != 0){

        ADD(1);
        signLength++;

        SHIFT(1);
        sign<<=1;

        BRANCH(1);
        if(t1 < 0){

          LOGIC(1);
          sign|=1;

          MISC(1);
          t1=abs(t1);
        }
      }

      MOVE(1);
      t2 = values[i+2];

      BRANCH(1);
      if(t2 != 0){

        ADD(1);
        signLength++;

        SHIFT(1);
        sign<<=1;

        BRANCH(1);
        if(t2 < 0){

          LOGIC(1);
          sign|=1;

          MISC(1);
          t2=abs(t2);
        }
      }

      MOVE(1);
      t3 = values[i+3];

      BRANCH(1);
      if(t3 != 0){

        ADD(1);
        signLength++;

        SHIFT(1);
        sign<<=1;

        BRANCH(1);
        if(t3 < 0){

          LOGIC(1);
          sign|=1;

          MISC(1);
          t3=abs(t3);
        }
      }
      
      INDIRECT(1);
      codeWord   = huff_ctab3[t0][t1][t2][t3];

      SHIFT(1); /* HI_LTAB() */ INDIRECT(1);
      codeLength = HI_LTAB(huff_ltab3_4[t0][t1][t2][t3]);

      FUNC(3);
      WriteBits(hBitstream,codeWord,codeLength);

      FUNC(3);
      WriteBits(hBitstream,sign,signLength);
    }
    break;

  case CODE_BOOK_4_NO:

    PTR_INIT(1); /* pointer for values[] */
    LOOP(1);
    for(i=0; i<width; i+=4) {

      MOVE(3);
      sign=0;
      signLength=0;
      t0 = values[i+0];

      BRANCH(1);
      if(t0 != 0){

        ADD(1);
        signLength++;

        SHIFT(1);
        sign<<=1;

        BRANCH(1);
        if(t0 < 0){

          LOGIC(1);
          sign|=1;

          MISC(1);
          t0=abs(t0);
        }
      }

      MOVE(1);
      t1 = values[i+1];

      BRANCH(1);
      if(t1 != 0){

        ADD(1);
        signLength++;

        SHIFT(1);
        sign<<=1;

        BRANCH(1);
        if(t1 < 0){

          LOGIC(1);
          sign|=1;

          MISC(1);
          t1=abs(t1);
        }
      }

      MOVE(1);
      t2 = values[i+2];

      BRANCH(1);
      if(t2 != 0){

        ADD(1);
        signLength++;

        SHIFT(1);
        sign<<=1;

        BRANCH(1);
        if(t2 < 0){

          LOGIC(1);
          sign|=1;

          MISC(1);
          t2=abs(t2);
        }
      }

      MOVE(1);
      t3 = values[i+3];

      BRANCH(1);
      if(t3 != 0){

        ADD(1);
        signLength++;

        SHIFT(1);
        sign<<=1;

        BRANCH(1);
        if(t3 < 0){

          LOGIC(1);
          sign|=1;

          MISC(1);
          t3=abs(t3);
        }
      }

      INDIRECT(1);
      codeWord   = huff_ctab4[t0][t1][t2][t3];

      LOGIC(1); /* LO_TAB() */ INDIRECT(1);
      codeLength = LO_LTAB(huff_ltab3_4[t0][t1][t2][t3]);

      FUNC(3);
      WriteBits(hBitstream,codeWord,codeLength);

      FUNC(3);
      WriteBits(hBitstream,sign,signLength);
    }
    break;

  case CODE_BOOK_5_NO:

    PTR_INIT(1); /* pointer for values[] */
    LOOP(1);
    for(i=0; i<width; i+=2) {

      MOVE(2);
      t0         = values[i+0];
      t1         = values[i+1];

      INDIRECT(1);
      codeWord   = huff_ctab5[t0+4][t1+4];

      SHIFT(1); /* HI_LTAB() */ INDIRECT(1);
      codeLength = HI_LTAB(huff_ltab5_6[t0+4][t1+4]);

      FUNC(3);
      WriteBits(hBitstream,codeWord,codeLength);
    

    }
    break;

  case CODE_BOOK_6_NO:

    PTR_INIT(1); /* pointer for values[] */
    LOOP(1);
    for(i=0; i<width; i+=2) {

      MOVE(2);
      t0         = values[i+0];
      t1         = values[i+1];

      INDIRECT(1);
      codeWord   = huff_ctab6[t0+4][t1+4];

      LOGIC(1); /* LO_TAB() */ INDIRECT(1);
      codeLength = LO_LTAB(huff_ltab5_6[t0+4][t1+4]);

      FUNC(3);
      WriteBits(hBitstream,codeWord,codeLength);
      
    

    }
    break;

  case CODE_BOOK_7_NO:

    PTR_INIT(1); /* pointer for values[] */
    LOOP(1);
    for(i=0; i<width; i+=2){

      MOVE(3);
      sign=0;
      signLength=0;
      t0 = values[i+0];

      BRANCH(1);
      if(t0 != 0){

        ADD(1);
        signLength++;

        SHIFT(1);
        sign<<=1;

        BRANCH(1);
        if(t0 < 0){

          LOGIC(1);
          sign|=1;

          MISC(1);
          t0=abs(t0);
        }
      }
      
      MOVE(1);
      t1 = values[i+1];

      BRANCH(1);
      if(t1 != 0){

        ADD(1);
        signLength++;

        SHIFT(1);
        sign<<=1;

        BRANCH(1);
        if(t1 < 0){

          LOGIC(1);
          sign|=1;

          MISC(1);
          t1=abs(t1);
        }
      }

      INDIRECT(1);
      codeWord   = huff_ctab7[t0][t1];

      SHIFT(1); /* HI_LTAB() */ INDIRECT(1);
      codeLength = HI_LTAB(huff_ltab7_8[t0][t1]);

      FUNC(3);
      WriteBits(hBitstream,codeWord,codeLength);

      FUNC(3);
      WriteBits(hBitstream,sign,signLength);
    }
    break;

  case CODE_BOOK_8_NO:

    PTR_INIT(1); /* pointer for values[] */
    LOOP(1);
    for(i=0; i<width; i+=2) {

      MOVE(3);
      sign=0;
      signLength=0;
      t0 = values[i+0];

      BRANCH(1);
      if(t0 != 0){

        ADD(1);
        signLength++;

        SHIFT(1);
        sign<<=1;

        BRANCH(1);
        if(t0 < 0){

          LOGIC(1);
          sign|=1;

          MISC(1);
          t0=abs(t0);
        }
      }
      
      MOVE(1);
      t1 = values[i+1];

      BRANCH(1);
      if(t1 != 0){

        ADD(1);
        signLength++;

        SHIFT(1);
        sign<<=1;

        BRANCH(1);
        if(t1 < 0){

          LOGIC(1);
          sign|=1;

          MISC(1);
          t1=abs(t1);
        }
      }

      INDIRECT(1);
      codeWord   = huff_ctab8[t0][t1];

      LOGIC(1); /* LO_TAB() */ INDIRECT(1);
      codeLength = LO_LTAB(huff_ltab7_8[t0][t1]);

      FUNC(3);
      WriteBits(hBitstream,codeWord,codeLength);

      FUNC(3);
      WriteBits(hBitstream,sign,signLength);
    }
    break;

  case CODE_BOOK_9_NO:

    PTR_INIT(1); /* pointer for values[] */
    LOOP(1);
    for(i=0; i<width; i+=2) {

      MOVE(3);
      sign=0;
      signLength=0;
      t0 = values[i+0];

      BRANCH(1);
      if(t0 != 0){

        ADD(1);
        signLength++;

        SHIFT(1);
        sign<<=1;

        BRANCH(1);
        if(t0 < 0){

          LOGIC(1);
          sign|=1;

          MISC(1);
          t0=abs(t0);
        }
      }

      MOVE(1);
      t1 = values[i+1];

      BRANCH(1);
      if(t1 != 0){

        ADD(1);
        signLength++;

        SHIFT(1);
        sign<<=1;

        BRANCH(1);
        if(t1 < 0){

          LOGIC(1);
          sign|=1;

          MISC(1);
          t1=abs(t1);
        }
      }

      INDIRECT(1);
      codeWord   = huff_ctab9[t0][t1];
    
      SHIFT(1); /* HI_LTAB() */ INDIRECT(1);
      codeLength = HI_LTAB(huff_ltab9_10[t0][t1]);

      FUNC(3);
      WriteBits(hBitstream,codeWord,codeLength);

      FUNC(3);
      WriteBits(hBitstream,sign,signLength);
    }
    break;

  case CODE_BOOK_10_NO:

    PTR_INIT(1); /* pointer for values[] */
    LOOP(1);
    for(i=0; i<width; i+=2) {

      MOVE(3);
      sign=0;
      signLength=0;
      t0 = values[i+0];

      BRANCH(1);
      if(t0 != 0){

        ADD(1);
        signLength++;

        SHIFT(1);
        sign<<=1;

        BRANCH(1);
        if(t0 < 0){

          LOGIC(1);
          sign|=1;

          MISC(1);
          t0=abs(t0);
        }
      }
      
      MOVE(1);
      t1 = values[i+1];

      BRANCH(1);
      if(t1 != 0){

        ADD(1);
        signLength++;

        SHIFT(1);
        sign<<=1;

        BRANCH(1);
        if(t1 < 0){

          LOGIC(1);
          sign|=1;

          MISC(1);
          t1=abs(t1);
        }
      }

      INDIRECT(1);
      codeWord   = huff_ctab10[t0][t1];

      LOGIC(1); /* LO_TAB() */ INDIRECT(1);
      codeLength = LO_LTAB(huff_ltab9_10[t0][t1]);

      FUNC(3);
      WriteBits(hBitstream,codeWord,codeLength);

      FUNC(3);
      WriteBits(hBitstream,sign,signLength);
    }
    break;

  case CODE_BOOK_ESC_NO:

    PTR_INIT(1); /* pointer for values[] */
    LOOP(1);
    for(i=0; i<width; i+=2) {

      MOVE(3);
      sign=0;
      signLength=0;
      t0 = values[i+0];

      BRANCH(1);
      if(t0 != 0){

        ADD(1);
        signLength++;

        SHIFT(1);
        sign<<=1;

        BRANCH(1);
        if(t0 < 0){

          LOGIC(1);
          sign|=1;

          MISC(1);
          t0=abs(t0);
        }
      }

      MOVE(1);
      t1 = values[i+1];

      BRANCH(1);
      if(t1 != 0){

        ADD(1);
        signLength++;

        SHIFT(1);
        sign<<=1;

        BRANCH(1);
        if(t1 < 0){

          LOGIC(1);
          sign|=1;

          MISC(1);
          t1=abs(t1);
        }
      }

      ADD(2); BRANCH(2); MOVE(2);
      t00 = min(t0,16);
      t01 = min(t1,16);
      
      INDIRECT(2); MOVE(2);
      codeWord   = huff_ctab11[t00][t01];
      codeLength = huff_ltab11[t00][t01];

      FUNC(3);
      WriteBits(hBitstream,codeWord,codeLength);

      FUNC(3);
      WriteBits(hBitstream,sign,signLength);

      ADD(1); BRANCH(1);
      if(t0 >=16){
        int n,p;

        MOVE(2);
        n=0;
        p=t0;

        SHIFT(1); ADD(1); LOOP(1);
        while((p>>=1) >=16){

          FUNC(3);
          WriteBits(hBitstream,1,1);

          ADD(1);
          n++;
        }

        FUNC(3);
        WriteBits(hBitstream,0,1);

        ADD(2); SHIFT(1); FUNC(3);
        WriteBits(hBitstream,t0-(1<<(n+4)),n+4);
      }
      if(t1 >=16){
        int n,p;

        MOVE(2);
        n=0;
        p=t1;

        SHIFT(1); ADD(1); LOOP(1);
        while((p>>=1) >=16){

          FUNC(3);
          WriteBits(hBitstream,1,1);

          ADD(1);
          n++;
        }

        FUNC(3);
        WriteBits(hBitstream,0,1);

        ADD(2); SHIFT(1); FUNC(3);
        WriteBits(hBitstream,t1-(1<<(n+4)),n+4);
      }
    }
    break;
  
  default:
    break;
  }

  COUNT_sub_end();

  return(0);
}

int bitCountScalefactorDelta(signed int delta)
{
   COUNT_sub_start("bitCountScalefactorDelta");
   MOVE(1);
   COUNT_sub_end();

  return(huff_ltabscf[delta+CODE_BOOK_SCF_LAV]);
}

int codeScalefactorDelta(signed int delta, HANDLE_BIT_BUF hBitstream)
{

  int codeWord,codeLength;

  COUNT_sub_start("codeScalefactorDelta");
  
  MISC(1); ADD(1); BRANCH(1);
  if(labs(delta) >CODE_BOOK_SCF_LAV)
  {
    COUNT_sub_end();
    return(1);
  }
  
  INDIRECT(2); MOVE(2);
  codeWord   = huff_ctabscf[delta+CODE_BOOK_SCF_LAV];
  codeLength = huff_ltabscf[delta+CODE_BOOK_SCF_LAV];

  FUNC(3);
  WriteBits(hBitstream,codeWord,codeLength);

  COUNT_sub_end();

  return(0);
}



