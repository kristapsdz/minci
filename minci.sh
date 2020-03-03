#! /bin/sh

MAKE=make
API_SECRET=
API_KEY=
SERVER=
TESTS_GLOBAL="/usr/local/share/minci/deps"
TESTS_LOCAL="$HOME/.local/share/minci/deps"
STAGING="$HOME/.local/cache/minci"
CONFIG=
CONFIG_LOCAL="$HOME/.minci"
CONFIG_GLOBAL="/etc/minci"

# Start with the local then global configuration.
# Require at least one of them.

if [ ! -r "$CONFIG_LOCAL" ]
then
	if [ ! -r "$CONFIG_GLOBAL" ]
	then
		echo "$0: no config found" 1>&2
		echo "$0: ...tried: $CONFIG_GLOBAL" 1>&2
		echo "$0: ...tried: $CONFIG_LOCAL" 1>&2
		exit 1
	fi
	CONFIG="$CONFIG_GLOBAL"
else
	CONFIG="$CONFIG_LOCAL"
fi
echo "$0: using config: $CONFIG"

# Read the API secret from the configuration.

while read ln
do
	bsdmake="$(echo $ln | sed -n 's!^[ ]*bsdmake[ ]*=[ ]*!!p')"
	[ -z "$bsdmake" ] && continue
	MAKE="$bsdmake"
done < "$CONFIG"

# Read the API secret from the configuration.

while read ln
do
	api_secret="$(echo $ln | sed -n 's!^[ ]*apisecret[ ]*=[ ]*!!p')"
	[ -z "$api_secret" ] && continue
	API_SECRET="$api_secret"
done < "$CONFIG"
if [ -z "$API_SECRET" ]
then
	echo "$0: no API secret specified" 1>&2
	exit 1
fi

# Read the API key from the configuration.

while read ln
do
	api_key="$(echo $ln | sed -n 's!^[ ]*apikey[ ]*=[ ]*!!p')"
	[ -z "$api_key" ] && continue
	API_KEY="$api_key"
done < "$CONFIG"
if [ -z "$API_KEY" ]
then
	echo "$0: no API key specified" 1>&2
	exit 1
fi

# Read the server from the configuration.

while read ln
do
	server="$(echo $ln | sed -n 's!^[ ]*server[ ]*=[ ]*!!p')"
	[ -z "$server" ] && continue
	SERVER="$server"
done < "$CONFIG"
if [ -z "$SERVER" ]
then
	echo "$0: no server specified" 1>&2
	exit 1
fi

echo "$0: using API key: $API_KEY"
echo "$0: using API secret: $API_SECRET"
echo "$0: using server: $SERVER"

# Check or create where we'll put our repositories.

if [ ! -d "$STAGING" ]
then
	mkdir -p "$STAGING"
	if [ $? -ne 0 ]
	then
		echo "$0: could not create: $STAGING" 1>&2
		exit 1
	fi
	echo "$0: created: $STAGING"
fi

# Process each repository line.

