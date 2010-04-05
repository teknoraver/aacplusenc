/*
  Bit Buffer Management
*/

#include "stdio.h"
#include "assert.h"
#include "FFR_bitbuffer.h"
#include "FloatFR.h"

#include "counters.h" /* the 3GPP instrumenting tool */

/*
  The pointer will be incremented if the parameter cnt is positive. Otherwise,
  the read pointer will be decremented.

  return  none
*/
static void updateBitBufWordPtr(HANDLE_BIT_BUF hBitBuf,       /* handle to bit buffer structure */
                                unsigned char **pBitBufWord,  /* pointer to pointer to bitsteam buffer */
                                long cnt)                     /* number of words */
{
  COUNT_sub_start("updateBitBufWordPtr");

  ADD(1); STORE(1);
  *pBitBufWord += cnt;
   
  PTR_INIT(2); /* hBitBuf->pBitBufEnd
                  hBitBuf->pBitBufBase
               */

  ADD(1); BRANCH(1);
  if(*pBitBufWord > hBitBuf->pBitBufEnd) {

    ADD(3); STORE(1);
    *pBitBufWord -= (hBitBuf->pBitBufEnd - hBitBuf->pBitBufBase + 1);
  }

  ADD(1); BRANCH(1);
  if(*pBitBufWord < hBitBuf->pBitBufBase) {

    ADD(3); STORE(1);
    *pBitBufWord += (hBitBuf->pBitBufEnd - hBitBuf->pBitBufBase + 1);
  }

  COUNT_sub_end();
}


/*
  Creates and initializes a bit buffer instance;

  returns pointer to bit buffer instance
*/
HANDLE_BIT_BUF CreateBitBuffer(HANDLE_BIT_BUF hBitBuf,      /* handle to bit buffer structure */
                               unsigned char *pBitBufBase,  /* pointer to bitstream buffer */
                               unsigned int bitBufSize)     /* size of bitstream buffer in words */
{
  COUNT_sub_start("CreateBitBuffer");

  INDIRECT(1); PTR_INIT(1);
  hBitBuf->pBitBufBase = pBitBufBase;

  INDIRECT(1); ADD(2); STORE(1);
  hBitBuf->pBitBufEnd = pBitBufBase+bitBufSize-1;

  INDIRECT(2); PTR_INIT(2);
  hBitBuf->pReadNext = pBitBufBase;
  hBitBuf->pWriteNext = pBitBufBase;

  INDIRECT(2); MOVE(2);
  hBitBuf->wBitPos = 7;
  hBitBuf->rBitPos = 7;
  
  INDIRECT(1); MOVE(1);
  hBitBuf->cntBits = 0;

  INDIRECT(1); MULT(1); STORE(1);
  hBitBuf->size    = bitBufSize * 8;

  INDIRECT(1); MOVE(1);
  hBitBuf->isValid = 1;

  COUNT_sub_end();

  return hBitBuf;
}


/*
  Deletes a bit buffer instance
*/
void DeleteBitBuffer(HANDLE_BIT_BUF *hBitBuf)  /* handle to pointer to bit buffer structure */
{
  COUNT_sub_start("DeleteBitBuffer");

  INDIRECT(1); MOVE(1);
  (*hBitBuf)->isValid = 0;

  PTR_INIT(1);
  *hBitBuf = NULL;

  COUNT_sub_end();
}

/*
  Resets elements of a bit buffer instance
*/
void ResetBitBuf(HANDLE_BIT_BUF hBitBuf,      /* handle to bit buffer structure */
                 unsigned char *pBitBufBase,  /* pointer to bitstream buffer */
                 unsigned int bitBufSize)     /* size of bitstream buffer in words */
{
  COUNT_sub_start("ResetBitBuf");

  INDIRECT(1); PTR_INIT(1);
  hBitBuf->pBitBufBase = pBitBufBase;

  INDIRECT(1); ADD(2); STORE(1);
  hBitBuf->pBitBufEnd = pBitBufBase+bitBufSize-1;

  INDIRECT(2); PTR_INIT(2);
  hBitBuf->pReadNext = pBitBufBase;
  hBitBuf->pWriteNext = pBitBufBase;

  INDIRECT(2); MOVE(2);
  hBitBuf->rBitPos = 7;
  hBitBuf->wBitPos = 7;
    
  INDIRECT(1); MOVE(1);
  hBitBuf->cntBits = 0;

  COUNT_sub_end();
}


