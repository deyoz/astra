#!/bin/bash -e

function uab_check_version() {
    grep -w 'PION_VERSION[ ]\+\"5.0.6\"' $1/include/pion/config.hpp
}

uab_config_addon() {
    echo rebuild for boost1.76
}

function uab_config_and_build() {
    prefix=${1:?prefix as the 1st parameter}
    shift 1

    cxxflags=""
    [ -n "$CPP_STD_VERSION" ] && cxxflags="-std=$CPP_STD_VERSION"

    ./autogen.sh
    ./configure --with-boost="$PWD/../../boost" --with-pic --disable-tests --prefix="$prefix" CXXFLAGS="${cxxflags}" $@

    patch -p0 << EOF
--- src/tcp_server.cpp	2021-07-22 18:16:25.273908152 +0300
+++ src/tcp_server.cpp	2021-07-22 18:30:02.307582837 +0300
@@ -25,7 +25,7 @@
     m_active_scheduler(sched),
     m_tcp_acceptor(m_active_scheduler.get_io_service()),
 #ifdef PION_HAVE_SSL
-    m_ssl_context(m_active_scheduler.get_io_service(), boost::asio::ssl::context::sslv23),
+    m_ssl_context(boost::asio::ssl::context::sslv23),
 #else
     m_ssl_context(0),
 #endif
@@ -37,7 +37,7 @@
     m_active_scheduler(sched),
     m_tcp_acceptor(m_active_scheduler.get_io_service()),
 #ifdef PION_HAVE_SSL
-    m_ssl_context(m_active_scheduler.get_io_service(), boost::asio::ssl::context::sslv23),
+    m_ssl_context(boost::asio::ssl::context::sslv23),
 #else
     m_ssl_context(0),
 #endif
@@ -49,7 +49,7 @@
     m_default_scheduler(), m_active_scheduler(m_default_scheduler),
     m_tcp_acceptor(m_active_scheduler.get_io_service()),
 #ifdef PION_HAVE_SSL
-    m_ssl_context(m_active_scheduler.get_io_service(), boost::asio::ssl::context::sslv23),
+    m_ssl_context(boost::asio::ssl::context::sslv23),
 #else
     m_ssl_context(0),
 #endif
@@ -61,7 +61,7 @@
     m_default_scheduler(), m_active_scheduler(m_default_scheduler),
     m_tcp_acceptor(m_active_scheduler.get_io_service()),
 #ifdef PION_HAVE_SSL
-    m_ssl_context(m_active_scheduler.get_io_service(), boost::asio::ssl::context::sslv23),
+    m_ssl_context(boost::asio::ssl::context::sslv23),
 #else
     m_ssl_context(0),
 #endif
@@ -179,8 +179,7 @@
         // create a new TCP connection object
         tcp::connection_ptr new_connection(connection::create(get_io_service(),
                                                               m_ssl_context, m_ssl_flag,
-                                                              boost::bind(&server::finish_connection,
-                                                                          this, _1)));
+            [this](tcp::connection_ptr conn){ this->finish_connection(conn); }));
         
         // prune connections that finished uncleanly
         prune_connections();
--- src/http_server.cpp    2021-07-22 18:10:29.231377403 +0300
+++ src/http_server.cpp    2021-07-22 18:11:16.832177229 +0300
@@ -29,8 +29,7 @@
 void server::handle_connection(tcp::connection_ptr& tcp_conn)
 {
     request_reader_ptr my_reader_ptr;
-    my_reader_ptr = request_reader::create(tcp_conn, boost::bind(&server::handle_request,
-                                           this, _1, _2, _3));
+    my_reader_ptr = request_reader::create(tcp_conn, [this](auto&&a1, auto&&a2, auto&&a3){ this->handle_request(a1,a2,a3); });
     my_reader_ptr->set_max_content_length(m_max_content_length);
     my_reader_ptr->receive();
 }
--- include/pion/http/message.hpp       2021-07-23 13:30:57.938391404 +0300
+++ include/pion/http/message.hpp       2021-07-23 13:31:35.218500993 +0300
@@ -58,6 +58,7 @@
     typedef std::vector<char>   chunk_cache_t;

     /// data type for library errors returned during receive() operations
+#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
     struct receive_error_t
         : public boost::system::error_category
     {
--- include/pion/http/parser.hpp        2021-07-23 10:56:47.379983762 +0300
+++ include/pion/http/parser.hpp        2021-07-23 10:57:07.078910723 +0300
@@ -71,7 +71,8 @@
     };

     /// class-specific error category
-    class error_category_t
+#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
+    class error_category_t final
         : public boost::system::error_category
     {
     public:
--- include/pion/tcp/connection.hpp	2014-03-25 01:26:00.000000000 +0400
+++ include/pion/tcp/connection.hpp	2021-07-22 18:06:32.528373081 +0300
@@ -109,7 +109,7 @@
     explicit connection(boost::asio::io_service& io_service, const bool ssl_flag = false)
         :
 #ifdef PION_HAVE_SSL
-        m_ssl_context(io_service, boost::asio::ssl::context::sslv23),
+        m_ssl_context(boost::asio::ssl::context::sslv23),
         m_ssl_socket(io_service, m_ssl_context),
         m_ssl_flag(ssl_flag),
 #else
@@ -131,7 +131,7 @@
     connection(boost::asio::io_service& io_service, ssl_context_type& ssl_context)
         :
 #ifdef PION_HAVE_SSL
-        m_ssl_context(io_service, boost::asio::ssl::context::sslv23),
+        m_ssl_context(boost::asio::ssl::context::sslv23),
         m_ssl_socket(io_service, ssl_context), m_ssl_flag(true),
 #else
         m_ssl_context(0),
@@ -291,7 +291,7 @@
     {
         // query a list of matching endpoints
         boost::system::error_code ec;
-        boost::asio::ip::tcp::resolver resolver(m_ssl_socket.lowest_layer().get_io_service());
+        boost::asio::ip::tcp::resolver resolver(m_ssl_socket.get_executor());
         boost::asio::ip::tcp::resolver::query query(remote_server,
             boost::lexical_cast<std::string>(remote_port),
             boost::asio::ip::tcp::resolver::query::numeric_service);
@@ -656,12 +656,12 @@
     inline unsigned short get_remote_port(void) const {
         return get_remote_endpoint().port();
     }
-    
+/*
     /// returns reference to the io_service used for async operations
     inline boost::asio::io_service& get_io_service(void) {
         return m_ssl_socket.lowest_layer().get_io_service();
     }
-
+*/  decltype(auto) get_io_service() { return m_ssl_socket.get_executor(); }
     /// returns non-const reference to underlying TCP socket object
     inline socket_type& get_socket(void) { return m_ssl_socket.next_layer(); }
     
@@ -692,7 +692,7 @@
                   connection_handler finished_handler)
         :
 #ifdef PION_HAVE_SSL
-        m_ssl_context(io_service, boost::asio::ssl::context::sslv23),
+        m_ssl_context(boost::asio::ssl::context::sslv23),
         m_ssl_socket(io_service, ssl_context), m_ssl_flag(ssl_flag),
 #else
         m_ssl_context(0),
EOF
    make -j${MAKE_J:-3}
    make install
}

function uab_pkg_tarball() {
    echo pion-5.0.6.tar.bz2
}
