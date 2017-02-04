#
# Copyright Peter Maersk-Moller (C) 2014-2015
# All rigths reserved
#

source [file join $env(SNOWMIX) tcl connection.inc]

#  - type : snowmix or relay
#  - host : hostname or IP address
#  - port : server port number
proc AddServer { type host port } {
  global comms
  set servers [format "%s_servers" $type]
  if {[info exist comms($servers)] && [lsearch -exact $comms($servers) "$host $port"] > -1} {
    puts stderr "$servers server $host $port was already added. Ignoring and skipping it."
    return
  }
  if { $type == "snowmix" } {
    set tag snowmix
  } elseif { $type == "relay" } {
    set tag relay
  } elseif { $type == "snowaudio" } {
    set tag snowaudio
  } else {
    puts "WARNING. Unknown type $type for AddServer. Ignoring that it is unknown."
    #return
    set tag $type
  }
  set handle [ServerConnect $tag $host $port]
  if { $handle == "" } {
    puts stderr "ServerConnect returned empty handle trying to connect to $host:$port"
    return
  }
  lappend comms($servers) "$host $port"
  puts stderr "Adding Server $host:$port. Server list = $servers."
  return $handle
}

# ServerConnects gets called to setup communication with server
proc ServerConnect { tag host port } {
  global comms
  if { [info procs SetServerLed] != "" } { SetServerLed $host $port orange } else {
    puts stderr "Warning: NO SetServerLed function for ServerConnects $tag $host:$port"
  }
  set comms(setserverled,$tag,$host,$port) 1
  set comms(serverready,$tag,$host,$port) 0
  if {[info exist comms(pending,$tag,$host,$port)] && $comms(pending,$tag,$host,$port) != ""} {
    puts stderr "Cancelling for $tag $host $port before start : $comms(pending,$tag,$host,$port)"
    set comms(pending,$tag,$host,$port) ""
  }
  set handle [AsyncSetupConn $tag $host $port ServerReady ServerClosed ServerTimeout ServerFail 5000]

  if { $handle == "" } {
    puts stderr "server $tag $host $port could not be connected. Skipping server."
    ServerFail "" $tag $host $port
    return
  }
  set comms(serverhandle,$tag,$host,$port) $handle
  if {$tag == "relay"} {
    # Do some snowrelay stuff
  } elseif { $tag == "snowmix" } {
    # Do some snowmix Scene stuff
  } elseif { $tag == "snowaudio" } {
    # Do some Snowmix audio stuff
  }
  return $handle
}

# ServerReady gets called when the connection is established and ready for command
proc ServerReady { handle tag host port } {
  global comms
  if {$comms(setserverled,$tag,$host,$port)} {
    set comms(setserverled,$tag,$host,$port) 0
    if { [info procs SetServerLed] != "" } { SetServerLed $host $port green } else {
      puts stderr "Warning: No SetServerLed function for ServerReady $tag $host:$port"
    }
  }
  set comms(serverready,$tag,$host,$port) 1
  set res ""
  catch {
    if {[info exist comms(pending,$tag,$host,$port)]} {
      set command [lindex $comms(pending,$tag,$host,$port) 0]
      if { $command != "" } {
        set comms(pending,$tag,$host,$port) [lreplace $comms(pending,$tag,$host,$port) 0 0]
        set res [eval $command]
        set comms(serverready,$tag,$host,$port) 0
      }
    }
  } res
#  puts "Server connected $tag $host $port done : $res"
  return 0
}

# ServerClosed gets called when server is closing its connection
proc ServerClosed { handle tag host port } {
  global comms
puts stderr "Server $tag $host $port closed."
  unset -nocomplain comms(serverhandle,$tag,$host,$port)
  set comms(serverready,$tag,$host,$port) 0
  ServerNotConnected $tag $host $port
}

proc ServerTimeout { handle tag host port } {
  global comms
puts stderr "Snowmix $tag $host $port Timeout."
  unset -nocomplain comms(serverhandle,$tag,$host,$port)
  set comms(serverready,$tag,$host,$port) 0
  ServerNotConnected $tag $host $port
}

