#ifndef _PRIMATE_CTRL_
#define _PRIMATE_CTRL_

#include <systemc.h>
#include <string>
#include <mc_connections.h>
#include <nvhls_module.h>

template<int TAG_W, int OP_W, int ADDR_W, int DATA_W>
class primate_ctrl
{
public:
    struct cmd_t {
        sc_uint<TAG_W>          ar_tag;
        sc_uint<OP_W>           ar_opcode;
        sc_uint<ADDR_W>         ar_rd;
        sc_biguint<DATA_W>      ar_bits;
        sc_biguint<DATA_W>      ar_imm;

        static const unsigned int width = TAG_W+OP_W+ADDR_W+DATA_W*2;

        cmd_t(sc_uint<TAG_W> ar_tag = 0, sc_uint<OP_W> ar_opcode = 0, sc_uint<ADDR_W> ar_rd = 0, 
            sc_biguint<DATA_W> ar_bits = 0, sc_biguint<DATA_W> ar_imm = 0) : 
            ar_tag(ar_tag), ar_opcode(ar_opcode), ar_rd(ar_rd), ar_bits(ar_bits), ar_imm(ar_imm) {}

        template <unsigned int Size>
        void Marshall(Marshaller<Size> &m) {
            m &ar_tag;
            m &ar_opcode;
            m &ar_rd;
            m &ar_bits;
            m &ar_imm;
        }

        cmd_t& operator= (const cmd_t& val) {
            ar_tag = val.ar_tag;
            ar_opcode = val.ar_opcode;
            ar_rd = val.ar_rd;
            ar_bits = val.ar_bits;
            ar_imm = val.ar_imm;
            return (*this);
        }

        bool operator== (const cmd_t& val) const {
            return ((ar_tag == val.ar_tag) && (ar_opcode == val.ar_opcode) &&
                (ar_rd == val.ar_rd) && (ar_bits == val.ar_bits) && (ar_imm == val.ar_imm));
        }

        inline friend std::ostream& operator<<(std::ostream& os, const cmd_t& val) {
            os << "tag = " << val.ar_tag << hex << "; opcode = " << val.ar_opcode << 
                "; rd = " << val.ar_rd << "; bits = " << val.ar_bits <<
                "; imm = " << val.ar_imm << dec << std::endl;
            return os;
        }

        inline friend void sc_trace(sc_trace_file*& f, const cmd_t& val, std::string name) {
            sc_trace(f, val.ar_tag, name + ".ar_tag");
            sc_trace(f, val.ar_opcode, name + ".ar_opcode");
            sc_trace(f, val.ar_rd, name + ".ar_rd");
            sc_trace(f, val.ar_bits, name + ".ar_bits");
            sc_trace(f, val.ar_imm, name + ".ar_imm");
        }
    };

    template <Connections::connections_port_t PortType = AUTO_PORT>
    class chan {
    public:
        Connections::Combinational<cmd_t, PortType> t;

        chan() {}

        chan(const char *name)
            : t(nvhls_concat(name, "_t")) {};

        // master side

        void reset_master() {
            t.ResetWrite();
        }

        void write(cmd_t cmd) {
            t.Push(cmd);
        }

        bool nb_write(const cmd_t &cmd) {
            return t.PushNB(cmd);
        }

        // slave side

        void reset_slave() {
            t.ResetRead();
        }

        cmd_t read() {
            return t.Pop();
        }

        bool nb_read(cmd_t &cmd) {
            cmd_t tmp;
            if (t.PopNB(tmp)) {
                cmd = tmp;
                return true;
            } else {
                return false;
            }
        }
    }; // primate_ctrl::chan

    template <Connections::connections_port_t PortType = AUTO_PORT>
    class master {
    public:
        Connections::Out<cmd_t, PortType> t;

        master() {}

        master(const char *name)
            : t(nvhls_concat(name, "_t")) {}

        void reset() {
            t.Reset();
        }

        void write(cmd_t cmd) {
            t.Push(cmd);
        }

        bool nb_write(const cmd_t &cmd) {
            return t.PushNB(cmd);
        }

        template<class CHANNEL>
        void operator () (CHANNEL &c) {
            t(c.t);
        }
    }; // primate_ctrl::master

