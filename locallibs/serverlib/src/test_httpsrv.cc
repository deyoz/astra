void init_httpsrv_tests() {}

#ifndef ENABLE_PG_TESTS
#ifdef XP_TESTING
#define NICKNAME "DMITRYVM"
#define NICKTRACE DMITRYVM_TRACE

#include <iostream>
#include "httpsrv.h"
#include "xp_test_utils.h"
#include "checkunit.h"
#include "slogger.h"
#include "test.h"
#include <atomic>
#include <thread>
#include <future>
#include <random>
#include <unordered_set>
#include "stream_holder.h"
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include "cursctl.h"
#include "dump_table.h"
#include "query_runner.h"

namespace {
const std::vector<std::string> CA_certs = {
"-----BEGIN CERTIFICATE-----\n"
"MIIGjDCCBHSgAwIBAgIJANRmqfjbPnjMMA0GCSqGSIb3DQEBDQUAMIGKMQswCQYD\n"
"VQQGEwJSVTEPMA0GA1UECBMGTW9zY293MREwDwYDVQQKEwhLT01URVgtSDEQMA4G\n"
"A1UECxMHSFRUUFNSVjEcMBoGA1UEAxQTSFRUUFNSVl9UZXN0X1Jvb3RDQTEnMCUG\n"
"CSqGSIb3DQEJARYYYS52b3JvbmtvdkBzaXJlbmEyMDAwLnJ1MB4XDTE3MTEyMzE0\n"
"MTYxOFoXDTM5MTAxOTE0MTYxOFowgYoxCzAJBgNVBAYTAlJVMQ8wDQYDVQQIEwZN\n"
"b3Njb3cxETAPBgNVBAoTCEtPTVRFWC1IMRAwDgYDVQQLEwdIVFRQU1JWMRwwGgYD\n"
"VQQDFBNIVFRQU1JWX1Rlc3RfUm9vdENBMScwJQYJKoZIhvcNAQkBFhhhLnZvcm9u\n"
"a292QHNpcmVuYTIwMDAucnUwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoIC\n"
"AQDIE0jY6otQVRQ0Rhl6sQt6aY+vurMpQgqrKBar6XwaQa0fgcemyfMAgtgv7yIh\n"
"28arL1aD/2bcECICjXWRJUZ3qD0UyZ4uSnFOYxXUchSOyiCO9MD59MHt8kJDv1ZI\n"
"SVf/ZeiUaCaOiLbcWAcqRyyJG8gypPbHEqU0Wqfta3g6Zg16MqDdtDxnaRYlnMPk\n"
"wVTiSCUqCt7pfhi+FNbCHYCZwrrOZlxN8Mo445aLFoRNiERyqSrxKPunnRdNsuEN\n"
"yIpk29yEulut06l/PySEZ2+SbzmCeX/1ocJDGtThtgo9vzlvs5p6LfpKEi4oHUv/\n"
"hOnFk5kfXgOgilINH04F1UJbTTI+7jGjmbd5Wwgno2NYhr9gtXWcbTSHtC9xJ+K0\n"
"zBZu01nEKD+jzxfiaphM/OkvN6k1tcoCjULOvpdQUI9d3v8VTXn5Md6OLbeVHv4B\n"
"HoxwiP0pQI5e8O1P0wHP+u+YoA9pM/eFCFqZK4ahRNlBrUkclMnvdWIo6+zBKE4D\n"
"cRflGfY+ZvTZRa4bVW4WDCM8+7h3iGfWNnsgo0/9qAsi++fJzNoX+IoSaZAReSdX\n"
"nW9GV5CXAAekSe1xA4VPPklKXxYIgBLmke5kfgG8KerNNacZ9xg7SJCvHtjNPyu6\n"
"gjdW4cuB/1VALCptqc6DMxhr1jJp0XChhdZLwNkXcEg8LwIDAQABo4HyMIHvMAwG\n"
"A1UdEwQFMAMBAf8wHQYDVR0OBBYEFHl8LNt7ajxjhebsCGfZGs06Mm8ZMIG/BgNV\n"
"HSMEgbcwgbSAFHl8LNt7ajxjhebsCGfZGs06Mm8ZoYGQpIGNMIGKMQswCQYDVQQG\n"
"EwJSVTEPMA0GA1UECBMGTW9zY293MREwDwYDVQQKEwhLT01URVgtSDEQMA4GA1UE\n"
"CxMHSFRUUFNSVjEcMBoGA1UEAxQTSFRUUFNSVl9UZXN0X1Jvb3RDQTEnMCUGCSqG\n"
"SIb3DQEJARYYYS52b3JvbmtvdkBzaXJlbmEyMDAwLnJ1ggkA1Gap+Ns+eMwwDQYJ\n"
"KoZIhvcNAQENBQADggIBAB/Z8EYQY9JMcbZatV/axHzPyydtfyBDVoIUFbj9kG/Q\n"
"cHFJHeRa10UQVxSXKDApHbdMRsHCC8Hc+U7vrTHPVoa04iMjqtS6QpHPSY8mMsqA\n"
"KJQGhhRsiyH3GcdZNkdMCT3PBFLJImGxVIWy58jnjQ8hXcNN+gSZEDDpjznWpNei\n"
"PbLeyXeOSRhR7KWerRoNzjSDs87D0+7O4k1THS6gya9yvwguOz9GiFUYxXMPZ1j4\n"
"PkjeTvvMUgy6bWGCV06QCfmjW9uLy8DTZL575sl60GVmjbPhsBPK8rTUpbjhsR4m\n"
"w3+vta4ERzqS/6NCuFYEcXfJp2DiaBMSNBVZgGgcU/ChRWrDouJ98/fYMgtU6469\n"
"cMUfo7ho8egtFMKmHzEI5bw7WkykVzA0rXIO7nhj+kuomOzOtzDN+WqdFXAFWpbu\n"
"KmLkyx27HTvnMEwnZys/8VYz7PYJ8BcZbpPqDXaFP3JmvssTguxWjH52ar2veqke\n"
"w6KWC69qNIfuNi3dh2EsF8UIJdXnfnTalNu2irPvV+U/A3hH4fLyP+PWea1hzg+T\n"
"yYSUlQcGpBKWRVf5XH85YoIsOYRD18EkZRv0W9J8nCHwwbuMO7HRnL10Q904ooyR\n"
"SuSZyTLbUoCU03ohOAJlrjG04+UEYePNbUOr4C0EKOYIw+SkyRPjOk/od9WX9piS\n"
"-----END CERTIFICATE-----\n"
, //root
"-----BEGIN CERTIFICATE-----\n"
"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\n"
"CK8GMA==\n"
"-----END CERTIFICATE-----\n"
, //lev1
"-----BEGIN CERTIFICATE-----\n"
"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\n"
"ShFARTowOMc=\n"
"-----END CERTIFICATE-----\n"
, //lev2
"-----BEGIN CERTIFICATE-----\n"
"MIIGmDCCBICgAwIBAgIJAMxl+c6lLMVlMA0GCSqGSIb3DQEBDQUAMIGOMQswCQYD\n"
"VQQGEwJSVTEPMA0GA1UECBMGTW9zY293MREwDwYDVQQKEwhLT01URVgtSDEQMA4G\n"
"A1UECxMHSFRUUFNSVjEgMB4GA1UEAxQXSFRUUFNSVl9UZXN0X0xldmVsXzJfQ0Ex\n"
"JzAlBgkqhkiG9w0BCQEWGGEudm9yb25rb3ZAc2lyZW5hMjAwMC5ydTAeFw0xNzEx\n"
"MjMxNDE3MDBaFw0zOTEwMTkxNDE3MDBaMIGOMQswCQYDVQQGEwJSVTEPMA0GA1UE\n"
"CBMGTW9zY293MREwDwYDVQQKEwhLT01URVgtSDEQMA4GA1UECxMHSFRUUFNSVjEg\n"
"MB4GA1UEAxQXSFRUUFNSVl9UZXN0X0xldmVsXzNfQ0ExJzAlBgkqhkiG9w0BCQEW\n"
"GGEudm9yb25rb3ZAc2lyZW5hMjAwMC5ydTCCAiIwDQYJKoZIhvcNAQEBBQADggIP\n"
"ADCCAgoCggIBAODUjctYndoIsov+4X5QMYcr06KNaZUqBA+bnDoh0yP9ZGQkuGgQ\n"
"AxJ4axqbe8Iqu9NaAotj8RuzTE7pApemO89g2ywJcc5DThP7UXAo7vtNbn/GFzH1\n"
"+gSNZBk00GKnirH6g+QyLNcP5jAbuYKcGHt3YrH0jDOj1/OWWt3CXiSAoE53nzw1\n"
"UHc//4zkSLlu72cupv8T4B2sPyGctuIfyitdhMaLS5x0DOO731AwkggC6nvn2ZzO\n"
"bC5AQx7zRcApWb8OGoxt4ayqVZv/wsriQm11wmyCXgNFh2y4dk/IIlsxpxraBd7M\n"
"0Pg9T+MkhBLzpZCl2Igzw2IMJB5CPTcQGOFg9h7JU0yPMEvV1DF4A2SORacQhVLN\n"
"kiTPWHFfRoxcVLF8hXjhN8ahuTL4W4ryb6IhJ4KZ3y1bfmf0T6ixfEqlZZhrUVs+\n"
"wt9HrOJ3xtKlCP45flXDi06qoxyazs3Ox4fh0WGJ4MbVjaRYdA2Or0bx7aOkCx4W\n"
"JgkqLMlaSVogK4epcpzyJ0xJCVgGFalLw9xgxyZvdG9NPzGzpZnJcAZMxdbX08mB\n"
"EZTy5JZmNDY25qUq5oRDkqMsOgztrq5neIYVVLBnAO1pf+04PWfg+NpnZAZvtgbw\n"
"Tbh/ABJ+bCuewHk7VP+ajcc22u+7KB0+fURf3baH7k5VWWlLD7zvX7iXAgMBAAGj\n"
"gfYwgfMwDAYDVR0TBAUwAwEB/zAdBgNVHQ4EFgQUo7YhaB0TMAmjswgw2/m/6A9r\n"
"FYIwgcMGA1UdIwSBuzCBuIAUBWquhtF4kn+Y2MGA2yrpJYgCX/WhgZSkgZEwgY4x\n"
"CzAJBgNVBAYTAlJVMQ8wDQYDVQQIEwZNb3Njb3cxETAPBgNVBAoTCEtPTVRFWC1I\n"
"MRAwDgYDVQQLEwdIVFRQU1JWMSAwHgYDVQQDFBdIVFRQU1JWX1Rlc3RfTGV2ZWxf\n"
"MV9DQTEnMCUGCSqGSIb3DQEJARYYYS52b3JvbmtvdkBzaXJlbmEyMDAwLnJ1ggkA\n"
"9xv6mA2HB9YwDQYJKoZIhvcNAQENBQADggIBAE3Fk0KQLFsP/GpYO33zJ+QpY99u\n"
"KMlkU8vDLAebLpfRO3lUMXXfnTm3ZYotKQQbGSOgBJv7XJDImO7XS0zdtrwyxsn9\n"
"oOOPnuoEbwQOgPx6y3JwPvLgWetysDnUBPL6odK25vIEr5hW0GbLjZnRIOJzxqiu\n"
"hqTX3WV/6TsvDMCoJj68GruKfr50YycxmkeE51UtCAkS+w0ZNOlYvjo0fjgabe9/\n"
"mPPYojNUNTTP6IVIaZoQN0lXyuNcPa2KiuKgviqPAjJNl0WHK4Xe5AKaQc9vN8oU\n"
"mvR8/okn8uJTjF798+ISbX1zuKPkDtFVjsXO/Y5II2rV4Y+7qK6xbEgXDsXR5KIJ\n"
"kA1NyLTeF9N1XpLAv0LOjGmtuDhJFJudqp22w73GkPlSsdK7mJxyNY3Gl3nZKxal\n"
"Dn7/hoB8x4BcEECU/9GgZmmtGlDUkz4gDshozQTjdzYpNyfPQu8x8PSC1Vlg6bLi\n"
"oop6L2x8yxCnMvrfrm18hnNd3HeuXJD2vzgdeYKXtR5Dfrz95bh8+uCe2/EeHiq4\n"
"T/wbQmxRcheQ7JweZc9opf8KS4c2nvhDmrfi/uj80i/n57IWUL7ogNLuJvqOgw30\n"
"e0MUECYE1Xtv+Rzcu6GYUyuVwDWMjzOSg1iduwcvJZ5DiPmAsLMxu5qAvE1k8oz+\n"
"gHu3lNLEsWfIeGZJ\n"
"-----END CERTIFICATE-----\n"
}; // lev3

const std::pair<std::string, std::string> ServCertKey = {
"-----BEGIN CERTIFICATE-----\n"
"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\n"
"-----END CERTIFICATE-----\n"
,
"-----BEGIN PRIVATE KEY-----\n"
"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\n"
"5awfbwl7dTXV5C05GIq8resLOis6mw==\n"
"-----END PRIVATE KEY-----\n"
};


const std::pair<std::string, std::string> ClientCertKey = {
"-----BEGIN CERTIFICATE-----\n"
"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\n"
"-----END CERTIFICATE-----\n"
,
"-----BEGIN PRIVATE KEY-----\n"
"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\n"
"qUL1HVn0YAJ+Iwrnzz5glNTv+OfzjeA=\n"
"-----END PRIVATE KEY-----\n"
};

const std::string HttpReq =
    "GET / HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux i686; rv:11.0) Gecko/20100101 Firefox/11.0\r\n"
    "Accept-Encoding: identity\r\n"
    "Accept: text/html\r\n\r\n";

const std::string HttpResp =
    "HTTP/1.1 200 OK\r\n"
    "Server: Dumb Test HTTP Server\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n"
    "Content-Length: 20\r\n"
    "Connection: close \r\n"
    "\r\n"
    "That's all, Folks!\r\n"
    ;

const std::string HttpErr =
    "HTTP/1.1 400 Bad Request\r\n\r\n";



class connection;
typedef std::shared_ptr<connection> connection_ptr;
typedef std::unordered_set<connection_ptr> connection_list;


class connection
  : public std::enable_shared_from_this<connection>
{
public:
  connection(const connection&) = delete;
  connection& operator=(const connection&) = delete;
  static void create(sirena_net::stream_holder&& stream, connection_list& list, bool is_blackhole)
  {
      connection_ptr p(new connection(std::move(stream), list));
      p->start(is_blackhole);
      list.emplace(p);
  }
  /// Start the first asynchronous operation for the connection.
  void start(bool is_blackhole)
  {
//      std::cerr << "New connection " << this << " (" << is_blackhole << ")\n";
      boost::asio::async_read_until(stream_, buffer_, "\r\n\r\n",
              [this, self = shared_from_this(), is_blackhole] (auto err, auto count) {
                  if (err) {
                      self->stop();
                  } else {
                      auto cb = [self](auto err, auto count) { self->stop(!err); };
                      std::string req(boost::asio::buffers_begin(buffer_.data()), boost::asio::buffers_begin(buffer_.data()) + count);
                      buffer_.consume(count);
                      if (HttpReq.compare(req)) {
                          boost::asio::async_write(stream_,
                              boost::asio::buffer(HttpErr + req),
                              cb
                              );
                      } else if (!is_blackhole) {
                          boost::asio::async_write(stream_, boost::asio::buffer(HttpResp), cb );
                      } else { //will read until connection close
                          boost::asio::async_read(stream_, buffer_, boost::asio::transfer_at_least(1), cb);
                      }
                  }
              });
  }

  /// Stop all asynchronous operations associated with the connection.
  void stop(bool graceful = false)
  {
      if (graceful) {
          stream_.async_shutdown([self = shared_from_this()](auto err) { self->close();} );
      } else {
          close();
      }
  }
  void close() {
      stream_.close();
      list_.erase(shared_from_this());
  }

private:
  connection(sirena_net::stream_holder&& stream, connection_list& list)
      : stream_(std::move(stream)), list_(list)
  {}
  boost::asio::streambuf buffer_;
  sirena_net::stream_holder stream_;
  connection_list& list_;
};


class dumb_http_srv {
public:
static auto create_context(const std::pair<std::string, std::string>& CertKey, const std::vector<std::string>& CA)
{
    auto ctx = std::make_unique<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
    ctx->use_certificate(boost::asio::buffer(std::get<0>(CertKey)), boost::asio::ssl::context::pem);
    ctx->use_private_key(boost::asio::buffer(std::get<1>(CertKey)), boost::asio::ssl::context::pem);
    for (const auto& CA_cert: CA) {
        ctx->add_certificate_authority(boost::asio::buffer(CA_cert));
    }
    return ctx;
}

explicit dumb_http_srv(std::unique_ptr<boost::asio::ssl::context> ctx = nullptr)
    : is_ssl_(ctx), address("localhost"), port_(1000), answer_at_{1}
{
    running.test_and_set();
    std::promise<short> port_ret;
    auto port = port_ret.get_future();
    std::packaged_task<int (std::promise<short>, std::unique_ptr<boost::asio::ssl::context>) >
        task(std::bind(&dumb_http_srv::test_http_srv_thread, this, std::placeholders::_1, std::placeholders::_2));
    serv_ret = task.get_future();
    serv_thread = std::thread(std::move(task), std::move(port_ret), std::move(ctx));
    try {
        port_ = port.get();
    } catch (const std::future_error& e) { // exception before accepting
        LogTrace(TRACE1) << "Future Error: " << e.what();
        auto ret = serv_ret.get();
        LogTrace(TRACE1) << "Thread returned: " << ret;
        throw;
    } catch (const std::exception& e) {
        LogTrace(TRACE1) << "Error in Thread: " << e.what();
        throw;
    }
    LogTrace(TRACE1) << "Listener started at " << port_;
}

~dumb_http_srv()
{
    running.clear();
    LogTrace(TRACE1) << "Shutting down";
    try {
        auto ret = serv_ret.get();
        LogTrace(TRACE1) << "Thread exit with code: " << ret;
    } catch (const std::exception& e) {
        LogTrace(TRACE1) << "Thread exit with exception: " << e.what();
    }
    if (serv_thread.joinable()) {
        LogTrace(TRACE5) << "Join";
        serv_thread.join();
    }

}
void answer_at(unsigned int val)
{
    ASSERT(val > 0);
    answer_at_.store(val);
}

unsigned short port() const 
{
    return port_;
}

bool is_ssl() const
{
    return is_ssl_;
}

private:
bool is_ssl_;
std::thread serv_thread;
std::future<int> serv_ret;
std::atomic_flag running;
std::string address;
unsigned short port_;
std::atomic<unsigned int> answer_at_;

int test_http_srv_thread(std::promise<short> port_ret, std::unique_ptr<boost::asio::ssl::context> ctx)
{

    boost::asio::io_service ios;
    boost::asio::ip::tcp::resolver resolver(ios);
    boost::asio::ip::tcp::acceptor acceptor(ios);
    boost::asio::steady_timer timer(ios);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned short> dis(10000U, 60000U);

    constexpr int max_try = 100;
    unsigned short port = 0;
    for (int i = 0 ; ; ++i ) {
        port = dis(gen);
        boost::asio::ip::tcp::resolver::query query(address, std::to_string(port));
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
        acceptor.open(endpoint.protocol());
        acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        boost::system::error_code ec;
        acceptor.bind(endpoint, ec);
        if (!ec) {
            break;
        } else if ( i > max_try) {
            port_ret.set_exception(std::make_exception_ptr(boost::system::system_error(ec)));
            return 1;
        }
        acceptor.close();
    };
    acceptor.listen();

    connection_list connections;
    std::function<void()> do_accept;
    unsigned int number = 0;
    do_accept = [&, this] {
        sirena_net::stream_holder new_stream = sirena_net::stream_holder::create_stream(ios, ctx.get());
        acceptor.async_accept(
                new_stream.lowest_layer(),
                [new_stream, this, &acceptor, &connections, &number, &do_accept](auto err) mutable {
                    if (!err) {
                    new_stream.async_handshake(sirena_net::stream_holder::server,
                            [new_stream, this, &acceptor, &connections, &number] (auto err) mutable {
                                if (acceptor.is_open() && !err) {
                                    connection::create(std::move(new_stream), connections, (number = (number + 1) % answer_at_));
                                } else {
                                    new_stream.close();
                                }
                            });
                        do_accept();
                    } else {
                        new_stream.close();
                    }
                });
    };


    const auto check_period = std::chrono::milliseconds(250);

    std::function<void()> do_timer;

    do_timer = [&] {
        timer.expires_from_now(check_period);
        timer.async_wait([&](auto err) {
            if (running.test_and_set()) {
            do_timer();
            } else {
                acceptor.close();
                while (!connections.empty()) {
                    (*connections.begin())->stop();
                }
            }
            });
    };

    do_timer();
    do_accept();
    port_ret.set_value(port);
    ios.run();
//    std::cerr << "Dumb Server Stopped" << "\n";
    return 0;
}


};



template <typename C>
void DoLocalRequest(C&& corr, const std::string& domain, unsigned short port, bool is_ssl, bool with_auth = false)
{
    httpsrv::DoHttpRequest r(
            std::forward<C>(corr),
            httpsrv::Domain(domain),
            httpsrv::HostAndPort("localhost", port),
            HttpReq);
    r.setTimeout(boost::posix_time::seconds(5))
        .setSSL(httpsrv::UseSSLFlag(is_ssl));
    if (with_auth) {
        r.setClientAuth(
                httpsrv::ClientAuth(
                    httpsrv::Certificate(std::get<0>(ClientCertKey)),
                    httpsrv::PrivateKey(std::get<1>(ClientCertKey))
                    ));
    }
    r();
}


template <typename C>
void DoRequest(C&& corr, const std::string& domain)
{
    const char* HTTPREQ =
        "GET / HTTP/1.1\r\n"
        "Host: www.openssl.org\r\n"
        "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux i686; rv:11.0) Gecko/20100101 Firefox/11.0\r\n"
        "Accept-Encoding: identity\r\n"
        "Accept: text/html\r\n\r\n";

    httpsrv::DoHttpRequest(
            std::forward<C>(corr),
            httpsrv::Domain(domain),
            httpsrv::HostAndPort("www.openssl.org", 443),
            HTTPREQ)
        ();
}

template <typename C>
void CheckResponses(STDLOG_SIGNATURE,
        C&& corr,
        const std::string& domain,
        size_t expectedNumResp)
{
    const std::vector<httpsrv::HttpResp> responses = httpsrv::FetchHttpResponses(
            std::forward<C>(corr),
            httpsrv::Domain(domain));
    LogTrace(getTraceLev(TRACE5),nick,file,line)
      << __FUNCTION__ << ": responses.size() = " << responses.size();

    fail_unless(responses.size() == expectedNumResp);
    for (const httpsrv::HttpResp& resp:  responses) {
        fail_unless(resp.text.find("HTTP/1.1 200 OK") == 0, "Got %s", resp.text.c_str());
    }
}

template <typename C>
void CheckErr(STDLOG_SIGNATURE,
        C&& corr,
        const std::string& domain,
        size_t expectedNumResp,
        httpsrv::CommErrCode expected,
        httpsrv::CommErrCode alt_expected
        )
{
    const std::vector<httpsrv::HttpResp> responses = httpsrv::FetchHttpResponses(
            std::forward<C>(corr),
            httpsrv::Domain(domain));
    LogTrace(getTraceLev(TRACE5),nick,file,line)
      << __FUNCTION__ << ": responses.size() = " << responses.size();
    LogTrace(getTraceLev(TRACE5),nick,file,line)
      << __FUNCTION__ << ": expected = " << expected << " alt_expected = " << alt_expected;

    fail_unless(responses.size() == expectedNumResp);
    for (const httpsrv::HttpResp& resp:  responses) {
        fail_unless(resp.commErr);
        fail_unless(resp.commErr->code == expected || resp.commErr->code == alt_expected
           , " Wrong error : %d  expected: %d", resp.commErr->code, expected );
    }
}

template <typename C>
void CheckErr(STDLOG_SIGNATURE,
        C&& corr,
        const std::string& domain,
        size_t expectedNumResp,
        httpsrv::CommErrCode expected
        )
{
  return CheckErr<C>(nick,file,line,
        std::forward<C>(corr),domain,expectedNumResp,expected,expected);
}

namespace {
void check_with_different_corr(dumb_http_srv& srv)
{
    DoLocalRequest(httpsrv::Pult("PULT_A"), "DOMAIN_A", srv.port(), srv.is_ssl());
    DoLocalRequest(httpsrv::Pult("PULT_B"), "DOMAIN_A", srv.port(), srv.is_ssl());
    DoLocalRequest(httpsrv::Pult("PULT_B"), "DOMAIN_A", srv.port(), srv.is_ssl());
    ServerFramework::InternalMsgId id1(100U, 200U, 300U);
    ServerFramework::InternalMsgId id2(111U, 222U, 333U);
    DoLocalRequest(id1, "DOMAIN_A", srv.port(), srv.is_ssl());
    DoLocalRequest(id2, "DOMAIN_B", srv.port(), srv.is_ssl());
    DoLocalRequest(id1, "DOMAIN_B", srv.port(), srv.is_ssl());
    DoLocalRequest(httpsrv::CorrelationID("SLONOPOTAM"), "DOMAIN_B", srv.port(), srv.is_ssl());
    DoLocalRequest(httpsrv::CorrelationID("SLONOPOTAM"), "DOMAIN_A", srv.port(), srv.is_ssl());
    DoLocalRequest(id2, "DOMAIN_A", srv.port(), srv.is_ssl());
    DoLocalRequest(httpsrv::CorrelationID("SLONOPOTAM"), "DOMAIN_A", srv.port(), srv.is_ssl());
    DoLocalRequest(id1, "DOMAIN_B", srv.port(), srv.is_ssl());
    DoLocalRequest(httpsrv::CorrelationID("MY FAKE GUID"), "DOMAIN_A", srv.port(), srv.is_ssl());
    DoLocalRequest(id2, "DOMAIN_B", srv.port(), srv.is_ssl());
    DoLocalRequest(id1, "DOMAIN_B", srv.port(), srv.is_ssl());
    DoLocalRequest(httpsrv::Pult("PULT_B"), "DOMAIN_A", srv.port(), srv.is_ssl());
    DoLocalRequest(httpsrv::CorrelationID("MY FAKE GUID"), "DOMAIN_B", srv.port(), srv.is_ssl());
    DoLocalRequest(httpsrv::CorrelationID("MY FAKE GUID"), "DOMAIN_A", srv.port(), srv.is_ssl());
    DoLocalRequest(httpsrv::Pult("PULT_B"), "DOMAIN_B", srv.port(), srv.is_ssl());

    CheckResponses(STDLOG,httpsrv::Pult("PULT_A"), "DOMAIN_A", 1);
    CheckResponses(STDLOG,id1, "DOMAIN_B", 3);
    CheckResponses(STDLOG,httpsrv::Pult("PULT_B"), "DOMAIN_A", 3);
    CheckResponses(STDLOG,id1, "DOMAIN_A", 1);
    CheckResponses(STDLOG,httpsrv::Pult("PULT_B"), "DOMAIN_B", 1);
    CheckResponses(STDLOG,id2, "DOMAIN_B", 2);
    CheckResponses(STDLOG,id2, "DOMAIN_A", 1);
    CheckResponses(STDLOG,httpsrv::CorrelationID("SLONOPOTAM"), "DOMAIN_A", 2);
    CheckResponses(STDLOG,httpsrv::CorrelationID("SLONOPOTAM"), "DOMAIN_B", 1);
    CheckResponses(STDLOG,httpsrv::CorrelationID("MY FAKE GUID"), "DOMAIN_A", 2);
    CheckResponses(STDLOG,httpsrv::CorrelationID("MY FAKE GUID"), "DOMAIN_B", 1);

    CheckResponses(STDLOG,id1, "DOMAIN_A", 0);
    CheckResponses(STDLOG,id1, "DOMAIN_B", 0);
    CheckResponses(STDLOG,id2, "DOMAIN_B", 0);
    CheckResponses(STDLOG,id2, "DOMAIN_A", 0);
    CheckResponses(STDLOG,httpsrv::Pult("PULT_A"), "DOMAIN_A", 0);
    CheckResponses(STDLOG,httpsrv::Pult("PULT_B"), "DOMAIN_A", 0);
    CheckResponses(STDLOG,httpsrv::Pult("PULT_B"), "DOMAIN_B", 0);
    CheckResponses(STDLOG,httpsrv::CorrelationID("SLONOPOTAM"), "DOMAIN_A", 0);
    CheckResponses(STDLOG,httpsrv::CorrelationID("SLONOPOTAM"), "DOMAIN_B", 0);
    CheckResponses(STDLOG,httpsrv::CorrelationID("MY FAKE GUID"), "DOMAIN_A", 0);
    CheckResponses(STDLOG,httpsrv::CorrelationID("MY FAKE GUID"), "DOMAIN_B", 0);


}    
}//anonymous_ns
START_TEST(Check_HTTP)
{
    dumb_http_srv srv;
    check_with_different_corr(srv);
}
END_TEST;

START_TEST(Check_HTTPS_NoAuth)
{
    dumb_http_srv srv(dumb_http_srv::create_context(ServCertKey, CA_certs));
    check_with_different_corr(srv);
}
END_TEST;

START_TEST(Check_HTTPS_Auth)
{
    std::string domain = "HTTPSRV_TEST_DOMAIN";
    httpsrv::CorrelationID cid("SLONOPOTAM");
    auto del = make_curs("delete from httpca WHERE domain = :domain");
    del.bind(":domain", domain).exec();
    commitInTestMode();


    {
        dumb_http_srv srv(dumb_http_srv::create_context(ServCertKey, CA_certs));
        DoLocalRequest(cid, domain, srv.port(), srv.is_ssl());
        CheckResponses(STDLOG,cid, domain, 1);
        DoLocalRequest(cid, domain, srv.port(), srv.is_ssl(), true);
        CheckResponses(STDLOG,cid, domain, 1);
    }
    {
        auto ctx = dumb_http_srv::create_context(ServCertKey, {CA_certs.back()} );
        ctx->set_verify_mode(boost::asio::ssl::context::verify_peer);
        dumb_http_srv srv(std::move(ctx));
        DoLocalRequest(cid, domain, srv.port(), srv.is_ssl());
        CheckResponses(STDLOG,cid, domain, 1);
        DoLocalRequest(cid, domain, srv.port(), srv.is_ssl(), true);
        
        // Всегда было: ошибка COMMERR_HANDSHAKE(5) : 
        // >>>>>DMITRYVM:httpsrv.cc:1182 onHandshakeComplete: 0x5556ae31c100: stream truncated: localhost:12903 GET / HTTP/1.1 CorrId: SLONOPOTAM

        // Сейчас: ошибка COMMERR_READ_HEADERS(7) : 
        // >>>>>DMITRYVM:httpsrv.cc:1302 onReadHeadersComplete: 0x55ce0e677b60: Connection reset by peer: localhost:26599 GET / HTTP/1.1 CorrId: SLONOPOTAM
        CheckErr(STDLOG,cid, domain, 1, httpsrv::COMMERR_HANDSHAKE, httpsrv::COMMERR_READ_HEADERS);
    }
    {
        auto ctx = dumb_http_srv::create_context(ServCertKey, CA_certs);
        ctx->set_verify_mode(boost::asio::ssl::context::verify_peer | boost::asio::ssl::context::verify_fail_if_no_peer_cert);
        dumb_http_srv srv(std::move(ctx));
        DoLocalRequest(cid, domain, srv.port(), srv.is_ssl());
        
        // Всегда было: ошибка COMMERR_HANDSHAKE(5) : 
        // >>>>>DMITRYVM:httpsrv.cc:1182 onHandshakeComplete: 0x5556ae31c100: stream truncated: localhost:12903 GET / HTTP/1.1 CorrId: SLONOPOTAM

        // Сейчас: ошибка COMMERR_READ_HEADERS(7) : 
        // >>>>>DMITRYVM:httpsrv.cc:1302 onReadHeadersComplete: 0x55ce0e677b60: Connection reset by peer: localhost:26599 GET / HTTP/1.1 CorrId: SLONOPOTAM
        CheckErr(STDLOG,cid, domain, 1, httpsrv::COMMERR_HANDSHAKE, httpsrv::COMMERR_READ_HEADERS);

        DoLocalRequest(cid, domain, srv.port(), srv.is_ssl(), true);
        CheckResponses(STDLOG,cid, domain, 1);
    }
    {
        auto ctx = dumb_http_srv::create_context(ServCertKey, CA_certs);
        ctx->set_verify_mode(boost::asio::ssl::context::verify_peer);
        dumb_http_srv srv(std::move(ctx));
        DoLocalRequest(cid, domain, srv.port(), srv.is_ssl());
        CheckResponses(STDLOG,cid, domain, 1);
        DoLocalRequest(cid, domain, srv.port(), srv.is_ssl(), true);
        CheckResponses(STDLOG,cid, domain, 1);

        std::string cert;
        auto ins = make_curs("insert into httpca(domain, cert) values(:domain,:cert) ");
        ins.bind(":domain", domain).unstb().bind(":cert", cert);
        cert = CA_certs.back();
        ins.exec();
        commitInTestMode();
        DoLocalRequest(cid, domain, srv.port(), srv.is_ssl());
        CheckErr(STDLOG,cid, domain, 1, httpsrv::COMMERR_HANDSHAKE);
        DoLocalRequest(cid, domain, srv.port(), srv.is_ssl(), true);
        CheckErr(STDLOG,cid, domain, 1, httpsrv::COMMERR_HANDSHAKE);
        del.exec();
        for (const auto& c: CA_certs) {
            cert = c;
            ins.exec();
        }
        commitInTestMode();
        DoLocalRequest(cid, domain, srv.port(), srv.is_ssl());
        CheckResponses(STDLOG,cid, domain, 1);
        DoLocalRequest(cid, domain, srv.port(), srv.is_ssl(), true);
        CheckResponses(STDLOG,cid, domain, 1);
        del.exec();
        commitInTestMode();
        commit();
    }


}
END_TEST;


START_TEST(Check_Timeouts)
{
    dumb_http_srv srv;
    const httpsrv::Domain domain("DOMAIN_A");
    //first request will fork httpsrv
    {
        const ServerFramework::InternalMsgId msgid0(0,0,0);
    httpsrv::DoHttpRequest(msgid0,
            domain,
            httpsrv::HostAndPort("localhost", srv.port()),
            HttpReq)
        .setSSL(httpsrv::UseSSLFlag(srv.is_ssl()))
        ();
    auto responses = httpsrv::FetchHttpResponses(msgid0, domain);
    fail_unless(responses.size() == 1);
    fail_unless(responses.front().text == HttpResp, "Invalid response %s", responses.front().text.c_str());
    }
    auto retries = 5U;
    srv.answer_at(retries);
    auto timeout = boost::posix_time::milliseconds(500);

    std::vector<decltype(std::chrono::high_resolution_clock::now())> tps;
    tps.reserve(retries);
    auto lap = [&tps]{ tps.emplace_back(std::chrono::high_resolution_clock::now()); };

    lap();
    for (unsigned i = 1; i <= retries; ++i) {
        httpsrv::DoHttpRequest(
                ServerFramework::InternalMsgId(i, i, i),
                domain,
                httpsrv::HostAndPort("localhost", srv.port()),
                HttpReq)
            .setTimeout(timeout * i)
            .setSSL(httpsrv::UseSSLFlag(srv.is_ssl()))
            ();
    }

    auto responses = httpsrv::FetchHttpResponses(
                ServerFramework::InternalMsgId(retries, retries, retries), domain);
    lap();
    fail_unless(responses.size() == 1);
    fail_unless(responses.front().text == HttpResp, "Invalid response %s", responses.front().text.c_str());
    fail_unless(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                tps.back() - tps.front()).count() < timeout.total_milliseconds());
    for (unsigned i = 1; i < retries; ++i) {
        auto responses = httpsrv::FetchHttpResponses(
                ServerFramework::InternalMsgId(i, i, i), domain);
        lap();
        auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                tps.back() - tps.front());
        fail_unless(responses.size() == 1);
        fail_unless(responses.front().commErr);
        fail_unless(responses.front().commErr->code == httpsrv::COMMERR_TIMEOUT);
        fail_unless(diff_ms.count() >= (timeout * i).total_milliseconds());
        fail_unless(diff_ms.count() < (timeout * (i + 1)).total_milliseconds());
    }

    const boost::optional<httpsrv::Stat> stat = httpsrv::GetStat();
    fail_unless((bool)stat);
    LogTrace(TRACE5) << __FUNCTION__ << ": stat: " << *stat;
    fail_unless(stat->totalConnErr == retries - 1);
    fail_unless(stat->totalConnErrTimeout == retries - 1);
    fail_unless(stat->activeConnections < stat->totalConnections);
    fail_unless(stat->activeSessions < stat->totalSessions);
    fail_unless(stat->totalConnections == retries + 1);
    fail_unless(stat->totalSessions == retries * 2 + 3);
}
END_TEST;


