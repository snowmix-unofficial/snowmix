#
# Copyright Peter Maersk-Moller (C) 2012-2016
# All Rights Reserved.
#

##!/bin/sh
## \
#if [ -x /usr/local/bin/wish ] ; then exec /usr/local/bin/wish "$0" ${1+"$@"} ; else if [ -x /usr/local/bin/wish ] ; then exec /usr/bin/wish "$0" ${1+"$@"} ; else exec /usr/bin/wish "$0" ${1+"$@"} ; fi ; fi
##

# Functions :
# - OpenSocket4Line { tag host port block } : Open a socket for line reading.
# - CloseSocket { handle } : Close a socket opened with OpenSocket4Line.
# - GetSocketStatus { handle } : Returns state for socket.
# - GetSocketStatus { tag host port } : Returns state for socket.
# - GetSocketFirstLine { handle } : Returns first read line
# - GetSocketKey { handle } : Returns key for handle if any
#
# Socket Status
# - UNDEFINED : Socket does not exist
# - PENDING : Socket has been opened, but first line has not been read yet.
# - RUNNING : Opened and first line has been read
# - DISCONNECTED : Socket has been closed.
#

#proc bgerror { message } {
#  puts "BGERROR : $message"
#}
# Opens a socket for line reading
# - tag : Only one tag,host,port can exist
# - host : Remote IP address or host name
# - port : Remote port
# - block : 1=blocking, 0=nonblocking
# Sets iohandle(status,$tag,$host,$port). This never gets unset
proc OpenSocket4Line { tag host port block } {
  global iohandle
  set verbose 0
  if {$verbose} { puts stderr "OPENSOCKET4LINE" }
  set status [GetSocketStatus $tag $host $port]
  if { $status != "UNDEFINED" && $status != "DISCONNECTED" } {
    puts stderr "WARNING. Can not OpenSocket4Line $tag ahost $port because it is already in use. Status = $status"
    return
  }
  set key "$tag,$host,$port"
  if {$verbose} { puts "Connecting to $host $port" }
  set handle ""
  set message "Opening $host $port"
  set iohandle(status,$key) PENDING
  if { $verbose } { puts stderr "Setting state for iohandle(status,$key) to PENDING" }
  if {[catch {
    if { $block } {
      set handle [socket $host $port]
      fconfigure $handle -blocking 1 -buffering line
      set iohandle(status,$key) RUNNING
    } else {
      set handle [socket -async $host $port]
      fconfigure $handle -blocking 0 -buffering line
    }
#puts stderr "SETTING key for iohandle($handle,key) $key"
    set iohandle($handle,key) $key
    set iohandle($handle,host) $host
    set iohandle($handle,port) $port
    set iohandle($handle,tag) $tag
    set iohandle($handle,verbose) $verbose
    if {!$block} {
      set iohandle($handle,after) [after 0 "ReadFirstLine $handle"]
    }
  } res]} {
    if { $verbose } { puts "Failed opening socket $host $port <$res>" }
    if { $handle != "" } {
      CloseSocket $handle
    } else {
      set iohandle(status,$key) DISCONNECTED
    }
    set handle ""
  } else {
    if { $verbose } {
      if { $block } {
        puts "Connected to $host $port."
      } else {
        puts "Connecting to $host $port nonblocking."
      }
    }
  }
  return $handle
}

