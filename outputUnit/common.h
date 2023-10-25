#ifndef _COMMON_
#define _COMMON_

#include <systemc.h>
#include <iostream>
#include <fstream>
#include "../include/primate_stream.h"
#include "../include/primate_ctrl.h"
#include "../include/primate_bfu_mc.h"

#define NUM_THREADS 16
#define NUM_THREADS_LG 4
#define REG_WIDTH 192
#define NUM_REGS_LG 5
#define OPCODE_WIDTH 6
#define IP_WIDTH 32

typedef primate_ctrl<NUM_THREADS_LG, OPCODE_WIDTH, NUM_REGS_LG, 32> primate_ctrl_ou;
typedef primate_bfu_mc::write_mc<NUM_THREADS_LG, IP_WIDTH, NUM_REGS_LG, REG_WIDTH> primate_bfu_iu;
typedef primate_bfu_mc::out_simple<NUM_THREADS_LG, IP_WIDTH> primate_bfu_ou;
typedef primate_bfu_mc::read_mc::req<NUM_THREADS_LG, NUM_REGS_LG> primate_bfu_rdreq_ou;
typedef primate_bfu_mc::read_mc::rsp<REG_WIDTH> primate_bfu_rdrsp_ou;
typedef primate_bfu_mc::read_mc::req<NUM_THREADS_LG, NUM_REGS_LG>::reqPayload_t primate_bfu_req_t;
typedef primate_bfu_mc::read_mc::rsp<REG_WIDTH>::rspPayload_t primate_bfu_rsp_t;
typedef primate_stream_512_4::payload_t primate_io_payload_t;


// struct pkt_buf_t {
//     sc_biguint<512>         data;
//     sc_uint<NUM_THREADS_LG> tag;
//     sc_uint<7>              empty;
//     bool                    last;

//     pkt_buf_t(sc_biguint<512> data = 0, sc_uint<NUM_THREADS_LG> tag = 0, sc_uint<7> empty = 0, bool last = true) : 
//         data(data), tag(tag), empty(empty), last(last) {}

//     pkt_buf_t& operator= (const pkt_buf_t& arg) {
//         data = arg.data;
//         tag = arg.tag;
//         empty = arg.empty;
//         last = arg.last;
//         return (*this);
//     }

//     bool operator== (const pkt_buf_t& arg) const {
//         return ((data == arg.data) && (tag == arg.tag) &&
//             (empty == arg.empty) && (last == arg.last));
//     }
// };

// inline std::ostream& operator<<(std::ostream& os, const pkt_buf_t& val) {
//     os << "data = " << val.data << "; tag = " << val.tag << 
//         "; empty = " << val.empty << "; last = " << val.last << std::endl;
//     return os;
// }

// inline void sc_trace(sc_trace_file*& f, const pkt_buf_t& val, std::string name) {
//     sc_trace(f, val.data, name + ".data");
//     sc_trace(f, val.tag, name + ".tag");
//     sc_trace(f, val.empty, name + ".empty");
//     sc_trace(f, val.last, name + ".last");
// }

#endif