START_TEST(Check_Perespros)
{
    httpsrv::xp_testing::set_need_real_commit(true);
    const httpsrv::Pult pult("PULT_A");
    const httpsrv::Domain domain("DOMAIN_A");
    auto timeout = boost::posix_time::milliseconds(500);
    auto retries = 10;
    int count = 0;
    dumb_http_srv srv(dumb_http_srv::create_context(ServCertKey, CA_certs));
    srv.answer_at(retries);
    make_curs("delete from EDI_HELP where PULT = :pult").bind(":pult", pult.str()).exec();
    commitInTestMode();
    ServerFramework::getQueryRunner().setPult(pult.str());

    ServerFramework::InternalMsgId id1(0xdeadbeef, 0xBAADF00D, 0xFEEDCAFE);
    set_internal_msg_id(reinterpret_cast<const int*>( id1.id().data() ));

    httpsrv::DoHttpRequest(
            pult,
            domain,
            httpsrv::HostAndPort("localhost", srv.port()),
            HttpReq)
        .setTimeout(timeout)
        .setMaxTryCount(retries)
        .setPeresprosReq("XXX")
        ();

    commitInTestMode();


    OciCpp::DumpTable dt("EDI_HELP");
    dt.exec(TRACE5);
    auto sel = make_curs("select count(*) from EDI_HELP where PULT = :pult");
    sel.bind(":pult", pult.str()).def(count);

    sel.EXfet();
    fail_unless(count == 1);
    std::this_thread::sleep_for(std::chrono::milliseconds((timeout * (retries / 2 + retries % 2)).total_milliseconds()));
    sel.EXfet();
    fail_unless(count == 1);
    std::this_thread::sleep_for(std::chrono::milliseconds((timeout * (retries / 2)).total_milliseconds()));

    auto wait_for_perespros = [&count, &sel, &timeout] {
            for (int i = 0 ; i < 40 && count; ++i) {
                if (i) {
                    LogTrace(TRACE1) << "Переспрос не вызван, высокая загрузка системы?";
                    std::this_thread::sleep_for(std::chrono::milliseconds(timeout.total_milliseconds()));
                }
                sel.EXfet();
            }
        };
    wait_for_perespros();
    fail_unless(count == 0);
    auto responses = httpsrv::FetchHttpResponses(pult, domain);

    fail_unless(responses.size() == 1);
    fail_unless(responses.front().text == HttpResp, "Invalid response %s", responses.front().text.c_str());


    LogTrace(TRACE5) << __FUNCTION__ << ": responses.size() = " << responses.size();

    auto stat = httpsrv::GetStat();
    fail_unless((bool)stat);
    LogTrace(TRACE5) << __FUNCTION__ << ": stat: " << *stat;
    fail_unless(stat->nRetriesOK == 1);
    fail_unless(stat->totalConnErrTimeout == retries - stat->nRetriesOK);
    fail_unless(stat->totalConnErr == stat->totalConnErr);
    fail_unless(stat->activeConnections < stat->totalConnections);
    fail_unless(stat->activeSessions < stat->totalSessions);
    fail_unless(stat->nRetries == retries - stat->nRetriesOK);

    auto retries2 = 15;
    srv.answer_at(retries2);

    httpsrv::DoHttpRequest(
            id1,
            domain,
            httpsrv::HostAndPort("localhost", srv.port()),
            HttpReq)
        .setTimeout(timeout)
        .setMaxTryCount(retries2)
        .setPeresprosReq("XXX")
        ();

    commitInTestMode();


    dt.exec(TRACE5);
    sel.EXfet();
    fail_unless(count == 1);
    std::this_thread::sleep_for(std::chrono::milliseconds((timeout * (retries2 / 2 + retries2 % 2)).total_milliseconds()));
    sel.EXfet();
    fail_unless(count == 1);
    std::this_thread::sleep_for(std::chrono::milliseconds((timeout * (retries2 / 2)).total_milliseconds()));
    wait_for_perespros();
    fail_unless(count == 0);
    responses = httpsrv::FetchHttpResponses(pult, domain);
    fail_unless(responses.size() == 0);
    responses = httpsrv::FetchHttpResponses(id1, domain);
    fail_unless(responses.size() == 1);
    fail_unless(responses.front().text == HttpResp, "Invalid response %s", responses.front().text.c_str());


    LogTrace(TRACE5) << __FUNCTION__ << ": responses.size() = " << responses.size();

    stat = httpsrv::GetStat();
    fail_unless((bool)stat);
    LogTrace(TRACE5) << __FUNCTION__ << ": stat: " << *stat;
    fail_unless(stat->nRetriesOK == 2);
    fail_unless(stat->totalConnErrTimeout == retries + retries2 - stat->nRetriesOK);
    fail_unless(stat->totalConnErr == stat->totalConnErr);
    fail_unless(stat->activeConnections < stat->totalConnections);
    fail_unless(stat->activeSessions < stat->totalSessions);
    fail_unless(stat->nRetries == retries + retries2 - stat->nRetriesOK);
    commit();

} END_TEST;

