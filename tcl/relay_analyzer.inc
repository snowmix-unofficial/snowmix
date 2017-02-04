# STAT: Analyzer 1 type ts bytes 3182994724 time 1434061527.927
# STAT: TS Analyser 1 packets 16930823 no sync 0 wrong size 0 pids 3
# STAT: Pid    0 count 121457 ps 121457 prio 0 scr 0 adap 121457:0  discon 0 rnd 0 aprio 0 pay 121457 priv 0 aext 0 pcr 0:0 opcr 0:0 last 5.2725 s
# STAT: Pid   32 count 122626 ps 122626 prio 0 scr 0 adap 122626:0  discon 0 rnd 0 aprio 0 pay 122626 priv 0 aext 0 pcr 0:0 opcr 0:0 last 0.3047 s
# STAT: Pid   65 count 16686740 ps 484470 prio 0 scr 0 adap 612003:0  discon 0 rnd 1617 aprio 0 pay 16686740 priv 0 aext 0 pcr 121870:0 opcr 0:0 last 0.0047 s
# STAT:
proc SetupGettingAnalyzerStatus { host port } {
  global relay
puts "Setup Getting Analyzer Status"
  set key [format "analyzerstatus,%s,%s" $host $port]
  set relay(pattern1,$key) "STAT:\ +\Analyzer\ +(\[0-9]+)\ +type\ +(\[^ ]+)\ +bytes\ +(\[0-9]+)\ +time\ +(\[\.0-9]+)"
  set relay(pattern2,$key) "STAT:\ +\TS\ +Analyzer\ +(\[0-9]+)\ +packets\ +(\[0-9]+)\ +no\ +sync\ +(\[0-9]+)\ +tei\ +(\[0-9]+)\ +wrong\ +size\ +(\[0-9]+)\ +psie\ +(\[0-9]+)\ +pids\ +(\[0-9]+)"
  set relay(pattern3,$key) "STAT:\ +Pid\ +(\[0-9]+)\ +count\ +(\[0-9]+)\ +ps\ +(\[0-9]+)\ +prio\ +(\[0-9]+)\ +scr\ +(\[0-9]+)\ +adap\ +(\[0-9\:]+)\ +discon\ +(\[0-9]+)\ +rnd\ +(\[0-9]+)\ +aprio\ +(\[0-9]+)\ +pay\ +(\[0-9]+)\ +priv\ +(\[0-9]+)\ +aext\ +(\[0-9]+)\ +pcr\ +(\[0-9\:]+)\ +\opcr\ +(\[0-9\:]+)\ +last\ +(\[0-9\.]+)"
}

