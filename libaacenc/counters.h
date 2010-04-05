/*
  Provides dummy functions/macros to be able to compile the code.
  It is planned to provide a complexity measuring tool that allows
  to actually exploit the instrumentation lateron.
 */

#ifndef COUNT_H
#define COUNT_H

#include <stdio.h>

#define ADD(c)
#define MULT(c)
#define MAC(c)
#define MOVE(c)
#define STORE(c)
#define LOGIC(c)
#define SHIFT(c)
#define BRANCH(c)
#define DIV(c)
#define SQRT(c)
#define TRANS(c)
#define FUNC(c)
#define LOOP(c)
#define INDIRECT(c)
#define PTR_INIT(c)
#define MISC(c)

/* external function prototypes */
#define COUNT_init()
#define COUNT_end()
#define COUNT_sub_start(name) 
#define COUNT_sub_end()
#define COUNT_frame_update()
#define COUNT_ops(op, count)
#define COUNT_mem(op, count)

#endif