START_TEST(Check_AezhSchedule)
{
    const char* HTTPREQ =
        "GET /schedule HTTP/1.1\r\n"
        "Host: aeroexpress.ru\r\n"
        "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux i686; rv:11.0) Gecko/20100101 Firefox/11.0\r\n"
        "Accept-Encoding: identity\r\n"
        "Accept: text/html\r\n\r\n";

    httpsrv::DoHttpRequest(
            httpsrv::Pult("PULT_A"),
            httpsrv::Domain("AEZH"),
            httpsrv::HostAndPort("aeroexpress.ru", 443),
            HTTPREQ)
        ();

    const std::vector<httpsrv::HttpResp> responses = httpsrv::FetchHttpResponses(
            httpsrv::Pult("PULT_A"),
            httpsrv::Domain("AEZH"));
    fail_unless(responses.size() == 1);
}
END_TEST;

START_TEST(Check_Timeout)
{
    const char* HTTPREQ =
        "GET /index.html HTTP/1.1\r\n"
        "Host: openssl.org\r\n"
        "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux i686; rv:11.0) Gecko/20100101 Firefox/11.0\r\n"
        "Accept-Encoding: identity\r\n"
        "Accept: text/html\r\n\r\n";

    const httpsrv::Pult pult("PULT_A");
    const httpsrv::Domain domain("DOMAIN_A");
    
    size_t connCount = 0;
    for (unsigned timeout_ms = 0; timeout_ms < 1000; timeout_ms += 10) {
        std::cout << "timeout_ms = " << timeout_ms << std::endl;
        ++connCount;
        httpsrv::DoHttpRequest(
                pult,
                domain,
                httpsrv::HostAndPort("openssl.org", 443),
                HTTPREQ)
            .setTimeout(boost::posix_time::milliseconds(timeout_ms))
            ();
        const std::vector<httpsrv::HttpResp> responses = httpsrv::FetchHttpResponses(pult, domain);
        LogTrace(TRACE5) << __FUNCTION__ << ": responses.size() = " << responses.size();
    }

    const boost::optional<httpsrv::Stat> stat = httpsrv::GetStat();
    fail_unless((bool)stat);
    LogTrace(TRACE5) << __FUNCTION__ << ": stat: " << *stat;
    fail_unless(stat->totalConnErr < stat->totalConnections);
    fail_unless(stat->totalConnErrTimeout > 0);
    fail_unless(stat->activeConnections < stat->totalConnections);
    fail_unless(stat->activeSessions < stat->totalSessions);
    fail_unless(stat->totalConnections == connCount);
    fail_unless(stat->totalSessions == connCount * 2 + 1);
}
END_TEST;

