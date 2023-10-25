// Primate BFU interface

#ifndef _PRIMATE_BFU_
#define _PRIMATE_BFU_

#include <systemc.h>
#include <string>
#include <mc_connections.h>
#include <nvhls_module.h>

namespace primate_bfu {

template<int TAG_W, int OP_W, int IMM_W, int DATA_W>
class bfu_in {
public:
    struct payload_t {
        sc_uint<TAG_W>     tag;
        sc_uint<OP_W>      opcode;
        sc_uint<IMM_W>     imm;
        sc_biguint<DATA_W> bits;

        static const unsigned int width = TAG_W+OP_W+IMM_W+DATA_W;

        payload_t(sc_uint<TAG_W> tag = 0, sc_uint<OP_W> opcode = 0,
            sc_uint<IMM_W> imm = 0, sc_biguint<DATA_W> bits = 0) : 
            tag(tag), opcode(opcode), imm(imm), bits(bits) {}

        template <unsigned int Size>
        void Marshall(Marshaller<Size> &m) {
            m &tag;
            m &opcode;
            m &imm;
            m &bits;
        }

        payload_t& operator= (const payload_t& val) {
            tag    = val.tag;
            opcode = val.opcode;
            imm    = val.imm;
            bits   = val.bits;
            return (*this);
        }

        bool operator== (const payload_t& val) const {
            return ((tag == val.tag) && (opcode == val.opcode) && (imm == val.imm) &&
                (bits == val.bits));
        }

        inline friend std::ostream& operator<<(std::ostream& os, const payload_t& val) {
            os << "tag = " << val.tag << hex << "; opcode = " << val.opcode << "; imm = " << val.imm <<
                "; bits = " << val.bits << dec << std::endl;
            return os;
        }

        inline friend void sc_trace(sc_trace_file*& f, const payload_t& val, std::string name) {
            sc_trace(f, val.tag, name + ".tag");
            sc_trace(f, val.opcode, name + ".opcode");
            sc_trace(f, val.imm, name + ".imm");
            sc_trace(f, val.bits, name + ".bits");
        }
    };

    template <Connections::connections_port_t PortType = AUTO_PORT>
    class chan {
    public:
        Connections::Combinational<payload_t, PortType> t;

        chan() {}

        chan(const char *name)
            : t(nvhls_concat(name, "_t")) {};

        // master side

        void reset_master() {
            t.ResetWrite();
        }

        void write(payload_t pl) {
            t.Push(pl);
        }

        bool nb_write(const payload_t &pl) {
            return t.PushNB(pl);
        }

        // slave side

        void reset_slave() {
            t.ResetRead();
        }

        payload_t read() {
            return t.Pop();
        }

        payload_t peek() {
            return t.Peek();
        }

        bool nb_read(payload_t &pl) {
            payload_t tmp;
            if (t.PopNB(tmp)) {
                pl = tmp;
                return true;
            } else {
                return false;
            }
        }
    }; // bfu_in::chan

    template <Connections::connections_port_t PortType = AUTO_PORT>
    class master {
    public:
        Connections::Out<payload_t, PortType> t;

        master() {}

        master(const char *name)
            : t(nvhls_concat(name, "_t")) {}

        void reset() {
            t.Reset();
        }

        void write(payload_t pl) {
            t.Push(pl);
        }

        bool nb_write(const payload_t &pl) {
            return t.PushNB(pl);
        }

        template<class CHANNEL>
        void operator () (CHANNEL &c) {
            t(c.t);
        }
    }; // bfu_in::master

    template <Connections::connections_port_t PortType = AUTO_PORT>
    class slave {
    public:
        Connections::In<payload_t, PortType> t;

        slave() {}

        slave(const char *name)
            : t(nvhls_concat(name, "_t")) {}

        void reset() {
            t.Reset();
        }

        payload_t read() {
            return t.Pop();
        }

        bool nb_read(payload_t &pl) {
            payload_t tmp;
            if (t.PopNB(tmp)) {
                pl = tmp;
                return true;
            } else {
                return false;
            }
        }

