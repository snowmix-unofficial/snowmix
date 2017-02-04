#
# Copyright Peter Maersk-Moller (C) 2012-2016
# All Rights Reserved.
#

#source ./server.tcl
#source ./cs_feed_commands.tcl

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

proc ServerNotConnectedSnowmix { tag host port } {
  global scene led_image system

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
  global relay led_image scene system
puts stderr "SetServerLed $host:$port $color"
#  if {![info exist relay(server,leds,$host,$port)]} {
#    puts "SetServerLed warning. Host $host port $port does not exist"
#    return
#  }
  if {![info exist led_image($color,10x12)]} {
    puts stderr "SetServerLed warning. Color $color does not exist for host $host port $port. Using dark grey"
    set color darkgrey
  }
  if {[info exist scene(server,leds,$host,$port)]} {
    foreach led $scene(server,leds,$host,$port) {
      $led configure -image $led_image($color,10x12)
    }
  }

  # See if we have a led on a pane
  if {[info exist scene(main_frame)]} {
    $scene(main_nb) itemconfigure $scene(hostmap,$host):$port -image $led_image($color,$system(led_size))
  }
}

proc AppendServerLed { host port label_item } {
  global scene
  puts "APPENDING SERVER LED : scene(server,leds,$host,$port) $label_item"
  lappend scene(server,leds,$host,$port) $label_item
}

# system frame rate
proc GetFrameRate { handle tag host port line } {
  global scene system
  set system(framerate,$host,$port) [lindex $line 5]
  puts "GOT frame rate for $host:$port = $system(framerate,$host,$port)"
  return 0
}

# image load
proc GetImageLoad { handle tag host port line } {
  global scene

#puts "GetImageLoad <$line>"
  if {[string compare "MSG:" $line] == 0} { return 0 }
  if {[regexp {MSG:\ +image\ +load\ +([0-9]+)\ +<([^ \>]+)>\ +([0-9]+)x([0-9]+)} $line match image_id filename width height]} {
#puts "GETTING image <$line>"
    if {$image_id > $scene(image,max_id,$host,$port)} { set scene(image,max_id,$host,$port) $image_id }
    set images(width,$image_id,$host,$port) $width
    set images(height,$image_id,$host,$port) $height
    set images(name,$image_id,$host,$port) $filename
    lappend scene(image,list,$host,$port) "$image_id [file tail $filename]"
  }
  return 1
}


# tcl eval ScenesList
proc GetScenes { handle tag host port line } {
  global scene system led_image
  if {[regexp {MSG:\ +Scenes\ += (.+)} $line match scenes]} {
    foreach scene_id $scenes {
      if {[info exist scene(scene,$host,$port,$scene_id)]} continue
      MakeSceneTab $host $port $scene_id
    }
  } else {
    puts stderr "Failed to get Scenes"
  }
  # This was the last line we expected to read and we can now return 0
  return 0
}

# tcl eval SceneList <id>
proc GetSceneList { handle tag host port line } {
  global scene
  
  if {[info exist scene(current_scene_list,$host,$port)]} {
    set scene_id $scene(current_scene_list,$host,$port)
  }
  if {[regexp {MSG:\ +\-\ +frame\ +([0-9]+)\ +active\ +([\-0-9]+)\ +\:\ +([0-9]+)x([0-9]+)\ +at\ +([\-0-9]+),([\-0-9]+)\ +source\ +([^ ,]+),([^ ,]+)\ +id\ +([\-0-9]+),([\-0-9]+)\ +shape\ +([0-9]+),([0-9]+)\ +place\ +([0-9]+),([0-9]+)} $line match frame_id active width height xoff yoff source_front source_back source_id_front source_id_back shape_front shape_back place_front place_back]} {
    foreach keyname { active width height xoff yoff shape_front shape_back place_front place_back} {
      upvar 0 $keyname keyval
      set scene($keyname,$scene_id,$frame_id,$host,$port) $keyval
    }
    set scene(source_front,$scene_id,$frame_id,$host,$port) "$source_front $source_id_front"
    set scene(source_back,$scene_id,$frame_id,$host,$port) "$source_back $source_id_back"
    if { $frame_id > $scene(maxframe_id,$scene_id,$host,$port) } { set scene(maxframe_id,$scene_id,$host,$port) $frame_id }

    # Do something with frames
    lappend scene(framelist,$scene_id,$host,$port) $frame_id
    BuildFrameControl $host $port $scene_id $frame_id

  } elseif {[regexp {MSG:\ +Scene\ +([0-9])+\ +active\ +([0-9]+)\ +WxH\ +([0-9]+)x([0-9]+)\ +at\ +([0-9\.]+),([0-9\.]+)\ +name\ +(.+)} $line match scene_id active width height xoff yoff name]} {
    set scene(current_scene_list,$host,$port) $scene_id
    foreach keyname { width height xoff yoff name } {
      upvar 0 $keyname keyval
      set scene($keyname,$scene_id,$host,$port) $keyval
      ScenesActiveState $scene_id $active $host $port
    }
  } elseif {[regexp {MSG:\ +\-\ +back\ +\:\ +image\ +([\-0-9])+\ +WxH\ +([0-9]+)x([0-9]+)\ +at\ +([0-9\.]+),([0-9\.]+)\ +shape\ +([0-9]+)\ +place\ +([0-9]+)} $line match image width height xoff yoff shape place]} {
puts "GOT BACK IMAGE FOR $scene_id $host $port"
    foreach keyname { image width height xoff yoff shape place } {
      upvar 0 $keyname keyval
      set scene(back,$keyname,$scene_id,$host,$port) $keyval
    }
    MakeSceneBackgroundImageSelection  $host $port $scene_id
    $scene(alphabg,scale,button,$scene_id,$host,$port) configure -state normal
  } elseif {[regexp {MSG:\ +\-\ +clock\ +([0-9])\ +active\ +([\-0-9]+)\ +\:\ at\ +([0-9\.]+),([0-9\.]+)\ +text\ +([0-9\.]+)\ +([0-9\.]+)\ +([0-9\.]+)\ +([0-9\.]+)\ +bg\ +([0-9\.]+)\ +([0-9\.]+)\ +([0-9\.]+)\ +([0-9\.]+)} $line match clock_id active xoff yoff red green blue alpha bg_red bg_green bg_blue bg_alpha]} {
    foreach keyname { image width height xoff yoff shape place } {
      upvar 0 $keyname keyval
      set scene(clock_$keyname,$scene_id,$clock_id,$host,$port) $keyval
    }
  } elseif {[string compare "MSG: " $line] == 0} {
    #puts "Got SceneList $scene(current_scene_list,$host,$port)"
    SceneFrameLayout $scene_id $host $port
    # This was the last line we expected to read and we can now return 0
    return 0
  }

  # We have more lines to read
  return 1
}

proc ScenesActiveState { scene_id active host port } {
  global scene system led_image
  if {![info exist scene(scene,active,$scene_id,$host,$port)] || $active != $scene(scene,active,$scene_id,$host,$port) } {
    if { $active > 0 } { set color green
      $scene(fadebutton,$scene_id,$host,$port) configure -state normal
      if { $scene(autoraise,$host,$port) } { $scene(nb,$host,$port) raise scene$scene_id }
    } elseif { $active == 0 } { set color red
      $scene(fadebutton,$scene_id,$host,$port) configure -state disabled
    } else { set color yellow }
    $scene(nb,$host,$port) itemconfigure scene$scene_id -image $led_image($color,$system(led_size))
  }
  set scene(scene,active,$scene_id,$host,$port) $active
  SetActivateColor $scene(activatebutton,$scene_id,$host,$port) $active
}

# tcl eval snowmix message \[SceneSetState]
proc GetScenesState { handle tag host port line } {
  global scene system led_image
  if {[string match "*Invalid*" $line]} {
    puts stderr "Got Invalid response for SceneSetState for snowmix $host $port. Stopping."
    return
  }
  foreach info_set [lrange $line 1 end] {
    set scene_id [lindex $info_set 0]
    set active [lindex $info_set 1]
    if {![info exist scene(scene,$host,$port,$scene_id)]} {
      # We discovered a new scene
      MakeSceneTab $host $port $scene_id
    }
    if { $active != $scene(scene,active,$scene_id,$host,$port) } {
      ScenesActiveState $scene_id $active $host $port
    }
  }
  after 500 [list $scene(nb,$host,$port) compute_size]
  after $scene(update_rate,$host,$port) [list AddServerCommand $tag $host $port "tcl eval snowmix message \[SceneSetState]" GetScenesState 1000]
  # This was the last line we expected to read and we can now return 0
  return 0
}

# tcl eval snowmix message \[snowmix info feed state all]
proc GetFeedState { handle tag host port line } {
  global scene
  if {[string match "*Invalid*" $line]} {
    puts stderr "Got Invalid response for FeedState for snowmix $host $port. Stopping."
    return 0
  }
  foreach info_set [lrange $line 1 end] {
    set feed_id [lindex $info_set 0]
    if { !$feed_id } continue
    set feed_state [lindex $info_set 1]
    if {![info exist scene(feed,state,$host,$port,$feed_id)]} {
      MakeFeedButton  $host $port $feed_id $feed_state
      set scene(feed,state,$host,$port,$feed_id) $feed_state
    } else {
      if { $feed_state != $scene(feed,state,$host,$port,$feed_id) } {
puts "Feed $feed_id changed state to $feed_state"
        set scene(feed,state,$host,$port,$feed_id) $feed_state
        SetFeedButtonColor $host $port $feed_id
      }
    }
  }
  after $scene(update_rate,$host,$port) [list AddServerCommand $tag $host $port "tcl eval snowmix message \[snowmix info feed state all]" GetFeedState 1000]
  return 0
}

proc SetActivateColor {button active} {
  if {$active > 0} {
    if {$active > 1} {
#puts "SetActivateColor $button $active"
      set bgcolor darkgray
      set activebg grey
   } else {
      set bgcolor #00E400
      set bgcolor #109810
      set activebg #10F010
    }
  } else {
    set bgcolor #F00000
    set activebg #FF4848
  }
  $button configure -bg $bgcolor -activebackground $activebg
  return
}

# SceneActivate scene_id fade
proc SceneActivate { scene_id fade host port} {
  global scene
puts "SceneActivate scene $scene_id fade $fade $host:$port"
  if {$scene(scene,active,$scene_id,$host,$port) > 0} {
    set active 0
    set scene(active) -1
  } else {
    set active 1
  }
  foreach id $scene(scenelist,$host,$port) {
    if {$scene(scene,active,$id,$host,$port) > 0} {
      if {$id != $scene_id} {
        ScenesActiveState  $id 0 $host $port
        SetActivateColor $scene(activatebutton,$id,$host,$port) 0
      }
    }
  }
  ScenesActiveState $scene_id $active $host $port
  SetActivateColor $scene(activatebutton,$scene_id,$host,$port) $scene(scene,active,$scene_id,$host,$port)

  if {$fade != {-}} {
puts " - SceneActivate $scene_id $active $fade"
    after 10 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneSetState $scene_id $active $fade]" GetOneLine 1000]
  }
  set scene(scene,active,$scene_id,$host,$port) $active
  #SetSceneLed
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
  
proc GetAlphaState4Scene { handle tag host port line } {
  global scene
  if {![info exist scene(getalpha4scene,$host,$port)] || $scene(getalpha4scene,$host,$port) == ""} {
    puts stderr "ERROR: scene(getalpha4scene,$host,$port) is not set in GetAlphaState4Scene"
    return 0
  }
  set scene_id $scene(getalpha4scene,$host,$port)
  set lines $scene(getalpha4scene,lines,$host,$port)
#puts stderr "GetAlphaState4Scene $host:$port $lines : <$line>"
  if { $scene(getalpha4scene,lines,$host,$port) > 1 } {
    GetAlpha4Scene $host $port $scene_id 1 $line
    set scene(getalpha4scene,lines,$host,$port) 1
    return 1
  } elseif { $scene(getalpha4scene,lines,$host,$port) == 1 } {
    GetAlpha4Scene $host $port $scene_id 0 $line
    set scene(getalpha4scene,lines,$host,$port) 0
    return 1
  } else {
    # DO Frame state stuff

    GetFrameStatus $host $port $scene_id $line
    #set scene(getalpha4scene,lines,$host,$port) 0
    # No more to read. Command response is over
  }

  # Check if the server is no longer graphical visible
  set serverraised [$scene(main_nb) raise]
  if { $serverraised == "" || $serverraised != "$scene(hostmap,$host):$port" } {
    puts stderr "SceneAlpha : Server $host:$port is no longer raised. Breaking"
    unset scene(getalpha4scene,$host,$port)
    return 0
  }

  # Check to see which scene is graphical visible
  set sceneraised [$scene(nb,$host,$port) raise]
  if { $sceneraised == "" } {
    puts stderr "SceneAlpha : Server $host:$port no tab is raised. Breaking"
    unset scene(getalpha4scene,$host,$port)
    return 0
  }
  if {![regexp {scene([0-9]+)} $sceneraised match raised_scene_id]} {
    puts stderr "SceneAlpha : Server $host:$port no Scene is raised. Breaking"
    unset scene(getalpha4scene,$host,$port)
    return 0
  }
  if {!$scene(alphaisset,$scene_id,$host,$port)} {
    puts "Alphaisset enabled for scene $scene_id"
  }
  set scene(alphaisset,$scene_id,$host,$port) 1
  set scene(getalpha4scene,$host,$port) $raised_scene_id
  set scene(getalpha4scene,lines,$host,$port) 2
#  puts stderr "REORDERING Scenealpha for Scene $raised_scene_id $host:$port"
  after 125 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneAlpha $raised_scene_id] \ntcl eval snowmix message \[SceneAlphaLink $raised_scene_id] \ntcl eval snowmix message \[SceneSetFrameActive $raised_scene_id]" GetAlphaState4Scene 1000]
  return 0
}