START_TEST(Check_TimeoutWithRetries)
{
    const char* HTTPREQ =
        "GET /index.html HTTP/1.1\r\n"
        "Host: openssl.org\r\n"
        "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux i686; rv:11.0) Gecko/20100101 Firefox/11.0\r\n"
        "Accept-Encoding: identity\r\n"
        "Accept: text/html\r\n\r\n";

    const httpsrv::Pult pult("PULT_A");
    const httpsrv::Domain domain("DOMAIN_A");
    
    size_t connCount = 0;
    for (unsigned timeout_ms = 0; timeout_ms < 1000; timeout_ms += 10) {
        std::cout << "timeout_ms = " << timeout_ms << std::endl;
        ++connCount;
        httpsrv::DoHttpRequest(
                pult,
                domain,
                httpsrv::HostAndPort("openssl.org", 443),
                HTTPREQ)
            .setTimeout(boost::posix_time::milliseconds(timeout_ms))
            .setMaxTryCount(3)
            ();
    }
    const std::vector<httpsrv::HttpResp> responses = httpsrv::FetchHttpResponses(pult, domain);
    LogTrace(TRACE5) << __FUNCTION__ << ": responses.size() = " << responses.size();

    const boost::optional<httpsrv::Stat> stat = httpsrv::GetStat();
    fail_unless((bool)stat);
    LogTrace(TRACE5) << __FUNCTION__ << ": stat: " << *stat;
    fail_unless(stat->totalConnErr < stat->totalConnections);
    fail_unless(stat->totalConnErrTimeout > 0);
    fail_unless(stat->activeConnections < stat->totalConnections);
    fail_unless(stat->activeSessions < stat->totalSessions);
    fail_unless(stat->totalConnections > connCount);
    fail_unless(stat->nRetries > 0);
    fail_unless(stat->nRetriesOK > 0);
    fail_unless(stat->nRetriesOK < stat->nRetries);
}
END_TEST;

