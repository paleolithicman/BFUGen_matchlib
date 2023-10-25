// Primate BFU multi-cycle interface

#ifndef _PRIMATE_BFU_MC_
#define _PRIMATE_BFU_MC_

#include <systemc.h>
#include <string>
#include <mc_connections.h>
#include <nvhls_module.h>
// #include "ready_valid.h"

namespace primate_bfu_mc {
template<int TAG_W, int IP_W, int ADDR_W, int DATA_W>
class write_mc {
public:
    struct payload_t {
        sc_uint<TAG_W>     tag;
        sc_uint<IP_W>      flag;
        bool               done;
        bool               wen0;
        sc_uint<ADDR_W>    addr0;
        sc_biguint<DATA_W> data0;
        bool               wen1;
        sc_uint<ADDR_W>    addr1;
        sc_biguint<DATA_W> data1;

        static const unsigned int width = TAG_W+IP_W+ADDR_W*2+DATA_W*2+3;

        payload_t(sc_uint<TAG_W> tag = 0, sc_uint<IP_W> flag = 0, bool done = false,
            bool wen0 = false, sc_uint<ADDR_W> addr0 = 0, sc_biguint<DATA_W> data0 = 0,
            bool wen1 = false, sc_uint<ADDR_W> addr1 = 0, sc_biguint<DATA_W> data1 = 0) : 
            tag(tag), flag(flag), done(done), wen0(wen0), addr0(addr0), 
            data0(data0), wen1(wen1), addr1(addr1), data1(data1) {}

        template <unsigned int Size>
        void Marshall(Marshaller<Size> &m) {
            m &tag;
            m &flag;
            m &done;
            m &wen0;
            m &addr0;
            m &data0;
            m &wen1;
            m &addr1;
            m &data1;
        }

        payload_t& operator= (const payload_t& val) {
            tag   = val.tag;
            flag  = val.flag;
            done  = val.done;
            wen0  = val.wen0;
            addr0 = val.addr0;
            data0 = val.data0;
            wen1  = val.wen1;
            addr1 = val.addr1;
            data1 = val.data1;
            return (*this);
        }

        bool operator== (const payload_t& val) const {
            return ((tag == val.tag) && (flag == val.flag) && (done == val.done) &&
                (wen0 == val.wen0) && (addr0 == val.addr0) && (data0 == val.data0) &&
                (wen1 == val.wen1) && (addr1 == val.addr1) && (data1 == val.data1));
        }

        inline friend std::ostream& operator<<(std::ostream& os, const payload_t& val) {
            os << "tag = " << val.tag << hex << "; flag = " << val.flag << "; done = " << val.done <<
                "; wen0 = " << val.wen0 << "; addr0 = " << val.addr0 << "; data0 = " << val.data0 <<
                "; wen1 = " << val.wen1 << "; addr1 = " << val.addr1 << "; data1 = " << val.data1 << dec << std::endl;
            return os;
        }

        inline friend void sc_trace(sc_trace_file*& f, const payload_t& val, std::string name) {
            sc_trace(f, val.tag, name + ".tag");
            sc_trace(f, val.flag, name + ".flag");
            sc_trace(f, val.done, name + ".done");
            sc_trace(f, val.wen0, name + ".wen0");
            sc_trace(f, val.addr0, name + ".addr0");
            sc_trace(f, val.data0, name + ".data0");
            sc_trace(f, val.wen1, name + ".wen1");
            sc_trace(f, val.addr1, name + ".addr1");
            sc_trace(f, val.data1, name + ".data1");
        }
    };

    template <Connections::connections_port_t PortType = AUTO_PORT>
    class chan {
    public:
        Connections::Combinational<payload_t, PortType> t;

        chan() {}

        chan(const char *name)
            : t(nvhls_concat(name, "_t")) {};

    }; // primate_stream::chan

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

        bool write_last(sc_uint<TAG_W> out_tag, sc_uint<IP_W> out_flag) {
            payload_t tmp;
            tmp.done = true;
            tmp.tag = out_tag;
            tmp.flag = out_flag;
            tmp.wen0 = false;
            tmp.wen1 = false;
            return t.PushNB(tmp);
        }

        void write(sc_uint<TAG_W> out_tag, sc_uint<IP_W> out_flag, sc_uint<ADDR_W> out_addr,
         sc_biguint<DATA_W> out_data, bool is_last = false) {
            payload_t tmp;
            tmp.done = is_last;
            tmp.tag = out_tag;
            tmp.flag = out_flag;
            tmp.wen0 = true;
            tmp.addr0 = out_addr;
            tmp.data0 = out_data;
            tmp.wen1 = false;
            t.Push(tmp);
        }

        void write(sc_uint<TAG_W> out_tag, sc_uint<IP_W> out_flag, sc_uint<ADDR_W> out_addr0,
         sc_biguint<DATA_W> out_data0, sc_uint<ADDR_W> out_addr1, sc_biguint<DATA_W> out_data1, bool is_last = false) {
            payload_t tmp;
            tmp.done = is_last;
            tmp.tag = out_tag;
            tmp.flag = out_flag;
            tmp.wen0 = true;
            tmp.addr0 = out_addr0;
            tmp.data0 = out_data0;
            tmp.wen1 = true;
            tmp.addr1 = out_addr1;
            tmp.data1 = out_data1;
            t.Push(tmp);
        }