proc GetAlpha4Scene { host port scene_id lines line } {
  global scene
  #puts "GetAlpha4Scene $host $port <$line>"
  if { $lines == 1 } {
    #puts stderr "SceneAlpha got first line for scene $scene_id for $host:$port"
    set scenealphas [lindex $line 1]
#puts "Got Scene $scene_id [expr [lindex $scenealphas 0] * 100]"
    set scene(alpha,$scene_id,$host,$port) [expr [lindex $scenealphas 0] * 100]
    set scene(alphabg,$scene_id,$host,$port) [expr [lindex $scenealphas 1] * 100]
    set scene(alphatext,$scene_id,$host,$port) [expr [lindex $scenealphas 2] * 100]
    set framealphas [lrange $line 2 end]
    foreach framealpha $framealphas {
      set frame_id [lindex $framealpha 0]
      set scene(scalefront,$scene_id,$frame_id,$host,$port) [expr [lindex $framealpha 1] * 100]
      set scene(scaleback,$scene_id,$frame_id,$host,$port) [expr [lindex $framealpha 2] * 100]
    }
    return
  }
  # else
  #puts stderr "SceneAlpha got second line for scene $scene_id for $host:$port"
  set i 0
  foreach alphaset $line {
    incr i
    # First is the MSG:
    if {$i == 1} continue
    # The second is a set of alphalinks for the scene
    if {$i == 2} {
      set bglink [lindex $alphaset 0]
      set textlink [lindex $alphaset 1]
      if {[info exist scene(alphabg,link,$scene_id,$host,$port)] &&
          $bglink != $scene(alphabg,link,$scene_id,$host,$port)} {
        set scene(alphabg,link,$scene_id,$host,$port) $bglink
        if {$bglink} { set relief sunken } else { set relief raised }
        $scene(alphabg,link,button,$scene_id,$host,$port) configure -relief $relief
      }
      if {[info exist scene(alphatext,link,$scene_id,$host,$port)] &&
          $textlink != $scene(alphatext,link,$scene_id,$host,$port)} {
        set scene(alphatext,link,$scene_id,$host,$port) $textlink
        if {$textlink} { set relief sunken } else { set relief raised }
        $scene(alphatext,link,button,$scene_id,$host,$port) configure -relief $relief
      }
    } else {
      # The rest are alphalinks for frames in scene
      set frame_id [lindex $alphaset 0]
      set alphalink [lindex $alphaset 1]
      if {[info exist scene(alphalink,$scene_id,$frame_id,$host,$port)] &&
          $scene(alphalink,$scene_id,$frame_id,$host,$port) != $alphalink} {
        # The link has changed.
        if { $alphalink > 0 } {
          $scene(alphafront,link,button,$scene_id,$frame_id,$host,$port) configure -relief sunken
          $scene(alphaback,link,button,$scene_id,$frame_id,$host,$port) configure -relief raised
        } elseif { $alphalink < 0 } {
          $scene(alphafront,link,button,$scene_id,$frame_id,$host,$port) configure -relief sunken
          $scene(alphaback,link,button,$scene_id,$frame_id,$host,$port) configure -relief sunken
        } else {
        $scene(alphafront,link,button,$scene_id,$frame_id,$host,$port) configure -relief raised
          $scene(alphaback,link,button,$scene_id,$frame_id,$host,$port) configure -relief raised
        }
      }
      set scene(alphalink,$scene_id,$frame_id) $alphalink
    }
  }
  return
}

proc GetAllAlpha4Scene { scene_id host port } {
  global scene

  # getalpha4scene exist, it is already running and will restart itself
  if {[info exist scene(getalpha4scene,$host,$port)]} {
    puts stderr "GetAllAlpha4Scene. Get alpha is already running and will self start if necessary"
    return
  }
puts "GetAllAlpha4Scene proceding for scene $scene_id $host:$port"
  # GetAlpha is not running. Proceding
  set scene(getalpha4scene,$host,$port) $scene_id
  set scene(getalpha4scene,lines,$host,$port) 2

  puts stderr "ORDERING Scenealpha for Scene $scene_id $host:$port"
  after 0 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneAlpha $scene_id] \ntcl eval snowmix message \[SceneAlphaLink $scene_id] \ntcl eval snowmix message \[SceneSetFrameActive]" GetAlphaState4Scene 1000]
  return
}

proc MakeSceneTab { host port scene_id } {
  global scene system led_image
  if {[info exist scene(scene,$host,$port,$scene_id)]} return
  set scene(scene,$host,$port,$scene_id) $scene_id
  set scene(scene,active,$scene_id,$host,$port) 2
  set scene(maxframe_id,$scene_id,$host,$port) -1
  set scene(alphaisset,$scene_id,$host,$port) 0
  $scene(nb,$host,$port) insert end scene$scene_id -text "  Scene $scene_id" \
	-image $led_image(darkgrey,$system(led_size)) \
	-raisecmd "focus . ; GetAllAlpha4Scene $scene_id $host $port"
  BuildSceneTab $host $port $scene_id
  return
}

proc BuildFrameControl { host port scene_id frame_id } {
  global scene
#puts "BUILDING scene $scene_id frame $frame_id"
  set scenefr $scene(scenefr,$scene_id,$host,$port)
  set bg [$scenefr cget -bg]
  set framefr [frame $scenefr.frame$frame_id -relief raised -bg $bg -bd 1 -padx 4 -pady 4]
  pack $framefr -side left -expand 1 -fill both
  
  set scene(activatebutton,$scene_id,$frame_id,$host,$port) \
	[Label $framefr.activebut		\
        -text "Frame $frame_id"			\
        -pady 2 -padx 5 -bd 1 -relief raised	\
        -bg $bg -width 25					\
        -helptext "Activate or deactivate\nframe $frame_id."]
  set framebot [frame $framefr.frbot -bd 0 -bg $bg]
  pack $framefr.activebut -side top -fill none
  SetActivateColor $scene(activatebutton,$scene_id,$frame_id,$host,$port) \
	$scene(active,$scene_id,$frame_id,$host,$port)
  bind $scene(activatebutton,$scene_id,$frame_id,$host,$port) \
	<ButtonPress> "$scene(activatebutton,$scene_id,$frame_id,$host,$port) configure -relief sunken ; FrameActivate $host $port $scene_id $frame_id -1"
  bind $scene(activatebutton,$scene_id,$frame_id,$host,$port) \
	<ButtonRelease> "$scene(activatebutton,$scene_id,$frame_id,$host,$port) configure -relief raised"

  # Next is a column for 3 frames
  set 3col [frame $framefr.3col   -bd 0 -bg $bg]
  set frl  [frame $3col.left  -bd 1 -bg $bg]
  set frm  [frame $3col.mid   -bd 1 -bg $bg]
  set frmt [frame $frm.top    -bd 0 -bg $bg]
  set frml [frame $frmt.left   -bd 0 -bg $bg]
  set frmm [frame $frmt.mid    -bd 0 -bg $bg]
  set frmr [frame $frmt.right  -bd 0 -bg $bg]
  set frr  [frame $3col.right -bd 1 -bg $bg]
  pack $3col $framebot -side top -fil x
  pack $frl -side left -anchor n -fill x
  pack $frmt -side top -fill x
  pack $frm $frml $frmm $frmr -side left -anchor n -fill x -expand 1
  pack $frr -side left -anchor n -fill x


  # Scene Cut and Fade Button
  set padx 4
  set pady 2
  set width 5
  set parents "$frml $frmm $frmr"
  set i 0
  foreach but_set { {cutbutton Cut "Cut Frame In/Out" 0 normal} {fadebutton Fade "Fade Frame In/Out" 1 normal} \
	{swipebutton Swipe "Swipe frame In/Out\nNot implemented yet." 2 disabled} \
	{flybutton Fly "Fly frame In/Out\nNot implemented yet." 2 disabled} \
	{growbutton Grow "Grow frame In/Out\nNot implemented yet." 2 disabled} \
	{shrinkbutton Shrink "Shrink frame In/Out\nNot implemented yet." 2 disabled} } {
    set button   [lindex $but_set 0]
    set text     [lindex $but_set 1]
    set helptext [lindex $but_set 2]
    set fade     [lindex $but_set 3]
    set state    [lindex $but_set 4]
    set parent   [lindex $parents $i]
    set scene($button,$scene_id,$frame_id,$host,$port) [Label $parent.$button -padx $padx -pady $pady \
	-text $text -helptext $helptext -relief raised -bd 1 -width $width -state $state]
    if { $state == "normal" } {
      bind $scene($button,$scene_id,$frame_id,$host,$port) <ButtonPress> \
	"$scene($button,$scene_id,$frame_id,$host,$port) configure -relief sunken ; FrameActivate $host $port $scene_id $frame_id $fade"
      bind $scene($button,$scene_id,$frame_id,$host,$port) <ButtonRelease> \
	"$scene($button,$scene_id,$frame_id,$host,$port) configure -relief raised"
    }
    AddButtonColors $scene($button,$scene_id,$frame_id,$host,$port)
    pack $scene($button,$scene_id,$frame_id,$host,$port) -side top -fill x -expand 1
    set i [expr ($i + 1) % 3]
  }

#  set scene(fadebutton,$scene_id,$frame_id,$host,$port) [Label $frmm.fade -padx $padx -pady $pady \
#    -text "Fade" -helptext "Fade Frame In/Out" -relief raised -bd 1 -bg $scene(butbgcolor) -fg $scene(butfgcolor) -width $width]
#  bind $scene(fadebutton,$scene_id,$frame_id,$host,$port) <ButtonPress> \
#	"$scene(fadebutton,$scene_id,$frame_id,$host,$port) configure -relief sunken ; FrameActivate $host $port $scene_id $frame_id 1"
#  bind $scene(fadebutton,$scene_id,$frame_id,$host,$port) <ButtonRelease> \
#	"$scene(fadebutton,$scene_id,$frame_id,$host,$port) configure -relief raised"
#  set scene(swipebutton,$scene_id,$frame_id,$host,$port) [Label $frmr.swipe -padx $padx -pady $pady \
#    -text "Swipe" -helptext "Swipe Frame In/Out\nNot implemented" -relief raised -bd 1 -bg $bg -state disabled  -width $width]
#  set scene(flybutton,$scene_id,$frame_id,$host,$port) [Label $frml.fly -padx $padx -pady $pady \
#    -text "Fly" -helptext "Fly Frame In/Out\nNot implemented" -relief raised -bd 1 -bg $bg -state disabled  -width $width]
#  set scene(growbutton,$scene_id,$frame_id,$host,$port) [Label $frmm.grow -padx $padx -pady $pady \
#    -text "Grow" -helptext "Grow Frame In/Out\nNot implemented" -relief raised -bd 1 -bg $bg -state disabled  -width $width]
#  set scene(shrinkbutton,$scene_id,$frame_id,$host,$port) [Label $frmr.shrink -padx $padx -pady $pady \
#    -text "Shrink" -helptext "Shrink Frame In/Out\nNot implemented" -relief raised -bd 1 -bg $bg -state disabled  -width $width]
#  bind $scene(swipebutton,$scene_id,$frame_id,$host,$port) <ButtonPress> \
#	"$scene(swipebutton,$scene_id,$frame_id,$host,$port) configure -relief sunken ; FrameActivate $host $port $scene_id $frame_id 1"
#  bind $scene(swipebutton,$scene_id,$frame_id,$host,$port) <ButtonRelease> \
#	"$scene(swipebutton,$scene_id,$frame_id,$host,$port) configure -relief raised"

#  foreach name { flybutton growbutton shrinkbutton } {
#    AddButtonColors $scene($name,$scene_id,$frame_id,$host,$port)
#  }
    

#  pack $scene(cutbutton,$scene_id,$frame_id,$host,$port) \
#	$scene(flybutton,$scene_id,$frame_id,$host,$port) -side top -fill x -expand 1
#  pack $scene(fadebutton,$scene_id,$frame_id,$host,$port) \
#	$scene(growbutton,$scene_id,$frame_id,$host,$port) -side top -fill x -expand 1
#  pack $scene(swipebutton,$scene_id,$frame_id,$host,$port) \
#	$scene(shrinkbutton,$scene_id,$frame_id,$host,$port) -side top -fill x -expand 1
  
  # We put the scene alpha scale in the left frame
  set l 150
  set w 12
  set troughcolor #002000
  set scene(alphalink,$scene_id,$frame_id,$host,$port) 5
  set scene(alphafront,link,button,$scene_id,$frame_id,$host,$port) \
	[Label $frl.l -text F -relief raised -helptext "Link front and back\nalpha slider." -bg $bg]
  bind $scene(alphafront,link,button,$scene_id,$frame_id,$host,$port) <ButtonPress> \
	"AlphaLinkButton $host $port $scene_id $frame_id 1"
  AddButtonColors $scene(alphafront,link,button,$scene_id,$frame_id,$host,$port)
  scale $frl.scale -from 100 -to 0 -orient vertical -showval 0 \
      -tickinterval 0 -length $l -width $w -repeatdelay 10 -repeatinterval 10 \
      -troughcolor $troughcolor -bg $bg \
	-variable scene(scalefront,$scene_id,$frame_id,$host,$port) \
        -command "AlphaSliderChange $host $port $scene_id $frame_id 1"
  AddButtonColors $frl.scale
  DynamicHelp::add $frl.scale -text "Front alpha level.\nPress and hold top or\nbottom for a steady fade."
  set scene(alphaback,link,button,$scene_id,$frame_id,$host,$port) \
  	[Label $frr.l -text M -relief raised -helptext "Mirror front and back\nalpha slider." -bg $bg]
  bind $scene(alphaback,link,button,$scene_id,$frame_id,$host,$port) <ButtonPress> \
        "AlphaLinkButton $host $port $scene_id $frame_id 0"
  AddButtonColors $scene(alphaback,link,button,$scene_id,$frame_id,$host,$port)
  scale $frr.scale -from 100 -to 0 -orient vertical -showval 0 \
      -tickinterval 0 -length $l -width $w -repeatdelay 10 -repeatinterval 10 \
      -troughcolor $troughcolor -bg $bg \
	-variable scene(scaleback,$scene_id,$frame_id,$host,$port) \
        -command "AlphaSliderChange $host $port $scene_id $frame_id 0"
  DynamicHelp::add $frl.scale -text "Front alpha level.\nPress and hold top or\nbottom for a steady fade."
  pack $frl.l $frl.scale $frr.l $frr.scale -side top

  # Full Frame Buttons
  set fullfr [frame $frm.fullb -bd 0 -pady 2 -bg $bg]
  pack $fullfr -side top -fill x -expand 1
  if { $scene_id == 1 && $frame_id == 1 } { set state disabled } else { set state normal }
  Label $fullfr.cut -text " Cut Full" -relief raised -bd 1 -bg $bg \
	-helptext "Cut the front\nof this frame to\nfullscreen Scene 1" -state $state
  Label $fullfr.fade -text "Fade Full" -relief raised -bd 1 -bg $bg \
	-helptext "Fade the front\nof this page to\nfullscreen Scene 1" -state $state
  pack $fullfr.cut $fullfr.fade -side left -fill x -expand 1
  bind $fullfr.cut <ButtonPress> "$fullfr.cut configure -relief sunken ; CutFadeToFull $host $port $scene_id $frame_id 0"
  bind $fullfr.cut <ButtonRelease> "$fullfr.cut configure -relief raised"
  bind $fullfr.fade <ButtonPress> "$fullfr.fade configure -relief sunken ; CutFadeToFull $host $port $scene_id $frame_id 1"
  bind $fullfr.fade <ButtonRelease> "$fullfr.fade configure -relief raised"
  AddButtonColors $fullfr.cut
  AddButtonColors $fullfr.fade

  # Buttons for Cut Switch Front/back for frame andcut switch between frames within a scene
  #frame $frm.spacer1 -bg $bg -height 0
  Label $frm.cutlabel -text "Cut Switch Frames" -relief raised -bd 1 -helptext "Cut/Switch Front\nand back sources."
  bind $frm.cutlabel <ButtonPress> "$frm.cutlabel configure -relief sunken ; FrameFadeFrontBack $host $port $scene_id $frame_id 0 ; UpdateOptionFrame $host $port $scene_id $frame_id front back"
  bind $frm.cutlabel <ButtonRelease> "$frm.cutlabel configure -relief raised"
  AddButtonColors $frm.cutlabel

  set scene(cutswitchframe,$scene_id,$frame_id,$host,$port) [frame $frm.cutfr -bd 0 -pady 0 -bg $bg]
  frame $frm.spacer2 -bg $bg -height 3
  Label $frm.fadelabel -text "Fade Switch Frames" -relief raised -bd 1 -bg $bg \
	-helptext "Fade/Switch Front\nand back sources."
  bind $frm.fadelabel <ButtonPress> "$frm.fadelabel configure -relief sunken ; FrameFadeFrontBack $host $port $scene_id $frame_id 1 ; UpdateOptionFrame $host $port $scene_id $frame_id front back"
  bind $frm.fadelabel <ButtonRelease> "$frm.fadelabel configure -relief raised"
  set scene(fadeswitchframe,$scene_id,$frame_id,$host,$port) [frame $frm.fadefr -bd 0 -pady 0 -bg $bg]
  AddButtonColors $frm.fadelabel

  frame $frm.spacer3 -bg $bg -height 3
  frame $frm.sel -bg $bg -bd 0
  pack $frm.cutlabel $frm.cutfr $frm.spacer2 $frm.fadelabel $frm.fadefr \
	$frm.spacer3 $frm.sel -side top -fill x -anchor n

  # This is a list of select buttons and their frame to display
  set scene(select_list,$scene_id,$frame_id,$host,$port) ""
  # This frame is for holding buttons to select subframe to display
  set scene(select_frame,$scene_id,$frame_id,$host,$port) $frm.sel
  set scene(display_frame,$scene_id,$frame_id,$host,$port) $framebot
  # We create a left and right and will fill in buttons left to right and then next line left to right
  set sell [frame $frm.sel.l -bd 0 -bg $bg]
  set selm [frame $frm.sel.m -bd 0 -bg $bg]
  set selr [frame $frm.sel.r -bd 0 -bg $bg]
  pack $sell $selm $selr -side left -fill x -expand 1 -anchor n

  set combofr [MakeComboFrame $host $port $scene_id $frame_id $framebot]
  set feedsfr [MakeSourcesFrame $host $port $scene_id $frame_id feed $framebot]
  set imagesfr [MakeSourcesFrame $host $port $scene_id $frame_id image $framebot]
  set othersfr [MakeSourcesFrame $host $port $scene_id $frame_id other $framebot]
  set lblfr [MakeLabelFrame $host $port $scene_id $frame_id $framebot]
  set timfr [MakeTimingFrame $host $port $scene_id $frame_id $framebot]
  set optfr [MakeOptionFrame $host $port $scene_id $frame_id $framebot]
  set scene(timingframe,$scene_id,$frame_id,$host,$port) $timfr
  set scene(optionframe,$scene_id,$frame_id,$host,$port) $optfr
#PMM
  AddSelectButton $host $port $scene_id $frame_id "Feeds" "$combofr $feedsfr" \
	 "Display feed sources\nFront and Back\n+quickset for feeds"
  AddSelectButton $host $port $scene_id $frame_id "Images" "$combofr $imagesfr" \
	 "Display image sources\nFront and Back\n+quickset for feeds"
  AddSelectButton $host $port $scene_id $frame_id "Others" "$combofr $othersfr" \
	 "Display other sources\nFront and Back\n+quickset for feeds"
  AddSelectButton $host $port $scene_id $frame_id "Timing"  $timfr "Display Timing Settings for fade up/pause/down"
  AddSelectButton $host $port $scene_id $frame_id "Labels"  "$combofr $lblfr" "Display Name and time toggles (for feeds)"
  AddSelectButton $host $port $scene_id $frame_id "Options" "$combofr $optfr" "Display optional sets" UpdateOptionFrame
  SelectButtonPressed $host $port $scene_id $frame_id 0

  #pack $srcfr -side top -expand 1 -fill both

#  puts stderr "Scene $scene_id frame $frame_id slaves [pack slaves $framebot]"

  if {[info exist scene(framelist,$scene_id,$host,$port)]} {

    # Add to this frame the frames we already have
    foreach other_frame_id $scene(framelist,$scene_id,$host,$port) {
      if { $frame_id == $other_frame_id } continue
      set but [Label $scene(cutswitchframe,$scene_id,$frame_id,$host,$port).$other_frame_id -text $other_frame_id \
	-relief raised -bd 1 -bg $bg -helptext "Cut/Switch \nframe $frame_id<->$other_frame_id"]
      bind $but <ButtonPress> "$but configure -relief sunken ; FadeFrames $host $port $scene_id $frame_id $other_frame_id 0 ; UpdateOptionFrame $host $port $scene_id $frame_id front ; UpdateOptionFrame $host $port $scene_id $other_frame_id front"
      bind $but <ButtonRelease> "$but configure -relief raised"
      AddButtonColors $but
      set but [Label $scene(fadeswitchframe,$scene_id,$frame_id,$host,$port).$other_frame_id -text $other_frame_id \
	-relief raised -bd 1 -bg $bg -helptext "Fade/Switch \nframe $frame_id<->$other_frame_id"]
      bind $but <ButtonPress> "$but configure -relief sunken ; FadeFrames $host $port $scene_id $frame_id $other_frame_id  ; UpdateOptionFrame $host $port $scene_id $frame_id front ; UpdateOptionFrame $host $port $scene_id $other_frame_id front1"
      bind $but <ButtonRelease> "$but configure -relief raised"
      AddButtonColors $but
      pack $scene(cutswitchframe,$scene_id,$frame_id,$host,$port).$other_frame_id -side left -fill x -expand 1
      pack $scene(fadeswitchframe,$scene_id,$frame_id,$host,$port).$other_frame_id -side left -fill x -expand 1
    }

    # Add this frame to all other frames
    foreach other_frame_id $scene(framelist,$scene_id,$host,$port) {
      if { $frame_id == $other_frame_id } continue
      set but [Label $scene(cutswitchframe,$scene_id,$other_frame_id,$host,$port).$frame_id -text $frame_id \
	-relief raised -bd 1 -bg $bg -helptext "Cut/Switch \nframe $other_frame_id<->$frame_id"]
      bind $but <ButtonPress> "$but configure -relief sunken ; FadeFrames $host $port $scene_id $frame_id $other_frame_id 0; UpdateOptionFrame $host $port $scene_id $frame_id front ; UpdateOptionFrame $host $port $scene_id $other_frame_id front"
      bind $but <ButtonRelease> "$but configure -relief raised"
      AddButtonColors $but

      set but [Label $scene(fadeswitchframe,$scene_id,$other_frame_id,$host,$port).$frame_id -text $frame_id \
	-relief raised -bd 1 -bg $bg -helptext "Fade/Switch \nframe $other_frame_id<->$frame_id"]
      bind $but <ButtonPress> "$but configure -relief sunken ; FadeFrames $host $port $scene_id $frame_id $other_frame_id 1;  UpdateOptionFrame $host $port $scene_id $frame_id front ; UpdateOptionFrame $host $port $scene_id $other_frame_id front"
      bind $but <ButtonRelease> "$but configure -relief raised"
      AddButtonColors $but
      pack $scene(cutswitchframe,$scene_id,$other_frame_id,$host,$port).$frame_id -side left -fill x -expand 1
      pack $scene(fadeswitchframe,$scene_id,$other_frame_id,$host,$port).$frame_id -side left -fill x -expand 1
    }
  }
    after 0 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneFadeSpeed all $scene_id $frame_id ]" GetSceneOverlayTimingLine 1000 $scene_id $frame_id]
}

