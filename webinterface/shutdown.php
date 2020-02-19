<?php
        // user www-data must be accessible to /sbin/shutdown, test with: sudo -H -u www-data bash -c '$(/sbin/shutdown -h now)' 
        // if not working, add this to 'sudoers': www-data  ALL= NOPASSWD: /sbin/shutdown -h now
        function runShutdown(){
                echo exec("sudo shutdown -h now");
        }

        if (isset($_GET['shutdown'])) {
                echo "Shutting down, Bye!";
                runShutdown();
        }
?>