    template <Connections::connections_port_t PortType = AUTO_PORT>
    class slave {
    public:
        Connections::In<cmd_t, PortType> t;

        slave() {}

        slave(const char *name)
            : t(nvhls_concat(name, "_t")) {}

        void reset() {
            t.Reset();
        }

        cmd_t read() {
            return t.Pop();
        }

        bool nb_read(cmd_t &cmd) {
            cmd_t tmp;
            if (t.PopNB(tmp)) {
                cmd = tmp;
                return true;
            } else {
                return false;
            }
        }

        template <class CHANNEL>
        void operator () (CHANNEL &c) {
            t(c.t);
        }
    }; // primate_ctrl::slave

}; // class primate_ctrl


template<int TAG_W, int DATA_W>
class primate_ctrl_simple
{
public:
    struct cmd_t {
        sc_uint<TAG_W>          ar_tag;
        sc_biguint<DATA_W>      ar_bits;

        static const unsigned int width = TAG_W+DATA_W;

        cmd_t(sc_uint<TAG_W> ar_tag = 0, sc_biguint<DATA_W> ar_bits = 0) : 
            ar_tag(ar_tag), ar_bits(ar_bits) {}

        template <unsigned int Size>
        void Marshall(Marshaller<Size> &m) {
            m &ar_tag;
            m &ar_bits;
        }

        cmd_t& operator= (const cmd_t& val) {
            ar_tag = val.ar_tag;
            ar_bits = val.ar_bits;
            return (*this);
        }

        bool operator== (const cmd_t& val) const {
            return ((ar_tag == val.ar_tag) && (ar_bits == val.ar_bits));
        }

        inline friend std::ostream& operator<<(std::ostream& os, const cmd_t& val) {
            os << "tag = " << val.ar_tag << "; bits = " << val.ar_bits << dec << std::endl;
            return os;
        }

        inline friend void sc_trace(sc_trace_file*& f, const cmd_t& val, std::string name) {
            sc_trace(f, val.ar_tag, name + ".ar_tag");
            sc_trace(f, val.ar_bits, name + ".ar_bits");
        }
    };

    template <Connections::connections_port_t PortType = AUTO_PORT>
    class chan {
    public:
        Connections::Combinational<cmd_t, PortType> t;

        chan() {}

        chan(const char *name)
            : t(nvhls_concat(name, "_t")) {};

        // master side

        void reset_master() {
            t.ResetWrite();
        }

        void write(cmd_t cmd) {
            t.Push(cmd);
        }

        bool nb_write(const cmd_t &cmd) {
            return t.PushNB(cmd);
        }

        // slave side

        void reset_slave() {
            t.ResetRead();
        }

        cmd_t read() {
            return t.Pop();
        }

        bool nb_read(cmd_t &cmd) {
            cmd_t tmp;
            if (t.PopNB(tmp)) {
                cmd = tmp;
                return true;
            } else {
                return false;
            }
        }
    }; // primate_ctrl_simple::chan

    template <Connections::connections_port_t PortType = AUTO_PORT>
    class master {
    public:
        Connections::Out<cmd_t, PortType> t;

        master() {}

        master(const char *name)
            : t(nvhls_concat(name, "_t")) {}

        void reset() {
            t.Reset();
        }

        void write(cmd_t cmd) {
            t.Push(cmd);
        }

        bool nb_write(const cmd_t &cmd) {
            return t.PushNB(cmd);
        }

        template<class CHANNEL>
        void operator () (CHANNEL &c) {
            t(c.t);
        }
    }; // primate_ctrl_simple::master

    template <Connections::connections_port_t PortType = AUTO_PORT>
    class slave {
    public:
        Connections::In<cmd_t, PortType> t;

        slave() {}

        slave(const char *name)
            : t(nvhls_concat(name, "_t")) {}

        void reset() {
            t.Reset();
        }

        cmd_t read() {
            return t.Pop();
        }

        bool nb_read(cmd_t &cmd) {
            cmd_t tmp;
            if (t.PopNB(tmp)) {
                cmd = tmp;
                return true;
            } else {
                return false;
            }
        }

        template <class CHANNEL>
        void operator () (CHANNEL &c) {
            t(c.t);
        }
    }; // primate_ctrl_simple::slave

}; // class primate_ctrl_simple

#endif