set scene(buttontypes) "binary  button  time frame_button combo_box"
proc UpdateOptionFrame { host port scene_id frame_id args } {
  global scene

  puts "UpdateOptionFrame : $host $port $scene_id $frame_id $args"
  if {![info exist scene(optionframe,$scene_id,$frame_id,$host,$port)]} return
  set optionfr $scene(optionframe,$scene_id,$frame_id,$host,$port)
  set bg [$optionfr cget -bg]
  if { [llength $args] } { set side_set $args } else { set side_set "front back" }
  foreach side $side_set {
    set slaves [pack slaves $optionfr.$side]
    foreach slave [pack slaves $optionfr.$side] {
#puts "SLAVE to destroy $slave"
      destroy $slave
    }
    set src_$side [$scene(combobox,source_$side,$scene_id,$frame_id,$host,$port) get]
    #$optionfr.$side.label "Options for $side : $sr
    set varname src_$side
    upvar 0 $varname value
    puts "UpdateOptionFrame $host $port scene $scene_id frame $frame_id $side source $value"
catch { destroy $optionfr.$side.label }
    Label $optionfr.$side.label -text "Options for $side : $value" -relief raised -bd 1 -bg grey20 -fg grey70
    pack $optionfr.$side.label -side top -fill x
    set type [lindex $value 0]
    if { $type == "-" } continue
    set type_id [lindex $value 1]
    set button_list $scene(frame_types,buttons,$type,$host,$port)
    if { $button_list == "" } {
      Label $optionfr.$side.no_op -text "No options for $type" -bg $bg
      pack $optionfr.$side.no_op -side top -fill x
      continue
    }
    puts "PMM Buttons $scene(frame_types,buttons,[lindex $value 0],$host,$port)"
    set i 0
    foreach button_list $scene(frame_types,buttons,[lindex $value 0],$host,$port) {
      set proc_name [lindex $button_list 0]
      set button_type [lindex $button_list 1]
      set name [lindex $button_list 2]
puts "BUTTON TYPE $button_type"
      if { [lsearch $scene(buttontypes) $button_type] < 0 } continue
      if { !($i % 2) } {
        set linefr [frame $optionfr.$side.$i -bd 0 -bg $bg]
        pack $linefr -side top -anchor n -fill x 
      }
      if { $button_type == "binary" } {
        Label $linefr.$i -text $name -relief raised -bd 1 -bg $bg
        bind $linefr.$i <ButtonPress> "$linefr.$i configure -relief sunken ; OptionButtonPressed $host $port $scene_id $frame_id $button_type $proc_name $type $type_id $side"
        bind $linefr.$i <ButtonRelease> "$linefr.$i configure -relief raised"
        pack $linefr.$i -side left -fill x -expand 1
        AddButtonColors $linefr.$i
      } elseif { $button_type == "button" || $button_type == "frame_button"} {
        Label $linefr.$i -text $name -relief raised -bd 1 -bg $bg
        bind $linefr.$i <ButtonPress> "$linefr.$i configure -relief sunken ; OptionButtonPressed $host $port $scene_id $frame_id $button_type $proc_name $type $type_id $side"
        bind $linefr.$i <ButtonRelease> "$linefr.$i configure -relief raised"
        pack $linefr.$i -side left -fill x -expand 1
        AddButtonColors $linefr.$i
      } elseif { $button_type == "combo_box" } {
        ComboBox $linefr.$i -values "{1 Abc} {2 Cde} {3 Fgh}" \
		-width 8 -justify right -modifycmd "puts ModifyCombo" \
		-helptext "Select TitleBarEntry." \
		-entrybg grey60 -foreground black \
		-selectbackground $scene(butfgcolor) -selectforeground $scene(butbgcolor) \
		-highlightcolor red 	  -highlightthickness 0 \
		-insertbackground yellow	  -insertborderwidth 50
        pack $linefr.$i -side left -fill x -expand 1
      } elseif { $button_type == "time" } {
puts "TIME"
        set timefr [frame $optionfr.$side.time$i -bd 0 -bg $bg ]
        pack $timefr -side top -anchor n -fill x
        Label $timefr.get -text Get -bg $bg -relief raised -bd 1
        AddButtonColors $timefr.get
        foreach name { hour minute second } {
          Entry $timefr.$name -bg $bg -width 2 -bd 1 -relief sunken \
		-highlightcolor darkgrey -insertbackground black -highlightthickness 1  -highlightbackground $bg \
		-textvariable scene(values,$name,$type,$type_id,$host,$port) -justify right \
		-command "SetTimerValues $host $port $proc_name $type $type_id"
#  	-fg blue  -selectbackground red -insertborderwidth 20
          bind $timefr.$name <FocusIn> "set scene(EntryHasFocus,$host,$port) 1"
          bind $timefr.$name <FocusOut> "unset scene(EntryHasFocus,$host,$port)"
        }
        Label $timefr.set -text Set -relief raised -bg $bg -bd 1
        AddButtonColors $timefr.set
        foreach name { get hour minute second set } {
          pack $timefr.$name -side left -fill both -expand 1
        }
        bind $timefr.get <ButtonPress> "$timefr.get configure -relief sunken ; GetTimerValues $host $port $proc_name $type $type_id"
        bind $timefr.get <ButtonRelease> "$timefr.get configure -relief raised"
        bind $timefr.set <ButtonPress> "$timefr.get configure -relief sunken ; SetTimerValues $host $port $proc_name $type $type_id"
        bind $timefr.set <ButtonRelease> "$timefr.get configure -relief raised"
        GetTimerValues $host $port $proc_name $type $type_id
      } else continue
      incr i
    }
  }
}

proc GetTimerValuesLine { handle tag host port line args } {
  global scene
  set args [lindex $args 0]
  if { $args == "" || [llength $args] < 2 } return
  if {[string match "*Invalid*" $line]} return
  set type [lindex $args 0]
  set type_id [lindex $args 1]
  set scene(values,hour,$type,$type_id,$host,$port) [lindex $line 1]
  set scene(values,minute,$type,$type_id,$host,$port) [lindex $line 2]
  set scene(values,second,$type,$type_id,$host,$port) [lindex $line 3]
  return 0
}
  
proc SetTimerValues { host port proc_name type type_id } {
  global scene
  set command "tcl eval snowmix message \[$proc_name $type_id $scene(values,hour,$type,$type_id,$host,$port) $scene(values,minute,$type,$type_id,$host,$port) $scene(values,second,$type,$type_id,$host,$port)]"
  after 0 [list AddServerCommand snowmix $host $port $command GetOneLinePuts 1000]
}
  
proc GetTimerValues { host port proc_name type type_id } {
  set command "tcl eval snowmix message \[$proc_name $type_id]"
  puts "COMMAND $command"
  after 0 [list AddServerCommand snowmix $host $port $command GetTimerValuesLine 1000 $type $type_id]
}

  

proc OptionButtonPressed { host port scene_id frame_id button_type proc_name type type_id side } {
  global scene

  puts "OptionButtonPressed button $button_type proc $proc_name"
  if { $button_type == "button" } {
    after 0 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[$proc_name $type_id]" GetOneLine 1000]
  } elseif { $button_type == "frame_button" } {
    after 0 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[$proc_name $type_id $scene_id $frame_id $side]" GetOneLine 1000]
  } elseif { $button_type == "binary" } {
    after 0 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[$proc_name $type_id]" ToggleBinaryButton 1000 $scene_id $frame_id $proc_name $type $type_id]
  }
}

