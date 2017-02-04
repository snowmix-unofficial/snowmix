#
# Copyright Peter Maersk-Moller (C) 2012-2016
# All Rights Reserved.
#
proc KeyInput { hello key keycode } {
  global scene
  if {![info exist scene(main_nb)]} {
    puts stderr "No tab created for keyboard input. Ignoring"
    return
  }
  set serverraised [$scene(main_nb) raise]
  puts stderr "KEYBOARD $hello $key $keycode raised tab <$serverraised>"
  if { $serverraised == "" } {
    puts stderr "KEYBOARD : serverraised was empty"
    return
  }
  if {![regexp {(.+):([0-9]+)$} $serverraised match hostmap port]} {
    puts "KEYBOARD : Got no host port config from tab"
  }
  set host $scene(host,$hostmap)
  if {[info exist scene(EntryHasFocus,$host,$port)]} {
    puts "Entry has focus. Skipping key <$key>"
    return
  }
  if { $key == "Return" } {
    if { [info exist scene(Return,$host,$port)] } {
      puts "Skipping return"
      unset scene(Return,$host,$port)
      return
    } else { puts "Not skipping" }
  }
  if {![info exist scene(nb,$host,$port)]} {
    puts stderr "No subtab created for keyboard input. Ignoring"
    return
  }
  set sceneraised [$scene(nb,$host,$port) raise]
  set scene_id ""
  regexp {scene([0-9]+)} $sceneraised match scene_id
  puts "KEYBOARD : Raised tab for $host:$port is $sceneraised <$scene_id> <$match>"
  switch $key {
    space { if { $scene_id != "" } { puts "Raising $scene_id" ; Fade2Scene $scene_id 0 $host $port } }
    a { puts "Got a" }
    0 -
    1 -
    2 -
    3 -
    4 -
    5 -
    6 -
    7 -
    8 -
    9 { catch { $scene(nb,$host,$port) raise scene$key } }
    F1 -
    F2 -
    F3 -
    F4 -
    F5 -
    F6 -
    F7 -
    F8 -
    F9 -
    F10 -
    F11 -
    F12 -
    F13 -
    F14 { set scene_id [string range $key 1 end]
		Fade2Scene $scene_id 0 $host $port
	}
    default { puts "Got default" }
  }
}

proc ServerNotConnectedAudio { tag host port } {
  global audio
  puts "PMM : ServerNotConnectedAudio"
  foreach feed_id $audio(feed,list,$host,$port) {
    set audio(feed,$feed_id,vals,state,$host,$port) UNDEFINED
    set audio(feed,$feed_id,oldvals,state,$host,$port) UNDEFINED
#    set audio(feed,$feed_id,vals,state,$host,$port) UNKNOWN
    SetFeedButColor $host $port $feed_id
  }
}

proc ServerNotConnectedSnowmix { tag host port } {
  global scene led_image system
puts "PMM : ServerNotConnectedSnowmix"
  if { $tag == "snowmix" } {
    if {[info exist scene(feedlist,$host,$port)]} {
      foreach feed_id $scene(feedlist,$host,$port) {
        if {[info exist scene(feed,state,$host,$port,$feed_id)]} {
          set scene(feed,state,$host,$port,$feed_id) "UNDEFINED"
          SetFeedButtonColor $host $port $feed_id
        }
      }
    }
    if {[info exist scene(scenelist,$host,$port)]} {
      foreach scene_id $scene(scenelist,$host,$port) {
        set scene(canvasmotion,scene,$host,$port) ""
        set scene(scene,active,$scene_id,$host,$port) 2
        SetActivateColor $scene(activatebutton,$scene_id,$host,$port) 2
        $scene(nb,$host,$port) itemconfigure scene$scene_id -image $led_image(darkgrey,$system(led_size))
        if {[info exist scene(framelist,$scene_id,$host,$port)]} {
          foreach frame_id $scene(framelist,$scene_id,$host,$port) {
            set scene(active,$scene_id,$frame_id,$host,$port) 5
            SetActivateColor $scene(activatebutton,$scene_id,$frame_id,$host,$port) \
	      $scene(active,$scene_id,$frame_id,$host,$port)
          }
        }
      }
    }
  } else {
    puts stderr "RegisterServerNotConnected unknown tag $tag. Ignoring"
  }
}

proc SetServerLed { host port color } {
  global relay led_image audio system
puts stderr "SetServerLed $host:$port $color"
#  if {![info exist relay(server,leds,$host,$port)]} {
#    puts "SetServerLed warning. Host $host port $port does not exist"
#    return
#  }
  if {![info exist led_image($color,10x12)]} {
    puts stderr "SetServerLed warning. Color $color does not exist for host $host port $port. Using dark grey"
    set color darkgrey
  }
  if {[info exist audio(server,leds,$host,$port)]} {
    foreach led $audio(server,leds,$host,$port) {
      $led configure -image $led_image($color,10x12)
    }
  }

  # See if we have a led on a pane
  if {[info exist audio(main_frame)]} {
    $audio(main_nb) itemconfigure $audio(hostmap,$host):$port -image $led_image($color,$system(led_size))
  }
}

proc AppendServerLed { host port label_item } {
  global audio
  puts "APPENDING SERVER LED : audio(server,leds,$host,$port) $label_item"
  lappend audio(server,leds,$host,$port) $label_item
}

proc SetActivateColor {button active} {
  if {$active > 0} {
    if {$active > 1} {
#puts "SetActivateColor $button $active"
      set bgcolor darkgray
      set activebg grey
   } else {
      set bgcolor #00E400
      set activebg #10F010
    }
  } else {
    set bgcolor #F00000
    set activebg #FF4848
  }
  $button configure -bg $bgcolor -activebackground $activebg
  return
}


# MSG: {5 0 {1 1} {2 1} {3 1} {4 0}}
#       scene_id active {frame_id active} 
proc GetFrameStatus { host port scene_id line } {
  global scene

  set line [lindex [lrange $line 1 end] 0]
  set scene_id [lindex $line 0]
  set scene_active [lindex $line 1]
  set frameset_list [lrange $line 2 end]
  #puts "FRAME scene $scene_id active $scene_active frames <$frameset_list>"
  foreach frameset $frameset_list {
    set frame_id [lindex $frameset 0]
    set frame_active [lindex $frameset 1]
    if {[info exist scene(active,$scene_id,$frame_id,$host,$port)]} {
      if {$scene(active,$scene_id,$frame_id,$host,$port) != $frame_active} {
        set scene(active,$scene_id,$frame_id,$host,$port) $frame_active
        SetActivateColor $scene(activatebutton,$scene_id,$frame_id,$host,$port) \
	  $scene(active,$scene_id,$frame_id,$host,$port)
      }
    }
  }
}
  

proc SetFeedButtonColor { host port feed_id } {
  global scene feed

  if {![info exist scene(feed,state,$host,$port,$feed_id)]} {
    puts "Feed button $host $port does not exist"
    return
  }
  set state $scene(feed,state,$host,$port,$feed_id)
  if {![info exist feed(state_color,$state)]} {
    puts "Color for feed state $state does not exist."
    return
  }
  set bg $feed(state_color,$state)
  if { $bg == "black" || $bg == "darkgrey" || $bg == "darkgren" } { set fg white } else { set fg black }
  set res ""
  catch { $scene(feed,butfr,$host,$port).feed$feed_id configure -bg $feed(state_color,$state) -fg $fg -helptext "Feed $feed_id\n$state" } res
}

proc ToggleServerCon { tag host port } {
  global comms audio
  if {[info exist comms(serverhandle,$tag,$host,$port)]} {
    if {$comms(serverhandle,$tag,$host,$port) == ""} {
      puts stderr "WARNING ToggleServerCon handle is empty. Ignoring."
      unset -nocomplain comms(serverhandle,$tag,$host,$port)
      ToggleServerCon $tag $host $port
      return
    }
    puts stderr "CLOSING Server connection"
    AsyncCloseConn $comms(serverhandle,$tag,$host,$port)
    if { $tag == "snowaudio" } {
      puts "MarkLoopAsStopped $host $port"
      MarkLoopAsStopped $host $port
    }
    return
  } else {
    # serverhandle does not exist. We assume we have to start up the connection
    ServerConnect $tag $host $port
    if { $tag == "relay" } {
      # Do relay stuff
      #SetupGettingStatus receiver $host $port
      #SetupGettingStatus sender $host $port
    } elseif { $tag == "snowmix" } {
      # Do Snowmix Stuff
      #after 100 [list AddServerCommand snowmix $host $port "feed info" GetFeedStates 1000]
      #after 300 [list AddServerCommand snowmix $host $port "system output status" GetOutputState 1000]
      StartSnowmix $host $port
      after 100 [list ServerTabRaised $host $port]
    } elseif { $tag == "snowaudio" } {
#puts stderr "MISSING CODE IN server.tcl ToggleServerCon $tag $host:$port"
      MarkLoopAsStopped $host $port
#puts stderr " - Current tab raised : [$audio(nb,$host,$port) raise]"
      $audio(nb,$host,$port) raise overview
      after 500 [list $audio(nb,$host,$port) raise feed]
#puts stderr " - Current tab raised : [$audio(nb,$host,$port) raise]"
#catch {
#      foreach format_type { info status extended } {
#        puts " - UPDATE audio(update_running,$format_type,$host,$port) $audio(update_running,$format_type,$host,$port)"
#      }
#}
#puts stderr " - Raising feed tab"
#      if {[info exist audio(nb,$host,$port)]} {
#        catch { $audio(nb,$host,$port) raise feed }
#        #$audio(nb,$host,$port) raise feed
#      }
#      after 300 [list ServerTabRaised $host $port]
    } else {
      puts stderr "ToggleServerCon was called with unknown tag $tag. Ignoring"
    }
  }
}

