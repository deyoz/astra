package provide tclmon 1.0
namespace eval tclmon {
namespace export *    


variable FIND_PROC_NAME "obrzap" 

variable GRPPATTERN_NAME "grp"
variable BALANCER_PATTERN_NAME "balancer"
variable GRPPATTERN_BEG  0

variable counter_no 0
variable counter_pid 0
variable array_no
variable array_pid
variable array_grp

proc create_arr_no { no action cmds } {
    variable counter_no
    variable counter_pid
    variable array_no
    variable array_pid
    variable array_grp
    variable GRPPATTERN_NAME
    variable BALANCER_PATTERN_NAME
    variable GRPPATTERN_BEG

#    variable entry_$counter_no
    upvar #0 entry_no_$counter_no arr_no
    set arr_no(no)  $no
    set arr_no(action) $action
    set arr_no(cmds) $cmds
    set arr_no(proc_name) ""
    set arr_no(proc_grp_name) ""
    set arr_no(proc_grp_num) "0"
    set i 0
    foreach ttt $cmds {
        set i [expr { $i + 1 } ]
        if { 1 == $i } then {
            set arr_no(proc_name) $ttt
        } elseif { 2 == $i } then {
            set arr_no(proc_grp_name) $ttt
            lappend array_grp($arr_no(proc_grp_name)) $no

            set grppattern_len [ expr [string length $GRPPATTERN_NAME ] -1 ]
            set grp_3char [ string range $arr_no(proc_grp_name) $GRPPATTERN_BEG $grppattern_len ]

            if { 0 == [ string compare $grp_3char "grp" ] } then {
                set arr_no(proc_grp_num) [ get_que_num_by_grp_name $arr_no(proc_grp_name) ]

            } elseif { 0 == [ string compare $arr_no(proc_name) $BALANCER_PATTERN_NAME ] } then {
                set arr_no(proc_grp_num) [ dict create ]
            }
        }
    }
    set arr_no(flag) 0
    set arr_no(access) [ clock seconds ]
    set arr_no(start) [ clock seconds ]
    set arr_no(pid) -1
    set arr_no(name) ""
    set arr_no(n_start) 0
    set arr_no(to_restart) 0
    set arr_no(n_zapr) 0
    set arr_no(last_answer) [ clock seconds ]
    set arr_no(nmes_input_que) 0
    set arr_no(max_input_que) 0
    set arr_no(write_que_to_log_flag) 0
    set arr_no(time_input_que) [ clock seconds ]
    set array_no($no) entry_no_$counter_no
    set arr_no(cur_req) ""
    incr counter_no
}


proc delete_arr_no { no } {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
variable array_grp

    if { [ info exists array_no($no) ] != 1} then {
      error "array_no($no) not exists"
    }
    upvar #0 $array_no($no) arr_no
    set i [ lsearch $array_grp($arr_no(proc_grp_name)) $arr_no(no) ]
    set array_grp($arr_no(proc_grp_name)) [ lreplace $array_grp($arr_no(proc_grp_name)) $i $i ]
    delete_arr_pid $arr_no(pid)
    unset array_no($no)
}

proc find_proc_for_stop { count cmds } {
variable array_grp
variable array_no

    set result ""
    foreach num $array_grp([ lindex $cmds 1 ]) {
        upvar #0 $array_no($num) arr_no
        if { ![ string compare -length [ string length $cmds] $arr_no(cmds) $cmds] } {
            lappend result $num
            incr count -1
            if { !$count } {
                break;
            }
        }
    }
    if { !$count } {
        return $result
    } else {
        error "Can't find $count processes"
    }
}

proc set_start_arr_no { no name pid action} {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
    if { [ info exists array_no($no) ] != 1} then {
      error "array_no($no) not exists"
    }
    upvar #0 $array_no($no) arr_no
    set arr_no(name) $name
    set arr_no(action) $action
    set arr_no(pid) $pid
    set arr_no(access) [ clock seconds ]
    set arr_no(start) [ clock seconds ]
    set arr_no(to_restart) 0
    incr arr_no(n_start)
    set arr_no(cur_req) ""
    create_pid_no $arr_no(no) $pid
}

proc set_stop_arr_no { no} {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
    if { [ info exists array_no($no) ] != 1} then {
      error "array_no($no) not exists"
    }
    upvar #0 $array_no($no) arr_no
    delete_arr_pid $arr_no(pid) 
    set arr_no(pid) -1
    set arr_no(access) [ clock seconds ]
    set arr_no(nmes_input_que) 0
    set arr_no(max_input_que) 0
    set arr_no(cur_req) ""
    set arr_no(write_que_to_log_flag) 0
}

proc set_action_arr_no { no action} {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
    if { [ info exists array_no($no) ] != 1} then {
      error "array_no($no) not exists"
    }
    upvar #0 $array_no($no) arr_no
    set arr_no(action) $action
}

proc reset_n_start_arr_no { no } {
variable array_no
    if { [ info exists array_no($no) ] != 1 } then {
        error "array_no($no) not exists"
    }
    upvar #0 $array_no($no) arr_no
    set arr_no(n_start) 0
}

proc get_cmds_arr_no { no } {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid

    if { [ info exists array_no($no) ] != 1} then {
      error "array_no($no) not exists"
    }
    upvar #0 $array_no($no) arr_no
return  $arr_no(cmds) 
}

proc get_handlers_group_arr_no { no } {
    variable array_no

    if { [info exists array_no($no) ] != 1} then {
        error "array_no($no) not exists"
    }

    upvar #0 $array_no($no) arr_no
    return [ lindex $arr_no(cmds) 1 ]
}

proc get_name_arr_no { no } {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
  if { [ info exists array_no($no) ] != 1} then {
    error "array_no($no) not exists"
  }
  upvar #0 $array_no($no) arr_no
return  $arr_no(name) 
}

proc set_flag_arr_no { pid to_rst n_zapr type_zapr subgroup } {
    variable counter_no
    variable counter_pid
    variable array_no
    variable array_pid
    variable BALANCER_PATTERN_NAME

    if { [ info exists array_pid($pid) ] != 1} then {
        error "array_pid($pid) not exists"
    }
    upvar #0 $array_pid($pid) arr_pid

    set no $arr_pid(no)
    if { [ info exists array_no($no) ] != 1} then {
        error "array_no($no) for pid $pid not exists"
    }

    upvar #0 $array_no($no) arr_no

    incr arr_no(flag);
    if { $to_rst >= 0 } then {
       set arr_no(to_restart) $to_rst
    }

    if { 0 == $to_rst && 1 != $type_zapr } then {
       set arr_no(cur_req) ""
    }

    set arr_no(access) [ clock seconds ]
    if { $n_zapr > 0 } then {
       set arr_no(last_answer) [ clock seconds ]
       set arr_no(n_zapr)  [expr { $arr_no(n_zapr) + $n_zapr } ]

       if { 0 == [ string compare $arr_no(proc_name) $BALANCER_PATTERN_NAME ] } then {
           if { 1 != [ dict exists $arr_no(proc_grp_num) $subgroup ] } then {
               dict append arr_no(proc_grp_num) $subgroup [ get_que_num_by_grp_name $arr_no(proc_grp_name)($subgroup) ]
           }

           add_zapr_to_potok $n_zapr $type_zapr [ dict get $arr_no(proc_grp_num) $subgroup ]
       } else {
           add_zapr_to_potok $n_zapr $type_zapr $arr_no(proc_grp_num)
       }
    }
}

proc arr_set_queue_size_proc { pid nmes_input_que max_input_que write_to_log_flag} {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid

  if { [ info exists array_pid($pid) ] != 1} then {
    error "array_pid($pid) not exists"
  }
  upvar #0 $array_pid($pid) arr_pid

  set no $arr_pid(no)
  if { [ info exists array_no($no) ] != 1} then {
    error "array_no($no) for pid $pid not exists"
  }
  upvar #0 $array_no($no) arr_no
  set arr_no(nmes_input_que) $nmes_input_que
  set arr_no(max_input_que) $max_input_que
  set arr_no(write_que_to_log_flag) $write_to_log_flag
  set arr_no(time_input_que) [ clock seconds ]
}

proc arr_get_queue_size_proc_for_log {} {

 return  [arr_get_queue_size_proc 1 ]
}

proc arr_get_queue_size_proc { log_flag } {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid

  set str_queue {}
  foreach {no name} [array get array_no] {
     upvar #0  $name  arr_no
     if { $arr_no(nmes_input_que) > 0  } then {
        set diff_times  [expr { [ clock seconds] - $arr_no(time_input_que) } ]
        if { $diff_times > 2 } then {
           set arr_no(nmes_input_que) 0
           set arr_no(write_que_to_log_flag) 0
        } else {
           if {  $arr_no(write_que_to_log_flag)==0 && $log_flag==1  } then {
              continue;
           } else {
              if { $log_flag==1 } then {
                append str_queue "\n"
              }
              append str_queue "$arr_no(proc_grp_name)($arr_no(nmes_input_que) of $arr_no(max_input_que)) "
              if { $log_flag==1 } then {
                set str_potok [ get_potok_by_que_name $arr_no(proc_grp_name)]
                if { [ string length $str_potok ] != 0 } then {
                  append  str_queue " $str_potok"
                }
              }
           }
        }
     }
  }
 return $str_queue
}

proc arr_set_cur_req_proc { pid a_cur_req} {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
  if { [ info exists array_pid($pid) ] != 1} then {
    error "array_pid($pid) not exists"
  }
  upvar #0 $array_pid($pid) arr_pid

  set no $arr_pid(no)
  if { [ info exists array_no($no) ] != 1} then {
    error "array_no($no) for pid $pid not exists"
  }
  upvar #0 $array_no($no) arr_no
  set arr_no(cur_req) $a_cur_req
}

proc arr_clear_cur_req_proc { pid} {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
  if { [ info exists array_pid($pid) ] != 1} then {
    error "array_pid($pid) not exists"
  }
  upvar #0 $array_pid($pid) arr_pid

  set no $arr_pid(no)
  if { [ info exists array_no($no) ] != 1} then {
    error "array_no($no) for pid $pid not exists"
  }
  upvar #0 $array_no($no) arr_no
  set arr_no(cur_req) ""
}

proc arr_get_cur_req_proc { pid} {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
  if { [ info exists array_pid($pid) ] != 1} then {
    error "array_pid($pid) not exists"
  }
  upvar #0 $array_pid($pid) arr_pid

  set no $arr_pid(no)
  if { [ info exists array_no($no) ] != 1} then {
    error "array_no($no) for pid $pid not exists"
  }
  upvar #0 $array_no($no) arr_no
  return $arr_no(cur_req)
}

proc get_n_proc_by_grp { grp } {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
variable FIND_PROC_NAME

  set all_proc 0
  set empty_proc 0
  set str_queue {}
  foreach {no name} [array get array_no] {
    upvar #0  $name  arr_no
    if { $arr_no(proc_grp_name) == $grp  && 
         $arr_no(proc_name) == $FIND_PROC_NAME } then {
      incr all_proc
      if { $arr_no(to_restart) == 0 } then {
       incr empty_proc
      }
    }
  }
  append str_queue "a=$all_proc/e=$empty_proc"
 return $str_queue
}

proc arr_test_restart_proc {} {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
  set lst {}
  foreach {no name} [array get array_no] {
  upvar #0  $name  arr_no
  if { $arr_no(to_restart) > 0  } then {
    if { ($arr_no(action) == "W") || ("F" == $arr_no(action)) || ("FR" == $arr_no(action)) } then {
        set t_o  [expr { [ clock seconds] - $arr_no(access) } ]
        if { $t_o > $arr_no(to_restart) } then {
          if { $arr_no(cur_req) != "" } then {
            append lst "$arr_no(name)(pid=$arr_no(pid),Req=$arr_no(cur_req)) "
          } else {
            append lst "$arr_no(name)(pid=$arr_no(pid)) "
          }
          if { ("W" == $arr_no(action)) || ("FR" == $arr_no(action))} then {
              restart_obr $arr_no(name) $arr_no(pid)
          } else {
              process_final $no
          }
        }
     }
  }
}
    return $lst
}

proc arr_test_not_asked_proc {} {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
  set lst {}
  foreach {no name} [array get array_no] {
  upvar #0  $name  arr_no
  if { $arr_no(to_restart) == 0  } then {
    if { $arr_no(action) == "W" } then {
        set t_o  [expr { [ clock seconds] - $arr_no(access) } ]
        if { $t_o > 60 } then {
           append lst "$arr_no(pid) "
        }
     }
  }
}
 return $lst
}

proc PS_get_head { type } {
  set Req ""
  if { $type == 1 } then {
   set Req "Req                                     "
  }
  set str_head  "No      Pid                 Name A  N_zapr  LAns  LAcc NSt  TO StartTime   $Req Command"
return $str_head
}

proc PS_get_item { no type } {
  
  upvar #0  $no  arr_no
  set l_ans 0
  if { $arr_no(start) != $arr_no(last_answer)  } then {
    set l_ans [ expr [ expr   [clock seconds]-$arr_no(last_answer)] % 10000 ]
  }
  set Req ""
  if { $type == 1 } then {
   set Req [ format " %-40.40s" $arr_no(cur_req) ]
  }
  set str_item  [format "%-3d %7d %20.20s %1.1s %7d %5d %5d %3d %3d %11.11s$Req %s" \
      $arr_no(no) \
      $arr_no(pid) \
      $arr_no(name) \
      $arr_no(action) \
      [ expr $arr_no(n_zapr) % 10000000 ]\
      $l_ans\
      [ expr [ expr   [clock seconds]-$arr_no(access)] % 10000 ]\
      [ expr $arr_no(n_start) % 1000 ]\
      [ expr $arr_no(to_restart) % 1000 ] \
      [clock format $arr_no(start) -format "%d.%m %H:%M" ] \
      $arr_no(cmds) ]
return $str_item
}

proc PS_show {} {
 return [ PS_show_type 0 ]
}

proc PS_show_type { type } {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
  set lst {}
  lappend lst [ PS_get_head $type ]
  foreach {no name} [array get array_no] {
    lappend lst [ PS_get_item $name $type ]
  }
return $lst
}

proc PS {} {
 return [ PS_type 0 ]
}

proc PS_type { type } {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
variable CR
  if { ! [ info exists CR ] } {
	set CR ""
  }
  set a_str ""
  append a_str [ PS_get_head $type ]
  append a_str "$CR\n"
  foreach {no name} [array get array_no] {
    append a_str [ PS_get_item $name $type ]
    append a_str "$CR\n"
  }
return $a_str
}

proc PS_show_all {} {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
 set lst {}
  foreach {no name} [array get array_no] {
  upvar #0  $name  arr_no
  lappend lst [array get arr_no] 
 }
return $lst
}

proc PS_system {} {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
 set a_str ""
 set a_full 0
 set a_work 0
 set a_restart 0;
  append a_str "Current time: "  [ clock format [clock seconds] -format "%H:%M %d.%m.%Y" ] "\n"
  foreach {no name} [array get array_no] {
    upvar #0  $name  arr_no
    incr a_full
    if { $arr_no(action) == "W" } then {
      incr a_work
    }
    if { $arr_no(action) == "R" } then {
      incr a_restart
    }
    
  }
  append a_str "Process: All-$a_full  Work-$a_work  Restart-$a_restart\n"
  append a_str  [ get_potok ]

  set queue_size [ arr_get_queue_size_proc 0 ]
  if { [ string length $queue_size ] != 0 } then {
     append a_str "\nInput queue: $queue_size"
  }

return $a_str
}

proc create_pid_no { no pid } {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
    upvar #0 entry_pid_$counter_pid pid_no
    set pid_no(no)  $no
    set pid_no(pid) $pid
    set array_pid($pid) entry_pid_$counter_pid
    incr counter_pid
} 

proc PS_pid {} {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
  set a_str "PID    NO\n"
  foreach {pid name} [array get array_pid] {
    upvar #0  $name  arr_pid
    append a_str  [format "%6d %3d\n" $arr_pid(pid)  $arr_pid(no) ]
  }
return $a_str
}

proc delete_arr_pid { pid } {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
  if { $pid == -1 } then {
     return;
  }
  if { [ info exists array_pid($pid) ] != 1} then {
    error "array_pid($pid) not exists"
  }
  unset array_pid($pid)
}

proc show_bufs {} {
return [ show_bufs_proc ]
}

proc kse_test1 {} {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
set test_grp "grp3"
set test_name "obrzap"
set que_name "levb"

  set i 0
  foreach {no name} [array get array_no] {
    upvar #0  $name  arr_no
    if { $arr_no(proc_grp_name) == $test_grp  && 
         $arr_no(proc_name) == $test_name } then {
       if { $i == 0 } then {
         set_flag_arr_no  $arr_no(pid) 40 0 1  
       }
       incr i
    }
  }
  foreach {no name} [array get array_no] {
    upvar #0  $name  arr_no
    if { $arr_no(proc_grp_name) == $test_grp  && 
         $arr_no(proc_name) == $que_name } then {
      arr_set_queue_size_proc $arr_no(pid) 12
    }
  }
}

proc kse_test { test_grp test_name que_name que_size max_i} {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
  set i 0
  foreach {no name} [array get array_no] {
    upvar #0  $name  arr_no
    if { $arr_no(proc_grp_name) == $test_grp  && 
         $arr_no(proc_name) == $test_name } then {
       if { $i < $max_i } then {
         set_flag_arr_no  $arr_no(pid) 40 0 1  
       }
       incr i
    }
  }
  foreach {no name} [array get array_no] {
    upvar #0  $name  arr_no
    if { $arr_no(proc_grp_name) == $test_grp  && 
         $arr_no(proc_name) == $que_name } then {
      arr_set_queue_size_proc $arr_no(pid) $que_size
    }
  }
}


proc test2 {} {
variable counter_no 
variable counter_pid 
variable array_no
variable array_pid
 create_arr_no 1  S {a1 a2}
 create_arr_no 2  S {b1 b2}
 set lst [ PS ]
 puts $lst
 set_start_arr_no 1 name1 100 W
 set_start_arr_no 2 name2 200 W
 set lst [ PS ]
 puts $lst
}

proc proc_grp_cmd {grp cmd} {
    variable array_no;
    foreach {no prcs} [array get array_no] {
        upvar #0 $prcs arr_no
       if { 0 == [ string compare $grp $arr_no(proc_grp_name) ] } then {
            proccmd $arr_no(proc_name) $arr_no(pid) $cmd
       }
    }
}

}
#test2
