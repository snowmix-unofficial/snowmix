#
# Copyright Peter Maersk-Moller (c) 2012-2015. All rights reserved.
#

proc AddFeedCommands { host port id button } {
  global relay
  set hostmap [string map {. _} $host]
  menu .feeds-$id-$hostmap -tearoff 1
  .feeds-$id-$hostmap add command -label "Drop Feed Frame" -command "FeedCommand $host $port dropfr $id"
  .feeds-$id-$hostmap add command -label "Kill Feed" -command "FeedCommand $host $port kvf $id"
  .feeds-$id-$hostmap add command -label "Restart & Kill Feed" -command "FeedCommand $host $port rkvf $id"
  .feeds-$id-$hostmap add separator
  .feeds-$id-$hostmap add command -label "Restart Cam" -command "FeedCommand $host $port restartcam $id"
  .feeds-$id-$hostmap add command -label "Stop Cam" -command "FeedCommand $host $port stopcam $id"
  .feeds-$id-$hostmap add command -label "Start Cam" -command "FeedCommand $host $port startcam $id"
  .feeds-$id-$hostmap add command -label "Reboot Cam" -command "FeedCommand $host $port rebootcam $id"
  .feeds-$id-$hostmap add separator
  .feeds-$id-$hostmap add command -label "Focus 0" -command "FeedCommand $host $port focus_0 $id"
  .feeds-$id-$hostmap add command -label "Focus Out" -command "FeedCommand $host $port focus_up $id"
  .feeds-$id-$hostmap add command -label "Focus In" -command "FeedCommand $host $port focus_down $id"
  .feeds-$id-$hostmap add command -label "Focus Auto" -command "FeedCommand $host $port focus_auto $id"
  .feeds-$id-$hostmap add separator
  .feeds-$id-$hostmap add command -label "Zoom 100" -command "FeedCommand $host $port zoom_100 $id"
  .feeds-$id-$hostmap add command -label "Zoom 150" -command "FeedCommand $host $port zoom_150 $id"
  .feeds-$id-$hostmap add command -label "Zoom 200" -command "FeedCommand $host $port zoom_200 $id"
  .feeds-$id-$hostmap add command -label "Zoom Out" -command "FeedCommand $host $port zoom_up $id"
  .feeds-$id-$hostmap add command -label "Zoom In" -command "FeedCommand $host $port zoom_down $id"
  bind $button "<ButtonPress>" "FeedPress %X %Y %b 1 .feeds-$id-$hostmap $id $host $port"
  bind $button "<ButtonRelease>" "FeedPress %X %Y %b 0 .feeds-$id-$hostmap $id $host $port"
}