proc StartGettingAnalyzerStatus { host port analyzer_id } {
  global relay
#  foreach analyzer_id $relay(analyzers,$host,$port) {
    set relay(analyzer,$analyzer_id,status,$host,$port) 1
    after 0 [list AddServerCommand relay $host $port "ana status $analyzer_id" AnalyzerStatus 1000]
#  }
}
proc AnalyzerStatus { handle tag host port line } {
  global relay
  set key [format "analyzerstatus,%s,%s" $host $port]
  set hostid "$host,$port"
  if {[regexp $relay(pattern3,$key) $line match pid packets ps prio scr adap discon rnd aprio pay priv aext pcr opcr last]} {
    set id $relay(analyzer,last_id,$host,$port)
    if {![info exist relay(analyzer,$id,status,$host,$port)]} {
      puts "Analyzer $id closed. Ignore status line."
      return 1
    }
    set aid "analyzer,$id"
    if {![info exist relay($aid,$pid,packets,$hostid)]} {
      # This is the first time this Pid was seen
      AddPidToAnalyzer $id $host $port $pid
    } else {
      set relay($aid,$pid,kbps,$host,$port) [expr int(1.504*($packets - $relay($aid,$pid,packets,$host,$port))/$relay($aid,delta_time,$host,$port))]
    }
    foreach keyname {packets ps prio scr adap discon rnd aprio pay priv aext pcr opcr last} {
      upvar 0 $keyname keyval
      set relay($aid,$pid,$keyname,$host,$port) $keyval
    }
    set adapt [string map {: \ } $adap]
    set relay($aid,$pid,adap,$host,$port) [lindex $adapt 0]
    set relay($aid,$pid,adaple,$host,$port) [lindex $adapt 0]
  } elseif {[regexp $relay(pattern1,$key) $line match id type bytes time]} {
    if {![info exist relay(analyzer,$id,status,$host,$port)]} { return 1 }
    set aid "analyzer,$id"
    if {[info exist relay($aid,time,$host,$port)]} {
      set delta_time [expr $time - $relay($aid,time,$host,$port)]
      set relay($aid,delta_time,$host,$port) $delta_time
      if {$delta_time > 0.0} {
        set bps [expr int(($bytes - $relay($aid,bytes,$host,$port))/$delta_time)]
puts "BPS $bps"
        set relay($aid,bps,$host,$port) $bps
        AddCoors $relay($aid,graph_id,$host,$port) 0 $relay($aid,x,$host,$port) $bps
        incr relay($aid,x,$host,$port)
      }
      set relay($aid,last_time,$host,$port) $relay($aid,time,$host,$port)
    } else {
      set relay($aid,last_time,$host,$port) $time
    }
    set relay($aid,type,$host,$port) $type
    set relay($aid,time,$host,$port) $time
    set relay($aid,bytes,$host,$port) $bytes
    set relay(analyzer,last_id,$host,$port) $id
  } elseif {[regexp $relay(pattern2,$key) $line match id packets nosync tei wrongsize psie pids]} {
#puts "L2 : id $id type packets $packets pids $pids"
    if {![info exist relay(analyzer,$id,status,$host,$port)]} { return 1 }
    set aid "analyzer,$id"
    if {[info exist relay($aid,packets,$host,$port)] && $relay($aid,packets,$host,$port) != ""} {
      set delta_packets [expr $packets - $relay($aid,packets,$host,$port)]
      if {$relay($aid,delta_time,$host,$port) != 0} {
        set relay($aid,pps,$host,$port) [expr int($delta_packets / $relay($aid,delta_time,$host,$port))]
        set relay($aid,kbps,$host,$port) [expr int(1.504 * $delta_packets / $relay($aid,delta_time,$host,$port))]
      }
    }
    foreach keyname {packets nosync tei wrongsize psie pids} {
      upvar 0 $keyname keyval
      set relay($aid,$keyname,$host,$port) $keyval
    }
    set relay(analyzer,last_id,$host,$port) $id
  } elseif {[string compare "STAT:" $line] == 0} {
#puts "Setup Getting AnalyzerStatus data"
    set id $relay(analyzer,last_id,$host,$port)
    if {[info exist relay(analyzer,$id,status,$host,$port)]} {
      after 2000 [list AddServerCommand relay $host $port "ana status $id" AnalyzerStatus 1000]
    }
    # This was the last line we expected to read and we can now return 0
    return 0
  }
  # Skipping other lines and expect more lines
  return 1
}

# STAT: Analyzer 1 source receiver 10
# STAT:
proc SetUpGettingAnalyzerSource { host port } {
  global relay
  set relay(pattern1,getanasource,$host,$port) "STAT:\ +\Analyzer\ +(\[0-9]+)\ +source\ +(\[^ ]+)\ +(\[0-9]+)"
  after 0 [list AddServerCommand relay $host $port "ana source" GetAnalyzerSource 1000]
}
proc GetAnalyzerSource { handle tag host port line } {
  global relay
  set key [format "getanasource,%s,%s" $host $port]
  if {[regexp $relay(pattern1,$key) $line match id sourcetype sourceid]} {
    set relay(analyzer,$id,sourcetype,$host,$port) $sourcetype
    set relay(analyzer,$id,sourceid,$host,$port) $sourceid
  } elseif {[string compare "STAT:" $line] == 0} {
    # This was the last line we expected to read and we can now return 0
    return 0
  }
  return 1;
}

# STAT: Analyzer 1 type ts name <MPEG-2 TS Analyzer>
proc SetupGettingAnalyzers { host port type id } {
  global relay
puts "Setup Getting Analyzers"
  set relay(analyzers,$host,$port) ""
  set relay(pattern1,getanalyzers,$host,$port) "STAT:\ +\Analyzer\ +(\[0-9]+)\ +type\ +(\[^ ]+)\ +name\ +<(\[^\>]+)>"
  after 0 [list AddServerCommand relay $host $port "ana add" GetAnalyzers 1000]
puts "ADDED GetAnalyzers"
}