# Reads the first line of a asynchronously nonblocked connection
# and place result in iohandle($handle,firstline).
# Socket status gets sets to RUNNING if succeslfull.
proc ReadFirstLine { handle } {
  global iohandle

#if {$iohandle($handle,verbose)} { puts stderr "READFIRSTLINE $handle" }

  if {$iohandle($handle,verbose) > 0} { puts "ReadFirstLine for $iohandle($handle,key)" }
  # Check that this procedure was called with the after command
  if {![info exist iohandle($handle,after)]} {
    puts "WARNING: ReadFirstLine iohandle($handle,after) does not exist."
    return
  }
  unset iohandle($handle,after)
  set tag $iohandle($handle,tag)
  set host $iohandle($handle,host)
  set port $iohandle($handle,port)
  set key $iohandle($handle,key)

  # Check for timeout
  if {[info exist iohandle($handle,timeout)]} {
    set timenow [clock clicks -milliseconds]
    if { $timenow >= $iohandle($handle,timeout) } {
      if {$iohandle($handle,verbose)} {
        puts stderr "ReadFirstLine for $key timeout. Closing socket"
        if {[info exist iohandle($handle,timeout_proc)] && [info procs $iohandle($handle,timeout_proc)] != ""} {
          puts stderr "ReadFirstLine has a timeout_proc $iohandle($handle,timeout_proc)"
        }
      }
      # CloseSocket $handle
      catch { unset -nocomplain iohandle($handle,firstline) }
      # We had time_out on order an asynchronous connection setup
      if {[info exist iohandle($handle,timeout_proc)] && [info procs $iohandle($handle,timeout_proc)] != ""} {
        set timeout_proc $iohandle($handle,timeout_proc)
        # The timeout_proc is only used once
        unset -nocomplain iohandle($handle,timeout_proc)
        if {[catch { $timeout_proc $handle $tag $host $port } res]} {
          puts stderr "timeout proc failed for $key"
        }
      } else {
        if {$iohandle($handle,verbose)} {
          puts stderr "ReadFirstLine for $key timeout had no valid timeout_proc."
        }
      }
      CloseSocket $handle
      return
    }
  }

  # Reading a line will either return n<0 for fail, n==0 for not ready or n>0 for success
  if {[catch { set nread [gets $handle iohandle($handle,firstline)] } res]} {
    if {$iohandle($handle,verbose)} {
      puts stderr " - Failed to read $handle <$res> [GetSocketStatus $handle]"
    }
    # CloseSocket $handle
    catch { unset -nocomplain iohandle($handle,firstline) }
    if {[info exist iohandle($handle,fail_proc)] && [info procs $iohandle($handle,fail_proc)] != ""} {
      set fail_proc $iohandle($handle,fail_proc)
      # We will only use this proc once
      unset -nocomplain iohandle($handle,fail_proc)
      if {[catch { $fail_proc $handle $tag $host $port } res]} {
        puts stderr "fail proc failed for $key"
      }
    }
    CloseSocket $handle
    return
  }

  # We read the first line and will change state of socket
  if {$nread > 0} {
    if {$iohandle($handle,verbose)} {
      puts stderr " - Got first line for $iohandle($handle,host) $iohandle($handle,port)"
    }
    SetSocketStatus $handle RUNNING
    if {[info exist iohandle($handle,async_conn_proc)] && [info procs $iohandle($handle,async_conn_proc)] != ""} {
      set conn_proc $iohandle($handle,async_conn_proc)
if {$iohandle($handle,verbose)} { puts stderr "FIRSTLINE has a async_conn_proc $conn_proc" }
      # We will only use this connection once
      unset -nocomplain iohandle($handle,async_conn_proc)
    } elseif {[info exist iohandle($handle,conn_proc)] && [info procs $iohandle($handle,conn_proc)] != ""} {
if {$iohandle($handle,verbose)} { puts stderr "FIRSTLINE has a conn_proc $conn_proc" }
      set conn_proc $iohandle($handle,conn_proc)
      # We will only use this connection once
      unset -nocomplain iohandle($handle,conn_proc)
    } else { set conn_proc = "" }
    if { $conn_proc != "" } {
if {$iohandle($handle,verbose)} { puts stderr "FIRSTLINE execute conn_proc $conn_proc" }
      if {[catch { $conn_proc $handle $tag $host $port } res]} {
if {$iohandle($handle,verbose)} { puts stderr "HANDLE <$handle>" }
        puts stderr "conn proc <$conn_proc> failed for $key : $res"
      }
    }
    return
  }

  if {!$nread} {
    # nread read 0 bytes
    if {$iohandle($handle,verbose) > 1} {
      puts " - wait on $handle [GetSocketStatus $handle]"
    }
    set iohandle($handle,after) [after 500 "ReadFirstLine $handle"]
    if {$iohandle($handle,verbose)} { puts stderr "ReadFirstLine: Not ready yet. Order new one." }
  } else {
    # nread < 0
    set isblocked [fblocked $handle]
    if {$iohandle($handle,verbose) > 1} {
      puts " - wait error $nread fblocked $isblocked [GetSocketStatus $handle]"
    }
    if { $isblocked } {
      set iohandle($handle,after) [after 500 "ReadFirstLine $handle"]
      if {$iohandle($handle,verbose)} { puts stderr "ReadFirstLine: Is blocked and not ready yet. Order new one." }
    } else {
      puts " - Giving up on $handle"
    }
  }
}

# Returns first read line of "handle" if any
proc GetSocketFirstLine { handle } {
  global iohandle
  if {[info exist iohandle($handle,firstline)]} {
    return $iohandle($handle,firstline)
  }
  return ""
}

# Returns key for handle if any
proc GetSocketKey { handle } {
  global iohandle
  if {[info exist iohandle($handle,key)]} {
    return $iohandle($handle,key)
  }
  return ""
}