        template<class CHANNEL>
        void operator () (CHANNEL &c) {
            t(c.t);
        }
    }; // class master

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

        template <class CHANNEL>
        void operator () (CHANNEL &c) {
            t(c.t);
        }
    }; // class slave
}; // write_mc


template<int TAG_W, int IP_W>
class out_simple {
public:
    class master {
    public:
        master(const char* name=sc_gen_unique_name("primate_bfu_mc_out_m")) :
            valid((std::string(name)+"_valid").c_str()),
            tag((std::string(name)+"_tag").c_str()),
            flag((std::string(name)+"_flag").c_str())
        {}

        template<typename CHANNEL>
        inline void bind(CHANNEL &channel) {
            valid(channel.valid);
            tag(channel.tag);
            flag(channel.flag);
        }

        template<typename CHANNEL>
        inline void operator () (CHANNEL &channel) {
            bind(channel);
        }

        inline void reset() {
            valid = false;
            tag.write(0);
            flag.write(0);
        }

        inline void write(sc_uint<TAG_W> out_tag, sc_uint<IP_W> out_flag) {
            valid.write(true);
            tag.write(out_tag);
            flag.write(out_flag);
            wait();
            valid.write(false);
        }

    public:
        sc_out<bool>                          valid;
        sc_out<sc_uint<TAG_W>>                tag;
        sc_out<sc_uint<IP_W>>                 flag;
    }; // master

    class slave {
    public:
        slave(const char* name=sc_gen_unique_name("primate_bfu_mc_out_s")) :
            valid((std::string(name)+"_valid").c_str()),
            tag((std::string(name)+"_tag").c_str()),
            flag((std::string(name)+"_flag").c_str())
        {}

        template<typename CHANNEL>
        inline void bind(CHANNEL &channel) {
            valid(channel.valid);
            tag(channel.tag);
            flag(channel.flag);
        }

        template<typename CHANNEL>
        inline void operator () (CHANNEL &channel) {
            bind(channel);
        }

        inline bool read(sc_uint<TAG_W> &out_tag, sc_uint<IP_W> &out_flag) {
            out_tag = tag;
            out_flag = flag;
            return valid.read();
        }

    public:
        sc_in<bool>                          valid;
        sc_in<sc_uint<TAG_W>>                tag;
        sc_in<sc_uint<IP_W>>                 flag;
    }; // slave

public:
    sc_signal<bool>                          valid;
    sc_signal<sc_uint<TAG_W>>                tag;
    sc_signal<sc_uint<IP_W>>                 flag;
}; // out_simple

namespace read_mc {

template<int TAG_W, int ADDR_W>
class req {
public:
    struct reqPayload_t {
        sc_uint<TAG_W>  tag;
        sc_uint<ADDR_W> addr0;
        sc_uint<ADDR_W> addr1;

        static const unsigned int width = TAG_W+ADDR_W*2;

        reqPayload_t(sc_uint<TAG_W> tag = 0, sc_uint<ADDR_W> addr0 = 0, sc_uint<ADDR_W> addr1 = 0) : 
            tag(tag), addr0(addr0), addr1(addr1) {}

        template <unsigned int Size>
        void Marshall(Marshaller<Size> &m) {
            m &tag;
            m &addr0;
            m &addr1;
        }

        reqPayload_t& operator= (const reqPayload_t& val) {
            tag = val.tag;
            addr0 = val.addr0;
            addr1 = val.addr1;
            return (*this);
        }

        bool operator== (const reqPayload_t& val) const {
            return ((tag == val.tag) && (addr0 == val.addr0) && (addr1 == val.addr1));
        }

        inline friend std::ostream& operator<<(std::ostream& os, const reqPayload_t& val) {
            os << "tag = " << val.tag << "; addr0 = " << val.addr0 << "; addr1 = " << val.addr1 << std::endl;
            return os;
        }

        inline friend void sc_trace(sc_trace_file*& f, const reqPayload_t& val, std::string name) {
            sc_trace(f, val.tag, name + ".tag");
            sc_trace(f, val.addr0, name + ".addr0");
            sc_trace(f, val.addr1, name + ".addr1");
        }
    };

    template <Connections::connections_port_t PortType = AUTO_PORT>
    class chan {
    public:
        Connections::Combinational<reqPayload_t, PortType> t;

        chan() {}

        chan(const char *name)
            : t(nvhls_concat(name, "_t")) {};

        // master side

        void reset_master() {
            t.ResetWrite();
        }

        void write(reqPayload_t pl) {
            t.Push(pl);
        }

        bool nb_write(const reqPayload_t &pl) {
            return t.PushNB(pl);
        }

        // slave side