proc ToggleBinaryButton { handle tag host port line args } {
  global scene
  if {![llength $args]} return
  set args [lindex $args 0]
  set scene_id [lindex $args 0]
  set frame_id_id [lindex $args 1]
  set proc_name [lindex $args 2]
  set type [lindex $args 3]
  set type_id [lindex $args 4]
  set value [lindex $line 1]
  if { $value } { set new_value 0 } else { set new_value 1 }
  set command "tcl eval snowmix message \[$proc_name $type_id $new_value]"
puts $command
  after 0 [list AddServerCommand snowmix $host $port $command GetOneLinePuts 1000]
  return 0
}
  

proc CutFadeToFull { host port scene_id frame_id fade } {
  global scene

  set source $scene(source_front,$scene_id,$frame_id,$host,$port)
  puts "CutFadeToFull $host $port $scene_id $frame_id $fade source $source"
  if { $scene_id != 1} {
    SetComboBox $scene(combobox,source_front,1,1,$host,$port) $source
    FrameSourceSet $host $port 1 1 front
    after 250 [list Fade2Scene 1 $fade $host $port]
    UpdateOptionFrame $host $port 1 1 front
  }
}

proc ToggleSceneFrameText { host port scene_id frame_id type side } {
#puts "ToggleSceneFrameText scene $scene_id frame $frame_id type $type side $side"
    #puts $snowmix "tcl eval snowmix message \[SceneFrameText $scene_id $frame_id $type\]"
    after 0 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneFrameText $scene_id $frame_id $type\]" ToggleSceneFrameTextGetLine 1000 $scene_id $frame_id $type $side]
}

proc ToggleSceneFrameTextGetLine { handle tag host port line args } {
  set argc [llength $args]

  # Check that we can get the scene_id
  if {!$argc} {
    puts "ToggleSceneFrameTextGetLine wass called without args for scene_id and frame_id."
    return 0
  }
  # Check we did get the scene_id
  set args1 $args
  set args [lindex $args 0]
  set scene_id [lindex $args 0]
  set frame_id [lindex $args 1]
  set type [lindex $args 2]
  set side [lindex $args 3]
#puts "ToggleSceneFrameText scene $scene_id frame $frame_id type $type side $side <$line>"
  if {$side} { set i 1 } else { set i 2 }
  set val [lindex $line $i]
  if {$val} { set val 0 } else { set val 1 }
  #puts $snowmix "tcl eval SceneFrameText $scene_id $frame_id $type $side $val"
  after 0 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneFrameText $scene_id $frame_id $type $side $val]" GetOneLine 1000]
  return 0
}

proc AddSelectButton { host port scene_id frame_id title fr helptext args } {
  global scene

  if { $frame_id == "-" } {
    set key "$scene_id,$host,$port"
  } else {
    set key "$scene_id,$frame_id,$host,$port"
  }
    
  set selfr $scene(select_frame,$key)
  set bg [$selfr cget -bg]
  # Get the number this entry will get
  set no [llength $scene(select_list,$key)]
  set slaves [pack slaves $selfr]
  set cols [llength $slaves]
  set pos [expr $no % $cols]
  set frcol [lindex $slaves $pos]

#  if { $no % $cols } { set side r } else { set side l }
  set node [string tolower $title]
  set node [string map {. _} $node]
  Label $frcol.$node -text $title -bd 1 -bg $bg -relief raised -width 6 -helptext $helptext
  pack $frcol.$node -fill x -side top -expand 1
  bind $frcol.$node <ButtonPress> "SelectButtonPressed $host $port $scene_id $frame_id $no"
  AddButtonColors $frcol.$node
#  bind $frcol.$node <Enter> "$frcol.$node configure -bg grey90"
#  bind $frcol.$node <Leave> "$frcol.$node configure -bg $bg"

  if {[llength $args]} {
    # We have a raiscmd
    lappend scene(select_list,$key) [list $title $frcol.$node $fr [lindex $args 0]]
  } else {
    lappend scene(select_list,$key) [list $title $frcol.$node $fr]
  }
#  puts "SELECT <$scene(select_list,$key)>"
}

proc SelectButtonPressed { host port scene_id frame_id no } {
  global scene
#puts "SelectButtonPressed scene $scene_id frame $frame_id np $no"
  focus .
  if { $frame_id == "-" } {
    set key "$scene_id,$host,$port"
  } else {
    set key "$scene_id,$frame_id,$host,$port"
  }
#  puts "SelectButtonPressed $host $port $scene_id $frame_id $no"
  set framebot $scene(display_frame,$key)
  set current_set [pack slaves $framebot]
  if { $current_set != "" } {
    foreach current $current_set {
      pack forget $current
    }
  }
  set n 0
  foreach but_set $scene(select_list,$key) {
    #set title [lindex $but_set 0]
    set node [lindex $but_set 1]
    if { $no != $n } {
      set relief raised
    } else {
      set relief sunken
      set displayfr_set [lindex $but_set 2]
      foreach displayfr $displayfr_set {
        pack $displayfr -side top -fill x -expand 0 -anchor n
      }
      set raisecmd [lindex $but_set 3]
      if { $raisecmd != "" } { $raisecmd $host $port $scene_id $frame_id  }
    }
    $node configure -relief $relief
    incr n
  }
}

proc MakeLabelFrame { host port scene_id frame_id parent } {

  set bg [$parent cget -bg]
  # Make a frame to hold the source frame
  set fr [frame $parent.lfr -bd 0 -bg $bg]

  # Make a top label
  Label $fr.label -text "Frame Labels Toggles" -relief raised -bd 1 -bg grey20 -fg grey70  -pady 0
  pack $fr.label -side top -fill x
  # Make a left and a right column
  set frl [frame $fr.l -bd 0 -bg $bg]
  set frr [frame $fr.r -bd 0 -bg $bg]
#  set bfront [frame $frl.b -bd 1 -bg $bg]
#  set bback [frame $frr.b -bd 1 -bg $bg]
  pack $frl $frr -side left -expand 1 -fill both
#  Label $frl.l -text Front -relief raised -bd 1 -bg darkgrey -pady 0
#  Label $frr.l -text "Back " -relief raised -bd 1 -bg darkgrey -pady 0
#  pack $frl.l $frr.l -side top -anchor n -fill x -expand 1

  set n 1
  foreach type {name time} {
    set first [string toupper [string index $type 0]]
    set name [string replace $type 0 0 $first]
    Label $frl.l$type -text $name -relief raised -bd 1 -bg $bg -helptext "Toggle $type label for front frame"
    Label $frr.l$type -text $name -relief raised -bd 1 -bg $bg -helptext "Toggle $type label for back frame"
    pack $frl.l$type $frr.l$type -side top -fill x -expand 1
    AddButtonColors $frl.l$type
    AddButtonColors $frr.l$type

    bind $frl.l$type <ButtonPress> "$frl.l$type configure -relief sunken ; ToggleSceneFrameText $host $port $scene_id $frame_id $type 1"
    bind $frl.l$type <ButtonRelease> "$frl.l$type configure -relief raised"

    bind $frr.l$type <ButtonPress> "$frr.l$type configure -relief sunken ; ToggleSceneFrameText $host $port $scene_id $frame_id $type 0"
    bind $frr.l$type <ButtonRelease> "$frr.l$type configure -relief raised"
  }
  return $fr
}

proc MakeTimingFrame { host port scene_id frame_id parent } {
  set bg [$parent cget -bg]
  # Make a frame to hold the timing frame
  set fr [frame $parent.tfr -bd 0 -bg $bg]
  Label $fr.l -text "Fade Time for Frame" -bg grey20 -fg grey70 -bd 1 -pady 0 -relief raised
  pack $fr.l -side top -expand 1 -fill x
  return $fr
}
proc MakeOptionFrame { host port scene_id frame_id parent } {
  set bg [$parent cget -bg]
  # Make a frame to hold the option frame
  set fr [frame $parent.ofr -bd 0 -bg $bg]
  set front [frame $fr.front -bd 1 -bg $bg]
  set back [frame $fr.back -bd 1 -bg $bg]
  pack $front $back -side top -fill x
  #pack $fr.l -side top -expand 1 -fill x
  return $fr
}

proc MakeComboFrame { host port scene_id frame_id parent } {
  global scene

  set bg [$parent cget -bg]
  # Make a frame to hold the combo frame
  set fr [frame $parent.combofr -bd 0 -bg $bg]
  set frl [frame $fr.l -bd 0 -bg $bg]
  set frr [frame $fr.r -bd 0 -bg $bg]
#  set bfront [frame $frl.b -bd 1 -bg $bg]
#  set bback [frame $frr.b -bd 1 -bg $bg]
  pack $frl $frr -side left -expand 1 -fill both
  Label $frl.l -text Front -relief raised -bd 1 -bg grey20 -fg grey70 -pady 0
  Label $frr.l -text "Back " -relief raised -bd 1 -bg grey20 -fg grey70 -pady 0
  pack $frl.l $frr.l -side top -anchor n -fill x -expand 1

  set frc $frl
  foreach side {front back} {
    set scene(combobox,source_$side,$scene_id,$frame_id,$host,$port) \
        [ComboBox $frc.source_$side -values "$scene(source_list,$host,$port)" \
         -width 8 -justify right -modifycmd "FrameSourceSet $host $port $scene_id $frame_id $side ; UpdateOptionFrame $host $port $scene_id $frame_id $side" \
        -helptext "Select source for $side." \
	-entrybg grey60 -foreground black \
	-selectbackground $scene(butfgcolor) -selectforeground $scene(butbgcolor) \
  -highlightcolor red 	  -highlightthickness 0 \
  -insertbackground yellow	  -insertborderwidth 50 ]

#PMM
#	-highlightbackground orange \

    SetComboBox $frc.source_$side $scene(source_$side,$scene_id,$frame_id,$host,$port)
    pack $frc.source_$side -side top -fill x -expand 1
    set frc $frr
  }
  return $fr
}

proc MakeSourcesFrame { host port scene_id frame_id type parent } {
  global scene

  set bg [$parent cget -bg]
  # Make a frame to hold the source frame
  set fr [frame $parent.$type -bd 0 -bg $bg]
  set frl [frame $fr.l -bd 0 -bg $bg]
  set frr [frame $fr.r -bd 0 -bg $bg]
  pack $frl $frr -side left -expand 1 -fill both

#puts "MakeSourceFrame $scene_id $frame_id $type <$scene(source_list,$host,$port)>"
  set frc $frl
  foreach side {front back} {
    set i 0
    foreach source $scene(source_list,$host,$port) {
      if {[string match "-" $source]} continue
      regexp {([^ ]+)\ +([0-9]+)} $source match first id
      if {$type == "feed"} {
        if {$first == "feed"} { set first "F" } else continue
      } elseif {$type == "image"} {
        if {$first == "image"} { set first "I" } else continue
      } elseif {$type == "other"} {
        if {$first == "image" || $first == "feed"} continue
      }
      if { !($i % 3) } {
        set n [expr $i / 3]
        set frb [frame $frc.f$n -bd 0 -bg $bg]
        pack $frb -side top -fill x -expand 1
      }
      Label $frb.b$i -text "$first$id" -helptext "Set Source\nto $source" -relief raised -bd 1 -bg $bg
      #bind $frb.b$i <ButtonPress> "$frb.b$i configure -relief sunken ; SetComboBox $scene(combobox,source_$side,$scene_id,$frame_id,$host,$port) \"$source\" ; FrameSourceSet $host $port $scene_id $frame_id $side"
      bind $frb.b$i <ButtonPress> "$frb.b$i configure -relief sunken ; SourceChange $host $port $scene_id $frame_id $side \"$source\" ; after 500 [list $frb.b$i configure -relief raised]"
      #bind $frb.b$i <ButtonRelease> "$frb.b$i configure -relief raised"
      pack $frb.b$i -side left -fill x -expand 1
      AddButtonColors $frb.b$i
#      puts "BUTTON scene $scene_id frame $frame_id source <$first> <$id>"
      incr i
    }
    set frc $frr
  }
  return $fr
}

proc SourceChange { host port scene_id frame_id side source } {
  global scene

  set type [lindex $source 0]
  set src_id [lindex $source 1]
  if { $type == "feed" && $src_id > 0 } {
    set state $scene(feed,state,$host,$port,$src_id)
    if { $state != "RUNNING" } {
      catch { destroy .dialog }
      set reply [tk_dialog .dialog "Sourcing Scene $scene_id frame $frame_id side $side." "Setting the source to feed $src_id, the feed is $state. Do you want to continue?" questhead 1 Yes No]
      if { $reply } return
    }
  }
  SetComboBox $scene(combobox,source_$side,$scene_id,$frame_id,$host,$port) $source
  FrameSourceSet $host $port $scene_id $frame_id $side
  UpdateOptionFrame $host $port $scene_id $frame_id $side
}

proc FrameSourceSet { host port scene_id frame_id side } {
  global scene
  set source [$scene(combobox,source_$side,$scene_id,$frame_id,$host,$port) get]
  set scene(source_$side,$scene_id,$frame_id,$host,$port) $source
puts "FrameSourceSet 1 : source is <$source> side $side"
#  if {[string match "-" $source]} {
#    return
#  }
  if {[string match front $side]} {
    set front 1
    set active $scene(active,$scene_id,$frame_id,$host,$port)
  } else {
    set front 0
    set active 0
  }
  if {[string match "-" $source]} {
    set command "tcl eval snowmix message \[SceneSetFrameSource $scene_id $frame_id - 0 $front $active]"
  } else {
    set source_type [lindex $source 0]
    set id [lindex $source 1]
    set command "tcl eval snowmix message \[SceneSetFrameSource $scene_id $frame_id $source_type $id $front $active]"
  }
#puts stderr "FrameSourceSet <$command>"
  after 0 [list AddServerCommand snowmix $host $port $command GetOneLine 1000]
  # FillButtonFrame $scene_id $frame_id $side 1
#  UpdateOptionFrame $host $port $scene_id $frame_id $side
  focus .
}