proc FeedCommand { host portno command id } {
  if {[string match "dropfr" $command]} {
    puts "list AddServerCommand snowmix $host $portno \"feed drop $id 1\" \"\" 1000"
    after 100 [list AddServerCommand snowmix $host $portno "tcl eval snowmix message \[snowmix parse feed drop $id 1]" GetOnePuts 1000]
  } elseif {[string match "rkvf" $command]} {
    if {$id > 0 && $id < 20} {
      if {[catch {exec ssh -l root cam$id "killall -9 gst-launch-1.0"} res]} {
        if {[lindex $::errorCode 0] eq "CHILDSTATUS"} {
          set status [lindex $::errorCode 2]
          puts "Failed killing videofeed $id for host $host : $status"
        } else {
          # Some kind of unexpected failure
          puts "Failed killing videofeed $id for host $host"
        }
      } else {
        puts "cam$id stopped"
      }
      FeedCommand $host $portno kvf $id
    }
  } elseif {[string match "kvf" $command]} {
    if {$id > 0 && $id < 20} {
      if {[catch {exec ssh -l stream $host "~/Projects/Snowmix-0.4.5/scripts/kill_xinetd_videofeed $id" &} res]} {
        if {[lindex $::errorCode 0] eq "CHILDSTATUS"} {
          set status [lindex $::errorCode 2]
          puts "Failed killing videofeed $id for host $host : $status"
        } else {
          # Some kind of unexpected failure
          puts "Failed killing videofeed $id for host $host"
        }
      } else {
        puts "cam$id stopped"
      }
    }
  } elseif {[string match "stopcam" $command]} {
    if {$id > 0 && $id < 5} {
      if {[catch {exec ssh -l root cam$id "systemctl stop bonecam-app.service" &} res]} {
        if {[lindex $::errorCode 0] eq "CHILDSTATUS"} {
          set status [lindex $::errorCode 2]
          puts "Failed stopping cam for cam$id : $status"
        } else {
          # Some kind of unexpected failure
          puts "Failed stopping cam for cam$id"
        }
      } else {
        puts "cam$id stopped"
      }
    }
  } elseif {[string match "startcam" $command]} {
    if {$id > 0 && $id < 5} {
      if {[catch {exec ssh -l root cam$id "systemctl start bonecam-app.service" &} res]} {
        puts "Failed starting cam for cam$id"
      } else {
        puts "cam$id started"
      }
    }
  } elseif {[string match "restartcam" $command]} {
    if {$id > 0 && $id < 5} {
      if {[catch {exec ssh -l root cam$id "killall -9 gst-launch-1.0" &} res]} {
        puts "Failed restarting cam for cam$id"
      } else {
        puts "cam$id started"
      }
    }
  } elseif {[string match "rebootcam" $command]} {
    if {$id > 0 && $id < 5} {
      if {[catch {exec ssh -l root cam$id "reboot" &} res]} {
        puts "Failed rebooting cam for cam$id"
      } else {
        puts "cam$id started"
      }
    }
  } elseif {[string match "focus*" $command]} {
    if {[string match "focus_auto" $command]} {
       set cam_command "cam-ctl.sh focus auto"
    } elseif {[string match "focus_up" $command]} {
       set cam_command "cam-ctl.sh focusup 1"
    } elseif {[string match "focus_down" $command]} {
       set cam_command "cam-ctl.sh focusdown 1"
    } else {
       set focus [string map {_ \ } $command]
       set val [lindex $focus 1]
       set cam_command "cam-ctl.sh focus $val"
    }
#    puts "<$command> <$focus>"
#    puts "/opt/bonecam/bin/$cam_command "
    if {[catch {exec ssh -l root cam$id "/opt/bonecam/bin/$cam_command " &} res]} {
      puts "Failed setting focus for cam cam$id"
    } else {
      puts "cam$id focus set"
    }
  } elseif {[string match "zoom*" $command]} {
    if {[string match "zoom_up" $command]} {
       set cam_command "cam-ctl.sh zoomup 1"
    } elseif {[string match "zoom_down" $command]} {
       set cam_command "cam-ctl.sh zoomdown 1"
    } else {
       set zoom [string map {_ \ } $command]
       set val [lindex $zoom 1]
       set cam_command "cam-ctl.sh zoom $val"
    }
    if {[catch {exec ssh -l root cam$id "/opt/bonecam/bin/$cam_command " &} res]} {
      puts "Failed setting zoom for cam cam$id"
    } else {
      puts "cam$id zoom set"
    }
  }
}

proc FeedPress {x y button press menu id host port} {
  global relay system
puts "Button $button press $press Menu count [$menu index end]"
  if {$button == 1 && $press == 1} {
    FeedCommand $host $port dropfr $id
  } elseif {($button == 3 || ($system(window_system) == "aqua" && $button == 2)) && $press == 1} {
#    set entries [$menu index end]
#    set i 0
#    while {$i <= $entries} {
#      set name [$menu entrycget $i -label]
#      set type [lindex $name 1]
#      set type_id [lindex $name 2]
#      $menu entryconfigure $i -command "AddSender receiver $id $type $type_id"
#      incr i
#    }
    tk_popup $menu $x $y
  }
}