proc SetSocketStatus { handle status } {
  global iohandle
  if { $iohandle($handle,verbose) } {
    puts "Setting state <$status> for handle $handle"
  }
  if {[info exist iohandle($handle,key)]} {
    set iohandle(status,$iohandle($handle,key)) $status
  } else {
    puts "Failed to set status <$status> for handle $handle"
  }
}

# GetSocketStatus sockethandle
# GetSocketStatus tag host port
proc GetSocketStatus { args } {
  global iohandle
  set val UNDEFINED
  if {[llength $args] == 1} {
    set handle $args
    if {[info exist iohandle($handle,key)] &&
        [info exist iohandle(status,$iohandle($handle,key))]} {
        return $iohandle(status,$iohandle($handle,key))
    }
  } elseif {[llength $args] == 3} {
    set key "[lindex $args 0],[lindex $args 1],[lindex $args 2]"
    if {[info exist iohandle(status,$key)]} {
      return $iohandle(status,$key)
    }
  }
  return UNDEFINED
}

proc CloseSocket { handle } {
  global iohandle
  if {[info exist iohandle($handle,verbose)] && $iohandle($handle,verbose)} { puts "Closing socket $handle" }
  if {[info exist iohandle($handle,after)]} {
    after cancel $iohandle($handle,after)
  }
  catch {
    SetSocketStatus $handle DISCONNECTED
    foreach name { after args key host port tag verbose async_conn_proc conn_proc close_proc fail_proc timeout_proc async_ready } {
      unset -nocomplain iohandle($handle,$name)
    }
    close $handle
  }
}

# Setup an asynchronous tcp connection to a host/port
#  - tag  : only one tag,host,port combination can exist
#  - host : host name or IP address
#  - port : port number
#  - conn_proc : procedure to call when connection has been established
#  - close_proc : procedure to call when connection is closed
proc AsyncSetupConn { tag host port conn_proc close_proc timeout_proc fail_proc timeout } {
  global iohandle
  set status [GetSocketStatus $tag $host $port]
  if { $status != "UNDEFINED" && $status != "DISCONNECTED" } {
    puts stderr "WARNING. Can not OpenSocket4Line $tag ahost $port because it is already in use. Status = $status"
    return
  }
  set handle [OpenSocket4Line $tag $host $port 0]
  if { $handle == "" } {
    # We failed to order an asynchronous connection setup
    if {[info procs $fail_proc] != ""} {
      if {[catch { $fail_proc $handle $tag $host $port } res]} {
        puts stderr "WARNING. AsyncSetupConn failed executing fail_proc $fail_proc : $res"
      }
    }
    return
  }
  set iohandle(handle,$tag,$host,$port) $handle
  set iohandle($handle,conn_proc) $conn_proc
  set iohandle($handle,async_conn_proc) AsyncConnected
  set iohandle($handle,close_proc) $close_proc
  set iohandle($handle,fail_proc) $fail_proc
  set iohandle($handle,timeout_proc) $timeout_proc
  set iohandle($handle,timeout) [expr [clock clicks -milliseconds] + $timeout]
  set iohandle($handle,async_ready) 0
  return $handle
}

proc AsyncCloseConn { handle } {
  global iohandle
  if {[info exist iohandle($handle,close_proc)]} {
    set close_proc $iohandle($handle,close_proc)
  } else { set close_proc "" }
  if {[info exist iohandle($handle,tag)]} { set tag $iohandle($handle,tag) } else { set tag "" }
  if {[info exist iohandle($handle,host)]} { set host $iohandle($handle,host) } else { set host "" }
  if {[info exist iohandle($handle,tag)]} { set port $iohandle($handle,port) } else { set port "" }
  puts stderr "AsyncCloseConn was called on handle $handle $tag $host:$port"
  # CloseSocket $handle
  if {[info exist iohandle(handle,$tag,$host,$port)]} { unset iohandle(handle,$tag,$host,$port) }
  if { $close_proc != "" } {
    puts stderr "AsyncCloseConn will call $close_proc for $handle $tag $host:$port."
    if {[catch { $close_proc $handle $tag $host $port } res]} {
      puts stderr "Failed to call close_proc $close_proc for $tag $host $port : $res"
    }
  }
  CloseSocket $handle
}