proc GetAnalyzers { handle tag host port line } {
  global relay
  set key [format "getanalyzers,%s,%s" $host $port]
  if {[regexp $relay(pattern1,$key) $line match id type name]} {
    if {[lsearch -exact $relay(analyzers,$host,$port) $id ] < 0} {
      lappend relay(analyzers,$host,$port) $id
    }
    set relay(analyzer,$id,type,$host,$port) $type
    set relay(analyzer,$id,name,$host,$port) $name
  } elseif {[string compare "STAT:" $line] == 0} {
    # This was the last line we expected to read and we can now return 0
    return 0
  }
  # Skipping other lines and expect more lines
  return 1
}

proc MakeAnalyzerWindow { host port type id } {
  global relay system
  set hostmap [string map {. _} $host]
  set analyzer_id 1
  set awin .analyzer-$id-$hostmap-$port
  set bg $system(default_bg)
  set aid "analyzer,$analyzer_id"
  if {![winfo exist $awin]} {
    SetupGettingAnalyzers $host $port $type $id
    SetUpGettingAnalyzerSource $host $port
    SetupGettingAnalyzerStatus $host $port

    toplevel $awin
    wm geometry $awin 700x300
    wm title $awin "$system(name) $system(version) Analyzer $host:$port: $type $id"
    wm protocol $awin WM_DELETE_WINDOW "CloseAnalyzerWindow $analyzer_id $host $port"

    # afr is the frame to hold the complete analyzer
    set afr [frame $awin.afr -padx 0 -pady 0 -bg $bg]
    pack $afr -side left -anchor nw -fill both -expand 1 

    # tfr is the frame within afr to hold info identifying the analyzer
    # mfr is the middle contain and bfr is the bottom container
    set tfr [frame $afr.tfr -padx 0 -pady 0 -bg $bg]
    set mfr [frame $afr.mfr -padx 0 -pady 0 -bg $bg]
    set bfr [frame $afr.bfr -padx 0 -pady 0 -bg $bg]
    pack $tfr $mfr $bfr -side top -expand 1 -fill x

    set relay($aid,tfr,$host,$port) $tfr
    set relay($aid,mfr,$host,$port) $mfr
    set relay($aid,bfr,$host,$port) $bfr

    # Info in the top frame
    label $tfr.name -text "Analyzer $analyzer_id : $host:$port" -relief raised -bd 1 -padx 4 -pady 0
    bind $tfr.name <ButtonPress> "$tfr.name configure -relief sunken ; RestartAnalyzer $analyzer_id $host $port"
    bind $tfr.name <ButtonRelease> "$tfr.name configure -relief raised"
    label $tfr.type -text "TS" -relief raised -bd 1 -padx 4 -pady 0
    pack $tfr.name $tfr.type -side left
    foreach name { kbps pps nosync tei wrongsize psie pids } {
      label $tfr.l$name  -text $name -relief raised -bd 1 -padx 2 -pady 0
      label $tfr.$name -textvariable relay($aid,$name,$host,$port) -relief sunken -bd 1 -padx 2 -pady 0
      pack $tfr.l$name $tfr.$name -side left
    }

    # Left, center and right container of the middle container
    set lfr [frame $mfr.lfr -padx 0 -pady 0 -bg $bg]
    set cfr [frame $mfr.cfr -padx 0 -pady 0 -bg $bg]
    set rfr [frame $mfr.rfr -padx 0 -pady 0 -bg $bg]
    pack $lfr $cfr $rfr -side left -anchor nw

    set labelfr [frame $rfr.lables -padx 0 -pady 0]
    pack $labelfr -side top -fill x -expand 1
    label $labelfr.fPID -text PID -relief raised -bd 1 -width 6 -pady 0
    pack $labelfr.fPID -side left
    foreach name { kbps Packets Last ALenErr Discon Prio Aprio} {
      label $labelfr.f$name -text $name -relief raised -bd 1 -width 8 -pady 0
      pack $labelfr.f$name -side left
    }

    set xpixels 400
    set ypixels 200
    set graph_id ana-$host-$port-$type-$id
    set graph [CreateGraph $cfr $graph_id $xpixels $ypixels 20 10 3]
    CreateLine $graph_id 0 "Bytes" [lindex $system(colors) 1] 49 8
    SetScaleLine $graph_id 0 1 0.00005
    puts "graph id $graph_id graph $graph"
    pack $graph -side top
    set relay($aid,awin,$host,$port) $awin
    set relay($aid,graph,$host,$port) $graph
    set relay($aid,graph_id,$host,$port) $graph_id
    set relay($aid,$xpixels,$host,$port) $xpixels
    set relay($aid,$ypixels,$host,$port) $ypixels
    set relay($aid,x,$host,$port) 1
    set relay($aid,pid_list,$host,$port) ""
    after 1000 [list StartGettingAnalyzerStatus $host $port $analyzer_id]

    set unsets [list awin tfr mfr bfr x xpixels ypixels pid_list graph graph_id status type time bytes last_time bps]
    set pid_unsets [list packets]

    set relay($aid,unsets,$host,$port) $unsets
    set relay($aid,pid_unsets,$host,$port) $pid_unsets
  } else { puts "Analyzer exist" }
}

