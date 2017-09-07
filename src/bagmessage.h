#ifndef _BAGMESSAGE_H_
#define _BAGMESSAGE_H_

#include <string>
#include <queue>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

/**************************************************************************/
/* ��������� ᮮ�饭��, ��ࠢ�塞��� � BagMessage ��� ����砥���� ���㤠 */
/**************************************************************************/
class BM_HEADER
{
  public:
  // �������� ��������
    std::string appl_id;           // �����䨪��� ������ - 8 ᨬ�����
    unsigned version;              // ����� ��������� - 2-���⮢�� �����������
    unsigned type;                 // ��� ᮮ�饭�� - 2-���⮢�� �����������
    unsigned message_id_number;    // �����䨪�樮��� ����� ᮮ�饭�� - 2-���⮢�� �����������
    unsigned data_length;          // ����� ⥪�⮢�� ������ ��᫥ ��������� - 2-���⮢�� �����������
                                   // � �� 4 ���� ��१�ࢨ஢���
  // ����⠭�� � ��⮤�
    static const int SIZE = 20;    // ����� ��।�������� ��������� � �����
    void loadFrom( char *buf );    // ����㧨�� ��������� �� ���� ��᫥ �ਥ��
    void saveTo( char *buf );      // ������� ��������� � ���� ��। ��।�祩
};

// ����饭�� ����� � ����������.
typedef struct
{
  BM_HEADER head;
  std::string text;
} BM_MESSAGE;

