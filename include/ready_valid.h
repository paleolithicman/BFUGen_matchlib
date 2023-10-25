#ifndef _READY_VALID_
#define _READY_VALID_

#include <systemc.h>
#include <string>

template<typename DT>
class ready_valid
{
public:
    class in {
    public:
        in(const char* name=sc_gen_unique_name("ready_valid_in")) :
            payload((std::string(name)+"_payload").c_str()),
            valid((std::string(name)+"_valid").c_str()),
            ready((std::string(name)+"_ready").c_str())
        {}

        template<typename CHANNEL>
        inline void bind(CHANNEL &channel) {
            valid(channel.valid);
            payload(channel.payload);
            ready(channel.ready);
        }

        template<typename CHANNEL>
        inline void operator () (CHANNEL &channel) {
            bind(channel);
        }

        inline bool nb_can_read() {
            return valid.read();
        }

        inline bool nb_read(DT &data) {
            data = payload;
            ready = true;
            if (valid.read()) {
                return true;
            } else {
                return false;
            }
        }

        inline void reset() {
            ready = false;
        }

        inline DT read() {
            ready = true;
            do { ::wait(); } while (valid.read() == false);
            ready = false;
            return payload.read();
        }

        inline operator DT () {
            return read();
        }


    public:
        sc_in<DT>    payload;
        sc_out<bool> ready;
        sc_in<bool>  valid;
    }; // class in

    class out {
    public:
        out(const char* name=sc_gen_unique_name("ready_valid_out")) :
            payload((std::string(name)+"_payload").c_str()),
            valid((std::string(name)+"_valid").c_str()),
            ready((std::string(name)+"_ready").c_str())
        {}

        template<typename CHANNEL>
        inline void bind(CHANNEL &channel) {
            valid(channel.valid);
            payload(channel.payload);
            ready(channel.ready);
        }

        template<typename CHANNEL>
        inline void operator () (CHANNEL &channel) {
            bind(channel);
        }

        inline bool nb_can_write() {
            return valid.read() == false || ready.read() == true;
        }

        inline bool nb_write(const DT &data) {
            payload = data;
            valid = true;
            if (ready.read() == true) {
                return true;
            } else {
                return false;
            }
        }

        inline void reset() {
            valid = false;
        }

        inline void write(const DT &data) {
            payload = data;
            valid = true;
            do { ::wait(); } while (ready.read() == false);
            valid = false;
        }

        inline const DT& operator = (const DT &data) {
            write(data);
            return data;
        }

    public:
        sc_out<DT>   payload;
        sc_in<bool>  ready;
        sc_out<bool> valid;
    }; // class out

public:
    sc_signal<DT>   payload;
    sc_signal<bool> ready;
    sc_signal<bool> valid;
}; // class ready_valid


#endif