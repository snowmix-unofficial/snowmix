# #!/bin/sh
# # \
# exec /usr/bin/wish "$0" ${1+"$@"}
# #

set snowrelay_port 9998
set snowrelay_ip 127.0.0.1
#set snowrelay_ip $snowmix_ip

#source ./connection.tcl
#source ./cs_feed_commands.tcl

# proc bgerror { message } {
#   puts "BGERROR : $message"
# }

set relay(servers) ""
set relay(snowmix_servers) ""

proc AppendServerLed { host port label_item } {
  global relay
  puts "APPENDING SERVER LED : relay(server,leds,$host,$port) $label_item"
  lappend relay(server,leds,$host,$port) $label_item
}

proc SetServerLed { host port color } {
  global relay led_image
  if {![info exist relay(server,leds,$host,$port)]} {
    puts "SetServerLed warning. Host $host port $port does not exist"
    return
  }
  if {![info exist led_image($color,10x12)]} {
    puts "SetServerLed warning. Color $color does not exist for host $host port $port. Using dark grey"
    set color darkgrey
  }
  foreach led $relay(server,leds,$host,$port) {
    $led configure -image $led_image($color,10x12)
  }
}

# AddServerCommand is used to add a command/response to be sent to a server
#  - type : snowmix or relay
#  - host : hostname or IP address
#  - port : server port number
#  - command : The command to send to the server
#  - response_proc : The procedure to read the response. This procedure will return 1 or 0 or -1. 1=Procedure want more input, 0=Procedure want no more input, -1=fail
#  - repeat_delay : 0=Only once n=repeat n ms after response_proc returns 0.
proc AddServerCommand { type host port command response_proc repeat_delay } {
  global relay
#if { $type == "relay" } { puts "ADDSERVERCOMMAND <$command> to $host $port" }
  if { $type == "snowmix" } { set tag relay_snowmix
  } elseif { $type == "relay" } { set tag relay
  } else {
    puts stderr "illegal type $type in AddServerCommand. Ignoring."
    return
  }
  set servers [format "%s_servers" $type]
  if {![info exist relay($servers)] || [lsearch -exact $relay($servers) "$host $port"] < 0} {
    puts stderr "Can not add command <$command> to $servers $host $port. Does not exist. Ignoring."
    return
  }
  if { $relay(serverready,$tag,$host,$port) } {
    if {[AsyncSendCommand $tag $host $port $command $response_proc SnowmixFail 2000]} {
      lappend relay(pending,$tag,$host,$port) [list AsyncSendCommand $tag $host $port $command $response_proc SnowmixFail 2000]
    } else {
      set relay(serverready,$tag,$host,$port) 0
    }
  } else {
    # We append the command to the pending list for the connection
    lappend relay(pending,$tag,$host,$port) [list AsyncSendCommand $tag $host $port $command $response_proc SnowmixFail 2000]
  }
}

#  - type : snowmix or relay
#  - host : hostname or IP address
#  - port : server port number
proc AddServer { type host port } {
  global relay
  set servers [format "%s_servers" $type]
  if {[info exist relay($servers)] && [lsearch -exact $relay($servers) "$host $port"] > -1} {
    puts stderr "$servers server $host $port was already added. Ignoring and skipping it."
    return
  }
  if { $type == "snowmix" } {
    set tag relay_snowmix
  } elseif { $type == "relay" } {
    set tag relay
  } else {
    puts "WARNING. Unknown type $type for AddServer. Ignoring it."
    return
  }
  ServerConnect $tag $host $port


  lappend relay($servers) "$host $port"
  puts stderr "Adding $servers $host $port."
}

proc ServerConnect { tag host port } {
  global relay
  SetServerLed $host $port orange
  set relay(setserverled,$tag,$host,$port) 1
  set relay(serverready,$tag,$host,$port) 0
  if {[info exist relay(pending,$tag,$host,$port)] && $relay(pending,$tag,$host,$port) != ""} {
    puts stderr "Cancelling for $tag $host $port before start : $relay(pending,$tag,$host,$port)"
    set relay(pending,$tag,$host,$port) ""
  }
  set handle [AsyncSetupConn $tag $host $port ServerReady ServerClosed SnowmixTimeout SnowmixFail 5000]

  if { $handle == "" } {
    puts stderr "server $tag $host $port could not be connected. Skipping server."
    SnowmixFail "" $tag $host $port
    return
  }
  set relay(serverhandle,$tag,$host,$port) $handle
  if {$tag == "relay"} {
  after 1000 [list AddServerCommand relay $host $port "recv verbose 1" "" 1000]
  after 1000 [list AddServerCommand relay $host $port "send verbose 1" "" 1000]
  }
}

# ServerReady gets called when the connection is established and ready for command
proc ServerReady { handle tag host port } {
  global relay
#  puts "Server Ready :$tag $host $port"
  if {$relay(setserverled,$tag,$host,$port)} {
    set relay(setserverled,$tag,$host,$port) 0
    SetServerLed $host $port green
  }
  set relay(serverready,$tag,$host,$port) 1
set res ""
catch {
  if {[info exist relay(pending,$tag,$host,$port)]} {
    set command [lindex $relay(pending,$tag,$host,$port) 0]
    if { $command != "" } {
      set relay(pending,$tag,$host,$port) [lreplace $relay(pending,$tag,$host,$port) 0 0]
      set res [eval $command]
      set relay(serverready,$tag,$host,$port) 0
    }
  }
} res
#  puts "Snowmix connected $tag $host $port done : $res"
  return 0
}

proc ServerNotConnected { tag host port } {
  global relay
  SetServerLed $host $port darkgrey
  set relay(setserverled,$tag,$host,$port) 1
  if { $tag == "relay" } {
    foreach type "receiver sender" {
      set types [format "%ss" $type]
      if {[info exist relay($types,$host,$port)]} {
        foreach id $relay($types,$host,$port) {
          StateChanged $type $id $host,$port "undefined"
        }
      }
    }
  } else {
    if {[info exist relay(feedlist,$host,$port)]} {
      foreach id $relay(feedlist,$host,$port) {
        if {[info exist relay(feed,state,$id,$host,$port)]} {
          set relay(feed,state,$id,$host,$port) "UNDEFINED"
          SetFeedButtonColor $host $port $id
        }
      }
    }
  }
  if {[info exist relay(pending,$tag,$host,$port)] && $relay(pending,$tag,$host,$port) != ""} {
    puts stderr "Cancelling for $tag $host $port : $relay(pending,$tag,$host,$port)"
    set relay(pending,$tag,$host,$port) ""
  }
}

proc ServerClosed { handle tag host port } {
  global relay
puts stderr "Server $tag $host $port closed."
  unset -nocomplain relay(serverhandle,$tag,$host,$port)
  set relay(serverready,$tag,$host,$port) 0
  ServerNotConnected $tag $host $port
}

proc SnowmixTimeout { handle tag host port } {
  global relay
puts stderr "Snowmix $tag $host $port Timeout."
  unset -nocomplain relay(serverhandle,$tag,$host,$port)
  set relay(serverready,$tag,$host,$port) 0
  ServerNotConnected $tag $host $port
}

proc SnowmixFail { handle tag host port } {
  global relay
puts stderr "Snowmix $tag $host $port Fail."
  unset -nocomplain relay(serverhandle,$tag,$host,$port)
  set relay(serverready,$tag,$host,$port) 0
  ServerNotConnected $tag $host $port
}

# STAT: recv id vers payload dubs ssrcs pay_change inv_size inv_vers packs lost
# STAT: recv 1 2 33 0 0 0 0 0 357330 81852
# STAT:
proc GetRTPStatus { handle tag host port line } {
  global relay
  if {[regexp {STAT:\ +recv\ +([0-9]+)\ +} $line match id]} {
    # puts "RTPSTATUS $host $port Values [lrange $line 2 end]"
    set i 3
    foreach varname $relay(rtpstatus,varnames,$host,$port) {
      set relay(rtpstatus,vars,$varname,$id,$host,$port) [lindex $line $i]
      incr i
    }
    if {![info exist relay(rtpstatus,vars,lost,offset,$id,$host,$port)]} {
      set relay(rtpstatus,vars,lost,offset,$id,$host,$port) 0
      set relay(rtpstatus,vars,lost,delta,$id,$host,$port) 0
    }
    set lost [expr $relay(rtpstatus,vars,lost,$id,$host,$port) - $relay(rtpstatus,vars,lost,offset,$id,$host,$port)]
    if { $lost != $relay(rtpstatus,vars,lost,delta,$id,$host,$port) && [info exist relay(receiver,$id,lostlabel,$host,$port)] } { $relay(receiver,$id,lostlabel,$host,$port) configure -bg orange }
    set relay(rtpstatus,vars,lost,delta,$id,$host,$port) $lost
    return 1
  } elseif {[regexp {STAT:\ +recv\ +id\ +} $line match]} {
    # puts "RTPSTATUS $host $port Varnames [lrange $line 3 end]"
    set relay(rtpstatus,varnames,$host,$port) [lrange $line 3 end]
    return 1
  } elseif {[string compare "STAT:" $line] == 0} {
    # This was the last line we expected to read and we can now return 0
#puts "Got RTP infor for $host $port"
    return 0
  } else {
    # We ignore rest, command error included
puts "RTPSTATUS $host $port got ignore <$line>"
    return 0
  }
  return 1
}