# This functions gets called the first time an async connection has been established and first line received
proc AsyncConnected { handle tag host port } {
  global iohandle
  if {$iohandle($handle,verbose)} {puts "ASSYNCCONNECTED installing async fileevent handler for $tag $host $port." }
  fileevent $handle readable [list AsyncFileEvent $handle $tag $host $port]
  set iohandle($handle,async_ready) 1

  # We can now signal that async is connected
  if {[info exist iohandle($handle,conn_proc)] && [info procs $iohandle($handle,conn_proc)] != ""} {
#puts "ASYNCCONNECTED to execute $iohandle($handle,conn_proc) $handle $tag $host $port"
    if {[catch { $iohandle($handle,conn_proc) $handle $tag $host $port } res]} {
      puts stderr "AsyncConnected: conn proc <$iohandle($handle,conn_proc)> failed for $iohandle($handle,key) : $res"
      AsyncCloseConn $handle
    }
  } else {
    if {$iohandle($handle,verbose)} { puts stderr "No connection proc for AsyncConnected for $iohandle($handle,key)" }
  }
  return
}

proc AsyncFileEvent { handle tag host port } {
  global iohandle

#puts stderr "AsyncFileEvent $handle $tag $host $port"
  # First we check if socket status is running
  set status [GetSocketStatus $handle]
  if { $status != "RUNNING" && $status != "PENDING" } {
    puts stderr " - Got fileevent on socket not ready for $tag $host $port"
    AsyncCloseConn $handle
    return 1
  }

  # Then we check for EOF
  if {[eof $handle]} {
    puts stderr " - AsyncFileEvent got eof on handle $handle $tag $host $port. Closing handle."
    AsyncCloseConn $handle
    return 0
  }

  # The we try read the line
  if {[catch { set nread [gets $handle  line] } res ]} {
    puts stderr "Failed to get data on async fileevent for $handle $tag $host $port. Closing : $res"
    AsyncCloseConn $handle
    return 0
  }

  # Then we see if we read anything and call the proc for that
  if {$nread > 0} {
    # Check if we have a function to give the line read
    if {[info exist iohandle($handle,line_proc)] && [info procs $iohandle($handle,line_proc)] != ""} {
      if { $iohandle($handle,args) != "" } {
        set res [$iohandle($handle,line_proc) $handle $tag $host $port $line $iohandle($handle,args)]
      } else {
        set res [$iohandle($handle,line_proc) $handle $tag $host $port $line]
      }
      # If the function returns 0, we are told it needs no more lines and we set the async_ready flag
      if { $res == 0 } { set iohandle($handle,async_ready) 1 }
    } else {
      # If no line_proc exist, we just dump the line and set the async_ready flag
      if { $iohandle($handle,verbose) } { puts stderr "Skipping line for $tag,$host,$port : <$line>" }
      set iohandle($handle,async_ready) 1
    }
    if { $iohandle($handle,async_ready) && [info exist iohandle($handle,conn_proc)] && [info procs $iohandle($handle,conn_proc)] != ""} {
      if {[catch { $iohandle($handle,conn_proc) $handle $tag $host $port } res]} {
        puts stderr " - AsyncFileEvent: conn proc <$iohandle($handle,conn_proc)> failed for $iohandle($handle,key) : $res"
        AsyncCloseConn $handle
      }
    }
    return 0
  }

  # Gets returned zero bytes or less. Because we checked for eof, we know it is because a full line is not available.
  # It is safe to ignore.
  if { $iohandle($handle,verbose) } { puts stderr "AsyncFileEvent read zero or less bytes on $tag $host $port. Ignoring." }
  return 0
}

proc AsyncNotReady { handle } {
  global iohandle
  if { $handle == "" || ![info exist iohandle($handle,async_ready)] || $iohandle($handle,async_ready) == 0 } { return 1 }
  return 0
}

# If connection is established and sending command is successfull
proc AsyncSendCommand { tag host port command line_proc commandfail_proc command_timeout args } {
  global iohandle
#  puts "AsyncSendCommand $command to $host $port. Returns to <$line_proc>"
  set handle $iohandle(handle,$tag,$host,$port)
  set status [GetSocketStatus $tag $host $port]
  if {[AsyncNotReady $handle]} {
    if {$iohandle($handle,verbose)} { puts stderr "WARNING. AsyncSendCommand not ready. <$command>" }
    return 1
  }
  if { $status != "RUNNING" } {
    puts stderr "AsyncSendCommand was called to send the command <$command> but the status was $status. Closing socket if open."
    AsyncCloseConn $handle
    if { $commandfail_proc != "" && [info procs $commandfail_proc] != ""} {
      $commandfail_proc $handle $tag $host $port
    }
    return 1
  }
  set iohandle($handle,line_proc) $line_proc
  set iohandle($handle,commandfail_proc) $commandfail_proc
  set iohandle($handle,async_ready) 0
  set iohandle($handle,args) [lindex $args 0]
#if {$args != "" } {
#puts "ASYNC args <$args> : $command"
#}
  puts $handle $command
  return 0
}
