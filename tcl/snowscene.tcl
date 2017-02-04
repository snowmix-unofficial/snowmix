#!/bin/sh
# \
if [ -x /usr/local/bin/wish ] ; then exec /usr/local/bin/wish "$0" ${1+"$@"} ; else exec /usr/bin/wish "$0" ${1+"$@"} ; fi
#
# !/usr/bin/wish

package require BWidget

proc print_and_exit { message } {
	puts stderr "$message"
	exit 1
}

if {![info exist env(SNOWMIX)]} {
  print_and_exit "The environment variable SNOWMIX must be set to the install directory of SNOWMIX"
}
cd $env(SNOWMIX)/tcl
if {[info exist env(HOME)]} {
  set system(HOME) $env(HOME)
} else {
  print_and_exit "Environment variable HOME does not exist. Exiting"
}


set system(default_geometry) "900x570"
set system(geometry_minsize) "400 225"
set system(default_bg) black
set system(default_bg) lightyellow
set system(default_bg) lightgrey
set system(default_bg) #D9D9D9
#11011001 = 13.9 = D9
set system(version) "0.51"
set system(window_system) [tk windowingsystem]
if {[string match "aqua" $system(window_system)]} {
	set system(very_large_font_size) 48
	set system(default_font_size) 11
	set system(small_font_size) 8
	if {[info exist env(SNOWMIX_PREFIX)]} {
		set system(SNOWMIX_PREFIX) $env(SNOWMIX_PREFIX)
	} else { set system(SNOWMIX_PREFIX) Snowmix }
} else {
	set system(very_large_font_size) 36
	set system(default_font_size) 8
	set system(small_font_size) 7
	if {[info exist env(SNOWMIX_PREFIX)]} {
		set system(SNOWMIX_PREFIX) $env(SNOWMIX_PREFIX)
	} else { set system(SNOWMIX_PREFIX) ".snowmix" }
}
set system(main_menu) "menubar"
set system(main_menu,bg) "gray"
set system(main_menu,entries) "File Edit"
set system(led_size) 10x12
set system(colors) "white yellow red lightgreen blue white grey lightblue magenta cyan bisque orange plum gold lightblue yellow2 coral pink wheat yellow2 maroon violet bisque purple"


set system(basename) [file tail $argv0]
if {[string match "snowrelay*" $system(basename)]} { 
	set system(program) snowrelay
	set system(name) SnowRelay
} elseif {[string match "snowscene*" $system(basename)]} { 
	set system(program) snowscene
	set system(name) SnowScene
} elseif {[string match "snowaudio*" $system(basename)]} { 
	set system(program) snowaudio
	set system(name) SnowAudio
} elseif {[string match "snowoutput*" $system(basename)]} { 
	set system(program) snowoutput
	set system(name) SnowOutput
} elseif {[string match "snowcomm*" $system(basename)]} { 
	set system(program) snowcommentator
	set system(name) SnowCommentator
} else {
	print_and_exit "Could not identify program name $system(name)"
	set system(program) unknown
	set system(name) Unknown
}
puts stderr "Running Program $system(program)"

set system(SNOWDIR) $system(HOME)/$system(SNOWMIX_PREFIX)/$system(program)

if {[catch {file mkdir $system(SNOWDIR)} res]} {
	print_and_exit "Could not create directory $system(SNOWDIR)"
}
set system(recent_servers_file) $system(SNOWDIR)/recent_servers
set system(recent_servers,$system(program)) ""

font create mySmallFont -family Sans -weight normal  -size $system(small_font_size)
option add *font mySmallFont
font create myDefaultFont -family Sans -weight normal  -size $system(default_font_size)
option add *font myDefaultFont

. config -bg grey

proc AddToRecentServers { type server port args } {
	global system
	if { $type != "snowmix" && $type != "snowrelay" && $type != "snowscene" && $type != "snowaudio" && $type != "snowoutput" } return
	if {[info exist system(recent_servers,$type)] && [set pos [lsearch -exact $system(recent_servers,$type) "$server $port"]] > -1} {
		set system(recent_servers,$type) [lreplace $system(recent_servers,$type) $pos $pos]
	}
	if {[llength $args] && [info exist system(recent_servers,$type)]} {
		set system(recent_servers,$type) [linsert $system(recent_servers,$type) 0 "$server $port"]
	} else {
		lappend system(recent_servers,$type) "$server $port"
	}
puts stderr "Recent Servers : $system(recent_servers,$type)"
}

