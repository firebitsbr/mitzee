<?php
header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");             // Date in the past
header("Last-Modified: " . gmdate("D, d M Y H:i:s") . "GMT"); // always modified
header("Cache-Control: no-cache, must-revalidate");           // HTTP/1.1
header("Pragma: no-cache");                                   // HTTP/1.0
?>
<html>
<head>
<title>Visitors</title>
<meta http-equiv="refresh" content="100">
<style type="text/css">
<!--
td {  font-family: Verdana, Arial, Helvetica, sans-serif}
-->
</style>
</head>
<body bgcolor="#000000" text="#000000" leftmargin="0" topmargin="0" marginwidth="0" marginheight="0">
<table border="0" cellspacing="1" cellpadding="2" align="center" width="400" bgcolor="#000000">
  <tr bgcolor="#006666">
    <td width="50"><font size="2" color="#FFFFFF">Time</font></td>
    <td width="95"><font size="2" color="#FFFFFF">IP Address</font></td>
    <td width="230"><font size="2" color="#FFFFFF">Host</font></td>
  </tr>
<?php

include ("./config.inc.php");
include ("./mysql.class.php");

if ($COUNT_CFG['use_db']) {
    $visits = new dcounter_sql();
    if ($visits->connect()) {
        $result = $visits->query("select * from $COUNT_TBL[visitors] order by time desc");
        while ($row = $visits->fetch_array($result)) {
            $time_format = strftime("%H:%M",$row['time']);
            echo "  <tr bgcolor=\"#e0e0e0\">\n";
            echo "    <td width=\"50\"><font size=\"-2\">$time_format</font></td>\n";
            echo "    <td width=\"95\"><font size=\"-2\">$row[ip]</font></td>\n";
            echo "    <td width=\"230\"><font size=\"-2\">$row[host]</font></td>\n  </tr>\n";
        }
        $visits->close_db();
    }
} else {
    if(file_exists($COUNT_CFG['logfile'])) {
        $ip_array = file($COUNT_CFG['logfile']);
        for ($i=sizeof($ip_array)-1;$i>=0;$i--) {
            list($time_stamp,$ip_addr,$hostname) = split("\|",$ip_array[$i]);
            $host = chop($hostname);
            $time_format = strftime("%H:%M",$time_stamp);
            echo "  <tr bgcolor=\"#e0e0e0\">\n";
            echo "    <td width=\"50\"><font size=\"-2\">$time_format</font></td>\n";
            echo "    <td width=\"95\"><font size=\"-2\">$ip_addr</font></td>\n";
            echo "    <td width=\"230\"><font size=\"-2\">$host</font></td>\n  </tr>\n";
        }
    }
} 
?>
  <tr align="right" bgcolor="#CCCCCC">
    <td colspan="3"><font size="-2" color="#000099"><u>Daily Counter 1.1</u></font></td>
  </tr>
</table>
</body>
</html>
