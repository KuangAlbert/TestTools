#!/bin/sh

app="/usr/bin/auto-testing"
PIDFILE="/var/run/auto-testing.pid"
FACTORY="/usr/bin/factory_upgrade"

test -x "$app" || exit 0

case "$1" in
  start)
    if  cat /proc/cmdline | grep "testing=1"; then
	dmesg -n 1
        echo "insmod filesystem"
        /etc/init.d/filesystem.sh
        /etc/init.d/udev_hotplug.sh
        echo "Starting $app"
        $app
        if [ $? = "0" ]; then
            sleep 1
            $FACTORY "testing"
        fi
    fi
    ;;

  stop)
    echo "$app"
    pidof auto-testing > $PIDFILE
    kill `cat $PIDFILE`
    ;;
  *)
    echo "Usage: /etc/init.d/auto-testing.sh {start|stop}"
    exit 1
esac

exit 0