proc GetOutputState { handle tag host port line } {
  global relay

  if {[string compare "STAT: " $line] == 0} {
    # This was the last line we expected to read and we can now return 0
    after 1000 [list AddServerCommand snowmix $host $port "system output status" GetOutputState 1000]
    return 0

  # STAT: Inuse                = 5 of 40 frame buffers
  } elseif {[regexp {STAT:\ +Inuse\ += ([0-9]+)\ +of\ +([0-9]+)} $line match inuse max_inuse]} {
    set relay(output,inuse,$host,$port) $inuse
    set relay(output,max_inuse,$host,$port) $max_inuse
    if { $max_inuse > 0 } {
      set h [$relay(outcanvas,$host,$port) cget -height]
      set x1 5
      set y1 $h
      set x2 10
      set y2 [expr $h * (1 - $inuse / double($max_inuse))]
      set color yellow
      $relay(outcanvas,$host,$port) delete inuse
      $relay(outcanvas,$host,$port) create rect $x1 $y1 $x2 $y2 -outline $color -fill $color -width 0.5 -tag inuse
    }
    return 1

  # STAT: Ready for output     = 1 frames
  } elseif {[regexp {STAT:\ +Ready\ +for\ +output\ += ([0-9]+)} $line match ready_output]} {
    set relay(output,ready_output,$host,$port) $ready_output
    if {[info exist relay(output,max_inuse,$host,$port)]} {
      if {$relay(output,max_inuse,$host,$port) > 0 } {
        set h [$relay(outcanvas,$host,$port) cget -height]
        set x1 15
        set y1 [expr $h - $ready_output]
        set x2 20
        set y2 $h
        set color lightblue
        $relay(outcanvas,$host,$port) delete out_ready
        $relay(outcanvas,$host,$port) create rect $x1 $y1 $x2 $y2  -outline $color -fill $color -width 1 -tag out_ready
      }
    }
    return 1

  # STAT: Frame p. min/ave/max = 40.0 40.0 40.0 ms
  } elseif {[regexp {STAT:\ +Frame\ +p\.\ +min\/ave\/max\ += ([.0-9]+)\ +([.0-9]+)\ +([.0-9]+)} $line match min ave max]} {
    set relay(output,min,$host,$port) $min
    set relay(output,ave,$host,$port) $ave
    set relay(output,max,$host,$port) $max
    AddCoors $relay(graph_id_output,$host,$port) 1 $relay(output_canvas,x,$host,$port) $min
    AddCoors $relay(graph_id_output,$host,$port) 2 $relay(output_canvas,x,$host,$port) $ave
    AddCoors $relay(graph_id_output,$host,$port) 3 $relay(output_canvas,x,$host,$port) $max
    incr relay(output_canvas,x,$host,$port)
#PMM2
    return 1
  }
  # We silently ignore all other lines.
  # We return 1 to tell we want more lines.
  return 1
}

proc GetFeedStates { handle tag host port line } {
  global relay feed

  # STAT: feed id : state islive oneshot geometry cutstart cutsize offset fifo good missed dropped <name>
  # STAT: feed 1 : RUNNING live continuously 1280x720 0,0 1280x720 80,45 0:10 0 61358 0 <Feed #1>
  if {[string compare "STAT: " $line] == 0} {
    # This was the last line we expected to read and we can now return 0
    after 1000 [list AddServerCommand snowmix $host $port "feed info" GetFeedStates 1000]
    return 0
  } elseif {[regexp {STAT:\ +feed\ +([1-9][0-9]*)\ +:\ +([^ ]+)\ +[^ ]+\ +[^ ]+\ +[^ ]+\ +[^ ]+\ +[^ ]+\ +[^ ]+\ +([0-9]+):([0-9]+)\ +([0-9]+)\ +([0-9]+)\ +([0-9]+)\ +} $line match id state fifo_depth fifo_max good missed dropped]} {
    # puts "Got for $host:$port $id state $state $fifo_depth:$fifo_max $good $missed $dropped"

    # If state for the feed does not exist, then this is a new feed
    if {![info exist relay(feed,state,$id,$host,$port)]} {
      lappend relay(feedlist,$host,$port) $id
      MakeFeedButton $host $port $id $state
      set relay(feed,state,$id,$host,$port) "UNDEFINED"
      SetFeedButtonColor $host $port $id
    }
    if {$state != $relay(feed,state,$id,$host,$port)} {
      set relay(feed,state,$id,$host,$port) $state
# puts "Setting Feed color for $host $port $id"
      SetFeedButtonColor $host $port $id
    }
    $relay(feedbutsfr,$host,$port).feed$id configure -helptext "Feed $id\n$state\nFifo $fifo_depth of $fifo_max\ngood $good\nmissed $missed\ndropped $dropped"
    set relay(feed,fifo,$id,$host,$port) "$fifo_depth $fifo_max"
    set relay(feed,good,$id,$host,$port) $good
    set relay(feed,missed,$id,$host,$port) $missed
    set relay(feed,dropped,$id,$host,$port) $dropped
    set n $relay(feed,button_n,$id,$host,$port)
    set x1 [expr 5 + 10 * $n]
    set x2 [expr $x1 + 5]
    set h [$relay(feedcanvas,$host,$port) cget -height]
    set y1 [expr $h + 2]
    set y2 [expr 1 - (double($fifo_depth) / $fifo_max)]
    if { $y2 > 0.8 } { set color green
    } elseif { $y2 > 0.7 } { set color lightgreen
    } elseif { $y2 > 0.55 } { set color lightblue
    } elseif { $y2 > 0.35 } { set color yellow
    } elseif { $y2 > 0.2 } { set color orange
    } else { set color red }
    set y2 [expr round($h * $y2 + 2)]
    $relay(feedcanvas,$host,$port) delete feed$id
    $relay(feedcanvas,$host,$port) create rect $x1 $y1 $x2 $y2 -outline $color -fill $color -width 1 -tag feed$id
  }
  # We silently ignore all other lines.
  # We return 1 to tell we want more lines.
  return 1
}

proc MakeFeedButton { host port id state } {
  global relay feed
  if {[info exist relay(feedbutsfr,$host,$port)]} {
    if { $feed(state_color,$state) == "black" } { set fg yellow
    } elseif { $feed(state_color,$state) == "green" } { set fg white } else { set fg black }
    Label $relay(feedbutsfr,$host,$port).feed$id -text $id -padx 4 -pady 1 -bg $feed(state_color,$state) -fg $fg -relief raised -bd 1 -helptext "Feed $id\n$state"
    pack $relay(feedbutsfr,$host,$port).feed$id -side left -padx 0
    if {![info exist relay(feedbutscount,$host,$port)]} { set relay(feedbutscount,$host,$port) 0
    } else { incr relay(feedbutscount,$host,$port) }
    set relay(feed,button_n,$id,$host,$port) $relay(feedbutscount,$host,$port)

    set canvasw [$relay(feedcanvas,$host,$port) cget -width]
    $relay(feedcanvas,$host,$port) configure -width [expr $canvasw + 10]

#    set hostmap [string map {. _} $host]

    if {[info procs AddFeedCommands] != ""} { AddFeedCommands $host $port $id $relay(feedbutsfr,$host,$port).feed$id }
  }
}

proc SetFeedButtonColor { host port id } {
  global relay feed
  if {![info exist relay(feed,state,$id,$host,$port)]} {
    puts "Feed button $host $port does not exist"
    return
  }
  set state $relay(feed,state,$id,$host,$port)
  if {![info exist feed(state_color,$state)]} {
    puts "Color for feed state $state does not exist."
    return
  }
  set bg $feed(state_color,$state)
  if { $bg == "black" || $bg == "darkgrey" || $bg == "darkgren" } { set fg white } else { set fg black }
  catch { $relay(feedbutsfr,$host,$port).feed$id configure -bg $feed(state_color,$state) -fg $fg } res
}

proc ConnectSnowrelay {host port} {
  global relay
  # Close
  if {[info exist relay(iohandle,$host,$port)]} {
    set iohandle $relay(iohandle,$host,$port)
    unset $relay(iohandle,$host,$port)
    catch { close $iohandle }
  }
  if {[catch { set iohandle [OpenSocket4LineOld $host $port 1] } res]} {
puts "Failed to open $host $port connection"
    return ""
  }
  if {[catch {
    gets $iohandle line
    puts $iohandle "receiver verbose 1"
    gets $iohandle line
    puts $iohandle "sender verbose 1"
    gets $iohandle line
  } res]} {
puts "Failed to initialize $host $port connection"
    return ""
  } else {
    set relay(iohandle,$host,$port) $iohandle
    puts "Connected $host $port"
  }
  return $relay(iohandle,$host,$port)
}