while read ln
do
	repo="$(echo $ln | sed -n 's!^[ ]*repo[ ]*=[ ]*!!p')"
	[ -z "$repo" ] && continue

	reponame="$(echo $repo | sed -e 's!.*/!!' -e 's!\.git$!!')"
	if [ -z "$reponame" ]
	then
		echo "$0: malformed repo: $repo" 1>&2
		exit 1
	fi

	# Now errors without || are fatal.

	set -e

	echo "$0: processing repo: $reponame"
	echo "$0: ...parsed from: $repo"

	cd "$STAGING"

	# Set all of our times to zero.
	# If a time is non-zero when we send our ping, that means that
	# we've finished a phase.

	TIME_start=0
	TIME_env=0
        TIME_depend=0
        TIME_build=0
        TIME_test=0
        TIME_install=0
        TIME_distcheck=0

	# This is wrapped in an infinite loop so we can just use `break`
	# to come out of error situations.

	exec 3>/tmp/minci.log
	TIME_start=`date +%s`

	while `true`
	do
		if [ -d "$reponame" ]
		then
			cd "$reponame"

			echo "$0: git fetch origin: $reponame"
			echo "$0: git fetch origin: $reponame" 1>&3
			git fetch origin 1>&3 2>&3 || break

			echo "$0: git reset --hard origin/master: $reponame"
			echo "$0: git reset --hard origin/master: $reponame" 1>&3
			git reset --hard origin/master 1>&3 2>&3 || break

			echo "$0: git clean -fdx: $reponame"
			echo "$0: git clean -fdx: $reponame" 1>&3
			git clean -fdx 1>&3 2>&3 || break
		else
			echo "$0: git clone $repo $reponame"
			echo "$0: git clone $repo $reponame" 1>&3
			git clone "$repo" "$reponame" 1>&3 2>&3 || break
			cd "$reponame"
		fi
		TIME_env=`date +%s`

		if [ -r "minci.cfg" ]
		then
			while read mln
			do
				dep="$(echo $ln | sed -n 's!^[ ]*dep[ ]*=[ ]*!!p')"
				[ -n "$dep" ] || continue

				echo "$0: pkg-config --exists $dep: $reponame"
				echo "$0: pkg-config --exists $dep: $reponame" 1>&3
				pkg-config --exists "$dep" 1>&3 2>&3 || break
			done < "minci.cfg"
			[ -n "$mln" ] || break
		fi
		TIME_depend=`date +%s`

		echo "$0: ./configure PREFIX=build: $reponame"
		echo "$0: ./configure PREFIX=build: $reponame" 1>&3
		./configure PREFIX=build 1>&3 2>&3 || break

		echo "$0: ${MAKE}: $reponame"
		echo "$0: ${MAKE}: $reponame" 1>&3
		${MAKE} 1>&3 2>&3 || break
		TIME_build=`date +%s`

		echo "$0: ${MAKE} regress: $reponame"
		echo "$0: ${MAKE} regress: $reponame" 1>&3
		${MAKE} regress 1>&3 2>&3 || break
		TIME_test=`date +%s`

		echo "$0: ${MAKE} install: $reponame"
		echo "$0: ${MAKE} install: $reponame" 1>&3
		${MAKE} install 1>&3 2>&3 || break
		TIME_install=`date +%s`

		echo "$0: ${MAKE} distcheck: $reponame"
		echo "$0: ${MAKE} distcheck: $reponame" 1>&3
		${MAKE} distcheck 1>&3 2>&3 || break
		TIME_distcheck=`date +%s`
		break
	done

	exec 3>&-
	set +e

	if [ $TIME_distcheck -eq 0 ]
	then
		echo "$0: FAILURE: $reponame"
	else
		echo "$0: success: $reponame"
	fi

	echo "$0: computing signature"

	UNAME_M=`uname -m | sed -e 's!^[ ]*!!g' -e 's![ ]*$!!g'`
	UNAME_N=`uname -n | sed -e 's!^[ ]*!!g' -e 's![ ]*$!!g'`
	UNAME_R=`uname -r | sed -e 's!^[ ]*!!g' -e 's![ ]*$!!g'`
	UNAME_S=`uname -s | sed -e 's!^[ ]*!!g' -e 's![ ]*$!!g'`
	UNAME_V=`uname -v | sed -e 's!^[ ]*!!g' -e 's![ ]*$!!g'`

	# Create the signature for this entry.
	# It consists of all the MD5 hash of all arguments in
	# alphabetical order (by key), with the report-log (or /dev/null
	# if there's no need to report) being replaced by its MD5 hash.

	QUERY="project-name=${reponame}"
	QUERY="${QUERY}&report-build=${TIME_build}"
	QUERY="${QUERY}&report-distcheck=${TIME_distcheck}"
	QUERY="${QUERY}&report-env=${TIME_env}"
	QUERY="${QUERY}&report-depend=${TIME_depend}"
	QUERY="${QUERY}&report-install=${TIME_install}"
	if [ $TIME_distcheck -eq 0 ]
	then
		QUERY="${QUERY}&report-log=`openssl dgst -md5 -hex /tmp/minci.log | sed 's!^[^=]*= !!'`"
	else
		QUERY="${QUERY}&report-log=`openssl dgst -md5 -hex /dev/null | sed 's!^[^=]*= !!'`"
	fi
	QUERY="${QUERY}&report-start=${TIME_start}"
	QUERY="${QUERY}&report-test=${TIME_test}"
	QUERY="${QUERY}&report-unamem=${UNAME_M}"
	QUERY="${QUERY}&report-unamen=${UNAME_N}"
	QUERY="${QUERY}&report-unamer=${UNAME_R}"
	QUERY="${QUERY}&report-unames=${UNAME_S}"
	QUERY="${QUERY}&report-unamev=${UNAME_V}"
	QUERY="${QUERY}&user-apisecret=${API_SECRET}"

	SIGNATURE=`/bin/echo -n "$QUERY" | openssl dgst -md5 -hex | sed 's!^[^=]*= !!'`

	# Now actually send the report.
	# It includes the signature and optionally the build log (only
	# if we didn't get to the end).

	echo "$0: sending report: $SERVER"

	REPORT_LOG=
	if [ $TIME_distcheck -eq 0 ]
	then
		REPORT_LOG="-F report-log=</tmp/minci.log"
	else
		REPORT_LOG="-F report-log="
	fi
	curl ${REPORT_LOG} \
	     -F "project-name=${reponame}" \
	     -F "report-start=${TIME_start}" \
	     -F "report-env=${TIME_env}" \
	     -F "report-depend=${TIME_depend}" \
	     -F "report-build=${TIME_build}" \
	     -F "report-test=${TIME_test}" \
	     -F "report-install=${TIME_install}" \
	     -F "report-distcheck=${TIME_distcheck}" \
	     -F "report-unamem=${UNAME_M}" \
	     -F "report-unamen=${UNAME_N}" \
	     -F "report-unamer=${UNAME_R}" \
	     -F "report-unames=${UNAME_S}" \
	     -F "report-unamev=${UNAME_V}" \
	     -F "user-apikey=${API_KEY}" \
	     -F "signature=${SIGNATURE}" \
	     "${SERVER}"

	rm -f /tmp/minci.log
done < "$CONFIG"

exit 0