proc FadeFrames { host port scene_id frame_id other_frame_id fade } {
  global scene
puts "FadeFrames scene $scene_id frame $frame_id other frame $other_frame_id"
  set endkey1 "$scene_id,$frame_id,$host,$port"
  set endkey2 "$scene_id,$other_frame_id,$host,$port"
  set back1 $scene(source_back,$endkey1)
  set back2 $scene(source_back,$endkey2)

  # First we set source_back of first frame to source front of second frame and
  # source_back of second frame to source front of first frame
#  set scene(source_back,$endkey1) $scene(source_front,$endkey2)
#  set scene(source_back,$endkey2) $scene(source_front,$endkey1)
  # and then we update the combobox
#  SetComboBox $scene(combobox,source_back,$endkey1) $scene(source_back,$endkey2)
#  SetComboBox $scene(combobox,source_back,$endkey2) $scene(source_back,$endkey1)

  # We set back of combobox 1 to front of 2 and back of combobox 2 to front of 1
  SetComboBox $scene(combobox,source_back,$endkey1) $scene(source_front,$endkey2)
  SetComboBox $scene(combobox,source_back,$endkey2) $scene(source_front,$endkey1)
  # And we tell Snowmix to set the source of back of frame 1 and 2.
  # FrameSourceSet will get the settings from the comboboxes and update internals in this program
  FrameSourceSet $host $port $scene_id $frame_id back
  FrameSourceSet $host $port $scene_id $other_frame_id back

#puts "Fading scene $scene_id frame $frame_id to $scene(source_back,$endkey1) and frame $other_frame_id to $scene(source_back,$endkey2)"

  # And then we tell Snowmix to fade/cut from front to back and back to front for both frames
  FrameFadeFrontBack $host $port $scene_id $frame_id $fade
  FrameFadeFrontBack $host $port $scene_id $other_frame_id $fade

  # Now we have to restore the back source of both frames
#  set scene(source_back,$endkey1) $back1
#  set scene(source_back,$endkey2) $back2
  # Then we restore the back comboboxes and tell Snowmix to update this from the comboboxes and
  # update internals accodingly
  SetComboBox $scene(combobox,source_back,$endkey1) $back1
  SetComboBox $scene(combobox,source_back,$endkey2) $back2
  FrameSourceSet $host $port $scene_id $frame_id back
  FrameSourceSet $host $port $scene_id $other_frame_id back

#  UpdateOptionFrame $host $port $scene_id $frame_id front
#  UpdateOptionFrame $host $port $scene_id $other_frame_id front
  return
}

proc FrameFadeFrontBack { host port scene_id frame_id fade } {
  global scene
#puts "FadeFront2Back for scene $scene_id frame $frame_id $scene(source_back,$scene_id,$frame_id,$host,$port) <-> $scene(shape_back,$scene_id,$frame_id,$host,$port)"

  # Tell Snowmix to cut/fade frame front/back
  set command "tcl eval SceneFrameBack2Front $scene_id $frame_id $fade"
  after 0 [list AddServerCommand snowmix $host $port $command GetOneLine 1000]

  set endkey "$scene_id,$frame_id,$host,$port"
  set source $scene(source_front,$endkey)
  set scene(source_front,$endkey) $scene(source_back,$endkey)
  set scene(source_back,$endkey) $source
  set shape $scene(shape_front,$endkey)
  set scene(shape_front,$endkey) $scene(shape_back,$endkey)
  set scene(shape_back,$endkey) $shape
  set place $scene(place_front,$endkey)
  set scene(place_front,$endkey) $scene(place_back,$endkey)
  set scene(place_back,$endkey) $place
  foreach side {front back} {
    SetComboBox $scene(combobox,source_$side,$endkey) $scene(source_$side,$endkey)
    #SetComboBox $scene(combobox,source_back,$endkey) $scene(source_back,$endkey)
#    FillButtonFrame $scene_id $frame_id $side 1
   # UpdateOptionFrame $host $port $scene_id $frame_id $side
  }
  return
}


proc FrameActivate { host port scene_id frame_id fade } {
  global scene snowmix

  if {$scene(active,$scene_id,$frame_id,$host,$port) > 0} {
    if {$fade > 0} {
      set command "SceneFrameFade $scene_id $frame_id 0\n"
    } else {
      set command "SceneSetFrameActive $scene_id $frame_id 0 0\n"
    }
    set scene(active,$scene_id,$frame_id,$host,$port) 0
  } else {
    if {$fade > 0} {
      set command "SceneFrameFade $scene_id $frame_id 1\n"
    } else {
      if {!$fade} {
        set command "SceneSetFrameActive $scene_id $frame_id 1 0\n"
      } else {
        set command "SceneSetFrameActive $scene_id $frame_id -1 0\n"
      }
    }
    set scene(active,$scene_id,$frame_id,$host,$port) 1
  }
  SetActivateColor $scene(activatebutton,$scene_id,$frame_id,$host,$port) $scene(active,$scene_id,$frame_id,$host,$port)
  after 0 [list AddServerCommand snowmix $host $port "tcl eval $command" GetOneLine 1000]
  return
}


proc BuildSceneTab { host port scene_id } {
  global scene

  set scene_fr [$scene(nb,$host,$port) getframe scene$scene_id]
  set bg [$scene_fr cget -bg]
  $scene_fr configure -bg darkgreen -bd 0
  $scene_fr configure -bg $bg -bd 0
  set top_fr [frame $scene_fr.top -bd 0 -bg $bg]
  set bot_fr [frame $scene_fr.bot -bd 0 -bg $bg]
  pack $top_fr $bot_fr -side top -expand 0 -anchor w

  # bot_fr is the main frame holding the entire scene and scene_frames
  # bot_fr.scene is the frame holding the scene specific stuff
  # bot_fr.frame1 is holding the frame specific stuff for scene frame 1
  # bot_fr.frame2 is holding the frame specific stuff for scene frame 2 etc.
  set scene(scenefr,$scene_id,$host,$port) $bot_fr
  frame $bot_fr.scene -relief raised -bg $bg -bd 1 -padx 4 -pady 4
  pack $bot_fr.scene -side left -expand 1 -fill y -anchor n

  set sfr $bot_fr.scene
  set scene(activatebutton,$scene_id,$host,$port)		\
	[Label $sfr.activebut   				\
        -text "Scene $scene_id"        				\
        -pady 2 -padx 5 -bd 1 -relief raised                            \
	-helptext "Activate or deactivate\nscene $scene_id."	\
	-bg $bg -width 37]
  SetActivateColor $scene(activatebutton,$scene_id,$host,$port) $scene(scene,active,$scene_id,$host,$port)
  bind $scene(activatebutton,$scene_id,$host,$port) <ButtonPress> \
	"$scene(activatebutton,$scene_id,$host,$port) configure -relief sunken ; SceneActivate $scene_id -1 $host $port"
  bind $scene(activatebutton,$scene_id,$host,$port) <ButtonRelease> \
	"$scene(activatebutton,$scene_id,$host,$port) configure -relief raised"
  pack $scene(activatebutton,$scene_id,$host,$port) -side top -fill x -expand 0 -anchor n

  # Next is a column for 4 frames
  set 4col [frame $sfr.4col   -bd 0 -bg $bg]
  set frl  [frame $4col.left  -bd 1 -bg $bg]
  set frm  [frame $4col.mid   -bd 1 -bg $bg]
  set frrb [frame $4col.rbg   -bd 1 -bg $bg]
  set frrt [frame $4col.rtext -bd 1 -bg $bg]
  pack $4col -side top  -fill x -expand 0 -anchor n
  pack $frl  -side left -fill x           -anchor n
  pack $frm  -side left -fill x -expand 1 -anchor n
  pack $frrb $frrt -side left -anchor n -fill x

  # This is the frame to hold the bottom frame that change content
  set framebot [frame $sfr.botfr   -bd 0 -bg $bg]
  pack $framebot -side top -fill x -expand 0 -anchor n
  
  # We put the scene alpha scale in the left frame
  set l 150
  set w 12
  set troughcolor #002000
  #set scene(scene,alpha,$scene_id,$host,$port) 0
  Label $frl.l -text S -relief raised -helptext "Scene Alpha\nBlending." -bg $bg
  AddButtonColors $frl.l
  scale $frl.scale -from 100 -to 0 -orient vertical -showval 0 \
      -tickinterval 0 -length $l -width $w -repeatdelay 10 -repeatinterval 10 \
      -troughcolor $troughcolor -variable scene(alpha,$scene_id,$host,$port) \
      -command "AlphaSliderChange $host $port $scene_id -1 scene" -bg $bg
  AddButtonColors $frl.scale
  DynamicHelp::add $frl.scale -text "Scene Alpha Blending.\nPress and hold top or\nbottom for a steady fade."

  set scene(alphabg,link,$scene_id,$host,$port) 0
  set scene(alphatext,link,$scene_id,$host,$port) 0
  # We put the alpha background and alpha text elements scale in the right
  set scene(alphabg,link,button,$scene_id,$host,$port) [Label $frrb.l -text B -relief raised \
        -helptext "Background Alpha\nBlending. Link\nto scene alpha" -bd 2 -bg $bg]
  bind $scene(alphabg,link,button,$scene_id,$host,$port) <ButtonPress> \
          "AlphaLinkButton $host $port $scene_id -2"
  AddButtonColors $scene(alphabg,link,button,$scene_id,$host,$port)
  set scene(alphabg,scale,button,$scene_id,$host,$port) \
	[scale $frrb.scale -from 100 -to 0 -orient vertical -showval 0\
        -tickinterval 0 -length $l -width $w -repeatdelay 10 -repeatinterval 10 \
        -troughcolor $troughcolor -variable scene(alphabg,$scene_id,$host,$port) \
        -command "AlphaSliderChange $host $port $scene_id -2 scene" -state disabled -bg $bg]
  AddButtonColors $scene(alphabg,scale,button,$scene_id,$host,$port)
  DynamicHelp::add $frrb.scale -text "Background Alpha Blending.\nPress and hold top or\nbottom for a steady fade."
  set scene(alphatext,link,button,$scene_id,$host,$port) [Label $frrt.l -text T -relief raised \
        -helptext "Text Elements\nAlpha Blending. Link\nto scene alpha." -bd 2 -bg $bg]
  bind $scene(alphatext,link,button,$scene_id,$host,$port) <ButtonPress> \
          "AlphaLinkButton $host $port $scene_id -3"
  AddButtonColors $scene(alphatext,link,button,$scene_id,$host,$port)
  scale $frrt.scale -from 100 -to 0 -orient vertical -showval 0\
        -tickinterval 0 -length $l -width $w -repeatdelay 10 -repeatinterval 10 \
        -troughcolor $troughcolor -variable scene(alphatext,$scene_id,$host,$port) \
        -command "AlphaSliderChange $host $port $scene_id -3 scene" -bg $bg
  AddButtonColors $frrt.scale
  DynamicHelp::add $frrt.scale -text "Text Element Alpha Blending.\nPress and hold top or\nbottom for a steady fade."
  pack $frl.l $frl.scale $frrb.l $frrb.scale $frrt.l $frrt.scale -side top

  # Scene Cut and Fade Button
  foreach scene_set { {cutbutton "Scene Cut" "Cut Scene In/Out" cut 0} {fadebutton "Scene Fade" "Fade Scene In/Out" fade 1} } {
    set button   [lindex $scene_set 0]
    set text     [lindex $scene_set 1]
    set helptext [lindex $scene_set 2]
    set node     [lindex $scene_set 3]
    set cut      [lindex $scene_set 4]
    set scene($button,$scene_id,$host,$port) [Label $frm.$node -pady 1 -text $text -helptext $helptext -relief raised -bd 1]
    bind $scene($button,$scene_id,$host,$port) <ButtonPress> \
	"$scene($button,$scene_id,$host,$port) configure -relief sunken ; SceneCutFade $scene_id $cut $host $port"
    bind $scene($button,$scene_id,$host,$port) <ButtonRelease> \
	"$scene($button,$scene_id,$host,$port) configure -relief raised"
    AddButtonColors $scene($button,$scene_id,$host,$port)
  pack $frm.$node -side top -fill x -anchor n
  }
#  set scene(fadebutton,$scene_id,$host,$port) [Label $frm.fade -pady 1 \
#    -text "Scene Fade" -helptext "Fade Scene In/Out" -relief raised -bd 1 -bg $bg]
#  bind $scene(fadebutton,$scene_id,$host,$port) <ButtonPress> \
#	"$scene(fadebutton,$scene_id,$host,$port) configure -relief sunken ; SceneCutFade $scene_id 1 $host $port"
#  bind $scene(fadebutton,$scene_id,$host,$port) <ButtonRelease> \
#	"$scene(fadebutton,$scene_id,$host,$port) configure -relief raised"

  frame $frm.spacer1 -bg $scene(bgcolor) -relief flat -height 3 
  label $frm.cutlabel -text "Cut to Scene" -relief raised -bg grey20 -fg grey70  -bd 1
  set scene(cutframe,$scene_id,$host,$port) [frame $frm.cutfr -bd 0 -pady 0 -bg $bg]
  frame $frm.spacer2 -bg $scene(bgcolor) -relief flat -height 3
  label $frm.fadelabel -text "Fade to Scene" -relief raised -bg grey20 -fg grey70 -bd 1
  set scene(fadeframe,$scene_id,$host,$port) [frame $frm.fadefr -bd 0 -pady 0 -bg $bg]
  frame $frm.spacer3 -bg $scene(bgcolor) -relief flat -height 3

  # This frame is for holding buttons to select subframe to display
  frame $frm.sel -bg $bg -bd 0

  pack $frm.spacer1 $frm.cutlabel $frm.cutfr $frm.spacer2 $frm.fadelabel $frm.fadefr $frm.spacer3 $frm.sel \
	 -side top -fill x -anchor n

  # Create the buttons to cut/fade to other scenes
  foreach id $scene(scenelist,$host,$port) {
    AddSceneButtons $scene_id $id $host $port
    AddSceneButtons $id $scene_id $host $port
  }

  # This is a list of select buttons and their frame to display
  set scene(select_list,$scene_id,$host,$port) ""
  # This frame is for holding buttons to select subframe to display
  set scene(select_frame,$scene_id,$host,$port) $frm.sel
  set scene(display_frame,$scene_id,$host,$port) $framebot
  # We create a left and right and will fill in buttons left to right and then next line left to right
  set sell [frame $frm.sel.l -bd 0 -bg $bg]
  #set selm [frame $frm.sel.m -bd 0 -bg $bg]
  set selr [frame $frm.sel.r -bd 0 -bg $bg]
  pack $sell $selr -side left -fill x -expand 1
#puts "SLAVES $scene_id : [pack slaves $framebot]"
#  AddSelectButton $host $port $scene_id "-" "Place" $srcfr "Display Sources\nFront and Back\n+quickset for feeds"

  # Canvas Frame
  set canvasfr [frame $framebot.place -bd 0 -bg $bg]
  set scene(canvasframe,$scene_id,$host,$port) $canvasfr
  Label $canvasfr.label -text "Frame Placement for Scene" -bg grey20 -fg grey70 -bd 1 -pady 0 -relief raised
  pack $canvasfr.label -side top -fill x
  AddSelectButton $host $port $scene_id "-" "Placing" $canvasfr "Display frame placing for scene."

  # Background frame. Scene may have background. We discover that later
  set backgrfr [frame $framebot.backfr -bd 0 -pady 0 -padx 0 -bg $bg]
  set scene(backgroundframe,$scene_id,$host,$port) $backgrfr
  AddSelectButton $host $port $scene_id "-" "Backgr" $backgrfr "Display background selection for scene."

  # Timing frame
  set timingfr [frame $framebot.timing -bd 0 -bg $bg]
  Label $timingfr.label -text "Fade Timings for Scene" -bg grey20 -fg grey70 -bd 1 -pady 0 -relief raised
  pack $timingfr.label -side top -fill x
  set scene(timingframe,$scene_id,$host,$port) $timingfr
  AddSelectButton $host $port $scene_id "-" "Timing" $timingfr "Display timing options for scene."

  # Overlay Frame
  set overlayfr [frame $framebot.overlay -bd 0 -bg $bg -width 200]
  Label $overlayfr.label1 -text "Overlay Settings for Scene" -bg grey20 -fg grey70 -bd 1 -pady 0 -relief raised
  pack $overlayfr.label1 -side top -fill x
  set scene(overlayframe,$scene_id,$host,$port) $overlayfr
  AddSelectButton $host $port $scene_id "-" "Overlay" $overlayfr "Display Overlay options for scene."

  # Select the first button to be pressed
  SelectButtonPressed $host $port $scene_id - 0

#  set bfr [frame $sfr.buttons -bd 0 -bg $bg]
#  pack $bfr -side top -fill x

  lappend scene(scenelist,$host,$port) $scene_id
 
  after 200 [list AddServerCommand snowmix $host $port "tcl eval SceneList $scene_id" GetSceneList 1000]

  lappend scene(sceneoverlayorder,scene,$host,$port) $scene_id
  after 250 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneOverlayOrder $scene_id]" GetSceneOverlayOrderLine 1000]
  lappend scene(sceneoverlaytext,scene,$host,$port) $scene_id
  after 300 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneSetOverlayText $scene_id]" GetSceneOverlayTextLine 1000]
  #lappend scene(sceneoverlaytiming,scene,$host,$port) "$scene_id -"
  after 350 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneFadeSpeed all $scene_id - ]" GetSceneOverlayTimingLine 1000 $scene_id -]
}