        payload_t peek() {
            return t.Peek();
        }

        template <class CHANNEL>
        void operator () (CHANNEL &c) {
            t(c.t);
        }
    }; // bfu_in::slave
}; // bfu_in

template<int TAG_W, int IP_W, int DATA_W>
class bfu_out {
public:
    struct payload_t {
        sc_uint<TAG_W>     tag;
        sc_uint<IP_W>      flag;
        sc_biguint<DATA_W> bits;

        static const unsigned int width = TAG_W+IP_W+DATA_W;

        payload_t(sc_uint<TAG_W> tag = 0, sc_uint<IP_W> flag = 0,
            sc_biguint<DATA_W> bits = 0) : 
            tag(tag), flag(flag), bits(bits) {}

        template <unsigned int Size>
        void Marshall(Marshaller<Size> &m) {
            m &tag;
            m &flag;
            m &bits;
        }

        payload_t& operator= (const payload_t& val) {
            tag    = val.tag;
            flag = val.flag;
            bits   = val.bits;
            return (*this);
        }

        bool operator== (const payload_t& val) const {
            return ((tag == val.tag) && (flag == val.flag) && (bits == val.bits));
        }

        inline friend std::ostream& operator<<(std::ostream& os, const payload_t& val) {
            os << "tag = " << val.tag << hex << "; flag = " << val.flag <<
                "; bits = " << val.bits << dec << std::endl;
            return os;
        }

        inline friend void sc_trace(sc_trace_file*& f, const payload_t& val, std::string name) {
            sc_trace(f, val.tag, name + ".tag");
            sc_trace(f, val.flag, name + ".flag");
            sc_trace(f, val.bits, name + ".bits");
        }
    };

    template <Connections::connections_port_t PortType = AUTO_PORT>
    class chan {
    public:
        Connections::Combinational<payload_t, PortType> t;

        chan() {}

        chan(const char *name)
            : t(nvhls_concat(name, "_t")) {};

        // master side

        void reset_master() {
            t.ResetWrite();
        }

        void write(payload_t pl) {
            t.Push(pl);
        }

        bool nb_write(const payload_t &pl) {
            return t.PushNB(pl);
        }

        // slave side

        void reset_slave() {
            t.ResetRead();
        }

        payload_t read() {
            return t.Pop();
        }

        payload_t peek() {
            return t.Peek();
        }

        bool nb_read(payload_t &pl) {
            payload_t tmp;
            if (t.PopNB(tmp)) {
                pl = tmp;
                return true;
            } else {
                return false;
            }
        }
    }; // bfu_out::chan

    template <Connections::connections_port_t PortType = AUTO_PORT>
    class master {
    public:
        Connections::Out<payload_t, PortType> t;

        master() {}

        master(const char *name)
            : t(nvhls_concat(name, "_t")) {}

        void reset() {
            t.Reset();
        }

        void write(payload_t pl) {
            t.Push(pl);
        }

        bool nb_write(const payload_t &pl) {
            return t.PushNB(pl);
        }

        template<class CHANNEL>
        void operator () (CHANNEL &c) {
            t(c.t);
        }
    }; // bfu_out::master

    template <Connections::connections_port_t PortType = AUTO_PORT>
    class slave {
    public:
        Connections::In<payload_t, PortType> t;

        slave() {}

        slave(const char *name)
            : t(nvhls_concat(name, "_t")) {}

        void reset() {
            t.Reset();
        }

        payload_t read() {
            return t.Pop();
        }

        bool nb_read(payload_t &pl) {
            payload_t tmp;
            if (t.PopNB(tmp)) {
                pl = tmp;
                return true;
            } else {
                return false;
            }
        }

        payload_t peek() {
            return t.Peek();
        }

        template <class CHANNEL>
        void operator () (CHANNEL &c) {
            t(c.t);
        }
    }; // bfu_out::slave
}; // bfu_out

}; // primate_bfu


#endif