proc RestartAnalyzer { id host port } {
  global relay
  after 0 [list AddServerCommand relay $host $port "ana add ts $id" "" 1000]
  after 0 [list AddServerCommand relay $host $port "ana add ts $id MPEGT-2 TS" "" 1000]
  after 0 [list AddServerCommand relay $host $port "ana source $id receiver 10" "" 1000]
  after 0 [list AddServerCommand relay $host $port "ana start $id" "" 1000]
  return
  set aid "analyzer,$id"
  set hostid "$host,$port"
  set pids_fr $relay($aid,mfr,$hostid).rfr
  foreach pid $relay($aid,pid_list,$hostid) {
    if {[winfo exist $pids_fr.fr$pid]} {
      destroy $pids_fr.fr$pid
    }
    unset relay($aid,$pid,packets,$hostid)
  }
  set relay($aid,pid_list,$hostid) ""
}

# Cleaning up
proc CloseAnalyzerWindow { id host port } {
  global relay
  set aid "analyzer,$id"
  set win $relay($aid,awin,$host,$port)
  if {[winfo exist $win]} {
    destroy $win
  }
  foreach pid $relay($aid,pid_list,$host,$port) {
    foreach key $relay($aid,pid_unsets,$host,$port) {
#      puts "unset -nocomplain relay($aid,$pid,$key,$host,$port)"
      unset -nocomplain relay($aid,$pid,$key,$host,$port)
    }
  }
  foreach key $relay($aid,unsets,$host,$port) {
    unset -nocomplain relay($aid,$key,$host,$port)
  }
  unset relay($aid,unsets,$host,$port)
}

proc AddPidToList { analyzer_id host port pid } {
  global relay
  lappend relay(analyzer,$analyzer_id,pid_list,$host,$port) $pid
  set relay(analyzer,$analyzer_id,pid_list,$host,$port) [lsort -unique -integer $relay(analyzer,$analyzer_id,pid_list,$host,$port)]
}

proc RepackPidContainer { id host port } {
  global relay
  set aid "analyzer,$id"
  set hostid "$host,$port"
  set pids_fr $relay($aid,mfr,$hostid).rfr
  foreach pid $relay($aid,pid_list,$hostid) {
    if {[winfo exist $pids_fr.fr$pid]} {
      pack forget $pids_fr.fr$pid
    }
  }
  foreach pid $relay($aid,pid_list,$hostid) {
    pack $pids_fr.fr$pid -side top -anchor nw -fill x -expand 1
  }
}

proc AddPidToAnalyzer { id host port pid } {
  global relay system
  set aid "analyzer,$id"
  set hostid "$host,$port"
  set bg $system(default_bg)
  # Check if already added
  if {[info exist relay($aid,$pid,packets,$hostid)]} return
#puts "The relay($aid,$pid,packets,$hostid) does not exist"
  AddPidToList $id $host $port $pid

  set pids_fr $relay($aid,mfr,$hostid).rfr
  set pid_fr [frame $pids_fr.fr$pid]
  pack $pid_fr -side top
  RepackPidContainer $id $host $port
  label $pid_fr.name -text $pid -width 6 -padx 1 -pady 0 -bg $bg -relief raised
  pack $pid_fr.name -side left
  foreach name {kbps packets last adaple discon prio aprio} {
    Label $pid_fr.$name -textvariable relay($aid,$pid,$name,$hostid) -justify right -width 8 -relief sunken -pady 0 -bg $bg
    pack $pid_fr.$name -side left -fill x -expand 1
  }
}


