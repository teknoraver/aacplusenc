####################################################################
#
# Makefile for SBR encoder - library
#
####################################################################
################## user section: insert objlist here #######
LIB=libsbrenc.a

SRCS=code_env.c \
	env_est.c \
	env_bit.c \
	fram_gen.c \
	freq_sca.c \
	nf_est.c \
	qmf_enc.c \
	tran_det.c \
	mh_det.c \
	sbr_rom.c \
	sbr_ram.c \
	invf_est.c \
	sbr_misc.c \
	bit_sbr.c \
	sbr_main.c \
	ton_corr.c \
	ps_bitenc.c \
	ps_enc.c \
	hybrid.c

OBJS = $(SRCS:.c=.o)

$(LIB): $(OBJS)
	ar r $@ $(OBJS)

all: $(LIB)

clean:
	rm -f $(OBJS) $(LIB)