proc WriteRecentServers {} {
	global system
	set tmpname $system(recent_servers_file).[pid]
	set fp [open $tmpname w]
	foreach serverset $system(recent_servers,$system(program)) {
		puts $fp "$system(program) $serverset"
	}
	close $fp
	file rename -force $tmpname $system(recent_servers_file)
}

proc ReadRecentServers { } {
	global system
catch { puts stderr "RECENT $system(recent_servers_file)" }
	if {[file exist $system(recent_servers_file)]} {
		set fp [open $system(recent_servers_file) r]
		set recent_servers_data [read $fp]
		close $fp
		set new_list [split $recent_servers_data "\n"]
		foreach serverset $new_list {
			AddToRecentServers [lindex $serverset 0] [lindex $serverset 1] [lindex $serverset 2] 
		}
	}
}


proc MakeMainWindow { title } {
	global system
	wm geometry . $system(default_geometry)
	eval "wm minsize . $system(geometry_minsize)"
	. config -bg $system(default_bg)
	wm title . $title
}

proc MakeMainMenuBar { menu_name } {
	global system
	set menubar .$menu_name
	option add *tearOff 0
	if {[info exist system(main_menu,$menu_name,bg)]} {
		set bg $system(main_menu,$menu_name,bg)
	} else {
		set bg $system(main_menu,bg)
	}
	menu $menubar -bg $bg
	. config -menu $menubar -width 300

	if {[string match "aqua" $system(window_system)]} {
		$menubar add cascade -label $system(name) -menu [menu $menubar.apple]
		$menubar.apple add command -label About -command About
		$menubar.apple add command -label Preferences -command Preferences
		$menubar.apple add command -label Quit -command exit
	}
	foreach entry $system(main_menu,entries) {
		menu $menubar.m$entry
		$menubar add cascade -label $entry -menu $menubar.m$entry
	}
	#menu $menubar.mFile.recent
	$menubar.mFile add command -label "Add Server" -command AddServerSet
	$menubar.mFile add cascade -menu [menu $menubar.mFile.recent]  -label "Add Recent" -underline 4
	$menubar.mFile add command -label "Reconnect Server" -state disabled
	$menubar.mFile add separator
	$menubar.mFile add command -label "Close Relay Server"
	$menubar.mFile add separator
	$menubar.mFile add command -label "ComputeSize"
	$menubar.mFile add separator
	if {[string match "x11" $system(window_system)]} {
		$menubar.mFile add command -label "Quit" -command exit
	}

	if {[string match "aqua" $system(window_system)]} {
		$menubar add cascade -label Help -menu [menu $menubar.help]
	}
	foreach serverset $system(recent_servers,$system(program)) {
		if { $system(program) == "snowrelay" } {
			$menubar.mFile.recent add command -label "$serverset" -command "AddingServer [lindex $serverset 0] [lindex $serverset 1]"
		} elseif { $system(program) == "snowscene" } {
			$menubar.mFile.recent add command -label "$serverset" -command "AddingServer $serverset"
		} elseif { $system(program) == "snowaudio" } {
			$menubar.mFile.recent add command -label "$serverset" -command "AddingServer $serverset"
		} elseif { $system(program) == "snowoutput" } {
			$menubar.mFile.recent add command -label "$serverset" -command "AddingServer $serverset"
		}
	}
	
}

proc AddingServer { host port } {
	global system
#  if { $system(program) == "snowrelay" } {
#	AddServerSetup $host $port
#  } else {
#    AddServerSetup $host $port
#  }
puts "PMMA : Adding server $host $port"
	AddServerSetup $host $port
	AddToRecentServers $system(program) $host $port 0
	WriteRecentServers
	.menubar.mFile.recent delete 0 end
	foreach serverset $system(recent_servers,$system(program)) {
			.menubar.mFile.recent add command -label "$serverset" -command "AddingServer $serverset"
	}
	pack forget $system(basefr).recents
	destroy $system(basefr).recents
}

proc About {} {
	global system
	set bg $system(default_bg)
	set aboutw .about
	if {[winfo exist $aboutw]} return
	toplevel $aboutw -bg $bg
	wm geometry $aboutw 300x130
	label $aboutw.label -text "$system(name) - version $system(version)\n\nCopyright © Peter Maersk-Moller 2015\nAll Rights Reserved. Further details to be found\n at http://snowmix.sourceforge.net and\nhttps://sourceforge.net/projects/snowmix/\n" -bg $bg
	label $aboutw.close -text Close -bg $bg -relief raised -bd 1
	bind $aboutw.close <ButtonPress> "destroy $aboutw" 
#Button $aboutw.close -text Close -command "destroy $aboutw" -bg $bg -fg blue -highlightcolor red -highlightbackground green -activebackground grey -activeforeground brown -relief raised
	pack $aboutw.label $aboutw.close -side top
}