#PMM14
proc ServerTabRaised { host port } {
  global audio
  set raised [$audio(nb,$host,$port) raise]
  set raisecmd [$audio(nb,$host,$port) itemcget $raised -raisecmd]

  puts " - SERVERTAB RAISED : $host:$port = $raised"
  puts " - raisecmd($host:$port) = <$raisecmd>"
  if { $raisecmd != "" } { eval $raisecmd } else {
    puts stderr " - No raisecmd for tab $raised for $host $port"
  }
  focus .
}

proc AudioTabRaised { host port tab } {
  global audio

  puts " - AUDIOTAB RAISED : $tab $host:$port"
  if { $tab != "feed" && $tab != "sink" && ![string match "mixer*" $tab]} return

  if {[regexp {mixer([0-9]+)} $tab match id]} {
    puts " - TAB raised is mixer $id"
    set type mixer
  } else {
    set id all
    set type $tab
  }

  UpdateLoop $host $port $type info $id 1
  UpdateLoop $host $port $type status $id 1
  UpdateLoop $host $port $type extended $id 1

}

proc MarkLoopAsStopped { host port } {
  global audio

  foreach format_type { info status extended } {
    set audio(update_running,$format_type,$host,$port) 0
  }
  return
}

#                 localhost 9999 feed status all 
proc UpdateLoop { host port type format_type id start } {
  global audio

  if {$start && [info exist audio(update_running,$format_type,$host,$port)] && $audio(update_running,$format_type,$host,$port)} {
    puts "Update loop already running an update loop for $format_type"
    return 0
  }

  set raisedtab [$audio(nb,$host,$port) raise]
if ($start) { puts "TAB RAISED for UpdateLoop $raisedtab format_type $format_type" }
  if { $raisedtab != "feed" && $raisedtab != "sink" && ![string match "mixer*" $raisedtab]} {
    set audio(update_running,$format_type,$host,$port) 0
    return
  }

  set audio(update_running,$format_type,$host,$port) 1
  if {[regexp {mixer([0-9]+)} $raisedtab match id]} {
    set type mixer
    set source_format "source $format_type"
#puts "MIXER $id format $format_type : tcl eval snowmix message \[snowmix info audio $type $source_format $id]"
    after $audio(update_rate,$format_type,$host,$port) [list AddServerCommand snowaudio $host $port \
	  "tcl eval snowmix message \[snowmix info audio $type $source_format $id]" \
	  GetAudioStatusLine 1000 $type $source_format $id]
  } else {
    set type $raisedtab
    set id all
  }

  after $audio(update_rate,$format_type,$host,$port) [list AddServerCommand snowaudio $host $port \
	"tcl eval snowmix message \[snowmix info audio $type $format_type $id]" \
	GetAudioStatusLine 1000 $type $format_type $id UpdateLoop]
}


proc AddServerSetup { host port } {
  global audio led_image system
  
  if {[info exist audio(main_frame,$host,$port)]} {
    puts stderr "Can not add server setup for $host $port. Already existing"
    return
  }

  set audio(autoraise,$host,$port) $audio(autoraise)
  set audio(feed,list,$host,$port) ""
  set audio(mixer,list,$host,$port) ""
  set audio(sink,list,$host,$port) ""
  #set audio(mixersource_list,$mixer_id,$host,$port) ""

  # This tells us if an update loop is running. The update loop will restart itself
  set audio(update_running,$host,$port) 0
  set audio(update_rate,$host,$port) $audio(update_rate)
  set audio(update_rate,info,$host,$port) $audio(update_rate,info)
  set audio(update_rate,status,$host,$port) $audio(update_rate,status)
  set audio(update_rate,extended,$host,$port) $audio(update_rate,extended)

  # There can't be any '.' in a tab name
  set hostmap [string map {. _} $host]
  set audio(hostmap,$host) $hostmap
  set audio(host,$hostmap) $host

puts "HOSTMAP audio(hostmap,$host) $audio(hostmap,$host)"
puts "MAPHOST audio(host,$hostmap) $audio(host,$hostmap)"

  # Add tab to main_nb
  $audio(main_nb) insert end "$hostmap:$port" -text " Snowmix $host:$port" -image $led_image(darkgrey,$system(led_size))
  $audio(main_nb) raise "$hostmap:$port"
#  $audio(main_nb) itemconfigure $hostmap:$port" -raisecmd "ServerTabRaised $host $port"

  set bg [$audio(main_nb) cget -bg]

  set serverfr [$audio(main_nb) getframe $hostmap:$port]
  $serverfr configure -bg $bg -relief raised -bd 0 -padx 2 -pady 3

  if { 1 } {
    set serverfr [frame $serverfr.main -padx 0 -pady 0 -border 0 -bg $bg]
    pack $serverfr -side top -anchor nw -fill both -expand 1
    set sw [ScrolledWindow $serverfr.sw -bg $bg]
    #set sff [ScrollableFrame $sw.f -height 1000 -width 12000 -bg $bg -areawidth 0]
    #set sff [ScrollableFrame $sw.f -height 1000 -width 100  -bg $bg]
    set sff [ScrollableFrame $sw.f -height 0 -width 0  -bg $bg]
    $sw setwidget $sff
    pack $sw -side top -fill both -expand 1 -anchor n
    set serverfr [$sff getframe]
  }

  set audio(main_frame,$host,$port) $serverfr
  $audio(main_frame,$host,$port) configure -bg $bg -bd 0
  #pack configure $audio(main_frame,$host,$port) -expand 0

  set top_fr [frame $audio(main_frame,$host,$port).topfr -bd 0 -pady 2 -bg $audio(bgcolor)]
  pack $top_fr -side top -anchor w -fill both

  # Create a NoteBook for Audio Feeds, Mixers and Sinks for this server
  set nb [NoteBook $audio(main_frame,$host,$port).nb -side top -arcradius 4 -homogeneous 1 -tabbevelsize 2 \
	-bg $audio(butbgcolor) -fg $audio(butfgcolor)  -activebackground grey40  -activeforeground black]
  pack $nb -side top -fill both -anchor w -expand 0
  set audio(nb,$host,$port) $nb

  # Add the Overview tab for this server
  $nb insert end overview -text Overview
  set audio(overview,$host,$port) [$nb getframe overview]
  $nb raise overview

  if {1} {
    set fr $audio(overview,$host,$port)
    foreach name { info status extended } {
      set slfr [frame $fr.$name -bd 0 -bg $audio(bgcolor)]
      Label $slfr.lbl -text "$name" -bg $audio(bgcolor) -fg $audio(fgcolor)
      pack $slfr -side left
      set scale_l 150
      set scale_w 20
      set tick 10
      scale $slfr.scale -orient vertical -from 2000 -to 50 -length $scale_l -width $scale_w \
	-tickinterval $tick -showval 1 -troughcolor grey20 -bg $audio(butbgcolor) -fg grey70 \
	-highlightbackground grey40 -highlightcolor green \
	-variable audio(update_rate,$name,$host,$port) \
	-highlightthickness 1 -activebackground grey40  -bd 1 \
	-repeatinterval 10
      AddHoverColor $slfr.scale
      pack $slfr.lbl $slfr.scale -side top
    }
  }

  set o_fr [$nb getframe overview]
  set cen_fr [frame $o_fr.cenfr -bd 0 -bg $bg]
  set bot_fr [frame $o_fr.botfr -bd 0 -bg $bg]
  #pack $top_fr -side top -fill x -expand 1 -anchor n
  pack $cen_fr -side top -fill both -expand 1 -anchor n
  pack $bot_fr -side top -fill x -expand 1 -anchor n
  #$top_fr configure -bg $audio(bg)
#  $cen_fr configure -bg green
#  $bot_fr configure -bg blue

  # Make server Button
  set color darkgrey
  set server_but [frame $top_fr.server -padx 3 -pady 2 -bg $audio(butbgcolor) -relief raised -bd 1]
  set but_fr     [frame $top_fr.buts   -padx 3 -pady 2 -bg $audio(butbgcolor) -relief solid -bd 1]
  pack $server_but $but_fr -side left

  Label $server_but.img -image $led_image($color,$system(led_size)) -padx 1 -bg $audio(butbgcolor) -bd 0
  Label $server_but.label -text "Start/Stop." -padx 2 -pady 1 -bg $audio(butbgcolor) -fg $audio(butfgcolor) -bd 0 -width 10
  pack $server_but.img $server_but.label -side left -padx 3
  AppendServerLed $host $port $server_but.img

  Label $but_fr.label -text "Audio Feeds" -bg $audio(butbgcolor) -fg $audio(butfgcolor) -bd 0 -padx 2
  pack $but_fr.label -side left -anchor s
  set audio(server,but,$host,$port) $server_but
  set audio(feed,butfr,$host,$port) $but_fr

  bind $server_but <ButtonPress> "$server_but configure -relief sunken ; ToggleServerCon snowaudio $host $port"
  bind $server_but.img <ButtonPress> "$server_but configure -relief sunken ; ToggleServerCon snowaudio $host $port"
  bind $server_but.label <ButtonPress> "$server_but configure -relief sunken ; ToggleServerCon snowaudio $host $port"
  bind $server_but <ButtonRelease> "$server_but configure -relief raised"
  bind $server_but.img <ButtonRelease> "$server_but configure -relief raised"
  bind $server_but.label <ButtonRelease> "$server_but configure -relief raised"
  DynamicHelp::add $server_but -text "Click to start/stop\nSnowmix connection\nto $host:$port"

  if { [AddServer snowaudio $host $port] == "" } {
    puts stderr "Failed to add server $host:$port."
#    catch { destroy .dialog }
#    set reply [tk_dialog .dialog "Failed to add server $host:$port." "Failed to add server $host:$port. See console output for more information." questhead 0 Close]
#    destroy .dialog
    return
  }

  after 0 [list AddServerCommand snowaudio $host $port "system frame rate" GetFrameRate 1000]

  foreach type { feed mixer sink } {
    after 0 [list AddServerCommand snowaudio $host $port "audio $type verbose 1" GetOneLine 1000]
  }
  foreach type { feed mixer sink } {
    after 0 [list AddServerCommand snowaudio $host $port "audio $type add" GetAudioNames 1000]
  }
  foreach type { feed sink } {
    foreach format_type { info status extended } {
      after 0 [list AddServerCommand snowaudio $host $port "tcl eval snowmix message \[snowmix info audio $type $format_type format]" GetAudioInfoFormatLine 1000 $type $format_type]
    }
  }
  set type mixer
  foreach format_type { info status extended "source info" "source status" } {
    after 0 [list AddServerCommand snowaudio $host $port "tcl eval snowmix message \[snowmix info audio $type $format_type format]" GetAudioInfoFormatLine 1000 $type $format_type]
  }
  set format_type "source extended"
  after 0 [list AddServerCommand snowaudio $host $port "tcl eval snowmix message \[snowmix info audio $type $format_type format]" GetAudioInfoFormatLine 1000 $type $format_type "" BuildAudioTabs ]
  # StartSnowmix $host $port
  RaiseAndComputeSize $host $port feed
  $audio(main_nb) raise $hostmap:$port
}

