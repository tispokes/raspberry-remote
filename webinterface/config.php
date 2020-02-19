<?php
/*
 * Raspberry Remote
 * http://xkonni.github.com/raspberry-remote/
 * 
 * Fork from tispokes (32bit, dimming, IPv6):
 * https://github.com/tispokes/raspberry-remote
 *
 * configuration for the webinterface
 *
 */

/*
 * define ip address and port here
 */
$source = $_SERVER['SERVER_NAME']; // SERVER_NAME works out for me, SERVER_ADDR not
$target = '127.0.0.1';
$port = 11337;
$version = AF_INET; // AF_INET should work for both in this case. Set to AF_INET6 for IPv6 or to AF_INET for IPv4, if you have Problems.

/*
 * specify configuration of sockets to use
 *   array("systemcode", "group" , "plug", "description");
 *   systemcode 1: elro,
 *   systemcode 0: raw (group = 10 digits, plug = ""),
 *   systemcode 2
 * use empty string, w/o array, to create empty box
 *   "",
 */
$config=array(
  /*
   * Elro
   */
  array("1", "00001", "16", "Nr. 1", "n"),
  array("1", "00001", "08", "Nr. 2", "n"),
  array("1", "00001", "04", "Nr. 3", "n"),
  array("1", "00001", "02", "Nr. 4", "y"),
  array("1", "00001", "01", "Nr. 5", "y"),
  /*
   * Intertech
   */
  array("2", "01", "01", "IT 1", "n"),
  array("2", "01", "02", "IT 2", "n"),
  array("2", "01", "03", "IT 3", "n"),
  array("2", "01", "04", "IT 4", "n"),
  /*
   * 32 Bit raw data
   */
  array("0", "2888584704", "", "32 Bit", "n")
  )
?>