proc AlphaLinkButton {host port scene_id frame_id args} {
  global snowmix scene

  puts "AlphaLinkButton $host $port $scene_id $frame_id $args"
  if {![info exist scene(name,$scene_id,$host,$port)] || $frame_id == -1 || $frame_id < -3 } {
    puts stderr "AlphaLinkButton will not do anything with <$host $port $scene_id $frame_id $args>"
    return
  }
  if {$frame_id > -1} {
    set side [lindex $args 0]
    set front [$scene(alphafront,link,button,$scene_id,$frame_id,$host,$port) cget -relief]
    set back [$scene(alphaback,link,button,$scene_id,$frame_id,$host,$port) cget -relief]

puts " - $front $back"
    if {[string match sunken $front]} { set front 1 } else { set front 0 }
    if {[string match sunken $back]} { set back 1 } else { set back 0 }
puts " - $front $back"

    if {$side} {
      # Front was pressed
      if { $front } {
        # Front was sunken and must now be raised so linking stops
        #puts $snowmix "tcl eval SceneAlphaLink $scene_id $frame_id 0"
        set command "SceneAlphaLink $scene_id $frame_id 0"
      } else {
        # Front was raised and must now be sunken to start linking
        if {$back} {
          #puts $snowmix "tcl eval SceneAlphaLink $scene_id $frame_id -1"
          set command "SceneAlphaLink $scene_id $frame_id -1"
        } else {
          #puts $snowmix "tcl eval SceneAlphaLink $scene_id $frame_id 1"
          set command "SceneAlphaLink $scene_id $frame_id 1"
        }
      }
    } else {
      # back was pressed
      if { $back } {
        # Back is sunken and must be raised to stop inverse
        if {$front} {
          #puts $snowmix "tcl eval SceneAlphaLink $scene_id $frame_id 1"
          set command "SceneAlphaLink $scene_id $frame_id 1"
        } else {
          # puts $snowmix "tcl eval SceneAlphaLink $scene_id $frame_id 0"
          set command "SceneAlphaLink $scene_id $frame_id 0"
        }
      } else {
        # Back is raised and must be sunken to inverse
        #puts $snowmix "tcl eval SceneAlphaLink $scene_id $frame_id -1"
        set command "SceneAlphaLink $scene_id $frame_id -1"
      }
    }
    after 0 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[$command]" GetOneLine 1000]
    return
  } elseif {$frame_id == -2} {
    if {$scene(alphabg,link,$scene_id,$host,$port)} { set val 0 } else { set val 1 }
  } else {
    if {$scene(alphatext,link,$scene_id,$host,$port)} { set val 0 } else { set val 1 }
  }
  after 0 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneAlphaLink $scene_id $frame_id $val]" GetOneLine 1000]
  return
}


# Function to create Background image selection for scene
proc MakeSceneBackgroundImageSelection { host port scene_id } {
  global scene

  if {![info exist scene(backgroundframe,$scene_id,$host,$port)]} {
    puts stderr, "No frame for background image selection for scene $scene_id $host:$port"
    return
  }
  set fr $scene(backgroundframe,$scene_id,$host,$port)
  # Scene background Selection
  if {[info exist scene(back,image,$scene_id,$host,$port)]} {
    Label $fr.background -text "Background Image" -pady -1 -relief raised -bg grey20 -fg grey70 -bd 1
    set scene(combobox,bgimage,$scene_id,$host,$port) [ComboBox $fr.bgimage  -values \
      $scene(image,list,$host,$port) -width 30 -modifycmd "BackgroundImageChange $host $port $scene_id"]
    SetComboBox $fr.bgimage "$scene(back,image,$scene_id,$host,$port) *"
    pack $fr.background $fr.bgimage -side top -fill x -anchor w
  } else {
puts "HAVE NO scene(back,image,$scene_id,$host,$port)"
  }
}

# Send the order for changing background image for scene
proc BackgroundImageChange {host port scene_id} {
  global scene
  set image_id [lindex [$scene(combobox,bgimage,$scene_id,$host,$port) get] 0]
  set command "tcl eval SceneSetBackground $scene_id $image_id"
  after 0 [list AddServerCommand snowmix $host $port $command GetOneLine 1000]
  focus .
  return
}

# Function add buttons to cut and fade to other scenes
proc AddSceneButtons { add_id scene_id host port } {
  global scene
  set bg [$scene(cutframe,$scene_id,$host,$port) cget -bg]
#puts stderr "AddSceneButtons add $add_id scene $scene_id host $host port $port"
  set cutscene [Label $scene(cutframe,$scene_id,$host,$port).cut$add_id \
	-text $add_id -bd 1 -padx 0 -bg $bg -relief raised -helptext "Cut to Scene $add_id"]
  pack $cutscene -side left -expand 1 -fill x
  set fadescene [Label $scene(fadeframe,$scene_id,$host,$port).fade$add_id \
	-text $add_id -bd 1 -padx 0 -bg $bg -relief raised -helptext "Fade to Scene $add_id"]
  pack $fadescene -side left -expand 1 -fill x

  bind $cutscene <ButtonPress> "$cutscene configure -relief sunken ; Fade2Scene $add_id 0 $host $port"
  bind $cutscene <ButtonRelease> "$cutscene configure -relief raised"
  bind $fadescene <ButtonPress> "$fadescene configure -relief sunken ; Fade2Scene $add_id 1 $host $port"
  bind $fadescene <ButtonRelease> "$fadescene configure -relief raised"
  AddButtonColors $cutscene
  AddButtonColors $fadescene
}

# Function to draw FrameLayout in a canvas for a scene
proc SceneFrameLayout { scene_id host port} {
  global scene
  if {![info exist scene(canvasframe,$scene_id,$host,$port)]} {
    puts stderr "Could not create frame layout in canvas for scene $scene_id. Missing the frame to do it."
    return
  }
  set cfr $scene(canvasframe,$scene_id,$host,$port)
  set canvas [canvas $cfr.canvas -bg black \
    -width [expr $scene(width,$scene_id,$host,$port) / 5] \
    -height [expr $scene(height,$scene_id,$host,$port) / 5] -bd 0 -highlightthickness 0]
  pack $canvas -side top -fill x -anchor w
  set frame_id 0
  while { $frame_id <= $scene(maxframe_id,$scene_id,$host,$port) } {
    if {![info exist scene(width,$scene_id,$frame_id,$host,$port)]} {
      incr frame_id
      continue
    }
    set scene(canvas,scale,$scene_id,$frame_id,$host,$port) 5.0
    set scale $scene(canvas,scale,$scene_id,$frame_id,$host,$port)
    set xoff [expr $scene(xoff,$scene_id,$frame_id,$host,$port) / $scale]
    set yoff [expr $scene(yoff,$scene_id,$frame_id,$host,$port) / $scale]
    set x2 [expr ($scene(width,$scene_id,$frame_id,$host,$port) + $scene(xoff,$scene_id,$frame_id,$host,$port)) / $scale]
    set y2 [expr ($scene(height,$scene_id,$frame_id,$host,$port) + $scene(yoff,$scene_id,$frame_id,$host,$port)) / $scale]
    $canvas create rect $xoff $yoff $x2 $y2 -outline grey -fill lightblue -width 1 -tag frame_$frame_id
    set x [expr ($xoff + $x2)/2]
    set y [expr ($yoff + $y2)/2]
    set scene(canvas,$scene_id,$frame_id,$host,$port) $canvas
    set scene(canvaspress,$scene_id,$frame_id,$host,$port) 0
    set scene(canvasmotion,scene,$host,$port) ""
    $canvas create text $x $y -text "Frame $frame_id" -fill black -tag frame_$frame_id
    $canvas bind frame_$frame_id <ButtonPress> "scene_canvas_press $host $port $scene_id $frame_id %b press %x %y"
    $canvas bind frame_$frame_id <ButtonRelease> "scene_canvas_press $host $port $scene_id $frame_id %b release %x %y"
    $canvas bind frame_$frame_id <Motion> "scene_canvas_press $host $port $scene_id $frame_id %b motion %x %y"
    incr frame_id
  }
  if {[info exist scene(clocks,$scene_id,$host,$port)]} {
    set scale 5.0
    foreach clock_id $scene(clocks,$scene_id,$host,$port) {
      set x [expr $scene(clock_xoff,$scene_id,$clock_id,$host,$port) / $scale -40]
      set y [expr $scene(clock_yoff,$scene_id,$clock_id,$host,$port) / $scale -4]
      set x2 [expr $x + 70]
      set y2 [expr $y + 14]
      $canvas create rect $x $y $x2 $y2 -outline grey -fill lightblue -width 1 -tag clock_$clock_id
    }
  }
}

proc scene_canvas_press {host port scene_id frame_id but type x y } {
  global scene
  if {$scene(canvaspress,$scene_id,$frame_id,$host,$port) && [string match motion $type]} {
    if { [llength $scene(canvasmotion,scene,$host,$port)] } {
      puts stderr "Canvas motion $scene_id $host:$port is busy. Skipping $x,$y"
      return
    }
    set dx [expr ($x - $scene(canvas,motion_x,$scene_id,$frame_id,$host,$port))/5.0]
    set dy [expr ($y - $scene(canvas,motion_y,$scene_id,$frame_id,$host,$port))/5.0]
    if {$dx != 0 || $dy != 0} {
      $scene(canvas,$scene_id,$frame_id,$host,$port) move frame_$frame_id $dx $dy
      set coors [$scene(canvas,$scene_id,$frame_id,$host,$port) coords frame_$frame_id]
      set xoff [lindex $coors 0]
      set yoff [lindex $coors 1]
      set x2 [lindex $coors 2]
      set y2 [lindex $coors 3]
      set width [expr round(5.0 * ($x2 - $xoff))]
      set height [expr round(5.0 * ($y2 - $yoff))]
      set xoff [expr round(5.0 * $xoff)]
      set yoff [expr round(5.0 * $yoff)]
      #puts "offset $xoff $yoff $width $height"
      lappend scene(canvasmotion,scene,$host,$port) $scene_id
      set command "tcl eval ScenePlaceFrame $scene_id $frame_id $xoff $yoff $width $height"
      after 0 [list AddServerCommand snowmix $host $port $command GetScenePlaceFrame 1000]
      foreach var {width height xoff yoff} {
        eval "set value $$var"
        set scene($var,$scene_id,$frame_id,$host,$port) $value
      }
    }
    set scene(canvas,motion_x,$scene_id,$frame_id,$host,$port) $x
    set scene(canvas,motion_y,$scene_id,$frame_id,$host,$port) $y
  } elseif {[string match press $type] && $but == 1} {
    set scene(canvaspress,$scene_id,$frame_id,$host,$port) 1
    set scene(canvas,motion_x,$scene_id,$frame_id,$host,$port) $x
    set scene(canvas,motion_y,$scene_id,$frame_id,$host,$port) $y
  puts "$type scene $scene_id frame $frame_id but $but x $x y $y"
  } elseif {[string match release $type] && $but == 1} {
    set scene(canvaspress,$scene_id,$frame_id,$host,$port) 0
  puts "$type scene $scene_id frame $frame_id but $but x $x y $y"
  }
  return
}

proc GetScenePlaceFrame { handle tag host port line } {
  global scene
puts "GetScenePlaceFrame<$line>"
  # Check that we can get the scene_id
  if {![info exist scene(canvasmotion,scene,$host,$port)]} {
    puts "GetScenePlaceFrame was called when scene(canvasmotion,scene,$host,$port) did not exist"
    return 0
  }
 # Check we did get the scene_id
  set scene_id [lindex $scene(canvasmotion,scene,$host,$port) 0]
  set scene(canvasmotion,scene,$host,$port) [lrange $scene(canvasmotion,scene,$host,$port) 1 end]
  if { $scene_id == "" } {
    puts "GetScenePlaceFrame for $host:$port was called when no scene_id was available"
  }
  return 0
}