# system frame rate
proc GetFrameRate { handle tag host port line } {
  global audio
  set audio(framerate,$host,$port) [lindex $line 5]
  puts "GOT frame rate for $host:$port = $audio(framerate,$host,$port)"
  return 0
}

proc RaiseAndComputeSize { host port page } {
  global audio

  if {[lsearch [$audio(nb,$host,$port) pages] $page] < 0} {
    puts stderr "Waiting for completion of $page tab for server$host:$port."
    after 400 [list RaiseAndComputeSize $host $port $page]
  } else {
    puts stderr "Tab $page for server $host:$port completed."
    after 0000 [list $audio(nb,$host,$port) compute_size]
    after 0000 [list $audio(nb,$host,$port) raise $page ]
  }
  return
}

proc AddServerAudioTab { host port name text } {
  global audio

  if {![info exist audio(nb,$host,$port)]} {
    puts stderr "Can not add Audio Server Tab $name to $host:$port. Entry for Server tab does not exist."
    return
  }
  if {![winfo exist $audio(nb,$host,$port)]} {
    puts stderr "Can not add Audio Server Tab $name to $host:$port. Server tab does not exist."
    return
  }
  set pages [$audio(nb,$host,$port) pages]
  if {[lsearch -exact $pages $name] > -1} {
    puts stderr "Can not add Audio Server Tab $name to $host:$port. It already exists."
    return
  }
  $audio(nb,$host,$port) insert end $name -text $text
  $audio(nb,$host,$port) raise $name
  $audio(nb,$host,$port) itemconfigure $name -raisecmd "AudioTabRaised $host $port $name"

  set fr [$audio(nb,$host,$port) getframe $name]
  $fr configure -bd 1 -padx 5 -pady 2 -relief flat
#  $audio(nb,$host,$port) compute_size
  return
}

proc DelayWithUnit { delay } {
  if { $delay < 1000 } {
    set delay_text "$delay ms"
  } else {
    set delay_text [format "%.3f" [expr $delay / 1000.0]]
    set delay_text "$delay_text s"
  }
  return $delay_text
}

proc UpdateDelay { host port type id format_type value } {
  global audio

  set delay_max 0
  foreach val $value {
    if { $val > $delay_max } { set delay_max $val }
  }
  set delay_text [DelayWithUnit $delay_max]
  if {![info exist audio($type,$id,skip_delay_text,$host,$port)] || !$audio($type,$id,skip_delay_text,$host,$port)} {
    $audio($type,$id,canvas,$host,$port) itemconfigure delay -text $delay_text
  } else {
puts "Skipping $type $id $format_type delay $audio($type,$id,skip_delay_text,$host,$port)"
  }

  if { $type == "mixer" } {
    set mixer_id [lindex [string map {, \ } $id] 0]
    set index "mixer$mixer_id"
  } else { set index $type }

  set delay_max [expr 2*sqrt($delay_max)]
#if {$id == 1 } {puts "AddCoors $audio($type,$id,graph_id,$host,$port) 1 $audio($index,graph_x,$host,$port) $delay_max" }
  AddCoors $audio($type,$id,graph_id,$host,$port) 1 $audio($index,graph_x,$host,$port) $delay_max
  set coords [$audio($type,$id,canvas,$host,$port) coords delaybar]
  set y2 [lindex $coords 3]
  set y1 [expr $y2 - $delay_max]
  set coords [lreplace $coords 1 1 $y1]
  $audio($type,$id,canvas,$host,$port) coords delaybar $coords
  return
}

proc UpdateRMS { host port type id format_type value } {
  global audio

#  puts stderr "RMS CHANGE : $type $id $value"
  set i 0
  set rms_common 0
  set rms_max 0
  foreach val $value {
    set rms_common [expr $rms_common + $val]
    if { $val > $rms_max } { set rms_max $val }
    set val [expr 10*sqrt($val)]
    if { $val < 1 } { set val 1 } elseif { $val > 100 } { set val 100 }
    set coords [$audio($type,$id,canvas,$host,$port) coords rms$i]
    set y2 [lindex $coords 3]
    set y1 [expr $y2 - $val]
    set coords [lreplace $coords 1 1 $y1]
    $audio($type,$id,canvas,$host,$port) coords rms$i $coords
    incr i
  }
  $audio($type,$id,canvas,$host,$port) itemconfigure rmsval -text $rms_max
  if {$i} {set rms_common [expr $rms_common / $i]}

  if { $type == "mixer" } {
    set mixer_id [lindex [string map {, \ } $id] 0]
    set index "mixer$mixer_id"
  } else { set index $type }
  AddCoors $audio($type,$id,graph_id,$host,$port) 2 $audio($index,graph_x,$host,$port) $rms_common
  return
}


