#include "common.h"

struct ethernet_t {
    sc_biguint<48> dstAddr;
    sc_biguint<48> srcAddr;
    sc_biguint<16> etherType;

    void set(sc_biguint<112> bv) {
        dstAddr = bv.range(47, 0);
        srcAddr = bv.range(95, 48);
        etherType = bv.range(111, 96);
    }

    sc_biguint<192> to_uint() {
        sc_biguint<192> val = (0, etherType, srcAddr, dstAddr);
        return val;
    }
};

struct ptp_l_t {
    sc_biguint<40> transportSpecific_domainNumber;
    sc_biguint<8> reserved2;
    sc_biguint<112> flags_reserved3;

    void set(sc_biguint<160> bv) {
        transportSpecific_domainNumber = bv.range(39, 0);
        reserved2 = bv.range(47, 40);
        flags_reserved3 = bv.range(159, 48);
    }

    sc_biguint<192> to_uint() {
        sc_biguint<192> val = (0, flags_reserved3, reserved2, transportSpecific_domainNumber);
        return val;
    }
};

struct header_t {
    sc_biguint<16> field_0;
    sc_biguint<48> field_1_field_3;

    void set(sc_biguint<64> bv) {
        field_0 = bv.range(15, 0);
        field_1_field_3 = bv.range(63, 16);
    }

    sc_biguint<192> to_uint() {
        sc_biguint<192> val = (0, field_1_field_3, field_0);
        return val;
    }
};

struct arg_t {
    sc_biguint<32> hdr_count;

    void set(sc_biguint<32> bv) {
        hdr_count = bv.range(31, 0);
    }

    sc_biguint<32> to_uint() {
        sc_biguint<32> val = hdr_count;
        return val;
    }
};

SC_MODULE(outputUnit) {
    sc_in<bool> i_clk;
    sc_in<bool> i_rst;

    sc_signal<sc_uint<IP_WIDTH>> bt0;
    sc_signal<sc_uint<IP_WIDTH>> bt1;
    sc_signal<sc_uint<IP_WIDTH>> bt2;

    primate_stream_512_4::master<>    CCS_INIT_S1(stream_out);

    primate_ctrl_ou::slave<>          CCS_INIT_S1(cmd_in);

    primate_bfu_ou::master<>          CCS_INIT_S1(bfu_out);

    primate_bfu_rdreq_ou::master<>    CCS_INIT_S1(bfu_rdreq);
    primate_bfu_rdrsp_ou::slave<>     CCS_INIT_S1(bfu_rdrsp);

    primate_stream_512_4::slave<>     CCS_INIT_S1(pkt_buf_in);

    sc_signal<bool> init_done;
    sc_signal<bool> init_wb;
    sc_signal<sc_uint<NUM_THREADS_LG>> init_tag;
    sc_signal<bool> done;
    sc_signal<sc_uint<NUM_THREADS_LG>> done_tag;
    sc_fifo<sc_uint<NUM_THREADS_LG>> req_rsp_fifo;
    sc_signal<sc_uint<1>> hdr_done[NUM_THREADS];
    sc_signal<sc_biguint<32>> hdr_arg[NUM_THREADS];

    sc_uint<9> port;

    void outputUnit_cmd();
    void outputUnit_req();
    inline void outputUnit_req_core(sc_biguint<32> hdr_count, sc_uint<NUM_THREADS_LG> tag);
    void outputUnit_rsp();
    inline void outputUnit_rsp_core(sc_biguint<32> hdr_count, sc_uint<NUM_THREADS_LG> tag);

    SC_CTOR(outputUnit) : req_rsp_fifo(2) {
        SC_CTHREAD(outputUnit_cmd, i_clk.pos());
        reset_signal_is(i_rst, true);  // true is hihg, flase is low
        SC_CTHREAD(outputUnit_req, i_clk.pos());
        reset_signal_is(i_rst, true);  // true is hihg, flase is low
        SC_CTHREAD(outputUnit_rsp, i_clk.pos());
        reset_signal_is(i_rst, true);  // true is hihg, flase is low
    };
};

