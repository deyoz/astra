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

bool LIBRA_ENABLED();

}
