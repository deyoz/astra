#pragma once

#include <serverlib/oci8.h>
#include "jxtlib/JxtInterface.h"


class LibraInterface : public JxtInterface
{
public:
    LibraInterface() : JxtInterface("","libra")
    {
      AddEvent("request", JXT_HANDLER(LibraInterface, RequestHandler));
    }

    void RequestHandler(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);

};


namespace LIBRA
{

// ����� ����� ��� � �������騩 ������ ���᪠ ���������� ��� �����祭�� �� ३� ���ந�� ���� � �� �����?
// �᫨ ��諨 � ����, � ���� � ���� �������. �᫨ ��諨 - �����稫� �� �������騥 ᢮��⢠, �� ��諨 - ����� ���������� ⮫쪮 �� �᭮�� �����.
// ����� ������ ��������� ᢮��⢠. ���� ������� � ��� ⮫쪮 �� ����� ��� �� �� �������樨 � ⨯� ��?

class TCompon
{
private:
  void ComponFromBort( const std::string& bort );
  void ReadCompon( int comp_id );
  void ReadSections( int comp_id );
  void ConfigList( int comp_id );
public:
};


}