# recv stat
# MSG: receiver status 1413497282.899
# MSG: receiver id state started receivers received dropped
# MSG: receiver 1 running 1 0:1 11130458 0
# MSG: receiver 2 pending 1 0:1 0 0
# MSG: receiver 3 running 1 0:1 24814935 0
# MSG:
proc SetupGettingStatus { itemtype host port } {
  global relay
  set key [format "%ss,%s,%s" $itemtype $host $port]
  set relay(pattern1,getstatus,$key) "MSG:\ +$itemtype\ +(\[0-9]+)\ +(\[^ ]+)\ +(\[0-9]+)\ +(\[^ ]+)\ +(\[^ ]+)\ +(\[\-0-9]+)"
  set relay(pattern2,getstatus,$key) "MSG:\ +$itemtype\ +id\ +(\.+)"
  set relay(pattern3,getstatus,$key) "MSG:\ +$itemtype\ +status\ +(\[^ ]+)"
  if { $itemtype == "receiver" } {
    after 100 [list AddServerCommand relay $host $port "recv stat" GetRecvStatus 1000]
  } elseif { $itemtype == "sender" } {
    after 200 [list AddServerCommand relay $host $port "send stat" GetSendStatus 1000]
  }
}

proc GetRecvStatus { handle tag host port line } {
  return [GetStatus receiver $tag $host $port $line]
}
proc GetSendStatus { handle tag host port line } {
  return [GetStatus sender $tag $host $port $line]
}
proc GetStatus { itemtype tag host port line } {
  global relay
  set key [format "%ss,%s,%s" $itemtype $host $port]
  set hostid "$host,$port"
  if {[string match receiver $itemtype]} {
    set types "receivers"
    set graph_id recv-$hostid
  } elseif {[string match sender $itemtype]} {
    set types "senders"
    set graph_id send-$hostid
  } else {
    puts "Wrong type in GetStatus $itemtype"
    return 0
  }
  if {[regexp $relay(pattern1,getstatus,$key) $line match id state started receivers received dropped]} {
    foreach keyname {started receivers received dropped} {
      upvar 0 $keyname keyval
      set relay($itemtype,$id,$keyname,$hostid) $keyval
      #puts "relay($itemtype,$id,$keyname,$hostid) $keyval"
      if {$state != $relay($itemtype,$id,state,$hostid)} {
        StateChanged $itemtype $id $hostid $state
      }
    }
    # We want more lines and return 1
    return 1
  } elseif {[regexp $relay(pattern2,getstatus,$key) $line match format]} {
    set relay($itemtype,format,status,$hostid) "id $format"
    # We want more lines and return 1
    return 1
  } elseif {[regexp $relay(pattern3,getstatus,$key) $line match time]} {
    set relay($itemtype,time,status,$hostid) $time
    # We want more lines and return 1
    return 1
  } elseif {[string compare "MSG:" $line] == 0} {
    # This was the last line we expected to read and we can now return 0, but first we do some accounting and updating
    if {[info exist relay($itemtype,time,last,status,$hostid)]} {
      set deltatime [expr $relay($itemtype,time,status,$hostid) - $relay($itemtype,time,last,status,$hostid)]
#puts "GotStatus $itemtype $deltatime relay($types,$hostid) = <$relay($types,$hostid)>"
      foreach id $relay($types,$hostid) {
        set bps [expr int (8 * ($relay($itemtype,$id,received,$hostid) - $relay($itemtype,$id,received,last,$hostid)) / $deltatime)]
#puts " - bps $id = $bps"
        #set graph_id_recv recv-$hostid

	if {[catch { AddCoors $graph_id $id $relay($itemtype,feed_x,$hostid) $bps } res]} {
          puts stderr "Failed to add coors for graph $graph_id $id. Probably not ready yet. Ignoring. Cause : $res"
        }
      }
      incr relay($itemtype,feed_x,$hostid)
    }
    foreach id $relay($types,$hostid) {
      set relay($itemtype,$id,received,last,$hostid) $relay($itemtype,$id,received,$hostid)
    }
    set relay($itemtype,time,last,status,$hostid) $relay($itemtype,time,status,$hostid)
    if { $itemtype == "receiver" } {
      after 100 [list AddServerCommand relay $host $port "recv rtps" GetRTPStatus 1000]
      after 1000 [list AddServerCommand relay $host $port "recv stat" GetRecvStatus 1000]
    } elseif { $itemtype == "sender" } {
      after 1000 [list AddServerCommand relay $host $port "send stat" GetSendStatus 1000]
    }
    return 0
  } else {
    puts "STATUS skipping <$line>"
  }
}

proc SetupGettingInfo { itemtype host port } {
  global relay
  set key [format "%ss,%s,%s" $itemtype $host $port]
  set relay($key) ""
  set relay(pattern1,getinfo,$key) "MSG:\ +$itemtype\ +(\[0-9]+)\ +(\[^ ]+)\ +(\[^ ]+)\ +(\[0-9]+)\ +(\[0-9]+)\ +(\[^ ]+)\ +(\[^ ]+)\ +(\[^ ]+)\ +(\[^ ]+)\ +(\[\-0-9]+)\ +<(\[^\>]+)>"
  set relay(pattern2,getinfo,$key) "MSG:\ +$itemtype\ +id\ +(\.+)"
  set relay(pattern3,getinfo,$key) "MSG:\ +$itemtype\ +info\ +verbose\ +(\[0-9]+)\ +count\ +(\[0-9]+)\ +of\ +(\[0-9]+)"
  #set relay(pattern1,getsource,$key) "MSG:\ +\[^ ]+\ +(\[0-9]+)\ +source\ +(\[^ ]+)\ +(\[0-9]+)"
  set relay(pattern1,getsource,$key) "MSG:\ +\sender+\ +(\[0-9]+)\ +source\ +(\[^ ]+)\ +(\[0-9]+)"
  if { $itemtype == "receiver" } {
    after 0 [list AddServerCommand relay $host $port "recv info" GetRecvInfo 1000]
    after 50 [list AddServerCommand relay $host $port "recv rtp" GetRTPStatus 1000]
  } elseif { $itemtype == "sender" } {
    after 0 [list AddServerCommand relay $host $port "send info" GetSendInfo 1000]
  } elseif { $itemtype == "sendersource" } {
    after 0 [list AddServerCommand relay $host $port "send source" GetSendSource 1000]
  }
}

proc GetRecvInfo { handle tag host port line } {
  return [GetInfo receiver $host $port $line]
}
proc GetSendInfo { handle tag host port line } {
  return [GetInfo sender $host $port $line]
}
proc GetSendSource { handle tag host port line } {
  global relay
  set res [GetInfo sender $host $port $line]
  if { $res == 0 } {
   AddToOverviewPane overview $host $port
#    puts "UPDATE STATES ordered"
    SetupGettingStatus receiver $host $port
    SetupGettingStatus sender $host $port
  }
  return $res
}

proc GetInfo { itemtype host portno line } {
  global relay

  set key [format "%ss,%s,%s" $itemtype $host $portno]

  if {[regexp $relay(pattern1,getinfo,$key) $line match id type ip port mtu state start stop fds queues name]} {
    foreach keyname {type ip port mtu state start stop fds queues name} {
      upvar 0 $keyname keyval
      set relay($itemtype,$id,$keyname,$host,$portno) $keyval
    }
    lappend relay($key) $id
    return 1
  } elseif {[regexp $relay(pattern2,getinfo,$key) $line match format]} {
    set relay($itemtype,format,info,$host,$portno) "id $format"
    return 1
  } elseif {[regexp $relay(pattern3,getinfo,$key) $line match verbose count max_count]} {
    foreach keyname {verbose count max_count} {
      upvar 0 $keyname keyval
      set relay($itemtype,$keyname,$host,$portno) $keyval
    }
    return 1
  # elseif {[regexp {MSG:\ +sender\ +([0-9]+)\ +source\ +([^ ]+)\ +([0-9]+)} $line match id source sourceid]}
  } elseif {[regexp $relay(pattern1,getsource,$key) $line match id source sourceid]} {
if { $source == "undefined" } {
puts "PMMMM undefined source $source $sourceid"
}

    set relay(sender,$id,source,$host,$portno) $source
    set relay(sender,$id,sourceid,$host,$portno) $sourceid
    return 1
  } elseif {[string compare "MSG:" $line] == 0} {
    # This was the last line we expected to read and we can now return 0
    return 0
  }
  # Skipping other lines and expect more lines
  return 1
}