proc ServerFail { handle tag host port } {
  global comms
puts stderr "Server $tag $host $port Fail."
  unset -nocomplain comms(serverhandle,$tag,$host,$port)
  set comms(serverready,$tag,$host,$port) 0
  ServerNotConnected $tag $host $port
}

proc ServerNotConnected { tag host port } {
  global comms

  if { [info procs SetServerLed] != "" } { SetServerLed $host $port darkgrey } else {
    puts stderr "Warning: No SetServerLed function for ServerNotConnected $tag $host:$port"
  }
  set comms(setserverled,$tag,$host,$port) 1
  RegisterServerNotConnected $tag $host $port
  if {[info exist comms(pending,$tag,$host,$port)] && $comms(pending,$tag,$host,$port) != ""} {
    puts stderr "Cancelling for $tag $host $port : $comms(pending,$tag,$host,$port)"
    set comms(pending,$tag,$host,$port) ""
  }
}

proc RegisterServerNotConnected { tag host port } {
  if { $tag == "relay" } {
    if {[info procs ServerNotConnectedRelay] != "" } {
      ServerNotConnectedRelay $tag $host $port
    } else {
      puts stderr "No funtion to call to register ServerNotConnected for $tag"
    }
  } elseif { $tag == "snowmix" } {
    if {[info procs ServerNotConnectedSnowmix] != "" } {
      ServerNotConnectedSnowmix $tag $host $port
    } else {
      puts stderr "No funtion to call to register ServerNotConnected for $tag"
    }
  } elseif { $tag == "snowaudio" } {
    if {[info procs ServerNotConnectedAudio] != "" } {
      ServerNotConnectedAudio $tag $host $port
    } else {
      puts stderr "No funtion to call to register ServerNotConnected for $tag"
    }
  } else {
    puts stderr "No funtion to call to register ServerNotConnected for $tag"
  }
  return
}

# AddServerCommand is used to add a command/response to be sent to a server
#  - type : snowmix or relay
#  - host : hostname or IP address
#  - port : server port number
#  - command : The command to send to the server
#  - response_proc : The procedure to read the response. This procedure will return 1 or 0 or -1. 1=Procedure want more input, 0=Procedure want no more input, -1=fail
#  - repeat_delay : 0=Only once n=repeat n ms after response_proc returns 0.
proc AddServerCommand { type host port command response_proc repeat_delay args } {
  global comms

  set argc [llength $args]
  if { $type == "snowmix" } { set tag snowmix
  } elseif { $type == "relay" } { set tag relay
  } elseif { $type == "snowaudio" } { set tag snowaudio
  } elseif { $type == "snowoutput" } { set tag snowoutput
  } else {
    puts stderr "illegal type $type in AddServerCommand. Ignoring."
    return
  }
  set servers [format "%s_servers" $type]
  if {![info exist comms($servers)] || [lsearch -exact $comms($servers) "$host $port"] < 0} {
    puts stderr "Can not add command <$command> to $servers $host $port. Does not exist. Ignoring."
    return
  }
  if { $comms(serverready,$tag,$host,$port) } {
    if {$argc} {
      set res [AsyncSendCommand $tag $host $port $command $response_proc ServerFail 2000 $args]
    } else {
      set res [AsyncSendCommand $tag $host $port $command $response_proc ServerFail 2000]
    }

    if {$res} {
      if {$argc} {
        set command_list "[list AsyncSendCommand $tag $host $port $command $response_proc ServerFail 2000 $args]"
      } else {
        set command_list [list AsyncSendCommand $tag $host $port $command $response_proc ServerFail 2000 $args]
      }
      lappend comms(pending,$tag,$host,$port) $command_list
    } else {
      set comms(serverready,$tag,$host,$port) 0
    }
  } else {
    # We append the command to the pending list for the connection
    if {$argc} {
      set command_list "[list AsyncSendCommand $tag $host $port $command $response_proc ServerFail 2000 $args]"
    } else {
      set command_list [list AsyncSendCommand $tag $host $port $command $response_proc ServerFail 2000]
    }
    lappend comms(pending,$tag,$host,$port) $command_list
  }
}

proc GetOneLine { handle tag host port line } {
  return 0
}
proc GetOneLinePuts { handle tag host port line } {
  puts stderr "LINE $host:$port <$line>"
  return 0
}
