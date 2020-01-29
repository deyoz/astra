package require mime 
package require smtp 

package provide sirena_mail 1.1002
namespace eval sirena {
    namespace eval  mail {

        proc encode_utf8 { word } {
            return [ mime::word_encode utf-8 base64 $word -charset_encoded 0 ]
        }

# ::mime::word_encode --
#
#    Word encodes strings as per RFC 2047.
#
# Arguments:
#       charset   The character set to encode the message to.
#       method    The encoding method (base64 or quoted-printable).
#       string    The string to encode.
#       ?-charset_encoded   0 or 1      Whether the data is already encoded
#                                       in the specified charset (default 1)
#       ?-maxlength         maxlength   The maximum length of each encoded
#                                       word to return (default 66)
#
# Results:
#	Returns a word encoded string.
#http://www.koders.com/tcl/fid25DC130FF7040E601A6193E4ECF2DCF3B2CBCF52.aspx?s=%22David+N.+Welton%22#L10

        namespace export create_part create_msg
        proc create_part { cont type chset binar atta name } {
            set lcmd [list mime::initialize -canonical $type ]
            if { $binar == 0 } {
                lappend lcmd -param [ list charset $chset ]
            }
            set enc_filename [ encode_utf8 $name ]
            if { $name != "" } {
                lappend lcmd -param [ list name $enc_filename ]
            }
            lappend lcmd -encoding base64
            if { $atta == 0 } {
                lappend lcmd -header  [ list "Content-Disposition" "inline" ]
            } elseif { $name == "" } {
                lappend lcmd -header  [ list "Content-Disposition" "attachment" ]
            } else {
                lappend lcmd -header  [ list "Content-Disposition" [ format "Attachment; filename=\"%s\"" $enc_filename ] ]
            }
            lappend lcmd -string $cont
            set res [ eval $lcmd ]
            return $res
        }

        proc create_address_string { a } {
            set name [ lindex $a 0 ]
            set addr [ lindex $a 1 ]
            if { $name != "" } {
                set addr [ join [list  "<" $addr ">" ] "" ]
                set name [ join [list "\"" [ encode_utf8 $name ] "\""  ] ]
            }
            return [ join [ list   $name   $addr ] ""  ]
        }

        proc create_msg { subj from to parts  } {
            set m [ mime::initialize -canonical "multipart/mixed" -parts $parts ]
            mime::setheader $m Subject [ encode_utf8 $subj ]
            if { $from == "" } {
                set from $::SMTP(from)
            }
            mime::setheader $m From [create_address_string $from ]
            foreach a $to {
                mime::setheader $m To [create_address_string $a ] -mode append
            }
            set txt [ mime::buildmessage $m ]
            return $txt
        }
        proc send { message_text server username password  } {
            set mb [ mime::initialize -string $message_text ]
            if { [ catch  {
            set headerlist ""
            foreach headtype [ list To Subject Cc Bcc From   ]  {
                if { ![ catch { mime::getheader $mb $headtype  }  currlist ] } {
                    
                    foreach value $currlist  {
                        lappend headerlist "-header" 
                        lappend headerlist [list  $headtype $value  ]  
                    }   
                    mime::setheader $mb  $headtype "null" -mode delete
                }
            }
            if { $server == "" } {
                set server $::SMTP(server);
                set username $::SMTP(username);
                set password $::SMTP(password);
           }

            lappend trailer
            if {  $username != "" } {
                lappend trailer  -username $username 
                if {  $password != "" } {
                    lappend trailer  -password $password 
                }
            }
            eval smtp::sendmessage $mb  $headerlist \
                    [ concat $trailer ] -servers $server  
                    
            } mail_res ] } {
                set savedInfo $::errorInfo
            }
            mime::finalize $mb      
            if { [ info exists savedInfo ]  } {
                error $mail_res $savedInfo
            }
                
        }
        proc nuke { message_text  } {
            set mb [ mime::initialize -string $message_text ]
# mime::finalize $mb      
        }
    }
}