namespace {
void start()
{
    static auto qr = ServerFramework::TextQueryRunner();
    ServerFramework::setQueryRunner(qr);
    httpsrv::xp_testing::set_need_real_http();
    testInitDB();
}

void test_shutdown()
{
    testShutDBConnection();
    const httpsrv::Pult pult("PULT_A");
    make_curs("delete from EDI_HELP where PULT = :pult").bind(":pult", pult.str()).exec();
    commitInTestMode();
}

}//anonymous ns

#define SUITENAME "httpsrv"
TCASEREGISTER(start, test_shutdown)
    ADD_TEST(Check_Perespros);
    ADD_TEST(Check_HTTP);
    ADD_TEST(Check_HTTPS_NoAuth);
    ADD_TEST(Check_HTTPS_Auth);
    ADD_TEST(Check_Timeouts);
TCASEFINISH
#undef SUITENAME

#define SUITENAME "httpsrv_ext"
TCASEREGISTER(start, test_shutdown)
    ADD_TEST(Check_AezhSchedule);
    ADD_TEST(Check_Timeout);
    ADD_TEST(Check_TimeoutWithRetries);
TCASEFINISH
#undef SUITENAME 
} //anonymous ns
#endif /* #ifdef XP_TESTING */
#endif /*ENABLE_PG_TESTS*/
