<?php

header ("Content-type: video/MP2T");
ob_end_flush ();	# Flush and end buffering.
set_time_limit (0);	# The video feed may run for quite some time. Don't abort it.

#passthru ('(gst-launch-0.10 -v filesrc location=/var/www/recordings/pmm.ts ! filesink location=/dev/fd/3 buffer-mode=2 > /tmp/video.php.err 2>&1) 3>&1');
passthru ('(gst-launch-0.10 -v tcpclientsrc port=5010 ! mpegtsparse ! filesink location=/dev/fd/3 buffer-mode=2 > /tmp/video.php.err 2>&1) 3>&1');

?>
