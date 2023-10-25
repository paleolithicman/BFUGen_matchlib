#ifndef _PRIMATE_STREAM_
#define _PRIMATE_STREAM_

#include <systemc.h>
#include <string>
#include <mc_connections.h>
#include <nvhls_module.h>

template<typename Cfg>
class primate_stream
{
public:
    enum {
        DATA_WIDTH = Cfg::dataWidth,
        NUM_THREADS_LG = Cfg::numThreadsLG,
        EMPTY_WIDTH = (DATA_WIDTH >> 3),
    };
    typedef typename Cfg::Data_t Data;

    struct payload_t {
        sc_uint<NUM_THREADS_LG>  tag;
        Data                     data;
        sc_uint<EMPTY_WIDTH>     empty;
        bool                     last;

        static const unsigned int width = DATA_WIDTH+NUM_THREADS_LG+EMPTY_WIDTH+1;

        payload_t(Data data = 0, sc_uint<NUM_THREADS_LG> tag = 0, sc_uint<EMPTY_WIDTH> empty = 0, bool last = false) : 
            data(data), tag(tag), empty(empty), last(last) {}

        template <unsigned int Size>
        void Marshall(Marshaller<Size> &m) {
            m &tag;
            m &data;
            m &empty;
            m &last;
        }

        inline bool is_last() {
            return last;
        }

        payload_t& operator= (const payload_t& val) {
            data = val.data;
            tag = val.tag;
            empty = val.empty;
            last = val.last;
            return (*this);
        }

        bool operator== (const payload_t& val) const {
            return ((data == val.data) && (tag == val.tag) &&
                (empty == val.empty) && (last == val.last));
        }

        inline friend std::ostream& operator<<(std::ostream& os, const payload_t& val) {
            os << hex << "data = " << val.data << dec << "; tag = " << val.tag << 
                "; empty = " << val.empty << "; last = " << val.last << std::endl;
            return os;
        }

        inline friend void sc_trace(sc_trace_file*& f, const payload_t& val, std::string name) {
            sc_trace(f, val.data, name + ".data");
            sc_trace(f, val.tag, name + ".tag");
            sc_trace(f, val.empty, name + ".empty");
            sc_trace(f, val.last, name + ".last");
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

        payload_t peek() {
            return t.Peek();
        }

        template <class CHANNEL>
        void operator () (CHANNEL &c) {
            t(c.t);
        }
    }; // class slave
}; // class primate_stream

template <int DW, int NTLG>
struct cfg_biguint {

  typedef sc_biguint<DW> Data_t;

  enum {
    dataWidth = DW,
    numThreadsLG = NTLG,
  };

};
typedef primate_stream<cfg_biguint<512, 4>> primate_stream_512_4;


#endif