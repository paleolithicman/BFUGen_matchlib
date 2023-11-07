#ifndef _COMMON_
#define _COMMON_

#include <systemc.h>
#include <iostream>
#include <fstream>
#include "../include/primate_stream.h"
#include "../include/primate_ctrl.h"
#include "../include/primate_bfu_mc.h"
#include "../include/primate_bfu.h"

#define NUM_THREADS 16
#define NUM_THREADS_LG 4
#define REG_WIDTH 266
#define NUM_REGS_LG 5
#define OPCODE_WIDTH 6
#define IP_WIDTH 7

typedef primate_ctrl<NUM_THREADS_LG, OPCODE_WIDTH, NUM_REGS_LG, REG_WIDTH> primate_ctrl_iu;
typedef primate_bfu_mc::write_mc<NUM_THREADS_LG, IP_WIDTH, NUM_REGS_LG, REG_WIDTH> primate_bfu_iu;
typedef primate_stream<cfg_biguint<272, 4, 34>> primate_stream_272_4;
typedef primate_stream_272_4::payload_t primate_io_payload_t;
typedef primate_bfu::bfu_in<NUM_THREADS_LG, OPCODE_WIDTH, 12, REG_WIDTH> bfu_in;
typedef primate_bfu::bfu_out<NUM_THREADS_LG, IP_WIDTH, REG_WIDTH> bfu_out;
typedef primate_bfu::bfu_out<NUM_THREADS_LG, IP_WIDTH, 518> bfu518_out;
typedef bfu_in::payload_t bfu_in_pl_t;
typedef bfu_out::payload_t bfu_out_pl_t;
typedef bfu518_out::payload_t bfu518_out_pl_t;


#endif