proc GetSceneOverlayOrderLine { handle tag host port line } {
  global scene

  # Check that we can get the scene_id
  if {![info exist scene(sceneoverlayorder,scene,$host,$port)]} {
    puts "GetSceneOverlayOrderLine was called when scene(sceneoverlayorder,scene,$host,$port) did not exist"
    return 0
  }
 # Check we did get the scene_id
  set scene_id [lindex $scene(sceneoverlayorder,scene,$host,$port) 0]
  set scene(sceneoverlayorder,scene,$host,$port) [lrange $scene(sceneoverlayorder,scene,$host,$port) 1 end]
  if { $scene_id == "" } {
    puts "GetSceneOverlayOrderLine for $host:$port was called when no scene_id was available"
    return 0
  }
  # Check that we have a frame to place it in
  if {![info exist scene(overlayframe,$scene_id,$host,$port)]} {
    puts stderr "SceneOverlayOrder was called for scene $scene_id when frame was not available"
    return 0
  }
  set overlayfr $scene(overlayframe,$scene_id,$host,$port)
  set bg [$overlayfr cget -bg]
  set orderfr [frame $overlayfr.orderfr -relief raised -bd 1 -bg $bg]
  set textfr [frame $overlayfr.textfr -relief raised -bd 1 -bg $bg]
  pack $orderfr $textfr -side left -anchor nw

  Label $orderfr.label -text "Overlay Order" -bg $scene(bgcolor) -fg $scene(fgcolor)
  pack $orderfr.label -side top -anchor nw -fill x

  set orderlist [lrange $line 1 end]
  set scroll [scrollbar $orderfr.scroll -orient v -bg $scene(bgcolor)]
  set order [ListBox $orderfr.lbox -width 10 -height 5 -bg $scene(bgcolor) -fg $scene(fgcolor) -selectmode single -dragenabled 1 -yscrollcommand [list $scroll set] -highlightbackground red   -highlightcolor blue -selectforeground green -selectbackground yellow ]
  $scroll configure -command "$order yview"
  set bfr [frame $orderfr.bfr -bg $bg]
  pack $scroll $order $bfr -side left -fill y
  ArrowButton $bfr.up -dir top -state disabled -armcommand "OverlayOrderMove $order $bfr 1" \
	-bg $bg  -highlightbackground $bg -width 25 -type arrow -bd 1 -arrowrelief raised \
	-helptext "Click to move selected item up" -ipady 0
  ArrowButton $bfr.down -dir bottom -state disabled -armcommand "OverlayOrderMove $order $bfr 0" \
	-bg $bg  -highlightbackground $bg -width 25 -type arrow -bd 1 -arrowrelief raised \
	-helptext "Click to move selected item down" -ipady 0
  ArrowButton $bfr.update -dir right -state disabled -armcommand "OverlayOrderSet $order $bfr $scene_id $host $port" \
	-bg $bg  -highlightbackground $bg -width 25 -type arrow -bd 1 -arrowrelief raised \
	-helptext "Click to update changes" -height 20 -ipady 0
  pack $bfr.up $bfr.down $bfr.update -side top -anchor w -fill y -expand 1

#  $bfr.up configure \
#	-activebackground red \
#	-activeforeground green \
#	-bg black  \
#	-disabledforeground orange \
#	-fg yellow \
#	-highlightbackground $bg \
#	-highlightcolor darkred 
  set i 0
  foreach orderset $orderlist {
    set type [lindex $orderset 0]
    set id [lindex $orderset 1]
    set item $type$id
    if {$type == {f}} { set type Frame } else { set type Text }
    $order insert end $item -text "$type $id"
    incr i
  }
  $order bindText <ButtonPress> "OverlayOrderSelect %W $bfr"
  $order bindText <Double-1> "OverlayOrderSet $order $bfr $scene_id $host $port"
  return 0
}

# Move an entry in the Overlay list
proc OverlayOrderMove {lbox butfr dir} {
  set item [$lbox selection get]
  set index [$lbox index $item]
  if {$dir} {
    if {$index > 0} {
      incr index -1
    }
  } else {
    incr index
  }
  $lbox move "$item" $index
  $lbox see $item
  $butfr.update configure -state normal
}

# Enable up and down buttons when an entry has been selected
proc OverlayOrderSelect {lbox butfr index} {
  $butfr.up configure -state normal
  $butfr.down configure -state normal
}

# Send new overlay order to Snowmix
proc OverlayOrderSet {lbox butfr scene_id host port args} {
  global snowmix
  set orderlist [$lbox items]
  set newlist ""
  foreach itemset $orderlist {
    lappend newlist "[string range $itemset 0 0] [string range $itemset 1 end]"
  }
  # puts $snowmix "tcl eval SceneOverlayOrder $scene_id $newlist"
  after 0 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneOverlayOrder $scene_id $newlist]" GetOneLine 1000]
  $butfr.down configure -state disabled
  $butfr.up configure -state disabled
  $butfr.update configure -state disabled
  $lbox selection clear
}

proc GetSceneOverlayTextLine { handle tag host port line } {
  global scene

  # Check that we can get the scene_id
  if {![info exist scene(sceneoverlaytext,scene,$host,$port)]} {
    puts "GetSceneOverlayTextLine was called when scene(sceneoverlaytext,scene,$host,$port) did not exist"
    return 0
  }
 # Check we did get the scene_id
  set scene_id [lindex $scene(sceneoverlaytext,scene,$host,$port) 0]
  set scene(sceneoverlaytext,scene,$host,$port) [lrange $scene(sceneoverlaytext,scene,$host,$port) 1 end]
  if { $scene_id == "" } {
    puts "GetSceneOverlayTextLine for $host:$port was called when no scene_id was available"
    return 0
  }
  # Check that we have a frame to place it in
  if {![info exist scene(overlayframe,$scene_id,$host,$port)]} {
    puts stderr "SceneOverlayText was called for scene $scene_id when frame was not available"
    return 0
  }
  set textfr $scene(overlayframe,$scene_id,$host,$port)
  set bg [$textfr cget -bg]

  # Overlay Text box
  set textlist [lrange $line 1 end]
  set i 0
  Label $textfr.label -text "Overlay Text" -bg $scene(bgcolor) -fg $scene(fgcolor) -width 18
  pack $textfr.label -side top -anchor nw -fill x
  foreach textset $textlist {
    set text_id [lindex $textset 0]
    set scene(overlaytext,$scene_id,$text_id,$host,$port) [lindex $textset 1]
    checkbutton $textfr.$i -text "Text $text_id" \
        -variable scene(overlaytext,$scene_id,$text_id,$host,$port) \
        -command "TextOverLayToggle $scene_id $text_id $host $port" \
	-bg $scene(bgcolor) \
	-fg $scene(fgcolor) \

#	-activeforeground green \
#	-activebackground cyan \
#	-selectcolor red \
#	-highlightcolor yellow \
#	-highlightbackground blue \
#	-disabledforeground orange \
#	-highlightthickness 5 -indicatoron 1

    pack  $textfr.$i -side top -anchor nw
    incr i
  }
  return 0
}

proc TextOverLayToggle { scene_id text_id host port } {
  global scene
  #puts "TextOverLayToggle $scene_id $text_id $scene(overlaytext,$scene_id,$text_id,$host,$port)"
  puts "tcl eval SceneSetOverlayText $scene_id $scene(overlaytext,$scene_id,$text_id,$host,$port) $text_id"
  after 0 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneSetOverlayText $scene_id $scene(overlaytext,$scene_id,$text_id,$host,$port) $text_id]" GetOneLine 1000]
}

proc SceneCutFade {scene_id type host port} {
  global scene snowmix
  if {$scene(alpha,$scene_id,$host,$port) < 50} { set fade 1
  } else { set fade 0 }

  if {!$type} {
    #catch { puts $snowmix "tcl eval SceneAlpha $scene_id -1 $fade" }
    after 0 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneAlpha $scene_id -1 $fade]" GetOneLine 1000]
  } else {
    #catch { puts $snowmix "tcl eval SceneFade $scene_id $fade" }
    after 0 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneFade $scene_id $fade]" GetOneLine 1000]
  }
  return
}

proc TimingEntry { host port scene_id frame_id type } {
  global scene system
puts "TimingEntry scene $scene_id frame $frame_id type $type "
  if {$type != {up} && $type != {pause} && $type != {down}} return
  if {$frame_id == {-}} {
    set key "$scene_id,$host,$port"
  } else {
    set key "$scene_id,$frame_id,$host,$port"
  }
#  if {$frame_id == {-} || $frame_id < 0} {
    set min [expr 1000 * [$scene(timing$type,scale,$key) cget -from] / $system(framerate,$host,$port)]
    set max [expr 1000 * [$scene(timing$type,scale,$key) cget -to] / $system(framerate,$host,$port)]
#  } else {
#    set min [expr 1000 * [$scene(timing$type,scale,$scene_id,$frame_id) cget -from] / $system(framerate,$host,$port)]
#    set max [expr 1000 * [$scene(timing$type,scale,$scene_id,$frame_id) cget -to] / $system(framerate,$host,$port)]
#  }
  # Set focus to toplevel again and tell that Return is a valid key to act upon again
  focus .
  set scene(Return,$host,$port) 1

  if {![string is integer -strict $scene(timing$type,ms,$key)]} {
#    if {$frame_id == {-} || $frame_id < 0} {
      set scene(timing$type,ms,$key) [expr int(1000*$scene(timing$type,$key)/$system(framerate,$host,$port))]
#    } else {
#      set scene(timing$type,ms,$scene_id,$frame_id) [expr int(1000*$scene(timing$type,$scene_id,$frame_id)/$system(framerate,$host,$port))]
#    }
  } else {

    # WE CAN CLEAN THIS UP because we use a key set in the beginning. Question : Can frame_id < 0 ?
    if {$frame_id == {-} || $frame_id < 0} {
      if {$scene(timing$type,ms,$key) < $min} {
        set scene(timing$type,ms,$key) $min
      }
      if {$scene(timing$type,ms,$key) > $max} {
puts "MAX $scene(timing$type,ms,$key) $max"
        set scene(timing$type,ms,$key) $max
      }
#      set value [expr round($system(framerate,$host,$port)*$scene(timing$type,ms,$key)/1000)]
    } else {
      if {$scene(timing$type,ms,$key) < $min} {
        set scene(timing$type,ms,$key) $min
      }
      if {$scene(timing$type,ms,$key) > $max} {
        set scene(timing$type,ms,$key) $max
      }
#      set value [expr round($system(framerate,$host,$port)*$scene(timing$type,ms,$key)/1000)]
    }
    set value [expr round($system(framerate,$host,$port)*$scene(timing$type,ms,$key)/1000)]
    TimingSliderChange $host $port $scene_id $frame_id $type $value
  }
  return
}

proc GetSceneOverlayTimingLine { handle tag host port line args } {
  global scene system
#puts "GetSceneOverlayTimingLine <$line>"
  set argc [llength $args]

  # Check that we can get the scene_id
  if {!$argc} {
    puts "GetSceneOverlayTimingLine was called without args for scene_id and frame_id."
    return 0
  }
 # Check we did get the scene_id
  set args1 $args
  set args [lindex $args 0]
  set scene_id [lindex $args 0]
  set frame_id [lindex $args 1]
  if { $scene_id == "" } {
    puts " - GetSceneOverlayTimingLine for $host:$port was called when no scene_id was available"
    return 0
  }
  if {$frame_id == {-}} {
    set key "$scene_id,$host,$port"
    set l 258
  } else {
    set key "$scene_id,$frame_id,$host,$port"
    set l 1
    # 155
  }

  # Check that we have a frame to place it in
  if {![info exist scene(timingframe,$key)]} {
    puts stderr "SceneOverlayTiming was called for scene $key  when frame was not available"
    return 0
  }
  set timingfr $scene(timingframe,$key)
  set bg [$timingfr cget -bg]
  set fadetiming [frame $timingfr.timing -relief raised -bd 1 -bg $bg]
  pack $fadetiming -side top -anchor n -fill both 

  set w 10
  set troughcolor #002000
  set i 1
 
  foreach type {up pause down} {
    set scene(timing$type,$key) [lindex [lindex $line $i] 1]
    set scene(timing$type,ms,$key) [expr int(1000*$scene(timing$type,$key)/$system(framerate,$host,$port))]
    incr i
    set frlabel [frame $fadetiming.fr$type -bg $bg]
    Label $frlabel.label -text "Fade $type" -bg $bg
    Entry $frlabel.entry -width 5 -justify right \
        -textvariable scene(timing$type,ms,$key) \
        -helptext "Set the timing for\nfading $type in ms." \
	-bd 1 -highlightcolor darkgrey -insertbackground black -highlightthickness 1 \
	-highlightbackground $bg -bg $bg \
        -takefocus 0 \
        -command "TimingEntry $host $port $scene_id $frame_id $type"
    bind $frlabel.entry <FocusIn> "set scene(EntryHasFocus,$host,$port) 1"
    bind $frlabel.entry <FocusOut> "unset scene(EntryHasFocus,$host,$port)"
#    bind $frlabel.entry <Visibility> "puts \"Visibility $scene_id $frame_id\""
#    bind $frlabel.entry <Map> "puts \"Map $scene_id $frame_id\""
#    bind $frlabel.entry <Unmap> "puts \"unmap $scene_id $frame_id\""
    checkbutton $frlabel.check -variable scene(timing$type,all,$host,$port) -bg $bg
    Label $frlabel.ms -text "ms" -bg $bg
    Label $frlabel.all -text "All" -bg $bg
    pack $frlabel.label -side left
    pack $frlabel.ms $frlabel.entry -anchor e -side right
    pack $frlabel.check $frlabel.all -side right  -anchor e
    scale $fadetiming.scale$type -from 1 -to 200 -orient horizontal -showval 0 \
        -tickinterval 0 -length $l -width $w -repeatdelay 10 -repeatinterval 10 \
        -troughcolor $troughcolor -variable scene(timing$type,$key) \
	-bg $bg \
        -command "TimingSliderChange $host $port $scene_id $frame_id $type"
    DynamicHelp::add $fadetiming.scale$type -text "Set the timing for\nfading $type"
    set scene(timing$type,scale,$key) $fadetiming.scale$type
    pack $frlabel $fadetiming.scale$type -side top -anchor nw -fill x
  }
  return 0
}

proc TimingSliderChange { host port scene_id frame_id type value } {
  global scene system
  #puts "TimingSliderChange $host $port scene $scene_id frame $frame_id type $type value $value"
  if {$scene(timing$type,all,$host,$port)} {
    set scene_list $scene(scenelist,$host,$port)
    #puts $snowmix "tcl eval SceneFadeSpeed $type all $frame_id $value"
    after 0 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneFadeSpeed $type all $frame_id $value]" GetOneLine 1000]
  } else {
    set scene_list $scene_id
    #puts $snowmix "tcl eval SceneFadeSpeed $type $scene_id $frame_id $value"
    after 0 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneFadeSpeed $type $scene_id $frame_id $value]" GetOneLine 1000]
  }
  foreach scene_id $scene_list {
    if {$frame_id == {-}} { set key "$scene_id,$host,$port" } else { set key "$scene_id,$frame_id,$host,$port" }
    set scene(timing$type,$key) $value
    set scene(timing$type,ms,$key) [expr int(1000*$scene(timing$type,$key)/$system(framerate,$host,$port))]
  }
  #puts "SceneFadeSpeed $type $scene_id $frame_id $value"
  return
}



proc Fade2Scene {scene_id fade host port} {
  global scene
#puts "\nFade2Scene $scene_id fade $fade. Alpha is $scene(alpha,$scene_id,$host,$port)"
#GetAllAlpha4Scene $scene_id
#puts " - Alphas $scene(alpha,$scene_id) $scene(alphabg,$scene_id) $scene(alphatext,$scene_id)"

  #puts $snowmix "tcl eval SceneSetState $scene_id 1 $fade"
  after 0 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneSetState $scene_id 1 $fade]" GetOneLine 1000]

#  if {$fade} {
#    after 1200 "puts Raise ; $scene(pane) raise scene$scene_id ; puts Raised"
#  } else {
#    $scene(pane) raise scene$scene_id
#  }
  return
}



