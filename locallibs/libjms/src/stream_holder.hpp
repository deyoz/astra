#ifndef __STREAM_HOLDER_HPP
#define __STREAM_HOLDER_HPP

#include <boost/asio.hpp>
#include <memory>
#include <functional>


namespace net = boost::asio;
typedef boost::system::error_code net_error_code;

namespace sirena_net
{

#ifdef HAVE_SSL
    typedef net::ssl::context context_type;
#else
    class null_context
    {
    };

    typedef null_context context_type;
#endif

typedef std::shared_ptr<context_type> context_ptr;

class stream_holder
{
public:
    typedef stream_holder this_type;
    typedef net::ip::tcp::socket stream_type;
#ifdef HAVE_SSL
    typedef net::ssl::stream<net::ip::tcp::socket> ssl_stream_type;
    enum handshake_type
    {
        client = net::ssl::stream_base::client,
        /// Perform handshaking as a server.
        server = net::ssl::stream_base::server
    };
#else
    typedef net::ip::tcp::socket ssl_stream_type;
    enum handshake_type
    {
        /// Perform handshaking as a client.
        client,

        /// Perform handshaking as a server.
        server
    };
#endif

private:
    std::shared_ptr<void> stream_ptr_;
    bool is_ssl_;

    template <typename StreamType,  bool IsTls = (!std::is_same<ssl_stream_type, stream_type>::value && std::is_same<ssl_stream_type, StreamType>::value) > 
    stream_holder(StreamType* p)
        : stream_ptr_( std::shared_ptr<StreamType>(p))
        , is_ssl_(IsTls)
    {}

public:
    typedef ::sirena_net::context_type context_type;

    stream_holder() : is_ssl_(false) {}

    static stream_holder create_stream(net::io_service& io_service, context_ptr context)
    {
        return create_stream(io_service, context.get());
    }
 
    static stream_holder create_stream(net::io_service& io_service, context_type* context)
    {
#ifdef HAVE_SSL
        if(context)
            return stream_holder(new ssl_stream_type(io_service, *context));
        else
#endif
            return this_type::create_stream(io_service);
    }

    static stream_holder create_stream(net::io_service& io_service)
    {
        return stream_holder(new stream_type(io_service));
    }

    net::io_service& get_io_service() const
    {
        if(is_ssl_)
        {
            return ssl_stream().get_io_service();
        }
        return stream().get_io_service();
    }

    bool is_ssl() const
    {
        return is_ssl_;
    }

    explicit operator bool() const
    {
        return bool(stream_ptr_);
    }

    stream_type& stream() const
    {
        if (is_ssl_) throw std::logic_error("Casting ssl stream to non-ssl one");
        return *reinterpret_cast<stream_type *>(stream_ptr_.get());
    }

    ssl_stream_type& ssl_stream() const
    {
        if (!is_ssl_) throw std::logic_error("Casting non ssl stream to ssl one");
        return *reinterpret_cast<ssl_stream_type *>(stream_ptr_.get());
    }

    stream_type::lowest_layer_type& lowest_layer()
    {
        if(is_ssl_)
        {
            return ssl_stream().lowest_layer();
        }
        return stream().lowest_layer();
    }

    void shutdown()
    {
        net_error_code ignored_error;
#ifdef HAVE_SSL
        if(is_ssl_)
        {
            ssl_stream().shutdown(ignored_error);
            ignored_error = net_error_code();
        }
#endif
        lowest_layer().shutdown(net::ip::tcp::socket::shutdown_send, ignored_error);
    }

    void close()
    {
        net_error_code ignored_error;
        lowest_layer().close(ignored_error);
    }

    void cancel()
    {
        lowest_layer().cancel();
    }

    void reset()
    {
        stream_ptr_.reset();
    }

    void handshake(handshake_type type)
    {
#ifdef HAVE_SSL
        if(is_ssl_)
        {
            ssl_stream().handshake(net::ssl::stream_base::handshake_type(type));
        }
#endif
    }

    net_error_code handshake(handshake_type type,
            net_error_code& ec)
    {
#ifdef HAVE_SSL
        if(is_ssl_)
        {
            return ssl_stream().handshake(net::ssl::stream_base::handshake_type(type), ec);
        }
#endif
        ec = net_error_code();
        return ec;
    }


    template <typename HandshakeHandler>
    void async_handshake(handshake_type type, HandshakeHandler handler)
    {
#ifdef HAVE_SSL
        if(is_ssl_)
        {
            ssl_stream().async_handshake(net::ssl::stream_base::handshake_type(type), handler);
            return;
        }
#endif
        get_io_service().post(std::bind(handler,net_error_code()));
    }

    template <typename ShutdownHandler>
    void async_shutdown(ShutdownHandler handler)
    {
        auto internal_handler = [handler, this](const net_error_code& err) {
            net_error_code err2;
            lowest_layer().shutdown(net::ip::tcp::socket::shutdown_send, err2);
            handler(err ? err : err2);
        };
#ifdef HAVE_SSL
        if(is_ssl_)
        {
            ssl_stream().async_shutdown(internal_handler);
            return;
        }
#endif
        get_io_service().post(std::bind(internal_handler, net_error_code()));
    }

    template <typename ConstBufferSequence>
    std::size_t write_some(const ConstBufferSequence& buffers)
    {
        if(is_ssl_)
        {
            return ssl_stream().write_some(buffers);
        }
        return stream().write_some(buffers);
    }


    template <typename ConstBufferSequence>
    std::size_t write_some(const ConstBufferSequence& buffers,
            boost::system::error_code& ec)
    {
        if(is_ssl_)
        {
            return ssl_stream().write_some(buffers, ec);
        }
        return stream().write_some(buffers, ec);
    }


    template<typename ConstBufferSequence, typename WriteHandler>
    void async_write_some(
            const ConstBufferSequence & buffers,
            WriteHandler handler)
    {
        if(is_ssl_)
        {
            ssl_stream().async_write_some(buffers, handler);
        }
        else
        {
            stream().async_write_some(buffers, handler);
        }
    }

    template <typename MutableBufferSequence>
    std::size_t read_some(const MutableBufferSequence& buffers)
    {
        if(is_ssl_)
        {
            return ssl_stream().read_some(buffers);
        }
        return stream().read_some(buffers);
    }

    template <typename MutableBufferSequence>
    std::size_t read_some(const MutableBufferSequence& buffers,
            boost::system::error_code& ec)
    {
        if(is_ssl_)
        {
            return ssl_stream().read_some(buffers, ec);
        }
        return stream().read_some(buffers, ec);
    }

    template <typename MutableBufferSequence, typename ReadHandler>
    void async_read_some(const MutableBufferSequence& buffers,
            ReadHandler handler)
    {
        if(is_ssl_)
        {
            ssl_stream().async_read_some(buffers, handler);
        }
        else
        {
            stream().async_read_some(buffers, handler);
        }

    }
};

}

#endif//__STREAM_HOLDER_HPP