        void reset_slave() {
            t.ResetRead();
        }

        reqPayload_t read() {
            return t.Pop();
        }

        bool nb_read(reqPayload_t &pl) {
            reqPayload_t tmp;
            if (t.PopNB(tmp)) {
                pl = tmp;
                return true;
            } else {
                return false;
            }
        }
    }; // primate_stream::chan

    template <Connections::connections_port_t PortType = AUTO_PORT>
    class master {
    public:
        Connections::Out<reqPayload_t, PortType> t;

        master() {}

        master(const char *name)
            : t(nvhls_concat(name, "_t")) {}

        void reset() {
            t.Reset();
        }

        void write(reqPayload_t pl) {
            t.Push(pl);
        }

        bool nb_write(const reqPayload_t &pl) {
            return t.PushNB(pl);
        }

        template<class CHANNEL>
        void operator () (CHANNEL &c) {
            t(c.t);
        }
    }; // class master

    template <Connections::connections_port_t PortType = AUTO_PORT>
    class slave {
    public:
        Connections::In<reqPayload_t, PortType> t;

        slave() {}

        slave(const char *name)
            : t(nvhls_concat(name, "_t")) {}

        void reset() {
            t.Reset();
        }

        reqPayload_t read() {
            return t.Pop();
        }

        bool nb_read(reqPayload_t &pl) {
            reqPayload_t tmp;
            if (t.PopNB(tmp)) {
                pl = tmp;
                return true;
            } else {
                return false;
            }
        }

        template <class CHANNEL>
        void operator () (CHANNEL &c) {
            t(c.t);
        }
    }; // class slave
}; // class req

template<int DATA_W>
class rsp {
public:
    struct rspPayload_t {
        sc_biguint<DATA_W> data0;
        sc_biguint<DATA_W> data1;

        static const unsigned int width = DATA_W*2;

        rspPayload_t(sc_biguint<DATA_W> data0 = 0, sc_biguint<DATA_W> data1 = 0) : 
            data0(data0), data1(data1) {}

        template <unsigned int Size>
        void Marshall(Marshaller<Size> &m) {
            m &data0;
            m &data1;
        }

        rspPayload_t& operator= (const rspPayload_t& val) {
            data0 = val.data0;
            data1 = val.data1;
            return (*this);
        }

        bool operator== (const rspPayload_t& val) const {
            return ((data0 == val.data0) && (data1 == val.data1));
        }

        inline friend std::ostream& operator<<(std::ostream& os, const rspPayload_t& val) {
            os << hex << "data0 = " << val.data0 << "; data1 = " << val.data1 << dec << std::endl;
            return os;
        }

        inline friend void sc_trace(sc_trace_file*& f, const rspPayload_t& val, std::string name) {
            sc_trace(f, val.data0, name + ".data0");
            sc_trace(f, val.data1, name + ".data1");
        }
    };

    template <Connections::connections_port_t PortType = AUTO_PORT>
    class chan {
    public:
        Connections::Combinational<rspPayload_t, PortType> t;

        chan() {}

        chan(const char *name)
            : t(nvhls_concat(name, "_t")) {};

        // master side

        void reset_master() {
            t.ResetWrite();
        }

        void write(rspPayload_t pl) {
            t.Push(pl);
        }

        bool nb_write(const rspPayload_t &pl) {
            return t.PushNB(pl);
        }

        // slave side

        void reset_slave() {
            t.ResetRead();
        }

        rspPayload_t read() {
            return t.Pop();
        }

        bool nb_read(rspPayload_t &pl) {
            rspPayload_t tmp;
            if (t.PopNB(tmp)) {
                pl = tmp;
                return true;
            } else {
                return false;
            }
        }
    }; // primate_stream::chan

    template <Connections::connections_port_t PortType = AUTO_PORT>
    class master {
    public:
        Connections::Out<rspPayload_t, PortType> t;

        master() {}

        master(const char *name)
            : t(nvhls_concat(name, "_t")) {}

        void reset() {
            t.Reset();
        }

        void write(rspPayload_t pl) {
            t.Push(pl);
        }

        bool nb_write(const rspPayload_t &pl) {
            return t.PushNB(pl);
        }

        template<class CHANNEL>
        void operator () (CHANNEL &c) {
            t(c.t);
        }
    }; // class master

    template <Connections::connections_port_t PortType = AUTO_PORT>
    class slave {
    public:
        Connections::In<rspPayload_t, PortType> t;

        slave() {}

        slave(const char *name)
            : t(nvhls_concat(name, "_t")) {}

        void reset() {
            t.Reset();
        }

        rspPayload_t read() {
            return t.Pop();
        }

        bool nb_read(rspPayload_t &pl) {
            rspPayload_t tmp;
            if (t.PopNB(tmp)) {
                pl = tmp;
                return true;
            } else {
                return false;
            }
        }

        template <class CHANNEL>
        void operator () (CHANNEL &c) {
            t(c.t);
        }
    }; // class slave
}; // class rsp

}; // read_mc

}; // primate_bfu_mc


#endif