# recv info
# MSG: receiver info verbose 2 count 3 of 20
# MSG: receiver id type host port mtu queues state start stop fd queues
# MSG: receiver 1 udp 0.0.0.0 10040 1492 running 1413482325.942 0.000 5,-1 2 <Cam #1>
# MSG: receiver 2 udp 0.0.0.0 10042 1492 pending 1413482325.942 0.000 6,-1 2 <Cam #2>
# MSG: receiver 3 udp 0.0.0.0 10044 1492 running 1413482325.942 0.000 7,-1 2 <Cam #3>
# MSG:
proc GetRecvSend {itemtype host portno} {
  global relay
  if {[string match receiver $itemtype]} {
    set key "receivers,$host,$portno"
  } elseif {[string match sender $itemtype]} {
    set key "senders,$host,$portno"
  } else {
    puts "Wrong type in GetRecvSend $itemtype"
    return
  }
  set relay($key) ""
  if {![info exist relay(iohandle,$host,$portno)]} {
    puts "IO handle for $host $portno does not exist"
    return 1
  }
  if {[catch {puts $relay(iohandle,$host,$portno) "$itemtype info "} res]} {
puts "Failed to send command $itemtype info"
    return 1
  }
  set pattern1 "MSG:\ +$itemtype\ +(\[0-9]+)\ +(\[^ ]+)\ +(\[^ ]+)\ +(\[0-9]+)\ +(\[0-9]+)\ +(\[^ ]+)\ +(\[^ ]+)\ +(\[^ ]+)\ +(\[^ ]+)\ +(\[\-0-9]+)\ +<(\[^\>]+)>"
  set pattern2 "MSG:\ +$itemtype\ +id\ +(\.+)"
  set pattern3 "MSG:\ +$itemtype\ +info\ +verbose\ +(\[0-9]+)\ +count\ +(\[0-9]+)\ +of\ +(\[0-9]+)"
  while {[gets $relay(iohandle,$host,$portno) line] >= 0} {
#puts "line <$line>"
    if {[string compare "MSG:" $line] == 0} {
      break
    } elseif {[regexp $pattern1 $line match id type ip port mtu state start stop fds queues name]} {
      foreach keyname {type ip port mtu state start stop fds queues name} {
        upvar 0 $keyname keyval
        set relay($itemtype,$id,$keyname,$host,$portno) $keyval
        #puts "relay($itemtype,$id,$keyname,$host,$portno) $keyval"
      }
      lappend relay($key) $id
    } elseif {[regexp $pattern2 $line match format]} {
      set relay($itemtype,format,info,$host,$portno) "id $format"
    } elseif {[regexp $pattern3 $line match verbose count max_count]} {
      foreach keyname {verbose count max_count} {
        upvar 0 $keyname keyval
        set relay($itemtype,$keyname,$host,$portno) $keyval
      }
    } else {
puts "Skipping line <$line>"
    }
  }
  return 0
}

# MSG: receiver 1 stopped
# MSG: receiver 2 pending
# MSG:
proc GetStates { type hostid } {
  global relay
puts "GetState"
  if {![info exist relay(iohandle,$hostid)]} {
    puts "No iohandle for getting state for $type $hostid"
    return
  }
  set pattern1 "MSG:\ +$type\ +(\[0-9]+)\ +(\[^ ]+)"
SnowmixLock r1
  if {[catch {
    puts $relay(iohandle,$hostid) "$type start"
    while {[gets $relay(iohandle,$hostid) line] >= 0} {
      if {[string compare "MSG:" $line] == 0} break
      if {[regexp $pattern1 $line match id state]} {
        if {![info exist relay($type,$id,state,$hostid)]} {
          set relay($type,$id,state,$hostid) $state
          continue
        }
        if {$state != $relay($type,$id,state,$hostid)} {
          StateChanged $type $id $hostid $state
        }
      }
    }
  } res]} {
    puts "GetStates Failed for $hostid"
  }
SnowmixUnlock r1
}

proc StateChanged {type id hostid state} {
  global relay led_image
  if {![info exist relay($type,$id,state,$hostid)]} {
    puts "StateChanged for $type $id, but it does not exist."
    return
  }
#puts "State changed to $state for $type $id $hostid"
  if {[info exist relay($type,$id,leds,$hostid)]} {
    foreach led $relay($type,$id,leds,$hostid) {
      $led configure -image $led_image($relay(statecolor,$state),10x12)
      if {[info exist relay($type,$id,text,$hostid)]} {
        set text [$relay($type,$id,text,$hostid) cget -helptext]
        if {[regexp {.+State:\ +([^ \n]+)} $text match current_state]} {
#puts "Current state <$current_state> help <$text>"
          set newtext [string map "$current_state $state" $text]
#puts "new help <$newtext>\n"

          $relay($type,$id,text,$hostid) configure -helptext $newtext
        }
      }
    }
  }
  set relay($type,$id,state,$hostid) $state
}

proc ReceiverItem { pane type id host port} {
  global relay
  #puts "ReceiverItem $type $id $host $port"
  set hostid "$host,$port"
  set hostmap [string map {. _} $host]
  set fr [frame $pane.$type$id-$hostmap-$port]
  set frl [frame $fr.left]
  set frr [frame $fr.right]
  pack $frl $frr -side left -anchor nw
  label $frl.run -text Run
  label $frl.id -text ID -pady 1
  label $frl.name -text Name -pady 2
  label $frl.type -text Type -pady 3
  label $frl.ip -text IP -pady 2
  label $frl.port -text Port -pady 2
  label $frl.mtu -text MTU -pady 2
  label $frl.state -text State -pady 1
  label $frl.fds -text Fds -pady 1
  label $frl.queues -text Queues -pady 1
  pack $frl.run $frl.id $frl.name $frl.type $frl.ip $frl.port $frl.mtu $frl.state $frl.fds $frl.queues -side top -anchor nw

  frame $frr.run
  Button $frr.run.startstop -text Start -helptext "Start/stop $type $id" -pady 0 -armcommand "StartStop $type start $id $hostid"
  Button $frr.run.pause -text Pause -helptext "Pause $type $id" -pady 0 -armcommand "StartStop $type stop $id $hostid"
  pack $frr.run.startstop $frr.run.pause -side left -fill x -expand 1
  Label $frr.id -text "$type $id" -relief sunken
  
  # Name entry
  Entry $frr.name -textvariable relay($type,$id,newname,$hostid) -relief sunken -helptext "Name of $type $id"
  bind $frr.name <Key-Return> "ReceiverNameSet $type $id $hostid"
  bind $frr.name <FocusOut> "ReceiverNameSet $type $id $hostid"
  set relay($type,$id,newname,$hostid) $relay($type,$id,name,$hostid)

  ComboBox $frr.type -values {udp multicast tcpserver tcpclient} \
	-modifycmd "ReceiverTypeChange $frr.type $type $id $hostid"
  SetComboBox $frr.type $relay($type,$id,type,$hostid)
#  Label $frr.type -textvariable relay($type,$id,type,$hostid) -justify right -relief sunken \
#	-helptext "Type of $type $id"
  Entry $frr.ip -textvariable relay($type,$id,newip,$hostid) -justify right -relief sunken \
	-helptext "IP for $type $id\nFormat a.b.c.d"
  set relay($type,$id,newip,$hostid) $relay($type,$id,ip,$hostid)
  bind $frr.ip <Key-Return> "ReceiverIPSet $type $id $hostid"
  bind $frr.ip <FocusOut> "ReceiverIPSet $type $id $hostid"

  # Port
  Entry $frr.port -textvariable relay($type,$id,newport,$hostid) -justify right -relief sunken \
	-helptext "Port for $type $id.\n1023 < port < 65536"
  set relay($type,$id,newport,$hostid) $relay($type,$id,port,$hostid)
  bind $frr.port <Key-Return> "ReceiverPortSet $type $id $hostid"
  bind $frr.port <FocusOut> "ReceiverPortSet $type $id $hostid"

  # MTU
  Entry $frr.mtu -textvariable relay($type,$id,newmtu,$hostid) -justify right -relief sunken \
	-helptext "MTU for $type $id\nLocalhost : 0 < MTU < 65536\nAll others less than 1500"
  set relay($type,$id,newmtu,$hostid) $relay($type,$id,mtu,$hostid)
  bind $frr.mtu <Key-Return> "ReceiverMTUSet $type $id $hostid"
  bind $frr.mtu <FocusOut> "ReceiverMTUSet $type $id $hostid"

  Label $frr.state -textvariable relay($type,$id,state,$hostid) -justify right -relief sunken \
	-helptext "State of $type $id"
  Label $frr.fds -textvariable relay($type,$id,fds,$hostid) -justify right -relief sunken \
	-helptext "Fds of $type $id"
  Label $frr.queues -textvariable relay($type,$id,queues,$hostid) -justify right -relief sunken \
	-helptext "Queues for $type $id"

  set gfr [frame $frr.graph]
  set graph [CreateGraph $gfr $type-$id 160 60 10 5 3]
  pack $graph -side top

  pack $frr.run $frr.id $frr.name $frr.type $frr.ip $frr.port $frr.mtu $frr.state $frr.fds $frr.queues $gfr -side top -anchor nw -fill x -expand 1
  return $fr
}