proc AddServerSet {} {
	global system
	set bg $system(default_bg)
	set addservw .addservw
	if {[winfo exist $addservw]} return
	toplevel $addservw -bg $bg
	wm geometry $addservw 230x70
	wm title $addservw "Add Server"
	set fr [frame $addservw.f -bd 0 -bg $bg]
	set frl [frame $fr.l -bd 0 -bg $bg]
	set frr [frame $fr.r -bd 0 -bg $bg]
	label $frl.server -text Server -bd 1 -relief raised -bg $bg -pady 1
	label $frl.port -text Port -bd 1 -relief raised -bg $bg -pady 1
	#frame $frl.sep -bd 1 -bg $bg -pady 2
	Label $frl.cancel -text Cancel -bd 1 -relief raised -bg $bg -pady 1 \
		-helptext "Cancel adding server"
	bind $frl.cancel <ButtonPress> "destroy $addservw"
	Entry $frr.server -relief sunken -highlightcolor darkgrey -insertbackground black \
		-highlightthickness 1 -highlightbackground $bg -bg $bg \
		-helptext "Server: Specify servers name or IP address.\nExample: myserver or 192.168.1.23" -textvariable system(addserver,host)
	Entry $frr.port -relief sunken -highlightcolor darkgrey -insertbackground black \
		-highlightthickness 1 -highlightbackground $bg -bg $bg \
		-helptext "Port: Specify server port (0-65535)." -textvariable system(addserver,port)
	label $frr.add -text Add -bd 1 -relief raised -bg $bg
	bind $frr.add <ButtonPress> "AddServerSetAdd $addservw"
	pack $fr -side top
	pack $frl $frr -side left -anchor n
	pack $frl.server $frl.port -side top -expand 1 -fill x -pady 1
	pack $frl.cancel -side top -expand 1 -fill x -pady 3
	pack $frr.server $frr.port -side top -expand 1 -fill x
	pack $frr.add -side top -expand 1 -fill x -pady 3
#	focus $frr.server
}

proc AddServerSetAdd { window } {
	global system
	AddingServer $system(addserver,host) $system(addserver,port)
#	AddServerSetup $system(addserver,host) $system(addserver,port)
	destroy $window
#	destroy $system(basefr).recents
}

proc Preferences {} {
	global system
	set bg $system(default_bg)
	set prefsw .preferences
	if {[winfo exist $prefsw]} return
	toplevel $prefsw -bg $bg
	wm geometry $prefsw 200x150
	label $prefsw.label -text Preferences -bg $bg
	label $prefsw.close -text Close -bg $bg -relief raised -bd 1
	bind $prefsw.close <ButtonPress> "destroy $prefsw" 
	pack $prefsw.label $prefsw.close -side top
}

proc SetAndLoadLeds {size} {
	global feed led_image system
	set feed(led_size) $size
	set led_image(colors) "red orange green yellow blue black white cyan purple"
	append led_image(colors) " darkblue darkgrey darkgreen darkred"
	foreach color $led_image(colors) {
		if {$system(program) == "snowaudio" || $system(program) == "snowoutput" } {
			set name [format "%s-grey30-%s.gif" $color $size]
		} elseif {$system(program) == "snowscene" } {
			set name [format "%s-grey30-%s.gif" $color $size]
		} else {
			set name [format "%s-led-%s.gif" $color $size]
		}
		set led_image($color,$size) [ image create photo -file images/$name]
	}
	set feed(state_color,RUNNING) green
	set feed(state_color,PENDING) orange
	set feed(state_color,STALLED) red
	set feed(state_color,DISCONNECTED) black
	set feed(state_color,READY) cyan
	set feed(state_color,SETUP) yellow
	set feed(state_color,UNDEFINED) darkgrey
	set feed(state_color,NO_FEED) purple
}

proc GetArgs { } {
  global argc argv
  if {$argc < 1} return
  set arg_start 0
  set host_list ""
  while { $arg_start < $argc } {
    set host ""
    set port ""
    set host_set [lindex $argv $arg_start]
#puts "Checking <$host_set>"
    if {[regexp {(.+):(.+)} $host_set match host port]} {
    } else {
      set host $host_set
      incr arg_start
      set port [lindex $argv $arg_start]
    }
    if { $host == "" || $port == "" || [lsearch $host " "] > -1 || ![string is integer $port]} {
      puts stderr "mismatching host:port set"
      exit
    }
    lappend host_list "$host $port"
    incr arg_start
  }
  return $host_list
}

