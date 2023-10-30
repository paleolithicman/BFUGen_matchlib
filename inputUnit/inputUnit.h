#include "common.h"

typedef struct {
    sc_biguint<48> dstAddr;
    sc_biguint<48> srcAddr;
    sc_biguint<16> etherType;

    void set(sc_biguint<112> bv) {
        dstAddr = bv.range(47, 0);
        srcAddr = bv.range(95, 48);
        etherType = bv.range(111, 96);
    }

    sc_biguint<REG_WIDTH> to_uint() {
        sc_biguint<REG_WIDTH> val = (0, etherType, srcAddr, dstAddr);
        return val;
    }
} ethernet_t;

typedef struct {
    sc_biguint<40> transportSpecific_domainNumber;
    sc_biguint<8> reserved2;
    sc_biguint<112> flags_reserved3;

    void set(sc_biguint<160> bv) {
        transportSpecific_domainNumber = bv.range(39, 0);
        reserved2 = bv.range(47, 40);
        flags_reserved3 = bv.range(159, 48);
    }

    sc_biguint<REG_WIDTH> to_uint() {
        sc_biguint<REG_WIDTH> val = (0, flags_reserved3, reserved2, transportSpecific_domainNumber);
        return val;
    }
} ptp_l_t;

typedef struct {
    sc_biguint<192> data;

    void set(sc_biguint<192> bv) {
        data = bv;
    }

    sc_biguint<REG_WIDTH> to_uint() {
        return data;
    }
} ptp_h_t;

typedef struct {
    sc_biguint<16> field_0;
    sc_biguint<16> field_1;
    sc_biguint<16> field_2;
    sc_biguint<16> field_3;

    void set(sc_biguint<64> bv) {
        field_0 = bv.range(15, 0);
        field_1 = bv.range(31, 16);
        field_2 = bv.range(47, 32);
        field_3 = bv.range(63, 48);
    }

    sc_biguint<REG_WIDTH> to_uint() {
        sc_biguint<REG_WIDTH> val = (0, field_3, field_2, field_1, field_0);
        return val;
    }
} header_t;

typedef struct {
    header_t hdr0;
    header_t hdr1;

    void set(sc_biguint<128> bv) {
        hdr0.set(bv.range(63, 0));
        hdr1.set(bv.range(127, 64));
    }

    sc_biguint<REG_WIDTH> to_uint() {
        sc_biguint<64> hdr0_uint = hdr0.to_uint();
        sc_biguint<64> hdr1_uint = hdr1.to_uint();
        sc_biguint<REG_WIDTH> val = (0, hdr1_uint, hdr0_uint);
        return val;
    }
} header2_t;

typedef struct {
    sc_biguint<72> version_ttl;
    sc_biguint<8> protocol;
    sc_biguint<80> hdrChecksum_dstAddr;

    void set(sc_biguint<160> bv) {
        version_ttl = bv.range(71, 0);
        protocol = bv.range(79, 72);
        hdrChecksum_dstAddr = bv.range(159, 80);
    }

    sc_biguint<REG_WIDTH> to_uint() {
        sc_biguint<REG_WIDTH> val = (0, hdrChecksum_dstAddr, protocol, version_ttl);
        return val;
    }
} ipv4_t;

typedef struct {
    sc_biguint<160> srcPort_urgentPtr;

    void set(sc_biguint<160> bv) {
        srcPort_urgentPtr = bv;
    }

    sc_biguint<REG_WIDTH> to_uint() {
        return srcPort_urgentPtr;
    }
} tcp_t;

typedef struct {
    sc_biguint<64> srcPort_checksum;

    void set(sc_biguint<64> bv) {
        srcPort_checksum = bv;
    }

    sc_biguint<REG_WIDTH> to_uint() {
        return srcPort_checksum;
    }
} udp_t;

typedef struct {
    sc_biguint<16> egress_spec;
    sc_biguint<16> mcast_grp;

    void set(sc_biguint<32> bv) {
        egress_spec = bv.range(15, 0);
        mcast_grp = bv.range(31, 16);
    }

    sc_biguint<REG_WIDTH> to_uint() {
        sc_biguint<REG_WIDTH> val = (0, mcast_grp, egress_spec);
        return val;
    }
} standard_metadata_t;

SC_MODULE(inputUnit) {
    sc_in<bool> i_clk;
    sc_in<bool> i_rst;

    sc_uint<NUM_THREADS_LG> tag;
    sc_uint<OPCODE_WIDTH> opcode;
    sc_uint<IP_WIDTH> bt0;
    sc_uint<IP_WIDTH> bt1;

    primate_stream_512_4::slave<>     CCS_INIT_S1(stream_in);

    primate_ctrl_iu::slave<>          CCS_INIT_S1(cmd_in);

    primate_bfu_iu::master<>          CCS_INIT_S1(bfu_out);

    primate_stream_512_4::master<>    CCS_INIT_S1(pkt_buf_out);

    void inputUnit_main();

    inline void inputUnit_core();

    SC_CTOR(inputUnit) {
        SC_CTHREAD(inputUnit_main, i_clk.pos());
        reset_signal_is(i_rst, true);  // true is hihg, flase is low
    };
};