/*
  Copy source bit buffer instance to destination bit buffer instance
*/
void CopyBitBuf(HANDLE_BIT_BUF hBitBufSrc,  /* handle to source bit buffer structure */
                HANDLE_BIT_BUF hBitBufDst)  /* handle to destination bit buffer structure */
{
  int i;
  int bytesToGoSrc = (hBitBufSrc->pBitBufEnd - hBitBufSrc->pBitBufBase);
  int bytesToGoDst = (hBitBufDst->pBitBufEnd - hBitBufDst->pBitBufBase);

  COUNT_sub_start("CopyBitBuf");

  INDIRECT(4); ADD(2); /* counting previous operations */

  assert (bytesToGoSrc==bytesToGoDst);

  PTR_INIT(2); /* hBitBufDst->pBitBufBase[]
                  hBitBufSrc->pBitBufBase[]
               */
  LOOP(1);
  for (i=0; i<bytesToGoSrc;i++)
  {
    MOVE(1);
    hBitBufDst->pBitBufBase[i] = hBitBufSrc->pBitBufBase[i];
  }

  INDIRECT(4); MOVE(2);
  hBitBufDst->pReadNext  = hBitBufSrc->pReadNext;
  hBitBufDst->pWriteNext = hBitBufSrc->pWriteNext;

  INDIRECT(4); MOVE(2);
  hBitBufDst->rBitPos    = hBitBufSrc->rBitPos;
  hBitBufDst->wBitPos    = hBitBufSrc->wBitPos;
  
  INDIRECT(2); MOVE(1);
  hBitBufDst->cntBits    = hBitBufSrc->cntBits;

  INDIRECT(2); MOVE(1);
  hBitBufDst->isValid    = hBitBufSrc->isValid;

  COUNT_sub_end();
}


int GetBitsAvail(HANDLE_BIT_BUF hBitBuf)
{
  return hBitBuf->cntBits;
}
/*
  Read a certain number of bits from the bitstream buffer. The read direction is from left to right.
  The element cntBits will be decremented with the number of read bits. The element rBitPos will
  be set to the new "bit position"

  returns number of bits read
*/
unsigned long ReadBits(HANDLE_BIT_BUF hBitBuf,      /* handle to bit buffer structure */
                       unsigned char noBitsToRead)  /* number of bits to read */
{
  unsigned long returnValue;

  COUNT_sub_start("ReadBits");

  /* return value is of type unsigned int, it can hold up to 32 bits 
     this optimized code can read upto 25 Bits a time*/
  ADD(1); BRANCH(1);
  if (noBitsToRead >= 25) {
    COUNT_sub_end();
    return 0;
  }

 
  INDIRECT(2); ADD(2); STORE(2);
  hBitBuf->cntBits -= noBitsToRead;
  hBitBuf->rBitPos -= noBitsToRead;

  PTR_INIT(4); /* hBitBuf->rBitPos
                  hBitBuf->pReadNext
                  hBitBuf->pBitBufEnd
                  hBitBuf->pBitBufBase
               */

  MOVE(1);
  returnValue = (unsigned long)*hBitBuf->pReadNext;
  
  LOOP(1);
  while (hBitBuf->rBitPos < 0)
  {
    ADD(2); STORE(2);
    hBitBuf->rBitPos += 8;
    hBitBuf->pReadNext++;

    ADD(1); BRANCH(1);
    if(hBitBuf->pReadNext > hBitBuf->pBitBufEnd) {

      MOVE(1);
      hBitBuf->pReadNext = hBitBuf->pBitBufBase;
    }

    SHIFT(1);
    returnValue <<= 8;

    LOGIC(1);
    returnValue |= (unsigned long)*hBitBuf->pReadNext;
  } 
    
  ADD(3); SHIFT(2);
  returnValue = returnValue << (31 - noBitsToRead - hBitBuf->rBitPos) >> (32 - noBitsToRead);

  COUNT_sub_end();
  return (returnValue);
}