proc StatusChange { host port type id format_type valname value } {
  global audio

  if { $valname == "delay" } {
    UpdateDelay $host $port $type $id $format_type $value
  } elseif { $valname == "rms" } {
    UpdateRMS $host $port $type $id $format_type $value
  } elseif { $valname == "state" } {
#puts "Getting audio($type,id,canvas,$host,$port)"
    $audio($type,$id,canvas,$host,$port) itemconfigure state -text $value -fill $audio(feedcolor,$value)
    $audio($type,$id,canvas,$host,$port) itemconfigure stateled -fill $audio(feedcolor,$value)
    #$audio($type,$id,canvas,$host,$port) itemconfigure stateled -fill #10FF10
    if {$type == "feed"} { SetFeedButColor $host $port $id }
  } elseif { $valname == "mute" } {
    if { $value == "muted"} { set color #F00000 } else { set color #00C000 }
    $audio($type,$id,mutelabel,$host,$port) configure -fg $color
    if {$type == "feed"} { SetFeedButColor $host $port $id }
  } elseif { $valname == "volume" } {
    set channel 0
#puts "CHANGE audio($type,$id,vals,volume,$channel,$host,$port) = <$value>"
    foreach volume $value {
      set audio($type,$id,oldvals,volume,$channel,$host,$port) \
        [set audio($type,$id,vals,volume,$channel,$host,$port) [expr round(sqrt(2500.0*$volume))]]
      incr channel
    }
  } elseif { $valname == "clipped" } {
    puts "CLIPPING $type $id $format_type $value"
    if { $value > 75 } { set color #FF2222
    } elseif { $value > 0 } { set color #FF8800 } else { set color #00ff00 }
    set ch 0
    while {$ch < $audio($type,$id,vals,channels,$host,$port)} {
      $audio($type,$id,canvas,$host,$port) itemconfigure rms$ch -fill $color
      incr ch
    }
  }

  return
}

proc SliderChange { host port type id channel value } {
  global audio

  set ch 0
  set volume_list ""
  set source ""
  set volume [expr $value * $value / 2500.0]

  if { $type == "mixer" } {
    set id_list [string map {, \ } $id]
    if {[llength $id_list] > 1} { set source " source" }
  } else { set id_list $id }

  # If channels > 1 and channels are linked
  if { $audio($type,$id,vals,channels,$host,$port) > 1 && $audio($type,$id,linked,$host,$port)} {
    set delta [expr int($value - $audio($type,$id,vals,oldvolume,$channel,$host,$port))]
    set volume_list ""
    set ch 0
    while { $ch < $audio($type,$id,vals,channels,$host,$port) } {
      if {$ch == $channel} { append volume_list "$volume "
      } else {
#puts " - OLDVOL $audio($type,$id,vals,volume,$ch,$host,$port) "
        set new_volume [expr $audio($type,$id,vals,volume,$ch,$host,$port) + $delta]
#puts " - NEWVOL $new_volume"
        if { $new_volume < 0 } { set new_volume 0
        } elseif { $new_volume > 100 } { set new_volume 100 }
	if { $ch != $channel && $audio($type,$id,vals,oldvolume,$channel,$host,$port) == $audio($type,$id,vals,oldvolume,$ch,$host,$port) } {
          set audio($type,$id,vals,volume,$ch,$host,$port) \
            [set audio($type,$id,vals,oldvolume,$ch,$host,$port) $value]
	} else {
          set audio($type,$id,vals,volume,$ch,$host,$port) \
            [set audio($type,$id,vals,oldvolume,$ch,$host,$port) $new_volume]
        }
        set new_volume [expr $new_volume * $new_volume / 2500.0]
        append volume_list "$new_volume "
      }
      incr ch
    }
  } else {
    # Channels are not linked or only one channel
    while { $ch < $channel } { append volume_list "- " ; incr ch}
    append volume_list $volume
  }
#puts "SLIDER $id ch $channel $value volume_list $volume_list"

  # In case we called it directly and not from a scale event.
  set audio($type,$id,vals,volume,$channel,$host,$port) \
    [set audio($type,$id,vals,oldvolume,$channel,$host,$port) $value]

#  puts "SLIDERCHANGE : audio $type$source volume $id (list $id_list) $volume_list"
#  puts "  - $value $audio($type,$id,vals,volume,$channel,$host,$port)"
  after 0 [list AddServerCommand snowaudio $host $port "audio $type$source volume $id_list $volume_list" GetOneLine 1000]
  return
}

proc FadeAudioVolume { host port type id dir } {
  global audio

  set source ""
  if { $type == "mixer" } {
    set id_list [string map {, \ } $id]
    if {[llength $id_list] > 1} { set source " source" }
  } else { set id_list $id }

  set maxvol 0
  set minvol 100
  set ch 0
  while {$ch < $audio($type,$id,vals,channels,$host,$port)} {
    if {$audio($type,$id,vals,volume,$ch,$host,$port) > $maxvol} { set maxvol $audio($type,$id,vals,volume,$ch,$host,$port) }
    if {$audio($type,$id,vals,volume,$ch,$host,$port) < $minvol} { set minvol $audio($type,$id,vals,volume,$ch,$host,$port) }
    incr ch
  }
  set steps [expr int($audio(framerate,$host,$port))]
  if {$dir} {
    # Fade Up
    if { $minvol >= 50 } return
    set delta [expr 50 - $minvol]
    set delta [expr  $delta * $delta / ($steps * 2500.0)]
  } else {
#  puts "GOT frame rate for $host:$port = $audio(framerate,$host,$port)"
    # Fade Down
    set delta [expr  - $maxvol * $maxvol / ($steps * 2500.0)]
    incr steps
  }
  set ch 0
  set vol_list ""
  while {$ch < $audio($type,$id,vals,channels,$host,$port)} {
    append vol_list "$dir "
    incr ch
  }
  set command "command at +2 audio $type$source volume $id_list $vol_list"
  after 0 [list AddServerCommand snowaudio $host $port "$command \naudio $type$source move volume $id_list $delta $steps" GetOneLine 1000]
  return
}

proc ResetAudioVolume { host port type id } {
  global audio

  set link $audio($type,$id,linked,$host,$port)
  set audio($type,$id,linked,$host,$port) 0
  set ch 0
  while { $ch < $audio($type,$id,vals,channels,$host,$port) } {
    SliderChange $host $port $type $id $ch 50
    incr ch
  }
  set audio($type,$id,linked,$host,$port) $link
  return
}

proc ToggleAudioLink { host port type id linkbut } {
  global audio

  if {$audio($type,$id,linked,$host,$port)} { set link 0 ; set text "  Link   " } else { set link 1 ; set text UnLink }
  $linkbut configure -text "$text"
  set audio($type,$id,linked,$host,$port) $link
}

proc ToggleMute { host port type id } {
  global audio

  set source ""
  if { $type == "mixer" } {
    set id_list [string map {, \ } $id]
    if {[llength $id_list] > 1} { set source " source" }
  } else { set id_list $id }
  if { $audio($type,$id,vals,mute,$host,$port) == "muted" } { set mute off } else { set mute on }
  after 0 [list AddServerCommand snowaudio $host $port "audio $type$source mute $mute $id_list" GetOneLine 1000]
  return
}

proc AddSilence { host port type id ms } {
  global audio

  if { $ms >= 0 } { set command "add silence"
  } else { set command "drop"
    set ms [expr -$ms]
  }
  set source ""
  if { $type == "mixer" } {
    set id_list [string map {, \ } $id]
    if {[llength $id_list] > 1} { set source " source" }
  } else { set id_list $id }
  after 0 [list AddServerCommand snowaudio $host $port "audio $type$source $command $id_list $ms" GetOneLinePuts 1000]
  
}

# ####################################################################
# IF type is feed or sink, args should be empty. If type is mixer
# and args is empty, then it is the main audio tab for the mixer.
# If type is a mixer and the tab is for a source, then args should
# hold the source_id as first argument
#
proc BuildAudioTabItem { host port parent_name type id args } {
  global audio led_image system

  set source_id [lindex $args 0]
  set audio_fr [$audio(nb,$host,$port) getframe $parent_name]
  set bg [$audio_fr cget -bg]
  $audio_fr configure -padx 2 -pady 3 -bd 2 -bg $audio(bgcolor)
#  $audio(nb,$host,$port) configure -bg $audio(bgcolor) -fg $audio(fgcolor)
  if { $source_id != "" } { set id "$id,$source_id" }

#  set butbgcolor grey30

  # The frame fr is holding the whole item. The items are packed from left to right
  set fr [frame $audio_fr.$type$id -bd 1 -bg $audio(bgcolor) -relief raised  -padx 4 -pady 1]
  pack $fr -side left -fill both  -expand 0

  #
  set topfr [frame $fr.t -bd 0 -padx 0 -pady 4 -bg $audio(bgcolor)]
  set midfr [frame $fr.m -bd 0 -padx 0 -pady 0 -bg $audio(bgcolor)]
  set botfr [frame $fr.b -bd 0 -padx 0 -pady 0 -bg $audio(bgcolor)]
  pack $topfr $midfr $botfr -side top -fill both -expand 0 -anchor n
  pack $botfr -side top -fill both -expand 1 -anchor n

  #if { $audio($type,$id,vals,mute,$host,$port) == "muted"} { set color red } else { set color #00C000 }
#puts "MUTE $audio($type,$id,vals,mute,$host,$port)"
  if { $source_id != "" } {
    set tab_source $audio($type,$id,vals,source,$host,$port)
    set tab_source_id $audio($type,$id,vals,source_id,$host,$port)
    if {[info exist audio($tab_source,name,$tab_source_id,$host,$port)]} {
      set tabname $audio($tab_source,name,$tab_source_id,$host,$port)
    } else { set tabname NoName }
  } else { set tabname "$id - $audio($type,name,$id,$host,$port)" }
  Label $topfr.mutelabel -text $tabname \
	 -bg $audio(butbgcolor) -fg $audio(fgcolor) -bd 1 -padx 4 -pady 1 -relief raised \
	-helptext "Mute/unmute audio $type $id\nfor $host:$port"
  pack $topfr.mutelabel -side left -fill x -expand 1 -anchor n

  set audio($type,$id,mutelabel,$host,$port) $topfr.mutelabel
  StatusChange $host $port $type $id status mute $audio($type,$id,vals,mute,$host,$port)
  bind $topfr.mutelabel <ButtonPress> "$topfr.mutelabel configure -relief sunken ; ToggleMute $host $port $type $id"
  bind $topfr.mutelabel <ButtonRelease> "$topfr.mutelabel configure -relief raised"
  AddHoverColor $topfr.mutelabel
  

  # m1 and m2 are two top stacked frames in the mid section
  set m1fr [frame $midfr.m1 -bd 0 -bg black -padx 0 -pady 0]
  set m2fr [frame $midfr.m2 -bd 0 -bg black -padx 2 -pady 2]
  pack $m1fr $m2fr -side top -fill x -expand 1

  set canvasfr [frame $m1fr.cvfr -bd 0 -padx 0 -pady 2 -bg black \
    -highlightbackground red -highlightcolor green -highlightthickness 0 ]
  set slide_fr [frame $m1fr.slide -bd 1 -padx 0 -pady 1 -bg black]
  pack $canvasfr $slide_fr -side left -fill both -expand 1

  set cv_bgcolor black
  set cv_bars0 128
  set cv_width 90
  set canvas [canvas $canvasfr.cv -bg $cv_bgcolor -height 148 -width $cv_width -bd 0 \
    -highlightbackground grey10 \
    -highlightcolor green \
    -highlightthickness 0 \
    -insertbackground yellow \
    -insertborderwidth 0 \
    -insertwidth 0 \
    -relief flat \
    -selectbackground darkblue \
    -selectborderwidth 0 \
    -selectforeground lightblue ]
  set audio($type,$id,canvas,$host,$port) $canvas
  set audio($type,$id,cv_bars0,$host,$port) $cv_bars0
  
#set audio($type,$id,vals,state,$host,$port) DISCONNECTED
  $canvas create rect 0 0 [expr $cv_width - 1] 16 -outline grey30 -width 1
  $canvas create oval 4 4 11 11 -tag stateled -outline yellow -fill yellow -width 0
  $canvas create text 14 8  -tag state  -fill grey50 -text $audio($type,$id,vals,state,$host,$port) -anchor w -font mySmallFont 
  $canvas create rect 5 $cv_bars0 18 $cv_bars0 -tag delaybar -fill lightblue -width 0
  $canvas create text 23 140 -tag delay  -fill grey50 -text "000 ms" -font mySmallFont 
  $canvas create text 69 140 -tag rmsval -fill grey50 -text "0.0" -font mySmallFont 
  $canvas create rect  0 131 45 147 -outline grey30 -width 1
  $canvas create rect 49 131 [expr $cv_width - 1] 147 -outline grey30 -width 1
 # $canvas create text 70 7 -tag mute  -fill orange  -text MUTE -font mySmallFont 
  if {$type == "feed" } {
    set delay $audio($type,$id,vals,inidelay,$host,$port)
    set delay_y [expr $cv_bars0 - 2*sqrt($delay)]
    $canvas create text 20 $delay_y -tag inidelay -fill grey50 -text Delay -font mySmallFont -anchor w
    $canvas bind inidelay <ButtonPress> "CanvasDelayPress $host $port $type $id p inidelay %x %y"
    $canvas bind inidelay <ButtonRelease> "CanvasDelayPress $host $port $type $id r inidelay %x %y"
    $canvas bind inidelay <Enter> "$canvas itemconfigure inidelay -fill grey80"
    $canvas bind inidelay <Leave> "$canvas itemconfigure inidelay -fill grey50"
  } elseif { $source_id != "" } {
    set delay $audio($type,$id,vals,min_delay,$host,$port)
    set delay_y [expr $cv_bars0 - 2*sqrt($delay)]
    $canvas create text 20 $delay_y -tag min_delay -fill grey50 -text Min -font mySmallFont -anchor w
    set delay $audio($type,$id,vals,max_delay,$host,$port)
    set delay_y [expr $cv_bars0 - 2*sqrt($delay)]
    $canvas create text 20 $delay_y -tag max_delay -fill grey50 -text Max -font mySmallFont -anchor w
    $canvas bind min_delay <ButtonPress> "CanvasDelayPress $host $port $type $id p min_delay %x %y"
    $canvas bind min_delay <ButtonRelease> "CanvasDelayPress $host $port $type $id r min_delay %x %y"
    $canvas bind min_delay <Enter> "$canvas itemconfigure delaytext -fill grey80"
    $canvas bind min_delay <Leave> "$canvas itemconfigure delaytext -fill grey50"
    $canvas bind max_delay <ButtonPress> "CanvasDelayPress $host $port $type $id p max_delay %x %y"
    $canvas bind max_delay <ButtonRelease> "CanvasDelayPress $host $port $type $id r max_delay %x %y"
    $canvas bind max_delay <Enter> "$canvas itemconfigure delaytext -fill grey80"
    $canvas bind max_delay <Leave> "$canvas itemconfigure delaytext -fill grey50"
  }
  

  pack $canvas -side top -fill none -expand 0

#  puts stderr "$type $id channels : $audio($type,$id,vals,channels,$host,$port)"
  set fup      [frame $slide_fr.up       -bd 0 -padx 0 -pady 0 -bg $cv_bgcolor]
  set fspacer1 [frame $slide_fr.spacer1  -bd 0 -padx 0 -pady 0 -bg $cv_bgcolor -height 2]
  set fslide   [frame $slide_fr.slide    -bd 1 -padx 0 -pady 2 -bg $audio(butbgcolor) -relief raised]
  set fspacer2 [frame $slide_fr.spacer2  -bd 0 -padx 0 -pady 0 -bg $cv_bgcolor -height 2]
  set fdown    [frame $slide_fr.dw       -bd 0 -padx 0 -pady 0 -bg $cv_bgcolor]
  pack $fup $fspacer1 $fslide $fspacer2 $fdown -side top -fill x -expand 0
  Label $fup.up -text U   -bg $audio(butbgcolor) -fg grey70 -relief raised -bd 1 -helptext "Fade volume up to 1"
  Label $fdown.dw -text D -bg $audio(butbgcolor) -fg grey70 -relief raised -bd 1 -helptext "Fade volume down to 0"
  pack $fup.up $fdown.dw -side top -fill x -expand 0
  set fslide2 [frame $fslide.fr -bd 0 -padx 0 -pady 0]
  pack $fslide2 -side top

  bind $fup.up <ButtonPress>   "$fup.up configure -relief sunken ; FadeAudioVolume $host $port $type $id 1"
  bind $fup.up <ButtonRelease> "$fup.up configure -relief raised"
  bind $fdown.dw <ButtonPress>   "$fdown.dw configure -relief sunken ; FadeAudioVolume $host $port $type $id 0"
  bind $fdown.dw <ButtonRelease> "$fdown.dw configure -relief raised"
  AddHoverColor $fup.up
  AddHoverColor $fdown.dw

  set scale_l 100
  set scale_w 10
  set tick 0
  set channel 0
  set x1 50
  set w [expr ($cv_width - $x1 - 1) / $audio($type,$id,vals,channels,$host,$port)]
  if { $w > 12 } { set w 12 }
  set audio($type,$id,linked,$host,$port) 1
  while {$channel < $audio($type,$id,vals,channels,$host,$port)}  {
    set x2 [expr $x1 + $w - 2]
    $canvas create rect $x1 $cv_bars0 $x2 70 -tag rms$channel -fill #00ff00 -width 0
    incr x1 $w
#if {$source_id != ""} { puts "Scale for audio($type,$id,vals,volume,$channel,$host,$port)" }
    scale $fslide2.scale$channel -orient vertical -from 100 -to 0 -length $scale_l -width $scale_w \
	 -tickinterval $tick -showval 0 -troughcolor grey20 -bg $audio(butbgcolor) -fg yellow \
	-highlightbackground grey40 -highlightcolor green \
	-variable audio($type,$id,vals,volume,$channel,$host,$port) \
    	-highlightthickness 1 -activebackground grey40  -bd 1 \
	-repeatinterval 10 \
	-command "SliderChange $host $port $type $id $channel"
    set audio($type,$id,scale,$host,$port) $fslide2.scale
    AddHoverColor $fslide2.scale$channel

    pack $fslide2.scale$channel -side left -anchor n -fill none  -expand 0
    incr channel
  }

  #set graph_id $source-$id-$id2
  set graph_id "$audio(hostmap,$host):$port,$type-$id-"
  
  set graph_1 [CreateGraph $m2fr $graph_id 115 40 10 5 0]
  pack $graph_1 -side top -fill none -expand 0
  set audio($type,$id,graph,$host,$port) $graph_1
  set audio($type,$id,graph_id,$host,$port) $graph_id
  
  CreateLine $graph_id 1 "Delay" lightblue 50 2
  CreateLine $graph_id 2 "IRMS" yellow 50 2
  SetScaleLine $graph_id 1 1 0.4
  SetScaleLine $graph_id 2 2 0.4
  $m2fr.canvas create rect 2 2 32 22  -fill grey10 -outline darkgreen -tag label
  $m2fr.canvas create text 5 8  -fill lightblue -text DELAY -font mySmallFont -anchor w -tag DELAY
  $m2fr.canvas create text 5 17 -fill yellow    -text RMS -font mySmallFont -anchor w -tag RMS
  $m2fr.canvas bind DELAY <Enter> "GraphLineWidth $graph_id 1 3"
  $m2fr.canvas bind DELAY <Leave> "GraphLineWidth $graph_id 1 1"
  $m2fr.canvas bind RMS <Enter> "GraphLineWidth $graph_id 2 3"
  $m2fr.canvas bind RMS <Leave> "GraphLineWidth $graph_id 2 1"
  #set audio_graphs(min,$graph_id,1) 0
  #set audio_graphs(max,$graph_id,1) 0
  #set audio_graphs(min,$graph_id,2) 0
  #set audio_graphs(max,$graph_id,1) 0

  set butfr [frame $botfr.buttons -bd 0 -bg $audio(bgcolor) -padx 0 -pady 0]
  pack $butfr -side top -fill x -expand 0 -anchor n

  set addbut [Label $butfr.add -text "+" -bd 1 -bg $audio(butbgcolor) -fg $audio(butfgcolor) -relief raised -width 2 \
	-helptext "Add 50 ms of silent\nsamples to $type $id"]
  set dropbut [Label $butfr.drop -text "-" -bd 1 -bg $audio(butbgcolor) -fg $audio(butfgcolor) -relief raised -width 2 \
	-helptext "Drop 30 ms of\nsamples for $type $id"]
  bind $addbut <ButtonPress> "$addbut configure -relief sunken ; AddSilence $host $port $type $id 50"
  bind $addbut <ButtonRelease> "$addbut configure -relief raised"
  bind $dropbut <ButtonPress> "$dropbut configure -relief sunken ; AddSilence $host $port $type $id -30"
  bind $dropbut <ButtonRelease> "$dropbut configure -relief raised"
  AddHoverColor $addbut
  AddHoverColor $dropbut

  pack $addbut $dropbut -side left -fill none -expand 0
  set resetbut [Label $butfr.reset -text "Reset " -bd 1 -bg $audio(butbgcolor) -fg $audio(butfgcolor) -relief raised \
	-helptext "Reset volume\nlevel to 1.0"]
  pack $resetbut -side left -fill x -expand 1
  bind $resetbut <ButtonPress>  "$resetbut configure -relief sunken ; ResetAudioVolume $host $port $type $id"
  bind $resetbut <ButtonRelease> "$resetbut configure -relief raised"

  AddHoverColor $resetbut
  if { $audio($type,$id,vals,channels,$host,$port) > 1 } {
    set linkbut [Label $butfr.link  -text UnLink -bd 1 -bg $audio(butbgcolor) -fg $audio(butfgcolor) -relief raised \
	-helptext "Link/Unlink all sliders for $type $id"]
    pack $linkbut -side left -fill x -expand 1
    bind $linkbut <ButtonPress>  "$linkbut configure -relief sunken ; ToggleAudioLink $host $port $type $id $linkbut"
    bind $linkbut <ButtonRelease> "$linkbut configure -relief raised"
    AddHoverColor $linkbut
  }

  set sel_fr [frame $botfr.sel -bd 0 -bg $audio(bgcolor) -padx 0 -pady 0]
  if {[string match "aqua" $system(window_system)]} { set height 160 } else { set height 140 }
  set display_fr [frame $botfr.display -bd 0 -bg $audio(bgcolor) -padx 0 -pady 0 -height $height]
  set sell [frame $sel_fr.l -bd 0 -bg $audio(bgcolor) -padx 0 -pady 0]
  set selm [frame $sel_fr.m -bd 0 -bg $audio(bgcolor) -padx 0 -pady 0]
  set selr [frame $sel_fr.r -bd 0 -bg $audio(bgcolor) -padx 0 -pady 0]
  pack $sel_fr -side top -fill x -expand 0 -anchor n
  pack $display_fr -side top -fill both -expand 0 -anchor n
  pack $sell $selm $selr -side left -anchor n

  # This is a list of select buttons and their frame to display
  set audio(select_list,$type,$id,$host,$port) ""
  # This frame is for holding buttons to select subframe to display
  set audio(select_frame,$type,$id,$host,$port) $sel_fr
  set audio(display_frame,$type,$id,$host,$port) $display_fr

  set info_fr [MakeInfoFrame $host $port $type $id info $display_fr]
  set status_fr [MakeInfoFrame $host $port $type $id status $display_fr]
  set ext_fr [MakeInfoFrame $host $port $type $id extended $display_fr]

  AddSelectButton $host $port $type $id Info $info_fr "Display info\nfor $type $id"
  AddSelectButton $host $port $type $id Status $status_fr "Display status\ninfo for $type $id"
  AddSelectButton $host $port $type $id Ext $ext_fr "Display extended\ninfo for $type $id"
  #if {$id == 1} { SelectButtonPressed  $host $port $type $id 0 0 }
  #SelectButtonPressed  $host $port $type $id 0 0

  return
}
proc CanvasDelayPress { host port type id event tag x y } {
  global audio

  set canvas $audio($type,$id,canvas,$host,$port)
  if { $event == "p" } {
puts "Pressed $host $port $type $id $x $y"
    $canvas bind $tag <Motion> "CanvasDelayPress $host $port $type $id m $tag %x %y"
    set audio($type,$id,$tag,last_y,$host,$port) $y
    set audio($type,$id,skip_delay_text,$host,$port) 1
    set val $audio($type,$id,vals,$tag,$host,$port)
    $audio($type,$id,canvas,$host,$port) itemconfigure delay -text [DelayWithUnit $audio($type,$id,vals,$tag,$host,$port)]
  } elseif { $event == "r" } {
puts "Released $host $port $type $id $x $y"
    set audio($type,$id,skip_delay_text,$host,$port) 0
    $canvas bind $tag <Motion> ""
    if { $tag == "inidelay" } {
      set command "audio $type delay $id $audio($type,$id,vals,$tag,$host,$port)"
      puts "COMMAND <$command>"
      after 0 [list AddServerCommand snowaudio $host $port "$command" GetOneLinePuts 1000]
    } elseif { $tag == "min_delay" } {
      set id_list [string map {, \ } $id]
      set command "audio $type source mindelay $id_list $audio($type,$id,vals,$tag,$host,$port)"
      after 0 [list AddServerCommand snowaudio $host $port "$command" GetOneLinePuts 1000]
    } elseif { $tag == "max_delay" } {
      set id_list [string map {, \ } $id]
      set command "audio $type source maxdelay $id_list $audio($type,$id,vals,$tag,$host,$port)"
      after 0 [list AddServerCommand snowaudio $host $port "$command" GetOneLinePuts 1000]
    }
  } elseif { $event == "m" } {
    set delta_y [expr $audio($type,$id,$tag,last_y,$host,$port) - $y]
    set audio($type,$id,$tag,last_y,$host,$port) $y
    set coords [$canvas coords $tag]
    set old_y [lindex $coords 1]
    set new_y [expr $old_y - $delta_y]
    set max_y $audio($type,$id,cv_bars0,$host,$port)
    set min_y [expr $max_y - 100]
#puts "MOTION $x $y delta $delta_y coords $coords new_y $new_y max_y $max_y min_y $min_y"
    if { $new_y <= $max_y && $new_y >= $min_y } {
      set coords [lreplace $coords 1 1 $new_y]
#puts " - new coords for textdelay $coords"
      $canvas coords $tag  $coords
      set new_delay [expr $max_y - $new_y]
      #set audio($type,$id,vals,$tag,$host,$port) [expr int ($new_delay * $new_delay / 2)]
      set audio($type,$id,vals,$tag,$host,$port) [expr int ($new_delay * $new_delay / 4)]
      $audio($type,$id,canvas,$host,$port) itemconfigure delay -text [DelayWithUnit $audio($type,$id,vals,$tag,$host,$port)]
    }
  #set audio($type,$id,cv_bars0,$host,$port) $cv_bars0
#puts "Motion $host $port $type $id $x $y delta y $delta_y coords $coords"
  }
}

proc AddHoverColor { node args } {
  global audio

  bind $node <Enter> "$node configure -bg grey50"
  bind $node <Leave> "$node configure -bg $audio(butbgcolor)"
}

proc SelectButtonPressed { host port type id no compute} {
  global audio

  focus .
  set key "$type,$id,$host,$port"

  set framebot $audio(display_frame,$key)
  set current_set [pack slaves $framebot]
  if { $current_set != "" } {
    foreach current $current_set {
      pack forget $current
    }
  }
  set n 0
  foreach but_set $audio(select_list,$key) {
    #set title [lindex $but_set 0]
    set node [lindex $but_set 1]
    if { $no != $n } {
      set relief raised
    } else {
      set current_relief [$node cget -relief]
      if { $current_relief == "sunken" } {
         $node configure -relief raised
         return
      }
      set relief sunken
      set displayfr_set [lindex $but_set 2]
      foreach displayfr $displayfr_set {
        pack $displayfr -side top -fill both -expand 1 -anchor n
      }
      set raisecmd [lindex $but_set 3]
      if { $raisecmd != "" } { $raisecmd $host $port $type $id  }
    }
    $node configure -relief $relief
    incr n
  }
  if {$compute} {
#    after 0 [list $audio(nb,$host,$port) compute_size]
  }
}

proc AddSelectButton { host port type id title fr helptext args } {
  global audio

  set key "$type,$id,$host,$port"
    
  set selfr $audio(select_frame,$key)
  set bg [$selfr cget -bg]
  set fg $audio(butfgcolor)
  # Get the number this entry will get
  set no [llength $audio(select_list,$key)]
  set slaves [pack slaves $selfr]
  set cols [llength $slaves]
  set pos [expr $no % $cols]
  set frcol [lindex $slaves $pos]

#  if { $no % $cols } { set side r } else { set side l }
  set node [string tolower $title]
  set node [string map {. _} $node]
pack configure $frcol -expand 1 -fill x
  Label $frcol.$node -text $title -bd 1 -bg $audio(butbgcolor) -fg $fg -relief raised -helptext $helptext
  pack $frcol.$node -fill x -side top -expand 1 -anchor n
  bind $frcol.$node <ButtonPress> "SelectButtonPressed $host $port $type $id $no 1"
  AddHoverColor $frcol.$node
  #bind $frcol.$node <Enter> "$frcol.$node configure -bg grey40"
  #bind $frcol.$node <Leave> "$frcol.$node configure -bg $audio(butbgcolor)"

  if {[llength $args]} {
    # We have a raiscmd
    lappend audio(select_list,$key) [list $title $frcol.$node $fr [lindex $args 0]]
  } else {
    lappend audio(select_list,$key) [list $title $frcol.$node $fr]
  }
#  puts "SELECT <$scene(select_list,$key)>"
}

proc MakeInfoFrame { host port type id format_type parent } {
  global audio

  if { $format_type != "info" && $format_type != "status" &&
	$format_type == "extended" && $format_type != "source info" &&
	$format_type != "source status" && $format_type == "source extended" } return
  set frname [string map {, \ } $format_type]

  set bg [$parent cget -bg]
  # Make a frame to hold the info frame
  set fr [frame $parent.$frname -bd 0 -bg $bg -padx 0 -pady 0]
  set frl [frame $fr.l -bd 0 -bg $bg]
  set frr [frame $fr.r -bd 0 -bg $bg]
  pack $frl $frr -side left -expand 1 -fill x -anchor n

  set idmap [string map {, \ } $id]
  if {[llength $idmap] < 2} {
    # feed, sink mixer
    set valname_list [lrange $audio($type,format,$format_type,list,$host,$port) 1 end]
    Label $frl.type -text $type -bg $bg -fg grey40 -relief sunken -bd 1 -width 4 -anchor w
    Label $frr.id -text $id -bg $bg -fg grey40  -relief sunken -bd 1 -anchor e
  } else {
    # mixer source
    set format_type "source $format_type"
    if {[string match "source info" $format_type]} { set val_start 2 } else { set val_start 1 }
    set valname_list [lrange [lindex $audio($type,format,$format_type,list,$host,$port) $val_start] 1 end]
#puts "SOURCE $type $id $format_type <$valname_list>"
#puts "SOURCE $type $id $format_type <$audio($type,format,$format_type,list,$host,$port)>"
    Label $frl.type -text source -bg $bg -fg grey40 -relief sunken -bd 1 -width 4 -anchor w
    frame $frr.id -bd 1 -relief sunken -bg $bg -padx 0 -pady 0
    Label $frr.id.source -textvariable audio($type,$id,vals,source,$host,$port) -bg $bg -fg grey40  -relief flat -bd 0 -anchor e
    Label $frr.id.source_id -textvariable audio($type,$id,vals,source_id,$host,$port) -bg $bg -fg grey40  -relief flat -bd 0 -anchor e
    pack $frr.id.source_id $frr.id.source -side right -anchor e
  }
  pack $frl.type $frr.id -side top -fill x -expand 1 -anchor n
  foreach valname $valname_list {
#  if {[llength $idmap] > 1} { puts "Packing $id $valname = audio($type,$id,vals,$valname,$host,$port) $audio($type,$id,vals,$valname,$host,$port)" }
    Label $frl.$valname -text $valname -bg $bg -fg grey40 -relief sunken -bd 1 -width 4 -anchor w
    Label $frr.$valname -textvariable audio($type,$id,vals,$valname,$host,$port) -bg $bg -fg grey40  -relief sunken -bd 1 -anchor e
    pack $frl.$valname $frr.$valname -side top -fill x -expand 1
  }
  return $fr
}

proc SetFeedButColor { host port feed_id } {
  global audio

  if {![info exist audio(feed,$feed_id,vals,mute,$host,$port)]} return
  if {![info exist audio(feed,$feed_id,vals,state,$host,$port)]} return
puts "SetFeedButColor $host $port $feed_id"
puts " - audio(feed,butfr,$host,$port).feed$feed_id = $audio(feed,butfr,$host,$port).feed$feed_id"
  if { $audio(feed,$feed_id,vals,mute,$host,$port) == "muted" } { set fgcolor red ; set text "($feed_id)" } else { set fgcolor green ; set text " $feed_id " }
  set bgcolor $audio(feedcolor,$audio(feed,$feed_id,vals,state,$host,$port))
  if { $bgcolor == $fgcolor } { set fgcolor dark$fgcolor }
  if { $bgcolor == "green" } { set bgcolor #00FF00 }
  if { $bgcolor == "lightblue" } { set bgcolor $audio(butbgcolor) }
  if { $audio(feed,$feed_id,vals,state,$host,$port) == "UNDEFINED" } { set fgcolor grey70 }
  if {[winfo exist $audio(feed,butfr,$host,$port).feed$feed_id]} {
    $audio(feed,butfr,$host,$port).feed$feed_id configure -bg $bgcolor -fg $fgcolor -text $text
  }
  return
}
  
proc BuildAudioTabs { host port } {
  global audio

  puts stderr "Building Audio Tabs for $host:$port"
  # Build Audio Feed Tabs
  AddServerAudioTab $host $port feed "Audio Feeds"
  set audio(feed,graph_x,$host,$port) 0
  set butfr $audio(feed,butfr,$host,$port)
  foreach feed_id $audio(feed,list,$host,$port) {
    BuildAudioTabItem $host $port feed feed $feed_id
    Label $butfr.feed$feed_id -text $feed_id -relief raised -bd 1 -bg $audio(bgcolor) -fg $audio(fgcolor) -padx 5 -pady 2
puts "CREATING : $butfr.feed$feed_id"
    pack $butfr.feed$feed_id -side left
    SetFeedButColor $host $port $feed_id
    bind $butfr.feed$feed_id <ButtonPress> "$butfr.feed$feed_id configure -relief sunken ; ToggleMute $host $port feed $feed_id"
    bind $butfr.feed$feed_id <ButtonRelease> "$butfr.feed$feed_id configure -relief raised"
    
  }
  catch { $audio(nb,$host,$port) raise feed }
#  $audio(nb,$host,$port) raise feed

  foreach mixer_id $audio(mixer,list,$host,$port) {
    set audio(mixer$mixer_id,graph_x,$host,$port) 0
    if {![info exist audio(mixer,name,$mixer_id,$host,$port)] || $audio(mixer,name,$mixer_id,$host,$port) == ""} {
      set title "Audio Mixer $mixer_id"
    } else { set title $audio(mixer,name,$mixer_id,$host,$port) }
    AddServerAudioTab $host $port mixer$mixer_id $title
    BuildAudioTabItem $host $port mixer$mixer_id mixer $mixer_id
    set audio_fr [$audio(nb,$host,$port) getframe mixer$mixer_id]
    set bg [$audio_fr cget -bg]
    set fr [frame $audio_fr.spacer -bd 1 -bg $bg -relief flat  -bd 1 -padx 1 -pady 1 -width 12]
#$fr configure -bg grey20
    pack $fr -side left -fill both  -expand 0

    set format_type "source info"
    set valname [lindex $audio(mixer,format,$format_type,list,$host,$port) 1]
    set no_sources $audio(mixer,$mixer_id,vals,$valname,$host,$port)
#puts "SOURCES for mixer $mixer_id : $no_sources"
    set source_id 1
    while { $source_id <= $no_sources } {
#      puts "BUILDING mixer $mixer_id source $source_id"
      BuildAudioTabItem $host $port mixer$mixer_id mixer $mixer_id $source_id
      incr source_id
    }
  }
  catch {
    set mixer_id [lindex $audio(mixer,list,$host,$port) 0]
    $audio(nb,$host,$port) raise mixer$mixer_id
   }

  AddServerAudioTab $host $port sink "Audio Sinks"
  set audio(sink,graph_x,$host,$port) 0
  foreach sink_id $audio(sink,list,$host,$port) {
    BuildAudioTabItem $host $port sink sink $sink_id
  }

  return
}

proc GetAudioNames { handle tag host port line args } {
  global audio

  if {[regexp {STAT:\ +audio\ +([^ ]+)\ +([0-9]+)\ +<(.+)>} $line match type id name]} {
    #puts "Audio $type $id <$name>"
    if { $type == "feed" || $type == "mixer" || $type == "sink" } {
      set audio($type,name,$id,$host,$port) $name
      if {[lsearch -exact $audio($type,list,$host,$port) $id] < 0 } { lappend audio($type,list,$host,$port) $id }
    } else {
      puts stderr "Invalid audio type $type for GetAudioNames."
      after 0 "AsyncCloseConn $handle"
    }
  } elseif {[regexp {STAT:\ +$} $line match]} {
    if {[llength $audio(sink,list,$host,$port)]} {
      puts "Audio Lists: \n - feeds  : $audio(feed,list,$host,$port)\n - mixers : $audio(mixer,list,$host,$port)\n - sinks  : $audio(sink,list,$host,$port)"
    }
    return 0
  }
  return 1
}
proc GetAudioInfoFormatLine { handle tag host port line args } {
  global audio

  if {![llength $args]} {
    puts stderr "Missing argument type for GetAudioInfoFormatLine"
    after 0 "AsyncCloseConn $handle"
    return 0
  }
  if {[string match "*Invalid*" $line]} {
    puts stderr "Invalid line return for GetAudioInfoFormatLine <$line>"
    after 0 "AsyncCloseConn $handle"
    return 0
  }
  set args [lindex $args 0]
  set type [lindex $args 0]
  set format_type [lindex $args 1]
  set callback1 [lindex $args 2]
  set callback2 [lindex $args 3]
  set format_list [lrange $line 1 end]
  puts "GetAudioInfoFormatLine type <$type> <$format_type> <$format_list>"
  set audio($type,format,$format_type,list,$host,$port) $format_list

  after 0 [list AddServerCommand snowaudio $host $port "tcl eval snowmix message \[snowmix info audio $type $format_type all]" GetAudioValuesLine 1000 $type $format_type $callback2]
  if { $callback1 != "" && [info procs $callback1] != "" } { $callback1 $host $port }
  return 0
}

proc GetAudioValuesLine { handle tag host port line args } {
  global audio

#puts "GetAudioValuesLine <$line>"

  if {![llength $args]} {
    puts stderr "Missing argument type for GetAudioValuesLine"
    after 0 "AsyncCloseConn $handle"
    return 0
  }
  if {[string match "*Invalid*" $line]} {
    puts stderr "Invalid line return for GetAudioValuesLine <$line>"
    after 0 "AsyncCloseConn $handle"
    return 0
  }
  set args [lindex $args 0]
  set type [lindex $args 0]
  set format_type [lindex $args 1]
  set callback1 [lindex $args 2]
#puts "Getting data for audio $type $format_type"
  set value_set [lrange $line 1 end]
  if {![info exist audio($type,format,$format_type,list,$host,$port)]} {
    puts stderr "Does not have audio($type,format,$format_type,list,$host,$port) in GetAudioValuesLine"
    return 0
  }
#  puts "GOT Values for <$type> <$format_type> <$audio($type,format,$format_type,list,$host,$port)> <$value_set>"
  if {![string match "source *" $format_type]} {
    foreach data_set $value_set {
      set i 0
      set id [lindex $data_set 0]
      foreach valname $audio($type,format,$format_type,list,$host,$port) {
        if {$i == 0} { set i 1 ; continue }
        set audio($type,$id,vals,$valname,$host,$port) [lindex $data_set $i]
#         puts " $id : $i $valname [lindex $data_set $i]"
        incr i
      }
    }
    if {$format_type == "status"} {
      foreach data_set $value_set {
        set id [lindex $data_set 0]
        set channel 0
        foreach value $audio($type,$id,vals,volume,$host,$port) {
          set audio($type,$id,vals,oldvolume,$channel,$host,$port) \
          	[set audio($type,$id,vals,volume,$channel,$host,$port) [expr round(sqrt(2500.0*$value))]]
          incr channel
        }
      }
    }
   
  } else {
    foreach data_set $value_set {
      set i 1
      set id [lindex $data_set 0]
      if {[string match "source info" $format_type]} {
        set valname [lindex $audio($type,format,$format_type,list,$host,$port) 1]
        set audio($type,$id,vals,$valname,$host,$port) [lindex $data_set 1]
        set val_start 2
      } else {
        set val_start 1
      }
      set val_set [lindex $data_set $val_start]
      set valname_list [lindex $audio($type,format,$format_type,list,$host,$port) $val_start]
      set source_id 1
      foreach source_set $val_set {
        set ival 0
        foreach valname $valname_list {
          set audio($type,$id,$source_id,vals,$valname,$host,$port) [lindex $source_set $ival]
          incr ival
        }
        incr source_id
      }
    }
    if { $format_type == "source status" } {
      foreach data_set $value_set {
        set i 1
        set id [lindex $data_set 0]
        set source_id 1
        set max_source_id [llength [lindex $data_set 1]]
        while { $source_id <= $max_source_id } {
          set channel 0
          foreach value $audio($type,$id,$source_id,vals,volume,$host,$port) {
            set audio($type,$id,$source_id,vals,oldvolume,$channel,$host,$port) \
          	[set audio($type,$id,$source_id,vals,volume,$channel,$host,$port) [expr round(sqrt(2500.0*$value))]]
            incr channel
          }
          incr source_id
        }
      }
    }
  }
  if { $callback1 != "" && [info procs $callback1] != "" } { $callback1 $host $port }
  return 0
}

proc GetAudioStatusLine { handle tag host port line args } {
  global audio

  set args [lindex $args 0]
  set type [lindex $args 0]
  set format_type [lindex $args 1]
  set ids [lindex $args 2]
  set info_proc [lindex $args 3]
  
  # We need to know the type (feed,mixer,sink) and the format type (info,status,extended,source info ...)
  if { $type == "" || $format_type == ""} {
    puts stderr "Got empty type <$type> or format_type <$format_type> in GetAudioStatusLine"
    return 0
  }

#puts "TYPE <$type> ids <$ids> format <$format_type> $line"
#puts "TYPE <$type> ids <$ids> format <$format_type>"


  set data_sets [lrange $line 1 end]

  # This is the section for "source info", "source status" and "source extended"
  if {[string match "source *" $format_type]} {
    foreach data_set $data_sets {
      set i 1
      set id [lindex $data_set 0]
      if {[string match "source info" $format_type]} {
        set valname [lindex $audio($type,format,$format_type,list,$host,$port) 1]
        set audio($type,$id,vals,$valname,$host,$port) [lindex $data_set 1]
        set val_start 2
      } else {
        set val_start 1
      }
      set val_set [lindex $data_set $val_start]
      set valname_list [lindex $audio($type,format,$format_type,list,$host,$port) $val_start]
      set source_list ""
      set max_source_id 0
      set source_id 0
      foreach source_set $val_set {
        set ival 0
        incr source_id
        foreach valname $valname_list {
          set value [lindex $source_set $ival]
          set audio($type,$id,$source_id,oldvals,$valname,$host,$port) $audio($type,$id,$source_id,vals,$valname,$host,$port)
          set audio($type,$id,$source_id,vals,$valname,$host,$port) $value
          incr ival
        }
      }
      set max_source_id $source_id
      if {[string match "source status" $format_type]} {
        set source_id 1
        while {$source_id <= $max_source_id} {
          foreach valname { state volume mute } {
            if {$audio($type,$id,$source_id,oldvals,$valname,$host,$port) != $audio($type,$id,$source_id,vals,$valname,$host,$port)} {
              StatusChange $host $port $type $id,$source_id $format_type $valname $audio($type,$id,$source_id,vals,$valname,$host,$port)
            }
          }
          incr source_id
        }
      } elseif {[string match "source extended" $format_type]} {
        set source_id 1
        while {$source_id <= $max_source_id} {
          foreach valname { rms delay } {
            StatusChange $host $port $type $id,$source_id $format_type $valname $audio($type,$id,$source_id,vals,$valname,$host,$port)
          }
          foreach valname { clipped } {
            if {$audio($type,$id,$source_id,oldvals,$valname,$host,$port) != $audio($type,$id,$source_id,vals,$valname,$host,$port)} {
              StatusChange $host $port $type $id,$source_id $format_type $valname $audio($type,$id,$source_id,vals,$valname,$host,$port)
            }
          }
          incr source_id
        }
      }
    }
  } else {
  # This is the section for "info", "status" and "extended"
    foreach data_set $data_sets {
      set id [lindex $data_set 0]
      set i 1
      foreach value [lrange $data_set 1 end] {
        set valname [lindex $audio($type,format,$format_type,list,$host,$port) $i]
        set audio($type,$id,oldvals,$valname,$host,$port) $audio($type,$id,vals,$valname,$host,$port)
        set audio($type,$id,vals,$valname,$host,$port) $value
        incr i
      }
    }
    if {$format_type == "status"} {
      foreach data_set $data_sets {
        set id [lindex $data_set 0]
        foreach valname { state volume mute } {
          if {$audio($type,$id,oldvals,$valname,$host,$port) != $audio($type,$id,vals,$valname,$host,$port)} {
            StatusChange $host $port $type $id $format_type $valname $audio($type,$id,vals,$valname,$host,$port)
          }
        }
      }
    } elseif {$format_type == "extended"} {
      # We use this spot to increment the x-coordinate of the graphs
      if { $type == "mixer" } { set key "mixer$ids" } else { set key $type }
      incr audio($key,graph_x,$host,$port)
#puts "INCREMENT audio($key,graph_x,$host,$port) $audio($key,graph_x,$host,$port)"

      foreach data_set $data_sets {
        set id [lindex $data_set 0]
#puts "PMM $id"
        foreach valname { rms delay } {
          StatusChange $host $port $type $id $format_type $valname $audio($type,$id,vals,$valname,$host,$port)
        }
        foreach valname { clipped } {
          if {$audio($type,$id,oldvals,$valname,$host,$port) != $audio($type,$id,vals,$valname,$host,$port)} {
            StatusChange $host $port $type $id $format_type $valname $audio($type,$id,vals,$valname,$host,$port)
          }
        }
      }
    }
  }
  if {$info_proc != "" && [info procs $info_proc] != ""} {
#    puts "CALLING $info_proc $host $port $type $format_type $ids 0"
    $info_proc $host $port $type $format_type $ids 0
  }
  return 0
}

proc StartSnowmix { host port } {
  global scene
  unset -nocomplain scene(getalpha4scene,$host,$port)
  after 0 [list AddServerCommand snowmix $host $port "system frame rate" GetFrameRate 1000]
  after 0 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[snowmix info feed state all]" GetFeedState 1000]
  after 150 [list AddServerCommand snowmix $host $port "tcl eval ScenesList" GetScenes 1000]
  after 200 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[snowmix info feed state all]" GetFeedState 1000]
  after 1000 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneSetState]" GetScenesState 1000]
  return
}
  

# hostid			: $host:$port
# scene(main_frame)		: Main Frame for Scene
# scene(main_nb)		: Main NoteBook for Scenes
# scene(main_frame,$hostid)	: Main frame for Scenes for server $hostid
# scene(nb,$hostid)		: NoteBook for Scenes for server $hostid
# 

proc CleanUpServerSetup { $host $port } {
  global scene
  #unset -nocomplain 
}


proc SetupAudioPane { pane notebook page } {
  global audio 
  puts stderr "Setting up AudioPane for pane $pane notebook $notebook page $page"
  set bg [$pane cget -bg]
  set audio(main_frame) $pane
  set audio(bg) [$pane cget -bg]
  $pane configure -padx 0 -pady 0

  set audio(feedcolor,DISCONNECTED)	red
  set audio(feedcolor,READY)		lightblue
  set audio(feedcolor,PENDING)		orange
  set audio(feedcolor,RUNNING)		green
  set audio(feedcolor,STALLED)		yellow
  set audio(feedcolor,SETUP)		purple
  set audio(feedcolor,UNKNOWN)		magenta
  set audio(feedcolor,UNDEFINED)	grey30
  set audio(bgcolor)			grey10
  set audio(fgcolor)			grey80
  set audio(butbgcolor)			grey30
  set audio(butfgcolor)			grey70


  set audio(main_nb) [NoteBook $pane.nb -side top -arcradius 4 -homogeneous 1 -tabbevelsize 2 \
	-bg $audio(butbgcolor) -fg $audio(butfgcolor)  -activebackground grey40  -activeforeground black]
  pack $audio(main_nb) -side top -fill both -anchor w -expand 1
  set audio(autoraise) 0
  set audio(update_rate) 1000
  set audio(update_rate,info) 1500
  set audio(update_rate,status) 180
  set audio(update_rate,extended) 700

  set snowmix_port 9999

#  set host thinkie
#  AddServerSetup $host $snowmix_port
#  catch { $audio(main_nb) raise $audio(hostmap,$host):$snowmix_port }

#  set host 127.0.0.1
#  set host 192.168.3.132
#  set host 192.168.3.100
#  set host vostok01
#  set hostmap [string map {. _} $host]
#  AddServerSetup $host $snowmix_port
#  catch { $audio(main_nb) raise $audio(hostmap,$host):$snowmix_port }

#  bind . <Key> "KeyInput Hello1 %K %k"
#  bind $pane <Key> "KeyInput Hello2 %K %k"

  bind . <Enter> "focus ."
  return 
}