proc StartStop { type command id hostid } {
  global relay
  if {![info exist relay(iohandle,$hostid)]} { return }
  puts $relay(iohandle,$hostid) "$type $command $id"
  gets $relay(iohandle,$hostid)  line
}

proc ReceiverIPSet { type id hostid } {
  global relay
  if {![info exist relay(iohandle,$hostid)]} { return }
  set ip $relay($type,$id,newip,$hostid)
  if {[string length $ip] && [regexp {([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)} $ip match a b c d] &&
	$a >= 0 && $a < 256 && $b >= 0 && $b < 256 && $c >= 0 && $c < 256 && $d >= 0 && $d < 256} {
    #puts "recv host $id $a.$b.$c.$d"
    puts $relay(iohandle,$hostid) "$type host $id $a.$b.$c.$d"
    gets $relay(iohandle,$hostid) line
    if {[string match "*error*" $line]} {
      set relay($type,$id,newip,$hostid) $relay($type,$id,ip,$hostid)
    } else {
      set relay($type,$id,ip,$hostid) $a.$b.$c.$d
    }
  } else {
    set relay($type,$id,newip,$hostid) $relay($type,$id,ip,$hostid)
  }
}

proc ReceiverMTUSet { type id hostid } {
  global relay
  if {![info exist relay(iohandle,$hostid)]} { return }
  set mtu $relay($type,$id,newmtu,$hostid)
  if {[string length $mtu] && [string is integer -strict $mtu] && $mtu > 1 && $mtu < 65536} {
    puts $relay(iohandle,$hostid) "$type mtu $id $mtu"
    gets $relay(iohandle,$hostid) line
    if {[string match "*error*" $line]} {
      set relay($type,$id,newmtu,$hostid) $relay($type,$id,mtu,$hostid)
    } else {
      set relay($type,$id,mtu,$hostid) $mtu
    }
  } else {
    set relay($type,$id,newmtu,$hostid) $relay($type,$id,mtu,$hostid)
  }
}

proc ReceiverPortSet { type id hostid } {
  global relay
  if {![info exist relay(iohandle,$hostid)]} { return }
  set port $relay($type,$id,newport,$hostid)
  if {[string length $port] && [string is integer -strict $port] && $port > 1023 && $port < 65536} {
    puts $relay(iohandle,$hostid) "$type port $id $port"
    gets $relay(iohandle,$hostid) line
    if {[string match "*error*" $line]} {
      set relay($type,$id,newport,$hostid) $relay($type,$id,port,$hostid)
    } else {
      set relay($type,$id,port,$hostid) $port
    }
  } else {
    set relay($type,$id,newport,$hostid) $relay($type,$id,port,$hostid)
  }
}

proc ReceiverNameSet { type id hostid } {
  global relay
  if {![info exist relay(iohandle,$hostid)]} { return }
  if {[string length $relay($type,$id,newname,$hostid)]} {
    puts $relay(iohandle,$hostid) "$type name $id $relay($type,$id,newname,$hostid)"
    gets $relay(iohandle,$hostid) line
    if {[string match "*error*" $line]} {
      set relay($type,$id,newname,$hostid) $relay($type,$id,name,$hostid)
    } else {
      set relay($type,$id,name,$hostid) $relay($type,$id,newname,$hostid)
    }
  } else {
    set relay($type,$id,newname,$hostid) $relay($type,$id,name,$hostid)
  }
}

proc ReceiverTypeChange { combobox type id hostid } {
  global relay
  if {![info exist relay(iohandle,$hostid)]} { return }
  set recvtype [$combobox get]
  puts $relay(iohandle,$hostid) "$type type $id $recvtype"
  gets $relay(iohandle,$hostid) line
  if {[string match "*error*" $line]} {
    puts "Failed setting type"
    SetComboBox $combobox $relay($type,$id,type,$hostid)
  } else {
    set relay($type,$id,type,$hostid)
  }
}

proc CreateSubPane { pane tabid } {
  global relay

  set bg [$pane cget -bg]
  set recvfr [frame [$pane getframe $tabid].main -padx 0 -pady 0 -border 0 -bg $bg -bg red]
  pack $recvfr -side top -anchor nw -expand 1

  set sw [ScrolledWindow $recvfr.sw -bg $bg]
  #set sff [ScrollableFrame $sw.f -height 1000 -width 1200 -bg $bg]
  set sff [ScrollableFrame $sw.f -height 1000 -width 12000 -bg $bg]
  $sw setwidget $sff
  pack $sw -side top -fill both -expand 1 -anchor nw

  set recvfr [$sff getframe]

  $recvfr configure -bg $bg

#  ScrollableFrame $recvfr.scrollframe -constrainedwidth false -constrainedheight false \
#    -xscrollcommand "$recvfr.scrollbar set"
#  scrollbar $recvfr.scrollbar -command "$recvfr.scrollframe xview" -orient horizontal
#  pack $recvfr.scrollframe -side top -fill both -expand 1
#  pack $recvfr.scrollbar -side top -fill x -expand 1
#  set recvfr [$recvfr.scrollframe getframe]

  # sfr holds the controls
  #set sfr [frame $recvfr.$tabid -padx 5 -pady 6 -bd 2 -relief raised -bg green]
set bg [$pane cget -bg]
  set sfr [frame $recvfr.$tabid -padx 0 -pady 0 -border 0 -bg $bg]
  pack $sfr -fill both -side left -anchor nw -expand 1 -fill both
  set relay(pane,subpane,$tabid) $recvfr
  set relay(pane,controls,$tabid) $sfr
}

proc AddToSubPane { type } {
  global relay
  if {![info exist relay(servers)]} {
    puts "No servers"
    return
  }
  foreach hostset $relay(servers) {
    set host [lindex $hostset 0]
    set port [lindex $hostset 1]
    if {[string match receiver $type]} {
      set key "receivers,$host,$port"
    } elseif {[string match sender $type]} {
      set key "senders,$host,$port"
    } else {
      puts "Wrong type $type for AddToSubPane"
      return
    }
    set hostmap [string map {. _} $host]
    set subfr [frame $relay(pane,controls,$type).$hostmap-$port -relief raised -bd 2 -padx 5 -pady 5]
    frame $subfr.title
    label $subfr.title.label -text "Server: $host port $port"
    frame $subfr.container
    pack $subfr.title $subfr.container $subfr.title.label -side top -anchor nw
    pack $subfr -side left -anchor nw -expand 1 -padx 5 -pady 5
    foreach id $relay($key) {
      #set relay(pane,subpane,$type) [ReceiverItem $relay(pane,controls,$type) $type $id $host $port]
      set fr [ReceiverItem $subfr.container $type $id $host $port]
      pack $fr -side left -anchor nw -expand 1
    }
  }
}

#set relay(statecolor,undefined) lightgrey
set relay(statecolor,undefined) darkgrey
set relay(statecolor,setup) lightgrey
set relay(statecolor,ready) yellow
set relay(statecolor,pending) orange
set relay(statecolor,connected) blue
set relay(statecolor,running) green
set relay(statecolor,stalled) red
set relay(statecolor,disconnected) cyan
set relay(statecolor,stopped) purple

set relay(staterelief,undefined) raised
set relay(staterelief,setup) raised
set relay(staterelief,ready) raised
set relay(staterelief,pending) sunken
set relay(staterelief,connected) sunken
set relay(staterelief,running) sunken
set relay(staterelief,stalled) sunken
set relay(staterelief,disconnected) raised
set relay(staterelief,stopped) raised

menu .relay_recv -tearoff 0
.relay_recv add command -label "Start"
.relay_recv add command -label "Stop"
.relay_recv add command -label "Delete" -state disabled
.relay_recv add command -label "Analyze"
#.relay_recv add command -label Exit -command {exit}

proc RightClickCommand { type id host port name } {
  global relay system
  puts "RightClickCommand $type $id $host $port : $name"
  set state $relay($type,$id,state,$host,$port)
  if { $name == "Start" } {
    if {[lsearch -exact {stopped ready disconnected} $state] > -1} {
      after 0 [list AddServerCommand relay $host $port "$type start $id" "" 1000]
    }
  } elseif { $name == "Stop" } {
    if {[lsearch -exact {running pending connected stalled} $state] > -1} {
      after 0 [list AddServerCommand relay $host $port "$type stop $id" "" 1000]
    }
  } elseif { $name == "Analyze" } {
   set analyzer_id 1
   set awin .analyzer-$id
   if {![winfo exist $awin]} { MakeAnalyzerWindow $host $port $type $id }
  }
}

proc OverviewRecvPress {x y button press menu frame type id host port} {
  global relay system
  set hostid "$host,$port"
puts "OverviewRecvPress $x $y $button $press $menu $frame $type $id $host $port"
puts "Button $button press $press"
  set state $relay($type,$id,state,$hostid)
  if {($button == 3 || ($system(window_system) == "aqua" && $button == 2)) && $press == 1} {
    set entries [$menu index end]
    set i 0
    while {$i <= $entries} {
      set name [$menu entrycget $i -label]
      set entrystate [$menu entrycget $i -state]
      if { $name == "Start" } {
        if {[lsearch -exact {running pending connected stalled} $state] > -1} {
          set entrystate disabled
        } else { set entrystate normal }
      } elseif { $name == "Stop" } {
        if {[lsearch -exact {stopped ready disconnected} $state] < 0} { set entrystate normal
        } else { set entrystate disabled }
      }
      $menu entryconfigure $i -state $entrystate -command "RightClickCommand $type $id $host $port $name"
      incr i
    }

    tk_popup .relay_recv $x $y
  } elseif {$button == 1} {
    #set relief [$frame cget -relief]
    if {$press == 1} { set relief sunken } else { set relief raised }
    $frame configure -relief $relief
    if {$press == 1} {
#      if {![info exist relay(iohandle,$hostid)]} {
#        tk_messageBox -title Alert -message "$type $id $hostid has no iohandle."
#        $frame configure -relief raised
#        return
#      }
      if {![info exist relay($type,$id,state,$hostid)]} {
        tk_messageBox -title Alert -message "$type $id $hostid unknown."
        $frame configure -relief raised
	return
      }
      if {[lsearch -exact {running pending connected stalled} $state] > -1} {
        after 0 [list AddServerCommand relay $host $port "$type stop $id" "" 1000]
puts "ADDING : $type stop $id for $host $port"
#PMM3
#        puts $relay(iohandle,$hostid) "$type stop $id"
#        gets $relay(iohandle,$hostid) line
#        if {[string match "*error" $line]} {
#          tk_messageBox -title Alert -message "Failed to stop\n$type $id $hostid."
#          $frame configure -relief raised
#          return
#        }
#        GetStates $type $hostid
      } elseif {[lsearch -exact {stopped ready disconnected} $state] > -1} {
        after 0 [list AddServerCommand relay $host $port "$type start $id" "" 1000]
puts "ADDING : $type start $id for $host $port"
#        puts $relay(iohandle,$hostid) "$type start $id"
#        gets $relay(iohandle,$hostid) line
#        if {[string match "*error" $line]} {
#          tk_messageBox -title Alert -message "Failed to start\n$type $id $hostid."
#          $frame configure -relief raised
#          return
#        }
#        GetStates $type $hostid
      } else {
        tk_messageBox -title Alert -message "$type $id $hostid state $state\ncan not start/stop."
        $frame configure -relief raised
      }
    }
  }
} 

proc AddSender {source source_id type type_id} {
  puts ""
  puts "Adding $type $type_id to $source $source_id"
  puts " - $type source $type_id $source $source_id"
  puts ""
}

proc OverviewPlusPress {x y button press menu id hostid} {
  global relay
puts "Button $button press $press Menu count [$menu index end]"
  if {$button == 3 && $press == 1} {
    set entries [$menu index end]
    set i 0
    while {$i <= $entries} {
      set name [$menu entrycget $i -label]
      set type [lindex $name 1]
      set type_id [lindex $name 2]
      $menu entryconfigure $i -command "AddSender receiver $id $type $type_id"
      incr i
    }
    tk_popup $menu $x $y
  }
}

# Create a led button and return the led
proc MakeLedButton { frb color name helptext type id host port } {
  global led_image
  set bg [$frb cget -bg]
  # The frb is a frame used as a button.
  $frb configure -padx 3 -pady -1 -bd 1 -relief raised
  set frbi [Label $frb.img -image $led_image($color,10x12) -bg $bg]
  set frbt [Label $frb.txt -text $name -helptext $helptext -bg $bg]
  if {$id > -1} {
    foreach item "$frb $frbi $frbt" {
      bind $item "<ButtonPress>" "OverviewRecvPress %X %Y %b 1 .relay_recv $frb $type $id $host $port"
      bind $item "<ButtonRelease>" "OverviewRecvPress %X %Y %b 0 .relay_recv $frb $type $id $host $port"
    }
  }
  pack $frbi -padx 2 -side left
  pack $frbt -padx 2 -side left -fill x -expand 1 -anchor nw
  return "$frb $frbi $frbt"
}

proc TogglePack {toggle list} {
  if {[catch {pack info $toggle} res]} { set show 1 } else { set show 0 }
  set showlist ""
  foreach fr $list {
    if {[catch {pack info $fr } res]} { set show 0 } else { set show 1 }
    if {$toggle == $fr} { if {$show} { set show 0 } else { set show 1 } }
    if {$show} { lappend showlist $fr }
  }
  foreach fr $list { pack forget $fr }
  foreach fr $showlist {
    pack $fr -side left -anchor nw -expand 1 -fill y
  }
}

proc CheckPack {recv send graph} {
  if {[catch {pack info $recv} res]} { puts "receiver is not packed" } else { puts "receiver is packed" }
  if {[catch {pack info $send} res]} {
    puts "sender is not packed"
    pack forget $recv $send $graph
    pack $recv $send $graph -side left -anchor nw -expand 1
  } else {
    puts "sender is packed"
    pack forget $send
  }
}

proc ToggleSnowrelayCon { tag host port } {
  global relay

  if {[info exist relay(serverhandle,$tag,$host,$port)]} {
    if {$relay(serverhandle,$tag,$host,$port) == ""} {
      puts stderr "WARNING ToggleSnowrelayCon handle is empty. Ignoring."
      unset -nocomplain relay(serverhandle,$tag,$host,$port)
      ToggleSnowrelayCon $tag $host $port
      return
    }
    puts stderr "CLOSING relay connection"
    AsyncCloseConn $relay(serverhandle,$tag,$host,$port)
    return
  } else {
    # serverhandle does not exist. We assume we have to start up the connection
    ServerConnect $tag $host $port
    if { $tag == "relay" } {
      SetupGettingStatus receiver $host $port
      SetupGettingStatus sender $host $port
    } elseif { $tag == "relay_snowmix" } {
      after 100 [list AddServerCommand snowmix $host $port "feed info" GetFeedStates 1000]
      after 300 [list AddServerCommand snowmix $host $port "system output status" GetOutputState 1000]
    }
  }
}

proc ToggleSnowrelayConOld { host port } {
  global relay led_image
  set color red
PMM3
  if {[info exist relay(iohandle,$host,$port)]} {
    set iohandle $relay(iohandle,$host,$port)
    unset relay(iohandle,$host,$port)
    catch { close $iohandle }
    foreach type "receiver sender" {
      set types [format "%ss" $type]
      foreach id $relay($types,$host,$port) {
        StateChanged $type $id $host,$port "undefined"
      }
    }
  } else {
    ConnectSnowrelay $host $port
    if {[info exist relay(iohandle,$host,$port)]} {
      set color green
    }
  }
  foreach led $relay(server,leds,$host,$port) {
    $led configure -image $led_image($color,10x12)
  }
}

proc AddServerToOverviewPane { server_type host port } {
  global relay led_image system

  if { $server_type != "relay" } {
    puts stderr "AddServerToOverviewPane can only handle server type relay at the moment"
    return
  }
  set pane_id overview
  set panefr $relay(pane,controls,$pane_id)
  set bg [$panefr cget -bg]
  set hostmap [string map {. _} $host]

  # This is the frame to hold one complete server overview
  set subfr [frame $panefr.$hostmap-$port \
	-relief raised -bd 2 -padx 4 -pady 0 -bg $bg]
  pack $subfr -side left -anchor nw -pady 4
  set relay(overview,serverfr,$host,$port) $subfr

  # This is the frame to hold the top buttons for each server/port
  set topfr [frame $subfr.top -padx 0 -pady 0 -bg $bg]

  # This is the frame that hold buttons and canvases for the Snowmix Server
  set feedfr [frame $subfr.feeds -padx 0 -pady 0 -bg $bg]
  pack $topfr $feedfr -side top -anchor nw -pady 1

  # This is relay server related
  if { 1 } {
    set frb [frame $topfr.frb -padx 3 -pady 2 -bg $bg]
    set frbs [MakeLedButton $frb green "Relay $host:$port" \
	"Stop/Start server\ncommunication for\n$host $port" \
	receiver -1 $host $port]
    set fr [lindex $frbs 0]
    bind $fr "<ButtonPress>" "$fr configure -relief sunken ; ToggleSnowrelayCon relay $host $port"
    bind [lindex $frbs 1] "<ButtonPress>" "$fr configure -relief sunken ; ToggleSnowrelayCon relay $host $port"
    bind [lindex $frbs 2] "<ButtonPress>" "$fr configure -relief sunken ; ToggleSnowrelayCon relay $host $port"
    bind $fr "<ButtonRelease>" "$fr configure -relief raised"
    bind [lindex $frbs 1] "<ButtonRelease>" "$fr configure -relief raised"
    bind [lindex $frbs 2] "<ButtonRelease>" "$fr configure -relief raised"
    AppendServerLed $host $port [lindex $frbs 1]

    #Button $topfr.name -text "Server : $host:$port" -pady 0 -relief raised -bd 1 -helptext "Stop/Start server\communication for\n$host $port"
    Label $topfr.recv -text "Recv" -padx 3 -pady 3 -relief raised -bd 1 -bg $bg -helptext "Hide or Show\nReceiver Buttons"
    Label $topfr.send -text "Send" -padx 3 -pady 3 -relief raised -bd 1 -bg $bg -helptext "Hide or Show\nSender Buttons"
    Label $topfr.graph -text "Graph" -padx 3 -pady 3 -relief raised -bd 1 -bg $bg -helptext "Hide or Show\nRecv/Send Graphs"
    pack $frb $topfr.recv $topfr.send $topfr.graph -side left -anchor nw -pady 3

    # This is the frame to hold the receiver buttons
    set recvbfr [frame $subfr.recvb -padx 1 -pady 1 -relief sunken -bd 1 -bg $bg]
    # This is the frame to hold the senders
    set sendbfr [frame $subfr.sendb -pady 1 -relief sunken -bd 1 -bg $bg]
    # This is the frame to hold the graphs
    set graphfr [frame $subfr.graphs -pady 1 -padx 1 -relief sunken -bd 1 -bg $bg]

    set list "$recvbfr $sendbfr $graphfr"
    bind $topfr.recv <ButtonPress> "$topfr.recv configure -relief sunken ; TogglePack $recvbfr \{$list\}"
    bind $topfr.recv <ButtonRelease> "$topfr.recv configure -relief raised"
    bind $topfr.send <ButtonPress> "$topfr.send configure -relief sunken ; TogglePack $sendbfr \{$list\}"
    bind $topfr.send <ButtonRelease> "$topfr.send configure -relief raised"
    bind $topfr.graph <ButtonPress> "$topfr.graph configure -relief sunken ; TogglePack $graphfr \{$list\}"
    bind $topfr.graph <ButtonRelease> "$topfr.graph configure -relief raised"
    #$topfr.recv configure -armcommand "TogglePack $recvbfr \{$list\}"
    #$topfr.send configure -armcommand "TogglePack $sendbfr \{$list\}"
    #$topfr.graph configure -armcommand "TogglePack $graphfr \{$list\}"

    pack $recvbfr $sendbfr $graphfr -side left -anchor nw -expand 1 -fill y
    #pack $recvbfr $sendbfr $graphfr -side left -anchor nw -expand 1 -fill y
    set relay(overview,frame,recvbfr,$host,$port) $recvbfr
    set relay(overview,frame,sendbfr,$host,$port) $sendbfr
    set relay(overview,frame,graphfr,$host,$port) $graphfr

  }

  # This is Snowmix server related
  if { 1 } {
    # These are frames within the feed frame
    set feedserv [frame $feedfr.server -padx 3 -pady 1 -bg $bg -relief raised -bd 1]
    set feedbuts [frame $feedfr.buttons -padx 3 -pady 2 -bg $bg -relief sunken -bd 1]
    set feedcanvas [canvas $feedfr.canvas -bg black -height 20 -width 0 -bd 0]
    DynamicHelp::add $feedfr.canvas -text "Feed FIFOs\nReady Frames."
    set outcanvas [canvas $feedfr.outcanvas -bg black -height 20 -width 20 -bd 0]
    $outcanvas create line 0 20 25 20 -fill lightgrey -width 1

    set graph_id_output output-$host,$port
    set outputgfr [frame $feedfr.output -padx 0 -pady 0 -border 0 -bg $bg]
    set outputgraph [CreateGraph $outputgfr $graph_id_output 50 20 11 4 0]
    bind $outputgfr.canvas <Enter> "$outputgfr.canvas configure -height 40"
    bind $outputgfr.canvas <Leave> "$outputgfr.canvas configure -height 20"
    DynamicHelp::add $outputgfr.canvas -text "Output Frame Priods\nYellow = Minimum\nCyan = Average\nOrange = Maximum"
    set snowmix_port 9999
    CreateLine $graph_id_output 1 "$host $snowmix_port min" yellow 22 2
    CreateLine $graph_id_output 2 "$host $snowmix_port ave" cyan 22 2
    CreateLine $graph_id_output 3 "$host $snowmix_port max" orange 22 2
    SetScaleLine $graph_id_output 1 1 1.8
    SetScaleLine $graph_id_output 2 1 1.8
    SetScaleLine $graph_id_output 3 1 1.8
    SetOffsetLine $graph_id_output 1 -5 -53
    SetOffsetLine $graph_id_output 2 -5 -53
    SetOffsetLine $graph_id_output 3 -5 -53

    if {$system(window_system) == "aqua"} {
      GraphLineWidth $graph_id_output 1 2
      GraphLineWidth $graph_id_output 2 3
      GraphLineWidth $graph_id_output 3 2
    } else {
      GraphLineWidth $graph_id_output 1 1
      GraphLineWidth $graph_id_output 2 2
      GraphLineWidth $graph_id_output 3 1
    }

    pack $feedserv $feedcanvas $feedbuts $outcanvas $feedfr.output $outputgraph -side left 
    set relay(feedframe,$host,$snowmix_port) $feedfr
    set relay(feedservfr,$host,$snowmix_port) $feedserv
    set relay(feedcanvas,$host,$snowmix_port) $feedcanvas
    set relay(graph_id_output,$host,$snowmix_port) $graph_id_output
    set relay(outcanvas,$host,$snowmix_port) $outcanvas
    set relay(output_canvas,$host,$snowmix_port) $outputgfr.canvas
    set relay(output_canvas,x,$host,$snowmix_port) 1
    set relay(feedbutsfr,$host,$snowmix_port) $feedbuts

    set color darkgrey
    Label $feedserv.img -image $led_image($color,10x12) -padx 2 -bg $bg
    Label $feedserv.label -text "Snowmix $host:$snowmix_port" -padx 2 -pady 1 -bg $bg
    pack $feedserv.img $feedserv.label -side left
    AppendServerLed $host $snowmix_port $feedserv.img

    bind $feedserv "<ButtonPress>" "$feedserv configure -relief sunken ; ToggleSnowrelayCon relay_snowmix $host $snowmix_port"
    bind $feedserv.img "<ButtonPress>" "$feedserv configure -relief sunken ; ToggleSnowrelayCon relay_snowmix $host $snowmix_port"
    bind $feedserv.label "<ButtonPress>" "$feedserv configure -relief sunken ; ToggleSnowrelayCon relay_snowmix $host $snowmix_port"
    bind $feedserv "<ButtonRelease>" "$feedserv configure -relief raised"
    bind $feedserv.img "<ButtonRelease>" "$feedserv configure -relief raised"
    bind $feedserv.label "<ButtonRelease>" "$feedserv configure -relief raised"
  }
}

proc SetLostOffset { host port id } {
  global relay
  set relay(rtpstatus,vars,lost,offset,$id,$host,$port) $relay(rtpstatus,vars,lost,$id,$host,$port)
  set relay(rtpstatus,vars,lost,delta,$id,$host,$port) 0
  return
}

proc AddToOverviewPane { type host port } {
  global relay led_image system
  if {![info exist relay(servers)]} {
    puts "No servers"
    return
  }

  set keyrecv "receivers,$host,$port"
  set keysend "senders,$host,$port"
  set hostmap [string map {. _} $host]
  set bg [$relay(pane,controls,$type) cget -bg]
  # Menu for adding senders to a receiver
  menu .adds$hostmap -tearoff 0
  foreach idsend $relay($keysend) {
	.adds$hostmap add command -label "Add sender $idsend" \
	-command "AddSender" 
  }

  set recvbfr $relay(overview,frame,recvbfr,$host,$port)
  set sendbfr $relay(overview,frame,sendbfr,$host,$port)
  set graphfr $relay(overview,frame,graphfr,$host,$port)

set graph_id_recv recv-$host,$port
set graph_id_send send-$host,$port
  set recvgfr [frame $graphfr.recv -padx 0 -pady 0 -border 0 -bg $bg]
  set sendgfr [frame $graphfr.send -padx 0 -pady 0 -border 0 -bg $bg]
  label $graphfr.recvlabel -text Receivers -pady 2 -relief raised -bd 1 -bg $bg
  label $graphfr.sendlabel -text Senders -pady 2 -relief raised -bd 1 -bg $bg

  set ypixels 120
  set ylines 7
  set recvgraph [CreateGraph $recvgfr $graph_id_recv 160 $ypixels 10 7 3]
  set sendgraph [CreateGraph $sendgfr $graph_id_send 160 $ypixels 10 7 3]
  pack $graphfr.recvlabel $recvgfr $graphfr.sendlabel $sendgfr $recvgraph $sendgraph \
	-side top -fill x -expand 0 -anchor nw
  set relay(receiver,feed_x,$host,$port) 0
  set relay(sender,feed_x,$host,$port) 0

#set colors "white yellow red lightgreen blue white grey lightblue magenta cyan bisque orange plum gold lightblue yellow2 coral pink wheat yellow2 maroon violet bisque purple"
  set recvcolor 1
  set sendcolor 1
  foreach id $relay($keyrecv) {
    set color [lindex $system(colors) $recvcolor]
    if {$color == ""} { set color yellow }
    CreateLine $graph_id_recv $id "Receiver $id" $color 70 2
    SetScaleLine $graph_id_recv $id 1 [expr 0.00000100 * $ypixels / $ylines]
    if { $recvcolor == 1 } { LineLegendY $graph_id_recv $id }
    incr recvcolor

    set state $relay(receiver,$id,state,$host,$port)
    # The frb is a frame used as a button
    set frb [frame $recvbfr.recvb$id -bg $bg]
    bind $frb <Enter> "GraphLineWidth $graph_id_recv $id 4"
    bind $frb <Leave> "GraphLineWidth $graph_id_recv $id 1"
    # MakeLedButton { frb color name helptext type id host port }
    set frbs [MakeLedButton $frb $relay(statecolor,$state) \
	$relay(receiver,$id,name,$host,$port) \
	"Receiver $id\nState: $state\nPort: $relay(receiver,$id,port,$host,$port)" \
	receiver $id $host $port]
    lappend relay(receiver,$id,leds,$host,$port) [lindex $frbs 1]
    set relay(receiver,$id,text,$host,$port) $frb.txt
    pack $frb -side top -anchor nw -padx 2 -pady 1 -expand 0 -fill x
#puts "PMM $host $port $id"
    if {![info exist relay(rtpstatus,vars,lost,delta,$id,$host,$port)]} {
      set relay(rtpstatus,vars,lost,delta,$id,$host,$port) 0
    }
    if {![info exist relay(rtpstatus,vars,lost,$id,$host,$port)]} {
      set relay(rtpstatus,vars,lost,$id,$host,$port) 0
    }
    set rtplbl [Label $frb.lost$id -bg $bg -relief sunken -bd 1 -textvariable relay(rtpstatus,vars,lost,delta,$id,$host,$port) -helptext "RTP Info\nLost packets\nClick to reset."]
    bind $rtplbl <ButtonPress> "SetLostOffset $host $port $id ; $rtplbl configure -bg $bg"
    pack $rtplbl -side left -padx 0 -pady 0 -expand 0 -fill x
    set relay(receiver,$id,lostlabel,$host,$port) $rtplbl

    # This is the frame to hold one servers senders for one receiver
    set frrecvsend [frame $sendbfr.recv$id -bg $bg]
    pack $frrecvsend -side top -anchor nw -padx 2 -pady 1 -fill x -expand 0

    # The plus button to add senders to a receiver
#      Button $frrecvsend.plus$id -text "+" -padx 4 -pady 1 -bd 2 -relief raised -bg $bg
#      bind $frrecvsend.plus$id "<ButtonPress>" \
#	"OverviewPlusPress %X %Y %b 1 .adds$hostmap $id $host,$port"
#      bind $frrecvsend.plus$id "<ButtonRelease>" \
#	"OverviewPlusPress %X %Y %b 0 .adds$hostmap $id $host,$port"
#      pack $frrecvsend.plus$id -side left -anchor nw -padx 2 -fill x

    foreach idsend $relay($keysend) {
      if {$relay(sender,$idsend,sourceid,$host,$port) == $id && [string match receiver $relay(sender,$idsend,source,$host,$port)]} {
        set color [lindex $system(colors) $sendcolor]
        if {$color == ""} { set color yellow }
        CreateLine $graph_id_send $idsend "Sender $idsend" $color 70 2
        #SetScaleLine $graph_id_send $idsend 1 [expr 1.667 * 0.0000100 * 1.3]
    SetScaleLine $graph_id_send $idsend 1 [expr 0.00000100 * $ypixels / $ylines]
          if { $sendcolor == 1 } { LineLegendY $graph_id_send $idsend }
          incr sendcolor

        set relief "sunken"
        set state $relay(sender,$idsend,state,$host,$port)

        # The frb is a frame used as a button
        set frb [frame $frrecvsend.$idsend -bg $bg]
        bind $frb <Enter> "GraphLineWidth $graph_id_send $idsend 4"
        bind $frb <Leave> "GraphLineWidth $graph_id_send $idsend 1"
        # MakeLedButton { frb color name helptext type id host port }
        set frbs [MakeLedButton $frb $relay(statecolor,$state) \
		$relay(sender,$idsend,name,$host,$port) \
		"Sender $idsend\nState: $state\nHost: $relay(sender,$idsend,ip,$host,$port)\nPort: $relay(sender,$idsend,port,$host,$port)\nPMM" \
		sender $idsend $host $port]
        pack $frb -side left -anchor nw -padx 2 -fill x -expand 1
        lappend relay(sender,$idsend,leds,$host,$port) [lindex $frbs 1]
        set relay(sender,$idsend,text,$host,$port) $frb.txt
#          Button $frrecvsend.$idsend \
#		-text "$relay(sender,$idsend,name,$host,$port)" \
#		-padx 3 -pady -1 -bd 3 -relief $relief \
#		-bg $relay(statecolor,$state) \
#		-helptext "sender $idsend\nState: $state"
#          pack $frrecvsend.$idsend -side left -anchor nw -padx 2 -fill x -expand 1
      }
    }
  }
}


proc AddServerSetup { host port } {
  global relay
puts "Adding Relay server $host port $port"
  if {[lsearch -exact $relay(servers) "$host $port"] > -1} {
    puts "Server $host port $port already exist. Ignoring"
    return
  }
  AddServerToOverviewPane relay $host $port

  AddServer relay $host $port

  # Connect to Snowmix on the server, if any
  set snowmix_port 9999
  AddServer snowmix $host $snowmix_port

  SetupGettingInfo receiver $host $port
  SetupGettingInfo sender $host $port
  SetupGettingInfo sendersource $host $port

  after 100 [list AddServerCommand snowmix $host $snowmix_port "feed info" GetFeedStates 1000]
  after 300 [list AddServerCommand snowmix $host $snowmix_port "system output status" GetOutputState 1000]

#  if {[ConnectSnowrelay $host $port] == ""} {
#    puts "Failed to connect to Snowswitch $host $port"
#    return
#  }

#  if {[Connect2Snowmix $host $snowmix_port] == ""} {
#    puts "Failed to connect to Snowmix $host $snowmix_port"
#  } else {
#    if {![info exist relay(snowmix_servers)] || [lsearch -exact $relay(snowmix_servers) "$host $snowmix_port"] < 0} {
#      lappend relay(snowmix_servers) "$host $snowmix_port"
#      # Start polling status after 2000 ms
#puts "ORDERING GetFeedStatus in 2 secs"
#      # after 2000 GetFeedStatus $host $snowmix_port
#      OrderGetFeedStatus 2000 $host $snowmix_port
#    }
#  }

  lappend relay(servers) "$host $port"
  # AddToOverviewPane overview $host $port
  $relay(pane) raise overview
puts "done raise $host $port"
}


proc relay_pane { pane notebook page } {
  global relay snowrelay_ip snowrelay_port system

  set relay(notebook) $notebook
  set relay(notebook_page) $page

  set bg [$pane cget -bg]

#  ScrollableFrame $pane.scrollframe -constrainedwidth false -constrainedheight false \
#    -xscrollcommand "$pane.scrollbar set" -height 600
#  scrollbar $pane.scrollbar -command "$pane.scrollframe xview" -orient horizontal
#  pack $pane.scrollframe -side top -fill both -expand 1
#  pack $pane.scrollbar -side top -fill x -expand 1
#  set panefr [$pane.scrollframe getframe]

  # Re-packing pane frame
  set panefr $pane
  pack $pane -expand 1 -fill both

puts "Relay pane $pane : [pack info $pane]"
puts " - [$pane cget -width]"
#puts "Relay pane .nb : [pack info .nb]"
#puts " - [.nb cget -width -height]"

  set rp [NoteBook $panefr.nb -side top -arcradius 4 -homogeneous 1 -tabbevelsize 2 -bg $bg]
  pack $rp -side top -fill both -anchor w -expand 1
  set relay(pane) $rp

  $rp insert end overview -text " Overview"
  $rp insert end receiver -text " Receivers"
  $rp insert end sender -text " Senders"
  $rp insert end analyzer -text " Analyzers"

  CreateSubPane $relay(pane) overview
  CreateSubPane $relay(pane) receiver
  CreateSubPane $relay(pane) sender
  CreateSubPane $relay(pane) analyzer

  set relay(servers) ""
  set servers ""
#  set servers "localhost stream01"
#  set servers "vostok01 vostok02 stream01"
#  set servers "vostok01 vostok02"
#  set servers "vostok02"
#  set servers "vostok01"
#  set servers "vostok01 vostok02 stream01"
#  set servers "localhost"
#  set servers "192.168.3.100"

  foreach server $servers {
    puts "Adding $server $snowrelay_port"
    AddServerSetup $server $snowrelay_port
  }

puts raise
  $relay(pane) raise overview
puts done
}
