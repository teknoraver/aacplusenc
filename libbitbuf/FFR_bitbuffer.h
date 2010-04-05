/*
  Bit Buffer Management
*/


#ifndef FFR_BITBUFFER_H
#define FFR_BITBUFFER_H

enum direction
{
  forwardDirection,
  backwardDirection
};

enum
{
  ByteSize = 8,
  ShortSize = ByteSize*sizeof(short),
  LongSize = ByteSize*sizeof(long),
  Log2ByteSize = 2+sizeof(char)
};

struct BIT_BUF
{
  unsigned char  *pBitBufBase;            /* pointer points to first position in bitstream buffer */
  unsigned char  *pBitBufEnd;             /* pointer points to last position in bitstream buffer */

  unsigned char  *pReadNext;              /* pointer points to next available word in bitstream buffer to read */
  unsigned char  *pWriteNext;             /* pointer points to next available word in bitstream buffer to write */

  int             rBitPos;                /* 7 <= rBitPos <= 0 */
  int             wBitPos;                /* 7 <= wBitPos <= 0 */

  int             cntBits;                /* number of available bits in the bitstream buffer
                                             write bits to bitstream buffer  => increment cntBits
                                             read bits from bitstream buffer => decrement cntBits */
  int             size;                   /* size of bitbuffer in bits */
  int             isValid;                /* indicates whether the instance has been initialized */
};

/* Define pointer to bit buffer structure */
typedef struct BIT_BUF *HANDLE_BIT_BUF;


HANDLE_BIT_BUF CreateBitBuffer(HANDLE_BIT_BUF hBitBuf,
                               unsigned char *pBitBufBase,
                               unsigned int bitBufSize);

void DeleteBitBuffer(HANDLE_BIT_BUF *hBitBuf);

unsigned long ReadBits(HANDLE_BIT_BUF hBitBuf,
                       unsigned char noBitsToRead);

unsigned char  WriteBits(HANDLE_BIT_BUF hBitBuf,
               unsigned long writeValue,
               unsigned char noBitsToWrite);

void ResetBitBuf(HANDLE_BIT_BUF hBitBuf,
                 unsigned char *pBitBufBase,
                 unsigned int bitBufSize);

void CopyBitBuf(HANDLE_BIT_BUF hBitBufSrc,
                HANDLE_BIT_BUF hBitBufDst);

void WindBitBufferBidirectional(HANDLE_BIT_BUF hBitBuf,
                                long offset);

int GetBitsAvail(HANDLE_BIT_BUF hBitBuf);

#endif /* FFR_BITBUFFER_H */