/*
  Write a certain number of bits to the bitstream buffer. The write direction is from left to right.
  The element cntBits will be incremented with the number of written bits. It is actually irrelevant
  if the bits are really written to the bitstream buffer or only to the wCache.

  returns number of bits to write
*/
unsigned char WriteBits(HANDLE_BIT_BUF hBitBuf,       /* handle to bit buffer structure */
                        unsigned long writeValue,     /* write bits in word right bounded */
                        unsigned char noBitsToWrite)  /* number of bits to write */
{
  int bitsToWrite;
  unsigned char bitsWritten = noBitsToWrite;

  assert(noBitsToWrite <= LongSize);

  COUNT_sub_start("WriteBits");

  MOVE(1); /* counting previous operation */

  INDIRECT(1); ADD(1); STORE(1);
  hBitBuf->cntBits += noBitsToWrite;

  /* Bit Buffer Management: do not write more bits to input buffer than possible */
  assert ( hBitBuf->cntBits <= (hBitBuf->pBitBufEnd - hBitBuf->pBitBufBase + 1) * 8);
  
  PTR_INIT(4); /* hBitBuf->wBitPos
                  hBitBuf->pWriteNext
                  hBitBuf->pBitBufEnd
                  hBitBuf->pBitBufBase
               */
  LOOP(1);
  while (noBitsToWrite) {
    unsigned char tmp,msk;

    ADD(2); BRANCH(1); MOVE(1);
    bitsToWrite = min(hBitBuf->wBitPos + 1, noBitsToWrite);
	
    ADD(4); SHIFT(3);
    tmp = (unsigned char) ( writeValue << (32 - noBitsToWrite) >> (32 - bitsToWrite) << (hBitBuf->wBitPos + 1 - bitsToWrite) );

    ADD(1); SHIFT(2); LOGIC(1); /* (hBitBuf->wBitPos + 1 - bitsToWrite) --> already calculated */
    msk = ~(((1 << bitsToWrite) - 1) << (hBitBuf->wBitPos + 1 - bitsToWrite));

    LOGIC(2); STORE(1);
    *hBitBuf->pWriteNext &= msk;
    *hBitBuf->pWriteNext |= tmp;

    ADD(1); STORE(1);
    hBitBuf->wBitPos -= bitsToWrite;

    ADD(1);
    noBitsToWrite    -= bitsToWrite;

    BRANCH(1);
    if (hBitBuf->wBitPos < 0) {

      ADD(2); STORE(2);
      hBitBuf->wBitPos += 8;
      hBitBuf->pWriteNext++;

      ADD(1); BRANCH(1);
      if (hBitBuf->pWriteNext > hBitBuf->pBitBufEnd) {

        MOVE(1);
        hBitBuf->pWriteNext= hBitBuf->pBitBufBase;
      }
    }

  }

  COUNT_sub_end();

  return(bitsWritten);
}


/*
  The read pointer will be winded a certain number of bits in forward or backward direction. The forward direction
  is chosen if the offset is positive, and the backward direction is chosen of the offset is negative. The pointer
  pReadNext and the elements rCache and rBitsLeft will be updated. The element cntBits will be decremented if the
  read pointer is updated in forward direction, and incremented if the read pointer is updated in backward direction.
*/
void WindBitBufferBidirectional(HANDLE_BIT_BUF hBitBuf,  /* handle to bit buffer structure */
                                long offset)             /* positive number => wind offset in forward direction
                                                            negative number => wind offset in backward direction */
{
  COUNT_sub_start("WindBitBufferBidirectional");

  BRANCH(1);
  if (offset != 0)
  {
    int bOffset;
    
    INDIRECT(1); ADD(1); STORE(1);
    hBitBuf->rBitPos -= offset;

    SHIFT(1);
    bOffset           = hBitBuf->rBitPos >> 3;

    SHIFT(1); ADD(1); STORE(1);
    hBitBuf->rBitPos -= bOffset << 3;

    BRANCH(1);
    if (bOffset) {

      MULT(1); INDIRECT(1); PTR_INIT(1); FUNC(3);
      updateBitBufWordPtr(hBitBuf, &hBitBuf->pReadNext, -bOffset);
    }

    INDIRECT(1); ADD(1); STORE(1);
    hBitBuf->cntBits -= offset;
  }

  COUNT_sub_end();
}
