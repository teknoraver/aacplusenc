####################################################################
#
# Makefile for aac encoder - library
#
####################################################################
################## user section: insert objlist here #######
LIB = libaacenc.a

SRCS =  adj_thr.c \
	band_nrg.c \
	block_switch.c \
	grp_data.c \
	interface.c \
	ms_stereo.c \
	pre_echo_control.c \
	psy_configuration.c \
	psy_main.c \
	spreading.c \
	tns.c \
	tns_param.c \
	transform.c \
	adj_thr.c \
	dyn_bits.c \
	line_pe.c \
	qc_main.c \
	quantize.c \
	sf_estim.c \
	stat_bits.c \
	bit_cnt.c \
	bitenc.c \
	aacenc.c \
	stprepro.c \
	channel_map.c \
	aac_ram.c \
	aac_rom.c

OBJS = $(SRCS:.c=.o)

$(LIB): $(OBJS)
	ar r $@ $(OBJS)

all: $(LIB)

clean:
	rm -f $(OBJS) $(LIB)