set server_sets [GetArgs]

ReadRecentServers
MakeMainWindow "$system(name) $system(version)"
MakeMainMenuBar $system(main_menu)

# Set and load the LEDs we will use in the notebok tab header
SetAndLoadLeds $system(led_size)

set system(basefr) [frame .bfr -padx 10 -pady 0 -border 0 -bg $system(default_bg)]
pack $system(basefr) -fill both -expand true

source [file join $env(SNOWMIX) tcl utility.inc]
source [file join $env(SNOWMIX) tcl server.inc]
#source [file join $env(SNOWMIX) tcl connection.inc]

if { $system(program) == "snowscene" } {
  source [file join $env(SNOWMIX) tcl cs_feed_commands.inc]
  source [file join $env(SNOWMIX) tcl scenes.inc]
  SetupScenePane $system(basefr) "" scene
} elseif { $system(program) == "snowrelay" } {
  source [file join $env(SNOWMIX) tcl graph.inc]
  source [file join $env(SNOWMIX) tcl cs_feed_commands.inc]
  source [file join $env(SNOWMIX) tcl relay.inc]
  source [file join $env(SNOWMIX) tcl relay_analyzer.inc]
  relay_pane $system(basefr) "" relay
} elseif { $system(program) == "snowaudio" } {
  wm geometry . 1200x520
  source [file join $env(SNOWMIX) tcl graph.inc]
  source [file join $env(SNOWMIX) tcl audio.inc]
  SetupAudioPane $system(basefr) "" audio
} elseif { $system(program) == "snowoutput" } {
  wm geometry . 750x340
  source [file join $env(SNOWMIX) tcl graph.inc]
  source [file join $env(SNOWMIX) tcl output.inc]
  SetupOutPutPane $system(basefr) "" output
#  output_pane $system(basefr)
} elseif { $system(program) == "snowcommentator" } {
  wm geometry . 100x50
  # source [file join $env(SNOWMIX) tcl connection.inc]
  font create myVeryLargeFont -family Sans -weight normal  -size $system(very_large_font_size)
  option add *font myVeryLargeFont
  source [file join $env(SNOWMIX) tcl commentator.inc]
  SetupCommentator $system(basefr)
} else {
  puts stderr "Unsupported program $system(program)"
}

set fr [frame $system(basefr).recents -bd 0 -bg grey30]
set packlist [pack slaves $system(basefr)]
pack forget $packlist
pack $fr $packlist -side left -anchor n
#if {[llength $system(recent_servers,$system(program))] > 0} {
  pack $packlist -side left -expand 1 -fill both
  Label $fr.recents -text "Recent Servers" -bd 1 -fg grey70 -bg grey30
  pack $fr.recents -side top -fill x
  set i 0
  foreach host $system(recent_servers,$system(program)) {
    Label $fr.recents$i -text $host -bg grey30 -fg grey70 -relief raised
    pack $fr.recents$i -side top -fill x
    bind $fr.recents$i <ButtonPress> "$fr.recents$i configure -relief sunken ; AddingServer [lindex $host 0] [lindex $host 1]"
    bind $fr.recents$i <ButtonRelease> "catch { pack forget $system(basefr).recents ; destroy $system(basefr).recents }"
    bind $fr.recents$i <Enter> "$fr.recents$i configure -bg grey50"
    bind $fr.recents$i <Leave> "$fr.recents$i configure -bg grey30"
    incr i
  }
  Label $fr.cancel -text "Cancel" -relief raised -bd 1 -fg grey70 -bg grey30
  pack $fr.cancel -side top -fill x
  bind $fr.cancel <ButtonPress> "$fr.cancel configure -relief sunken"
  bind $fr.cancel <ButtonRelease> "catch { pack forget $system(basefr).recents ; destroy $system(basefr).recents }"
  bind $fr.cancel <Enter> "$fr.cancel configure -bg grey50"
  bind $fr.cancel <Leave> "$fr.cancel configure -bg grey30"
#}
if { $server_sets != "" } {
  pack forget $system(basefr).recents
  destroy $system(basefr).recents
  foreach server_set $server_sets {
    set host [lindex $server_set 0]
    set port [lindex $server_set 1]
    AddingServer $host $port
  }
}