/*****************************************/
/* ���������� � ��⥬�� SITA BagMessage */
/*****************************************/
class BMConnection
{
  public:
  // ����ﭨ� �ᨭ�஭��� ����樨
    typedef enum
    {
      BM_OP_NONE = 0,      // �� �뫮 ����⪨ ᤥ���� ������
      BM_OP_WAIT = 1,      // ������ ���� - ���� �����襭��
      BM_OP_READY = 2      // �����祭� ��ଠ�쭮
    } BM_OPERATION_STATUS;
  // ���� ᮮ�饭��, ����� ����� ���� ��᫠�� � ��⥬� SITA BagMessage ��� �ਭ��� �� ���.
    typedef enum
    {
      LOGIN_RQST = 1,      // ����� �室� � 㤠������ ��⥬�
      LOGIN_ACCEPT = 2,    // ������⥫�� �⢥� �� ����� �室�
      LOGIN_REJECT = 3,    // ����⥫�� �⢥� �� ����� �室�
      DATA = 4,            // ��।�� ������
      ACK_DATA = 5,        // ��।�� ������ � ����ᮬ ���⢥ত���� �ਥ��
      ACK_MSG = 6,         // ������⥫�� �⢥� �� ����� ���⢥ত���� �ਥ�� ������
      NAK_MSG = 7,         // ����⥫�� �⢥� �� ����� ���⢥ত���� �ਥ�� ������
      STATUS = 8,          // ����� - ���⢥ত���� ⮣�, �� ��� �� ����襭�
      DATA_ON = 9,         // �⬥�� ࠭�� �뤠����� DATA_OFF
      DATA_OFF = 10,       // ���졠 �ਮ�⠭����� ���뫪� ������
      LOG_OFF = 11         // ����� ������� �裡
    } BM_MESSAGE_TYPE;
  private:
    static const std::string messageTypes[12];
  public:
  // ���ᨨ ���������� ᮮ�饭��
    typedef enum
    {
      VERSION_2 = 2        // ���� �� ⮫쪮 ⠪��
    } BM_HEADER_VERSIONS;
  private:
    typedef struct
    {
      std::string host;    // IP-���� ���
      int port;            // ����
    } BM_HOST;
    static const int NUMLINES = 2;
  // ����ன��, ���⠭�� ����� �� ����᪥
    int line_number;            // ���浪��� ����� ᮥ������� � ��⥬�
    BM_HOST ip_addr[NUMLINES];  // ���� ��� ����� �裡 - �᭮���� � १�ࢭ� (१�ࢭ��?)
    std::string login;          // ����� � ��⥬� SITA BagMessage
    std::string password;       // ��஫� ⠬ ��
    int heartBeat;              // ���ᨬ���� ���ࢠ� �६��� ����� ᮮ�饭�ﬨ
  // ����稥 ��६����
  // ������
    bool configured;                  // �ਧ��� ⮣�, �� ���䨣���� ���⠭�
    bool paused;                      // �ਧ��� ⮣�, �� ��㣠� ��஭� ����ᨫ� ���� (�뤠�� DATA_OFF)
    BM_OPERATION_STATUS connected;    // �ਧ��� ⮣�, �� ��� ��⠭������
    BM_OPERATION_STATUS loginStatus;  // �ਧ��� ⮣�, �� �� ��諨 � �� ��⥬�
    BM_OPERATION_STATUS writeStatus;  // ����ﭨ� ��᫥���� ����樨 ��।�� ������
  // ���� � ��⮪���� ����
    boost::asio::ip::tcp::socket socket;  // ����� ��� �裡
    int activeIp;                         // ����� �� ����� �裡 ᥩ�� �ᯮ������?
    unsigned mes_num;                     // ����� ᮮ�饭�� ��� ��⮪��� �裡
    time_t lastAdmin;                     // �६� ��᫥����� ����⢨� �� �ࠢ����� ����ᮬ - ��� �࣠����樨 ����থ�
    boost::posix_time::ptime adminStartTime; // ��筮� �६� ��᫥���� ����樨 �� �ࠢ����� - ��� �業�� �६��� �� �믮������
  // �ਥ�
    time_t lastRecvTime; // �६� ��室� �।��饣� ᮮ�饭�� - ��᫥�������� ��� �業�� ࠡ��ᯮᮡ���� �����
    char *rbuf;          // ���� ��� �㭪樨 async_read
    BM_HEADER rheader;   // ��������� �ਭ�⮣� ᮮ�饭��
    int needSendAck;     // �㦭� ��।��� ���⢥ত���� �ਭ�⮣� ᮮ�饭�� � �⨬ ����஬
    std::queue<BM_MESSAGE> inputQueue; // ��।� �ਭ���� ᮮ�饭��, �� �� ��।������ �� ��ࠡ���
  // ��।��
    time_t lastSendTime; // �६� ��।�� �।��饣� ᮮ�饭�� - ��� ��।�� ����� �� �����
    BM_HEADER header;    // ��������� ᮮ�饭��, �ନ�㥬��� ��� ��।��
    std::string text;         // ����� ᮮ�饭��, �ନ�㥬��� ��� ��।��
    char *wbuf;          // ���� ��� �㭪樨 async_write
    int waitForAck;      // ��⮪���� ����� ᮮ�饭��, �� ���뫪� ���ண� ���⠢��� ����� ���⢥ত����,
                         // �⢥� �� ����� �� �� ����祭, ��� -1 �᫨ ��祣� �� ����
    int waitForAckId;    // ID ᮮ⢥�����饩 ⥫��ࠬ�� � ���� ������
    time_t waitForAckTime; // ����� ��᫠�� �� ᮮ�饭��
    void (*writeHandler)( int tlg_id, int status ); // ��ࠡ��稪 �����襭�� ��।�� � ���⢥ত�����
    boost::posix_time::ptime sendStartTime; // �६� ��砫� ����樨 ��।�� - ��� �筮�� ���᫥��� �६��� �� �믮������
  public:
    BMConnection( int i, boost::asio::io_service& io );
    ~BMConnection();
    void run();          // ������� ��-� � ࠡ�祬 横��
  private:
    bool isInit() { return configured; };      // �஢����, �� ���䨣���� ���⠭�
    void init();                               // ������ ���䨣���� � ��࠭��� � ��ꥪ�
    void connect();                            // ����������� � �ࢥ��
    void onConnect( const boost::system::error_code& err );                // ��ࠡ��稪 �����襭�� ����樨 ������祭��
    void onRead( const boost::system::error_code& error, std::size_t n );  // ��ࠡ��稪 �����襭�� ����樨 �ਥ�� ᮮ�饭��
    void onWrite( const boost::system::error_code& error, std::size_t n ); // ��ࠡ��稪 �����襭�� ����樨 ���뫪� ᮮ�饭��
    bool makeMessage( BM_MESSAGE_TYPE type, std::string text = "" , int msg_id = -1 );  // �����⮢��� ��������� � ⥪�� � ���뫪�
    void doSendMessage();                                                               // ��᫠�� �����⮢������ ᮮ�饭��
    bool sendMessage( BM_MESSAGE_TYPE type, std::string text = "", int msg_id = -1 );   // ��᫠�� � BagMessage ᮮ�饭�� - ��騥 ����
    void sendLogin();                     // ��᫠�� � �� ��⥬� ����� � ��஫�
    void checkInput();                    // �஢���� �室��� ��⮪ � ��-� � ��� ᤥ����
    void checkTimer();                    // �஢���� ⠩���� ᮮ�饭�� - ���� ����� ���㫠��
  public:
    int getNumber() { return line_number; }       // �뤠�� ����� ᮥ������� �����
    void sendTlg( std::string text );             // ��᫠�� BSM-⥫��ࠬ��
    void sendTlgAck( int id, std::string text, void (*handler)( int, int) );  // ��᫠�� BSM-⥫��ࠬ�� � ����ᮬ ���⢥ত���� �ਥ��
    bool readyToSend();                           // �஢����, �� ᮥ������� ����㯭� ��� ���뫪� ᮮ�饭��
    bool readyToReceive();                        // �஢����, �� ���� ��������� �ਭ��� ᮮ�饭��
    BM_MESSAGE getFirst();                        // ����� ��ࢮ� �ਭ�⮥ ᮮ�饭��
};

#endif // _BAGMESSAGE_H_