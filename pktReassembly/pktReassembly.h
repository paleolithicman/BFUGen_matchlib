#include "common.h"
#define TCP_FIN 0
#define TCP_SYN 1
#define TCP_RST 2
#define TCP_FACK 4
#define PROT_UDP 0x11
#define PKT_FORWARD 0
#define PKT_DROP 1
#define PKT_CHECK 2
#define INSERT 1
#define UPDATE 2
#define DELETE 3

struct meta_t {
    sc_biguint<8> prot;
    sc_biguint<96> tuple;
    sc_biguint<32> seq;
    sc_biguint<16> len;
    sc_biguint<30> hdr_len_flits_empty_pktID;
    sc_biguint<9> tcp_flags;
    sc_biguint<3> pkt_flags;
    sc_biguint<58> last_7_bytes_pdu_flag;

    inline void set(sc_biguint<252> bv) {
        prot = bv.range(7, 0);
        tuple = bv.range(103, 8);
        seq = bv.range(135, 104);
        len = bv.range(151, 136);
        hdr_len_flits_empty_pktID = bv.range(181, 152);
        tcp_flags = bv.range(190, 182);
        pkt_flags = bv.range(193, 191);
        last_7_bytes_pdu_flag = bv.range(251, 194);
    }

    inline sc_biguint<272> to_uint() {
        sc_biguint<272> val = (0, last_7_bytes_pdu_flag, pkt_flags, tcp_flags, hdr_len_flits_empty_pktID, 
            len, seq, tuple, prot);
        return val;
    }
};

struct fce_t {
    sc_biguint<96> tuple;
    sc_biguint<32> seq;
    sc_biguint<10> pointer;
    sc_biguint<10> slow_cnt;
    sc_biguint<104> addr3_addr2_addr1_addr0_last_7_bytes;
    sc_biguint<9> pointer2;
    sc_biguint<5> ch0_bit_map;

    inline void set(sc_biguint<266> bv) {
        tuple = bv.range(95, 0);
        seq = bv.range(127, 96);
        pointer = bv.range(137, 128);
        slow_cnt = bv.range(147, 138);
        addr3_addr2_addr1_addr0_last_7_bytes = bv.range(251, 148);
        pointer2 = bv.range(260, 252);
        ch0_bit_map = bv.range(265, 261);
    }

    inline sc_biguint<272> to_uint() {
        sc_biguint<272> val = (0, ch0_bit_map, pointer2, addr3_addr2_addr1_addr0_last_7_bytes, 
            slow_cnt, pointer, seq, tuple);
        return val;
    }
};

struct dymem_t{
    meta_t meta;
    sc_biguint<9> next;

    inline void set(sc_biguint<261> bv) {
        meta.set(bv.range(251, 0));
        next = bv.range(260, 252);
    }

    inline sc_biguint<272> to_uint() {
        sc_biguint<252> tmp = meta.to_uint();
        sc_biguint<272> val = (0, next, tmp);
        return val;
    }
};

struct arg_t {
    meta_t input;

    void set(sc_biguint<272> bv) {
        input.set(bv.range(271, 0));
    }

    sc_biguint<272> to_uint() {
        sc_biguint<272> val = (0, input.to_uint());
        return val;
    }
};

struct ftOut_t {
    meta_t pkt;
    fce_t fce;

    void set(sc_biguint<518> bv) {
        pkt.set(bv.range(251, 0));
        fce.set(bv.range(517, 252));
    }

    sc_biguint<518> to_uint() {
        sc_biguint<252> tmp0 = pkt.to_uint();
        sc_biguint<266> tmp1 = fce.to_uint();
        sc_biguint<518> val = (tmp1, tmp0);
        return val;
    }
};

SC_MODULE(pktReassembly) {
    sc_in<bool> i_clk;
    sc_in<bool> i_rst;

    sc_uint<NUM_THREADS_LG> tag;
    sc_uint<OPCODE_WIDTH> opcode;
    sc_uint<IP_WIDTH> bt0;
    sc_uint<IP_WIDTH> bt1;
    sc_uint<IP_WIDTH> bt2;
    sc_uint<IP_WIDTH> bt3;

    primate_stream_272_4::slave<>     CCS_INIT_S1(stream_in);
    primate_stream_272_4::master<>    CCS_INIT_S1(stream_out);
    primate_ctrl_iu::slave<>          CCS_INIT_S1(cmd_in);
    primate_bfu_iu::master<>          CCS_INIT_S1(bfu_out);

    bfu_in::master<>      CCS_INIT_S1(flow_table_read_req);
    bfu518_out::slave<>   CCS_INIT_S1(flow_table_read_rsp);

    bfu_in::master<>      CCS_INIT_S1(flow_table_write_req);
    // bfu_out::slave<>      CCS_INIT_S1(flow_table_write_rsp);

    void pktReassembly_main();
    inline void pktReassembly_core();

    SC_CTOR(pktReassembly) {
        SC_CTHREAD(pktReassembly_main, i_clk.pos());
        reset_signal_is(i_rst, true);  // true is hihg, flase is low
    };
};