proc AlphaSliderChange { host port scene_id frame_id side value } {
  global scene
  # Skip slider change if alpha is not set yet
  if {!$scene(alphaisset,$scene_id,$host,$port)} return
  puts "Alpha slider change scene $scene_id frame $frame_id side $side $host:$port $value"

  set value [expr $value / 100.0]
  # Is this a frame or a scene alpha change?

  if { $frame_id >= 0 } {
    #puts $snowmix "shape place alpha $scene(place_$side,$scene_id,$frame_id) $value"
    if {$side} {
      # puts $snowmix "tcl eval SceneAlpha $scene_id $frame_id $value"
      after 0 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneAlpha $scene_id $frame_id $value]" GetOneLine 1000]
    } else {
      # puts $snowmix "tcl eval SceneAlpha $scene_id $frame_id - $value"
      after 0 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneAlpha $scene_id $frame_id - $value]" GetOneLine 1000]
    }
  } else {
    # puts $snowmix "tcl eval SceneAlpha $scene_id $frame_id $value"
    after 0 [list AddServerCommand snowmix $host $port "tcl eval snowmix message \[SceneAlpha $scene_id $frame_id $value]" GetOneLine 1000]
  }
}


proc MakeFeedButton { host port id state } {
  global scene feed
  if {![info exist scene(feed,butfr,$host,$port)]} {
    puts stderr "No button frame for MakeFeedButton $host $port"
    return
  }
  # Set foreground color
  if { $feed(state_color,$state) == "black" } { set fg yellow
  } elseif { $feed(state_color,$state) == "green" } { set fg white } else { set fg black }

  Label $scene(feed,butfr,$host,$port).feed$id -text $id -padx 6 -pady 1 -bg $feed(state_color,$state) -fg $fg -relief raised -bd 1 -helptext "Feed $id\n$state"
  pack $scene(feed,butfr,$host,$port).feed$id -side left -padx 0
  lappend scene(feedlist,$host,$port) $id

  if {[info procs AddFeedCommands] != ""} { AddFeedCommands $host $port $id $scene(feed,butfr,$host,$port).feed$id }
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
  global comms
  if {[info exist comms(serverhandle,$tag,$host,$port)]} {
    if {$comms(serverhandle,$tag,$host,$port) == ""} {
      puts stderr "WARNING ToggleServerCon handle is empty. Ignoring."
      unset -nocomplain comms(serverhandle,$tag,$host,$port)
      ToggleServerCon $tag $host $port
      return
    }
    puts stderr "CLOSING Server connection"
    AsyncCloseConn $comms(serverhandle,$tag,$host,$port)
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
    } else {
      puts stderr "ToggleServerCon was called with unknown tag $tag. Ignoring"
    }
  }
}

proc ServerTabRaised { host port } {
  global scene
  puts "SERVERTAB RAISED : $host:$port"
  set raised [$scene(nb,$host,$port) raise]
  puts " - Raised $raised"
  if {[regexp {scene([0-9]+)} $raised match scene_id]} {
    set raisecmd [$scene(nb,$host,$port) itemcget $raised -raisecmd]
    if { $raisecmd != "" } { eval $raisecmd }
  }
  focus .
}

proc AddServerSetup { host port } {
  global scene led_image system
  
  if {[info exist scene(main_frame,$host,$port)]} {
    puts stderr "Can not add server setup for $host $port. Already existing"
    return
  }

  set scene(autoraise,$host,$port) $scene(autoraise)
  set scene(update_rate,$host,$port) $scene(update_rate)
  set scene(scenelist,$host,$port) ""
  set scene(timingup,all,$host,$port) 0
  set scene(timingpause,all,$host,$port) 0
  set scene(timingdown,all,$host,$port) 0
  set system(framerate,$host,$port) 1
  set scene(image,list,$host,$port) ""
  set scene(image,max_id,$host,$port) -1
  set scene(frame_types,$host,$port)  ""
  set scene(source_list,$host,$port)  "-"

  # There can't be any '.' in a tab name
  set hostmap [string map {. _} $host]
  set scene(hostmap,$host) $hostmap
  set scene(host,$hostmap) $host

  # Add tab to main_nb
  $scene(main_nb) insert end "$hostmap:$port" -text " Snowmix $host:$port" -image $led_image(darkgrey,$system(led_size)) \
	-raisecmd "ServerTabRaised $host $port"
  #$scene(main_nb) raise "$hostmap:$port"
  set fr [$scene(main_nb) getframe "$hostmap:$port"]
  $fr configure -pady 3 -padx 2 -bg $scene(bgcolor)

  set bg [$scene(main_nb) cget -bg]

  set serverfr [$scene(main_nb) getframe $hostmap:$port]
  $serverfr configure -bg $scene(bgcolor) -relief raised -bd 0 -padx 0

  if { 1 } {
    set serverfr [frame $serverfr.main -padx 0 -pady 0 -border 0 -bg $scene(bgcolor)]
    pack $serverfr -side top -anchor nw -fill both -expand 1
    set sw [ScrolledWindow $serverfr.sw -bg $bg]
    #set sff [ScrollableFrame $sw.f -height 1000 -width 12000 -bg $bg -areawidth 0]
    set sff [ScrollableFrame $sw.f -height 1000 -width 100  -bg $bg]
    $sw setwidget $sff
    pack $sw -side top -fill both -expand 1 -anchor n
    set serverfr [$sff getframe]
  }

  set scene(main_frame,$host,$port) $serverfr
  $scene(main_frame,$host,$port) configure -bg $scene(bg) -bd 0
  #pack configure $scene(main_frame,$host,$port) -expand 0

  set top_fr [frame $scene(main_frame,$host,$port).topfr -bd 0 -bg $scene(bgcolor)]
  pack $top_fr -side top -anchor w -fill both

  # Create a NoteBook for Scenes for this server
  set nb [NoteBook $scene(main_frame,$host,$port).nb -side top -arcradius 4 -homogeneous 1 -tabbevelsize 2 \
	-bg $scene(bgcolor) -fg $scene(fgcolor) -activebackground grey40  -activeforeground white ]
  pack $nb -side top -fill both -anchor w -expand 0
  set scene(nb,$host,$port) $nb

  # Add the Overview tab for this server
  $nb insert end overview -text Overview
  set scene(overview,$host,$port) [$nb getframe overview]
  $nb raise overview

  set o_fr [$nb getframe overview]
  set cen_fr [frame $o_fr.cenfr -bd 0]
  set bot_fr [frame $o_fr.botfr -bd 0]
  #pack $top_fr -side top -fill x -expand 1 -anchor n
  pack $cen_fr -side top -fill both -expand 1 -anchor n
  pack $bot_fr -side top -fill x -expand 1 -anchor n
  #$top_fr configure -bg $scene(bgcolor)
  $cen_fr configure -bg green
  $bot_fr configure -bg blue

  # Make server Button
  set bg $scene(bgcolor)
  set color darkgrey
  set server_but [frame $top_fr.server -padx 3 -pady 3 -bg $scene(bgcolor) -relief raised -bd 1]
  set but_fr [frame $top_fr.buts -padx 3 -pady 2 -bg $scene(bgcolor) -relief sunken -bd 1]
  Label $but_fr.label -text "Feed In" -bg $scene(bgcolor) -fg $scene(fgcolor) -bd 0 -padx 4
  pack $but_fr.label -side left
  set scene(server,but,$host,$port) $server_but
  set scene(feed,butfr,$host,$port) $but_fr
  pack $server_but $but_fr -side left
  Label $server_but.img -image $led_image($color,$system(led_size)) -padx 1 -bg $scene(bgcolor) -bd 0
  Label $server_but.label -text "Start/Stop." -padx 2 -pady 1 -bg $scene(bgcolor) -fg $scene(fgcolor) -bd 0 -width 10
  pack $server_but.img $server_but.label -side left -padx 3
  AppendServerLed $host $port $server_but.img
  bind $server_but <ButtonPress> "$server_but configure -relief sunken ; ToggleServerCon snowmix $host $port"
  bind $server_but.img <ButtonPress> "$server_but configure -relief sunken ; ToggleServerCon snowmix $host $port"
  bind $server_but.label <ButtonPress> "$server_but configure -relief sunken ; ToggleServerCon snowmix $host $port"
  AddButtonColors $server_but.label
  bind $server_but <Enter> "$server_but configure -bg $scene(buthoverbgcolor)"
  bind $server_but <Leave> "$server_but configure -bg $scene(butbgcolor)"

  bind $server_but <ButtonRelease> "$server_but configure -relief raised"
  bind $server_but.img <ButtonRelease> "$server_but configure -relief raised"
  bind $server_but.label <ButtonRelease> "$server_but configure -relief raised"
  DynamicHelp::add $server_but -text "Click to start/stop\nSnowmix connection\nto $host:$port"

  AddServer snowmix $host $port
  GetSceneInfo $host $port
  # StartSnowmix $host $port
  after 2000 [list $nb compute_size]
  after 2100 [list $scene(main_nb) raise "$hostmap:$port"]
}

proc GetSceneFrameTypes { handle tag host port line } {
  global scene

  if {[string match "*Invalid*" $line]} {
    puts stderr "Server $host:$port does not have scenes"
    AsyncCloseConn $handle
    return 0
  }
  if {$scene(frame_types,inputtype,$host,$port) == "types"} {
    if {[regexp {MSG:\ +Scene\ types\ +=} $line match]} {
      set scene(frame_types,$host,$port) [lrange $line 4 end]
      puts stderr "INFOPROC SceneFrameTypes : $scene(frame_types,$host,$port)"
      set scene(frame_types,total,$host,$port) [llength $scene(frame_types,$host,$port)]
      if {$scene(frame_types,total,$host,$port)} {
        set command ""
        foreach type $scene(frame_types,$host,$port) {
          append command "tcl eval SceneFrameTypeInfoProc $type\n"
        }
        set scene(frame_types,read,$host,$port) 0
        set scene(frame_types,inputtype,$host,$port) "infoproc"
        after 0 [list AddServerCommand snowmix $host $port $command GetSceneFrameTypes 1000]
      }
      return 0
    }
  } elseif {$scene(frame_types,inputtype,$host,$port) == "infoproc"} {
    if {[regexp {MSG:\ +InfoProc\ +=\ +([^ ]+)} $line match infoproc]} {
      set n $scene(frame_types,read,$host,$port)
      incr scene(frame_types,read,$host,$port)
      set type [lindex $scene(frame_types,$host,$port) $n]
      set scene(frame_types,infoproc,$type) $infoproc
      puts "INFOPROC for $type $infoproc"
      if {$scene(frame_types,read,$host,$port) < $scene(frame_types,total,$host,$port)} { return 1 }

      set command ""
      foreach type $scene(frame_types,$host,$port) {
        append command "tcl eval snowmix message \[$scene(frame_types,infoproc,$type) list]\n"
      }
      set scene(frame_types,read,$host,$port) 0
      set scene(frame_types,inputtype,$host,$port) "lists"
      after 0 [list AddServerCommand snowmix $host $port $command GetSceneFrameTypes 1000]
      return 0
    } else {
      puts stderr "Unrecognized line for GetSceneFrameTypes <$line>"
    }
    return 0
  } elseif {$scene(frame_types,inputtype,$host,$port) == "lists"} {
      set n $scene(frame_types,read,$host,$port)
      incr scene(frame_types,read,$host,$port)
      set type [lindex $scene(frame_types,$host,$port) $n]
      set scene(frame_types,list,$type,$host,$port) [lrange $line 1 end]
      puts "INFOPROC for $type $scene(frame_types,list,$type,$host,$port)"
      if {$scene(frame_types,read,$host,$port) < $scene(frame_types,total,$host,$port)} { return 1 }

      set command ""
      foreach type $scene(frame_types,$host,$port) {
        append command "tcl eval snowmix message \[$scene(frame_types,infoproc,$type) buttons]\n"
      }
      set scene(frame_types,read,$host,$port) 0
      set scene(frame_types,inputtype,$host,$port) "buttons"
      after 0 [list AddServerCommand snowmix $host $port $command GetSceneFrameTypes 1000]
      after 100 [list StartSnowmix $host $port]
      return 0
  } elseif {$scene(frame_types,inputtype,$host,$port) == "buttons"} {
      set n $scene(frame_types,read,$host,$port)
      incr scene(frame_types,read,$host,$port)
      set type [lindex $scene(frame_types,$host,$port) $n]
      set scene(frame_types,buttons,$type,$host,$port) [lrange $line 1 end]
      puts "INFOPROC buttons for $type $scene(frame_types,buttons,$type,$host,$port)"
      if {$scene(frame_types,read,$host,$port) < $scene(frame_types,total,$host,$port)} { return 1 }

      foreach type $scene(frame_types,$host,$port) {
        foreach id $scene(frame_types,list,$type,$host,$port) {
          lappend scene(source_list,$host,$port)  "$type $id"
        }
      }
      return 0
  }
  return 0
}

proc GetSceneInfo { host port } {
  global scene

  # Get Image Loaded
  after 0 [list AddServerCommand snowmix $host $port "image load" GetImageLoad 1000]
  set scene(frame_types,inputtype,$host,$port) "types"
  after 0 [list AddServerCommand snowmix $host $port "tcl eval SceneFrameTypesList" GetSceneFrameTypes 1000]
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

proc AddButtonColors { node args } {
  global scene

  bind $node <Enter> "$node configure -bg $scene(buthoverbgcolor) -fg $scene(buthoverfgcolor)"
  bind $node <Leave> "$node configure -bg $scene(butbgcolor) -fg $scene(butfgcolor)"
  $node configure -bg $scene(butbgcolor) -fg $scene(butfgcolor)
}


proc SetupScenePane { pane notebook page } {
  global scene 
  puts stderr "Setting up ScenePane for pane $pane notebook $notebook page $page"
  set bg [$pane cget -bg]
  set scene(main_frame) $pane
  set scene(bg) [$pane cget -bg]
  $pane configure -padx 0 -pady 0

  set scene(bgcolor) grey30
  set scene(fgcolor) grey70
  set scene(butbgcolor) grey30
  set scene(buthoverbgcolor) grey50
  set scene(butfgcolor) grey70
  set scene(buthoverfgcolor) white

  set scene(butbghighlight) grey40

  set nb [NoteBook $pane.nb -side top -arcradius 4 -homogeneous 1 -tabbevelsize 2 \
	-bg $scene(bgcolor) -fg $scene(fgcolor) -activebackground grey40  -activeforeground white ]

  pack $nb -side top -fill both -anchor w -expand 1
  set scene(main_nb) $nb
  set scene(autoraise) 1
  set scene(update_rate) 1000

  set port 9999

#  set host thinkie
#  AddServerSetup $host $port
#  catch { $nb raise $scene(hostmap,$host):$port }

#  set host 127.0.0.1
#  set host 192.168.3.132
#  set host 192.168.3.100
#  set host vostok02
#  set hostmap [string map {. _} $host]
#  AddServerSetup $host $port
#  catch { $nb raise $scene(hostmap,$host):$port }

  bind . <Key> "KeyInput Hello1 %K %k"
  bind $pane <Key> "KeyInput Hello2 %K %k"

  return 
}
