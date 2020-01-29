#ifndef SIRENA_ASIO_SSL_HPP_INCLUDED
#define SIRENA_ASIO_SSL_HPP_INCLUDED

#ifdef HAVE_SSL
#ifdef BOOST_ASIO_NO_OPENSSL_STATIC_INIT

// Just in case
#warning Turning off OPENSSL initialization inside ASIO

#ifdef _WIN32_WINNT
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#else
#include <mutex>
#endif

#include <boost/asio/ssl/detail/openssl_init.hpp>
namespace boost {
namespace asio {
namespace ssl {
namespace detail {

template <>
class openssl_init<true> : private openssl_init_base
{
#ifdef _WIN32_WINNT
typedef boost::mutex mutex;
typedef boost::lock_guard<mutex> lock_guard;
#else
typedef std::mutex mutex;
typedef std::lock_guard<mutex> lock_guard;
#endif

public:
  // Constructor.
  openssl_init()
    : ref_(instance())
  {
  }

  // Destructor.
  ~openssl_init()
  {
  }

  static boost::asio::detail::shared_ptr<do_init> instance()
  {
//
// singleton saving for more late destruction
// than Meyers' singleton inside  openssl_init_base::instance()
//
// double checked locking singleton here protects
// in case of openssl_init_base::instance() inlining also
//
      if (!singleton_) {
          lock_guard lck(mtx_);
          if (!singleton_) {
              singleton_ = openssl_init_base::instance();
          }
      }
      return singleton_;
  }



#if !defined(SSL_OP_NO_COMPRESSION) \
  && (OPENSSL_VERSION_NUMBER >= 0x00908000L)
  using openssl_init_base::get_null_compression_methods;
#endif // !defined(SSL_OP_NO_COMPRESSION)
       // && (OPENSSL_VERSION_NUMBER >= 0x00908000L)

private:
  //
  // singleton instance declaration, don't forget to define it somewhere in your CPP files
  // don't use whole this hack otherwise
  //
  static boost::asio::detail::shared_ptr<do_init> singleton_;
  static mutex mtx_;

  // Reference to singleton do_init object to ensure that openssl does not get
  // cleaned up until the last user has finished with it.
  boost::asio::detail::shared_ptr<do_init> ref_;
};

} // namespace detail
} // namespace ssl
} // namespace asio
} // namespace boost

#endif //BOOST_ASIO_NO_OPENSSL_AUTO_INIT


#include <boost/asio/ssl.hpp>
#endif // HAVE_SSL

#endif // SIRENA_ASIO_SSL_HPP_INCLUDED

