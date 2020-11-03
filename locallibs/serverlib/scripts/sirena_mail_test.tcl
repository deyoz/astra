package require mime
package require smtp
lappend auto_path "."
package require sirena_mail

set p1 [ sirena::mail::create_part "Hello there" "text/plain" "utf8" 0 0 "" ]

# create_msg subj from to parts
set txt [ sirena::mail::create_msg \
        "���� - ���/test" \
        [ list "�� - ��ਯ� Tcl" alexander.shkoruta@sirena2000.ru ] \
        [ list [list "��� - ����᪠⥫� Tcl" alexander.shkoruta@sirena2000.ru ]] \
        [list $p1 ]]

puts $txt

sirena::mail::send $txt smtp.sirena2000.ru "" ""

