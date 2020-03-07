# Introduction

This is a minimal CI ("continuous integration") system used by BSD.lv
projects.
It uses a shell script as the test runner and a
[BCHS](https://learnbchs.org) back-end server for recording results.
The server is assumed to run on a [OpenBSD](https://www.openbsd.org) system.

The CI test runner performs the following:

1. clone or freshen a project's git repository
2. configure the software (all use [oconfigure](https://github.com/kristapsdz/oconfigure))
3. build
4. run tests
5. fake install
6. distribution check (runs build, tests, install on distributed archive)

The results of this sequence (failure is implied if the last step isn't
reached) are sent to the CI report server.

Tested projects are simply identified by name.  There is currently no
way to verify that project results are actually from a project.

CI testers are identified by authentication tokens.  (These are
internally associated with an e-mail.)  These tokens are used to sign
submitted reports.

# Repositories

Repositories are identified only by name.  The test runner uses the name
component of the repository URL as its reported repository.  So with a
repository of `https://github.com/you/yourproj.git`, the name is
`yourproj`.

Add repositories to the database as follows:

```sh
name="foobar" ; \
echo "INSERT INTO project (name) VALUES (\"$name\")" | \
	sqlite3 path/to/minci.db
```

The unique name must be less than 32 bytes.  Adjust the database
filename as appropriate.

The repository so identified must conform to the following:

1. Accessible by git, with the name being the URL filename.
2. `./configure PREFIX=install_prefix`
3. make
4. make regress
5. make install
6. make distcheck

The `make` must be BSD make.

# CI testers

To generate a CI tester, add a row to the `user` table of the database.
Set the e-mail to anything you'd like.

The API key and secret are used to authenticate messages and should be
random.  Assuming your operating system has a good random number
generator, you can use:

```sh
email="foo@bar.com" ; \
apikey="$RANDOM" ; \
apisecret="`jot -r -c 32 'A' '{' | sed 's!\\$!-!g' | rs -g0`" ; \
ctime="`date +%s`" ; \
echo "INSERT INTO user (email,apikey,apisecret,ctime) \
	VALUES (\"$email\",\"$apikey\",\"$apisecret\",\"$date\")" | \
	sqlite3 path/to/minci.db
```

(The `sed` makes sure that the final character isn't an escape.)

Set `foo@bar.com` to be the user's email address and adjust the database
filename.

# Test Runner

The test runner is just a shell script.
If you'd like to participate in a **minci** CI community, the shell
script, server URL, and credentials are all you need.

**The test runner should be run as an ordinary user**.  Do not run it
with super user privileges.  Ideally it should have its own user, as a
repository might accidentally try to overwrite files it shouldn't.

First, download or otherwise acquire the test runner,
[minci.sh](minci.sh).  It should probably be in your *bin* directory
with execute permission.

Next, create *~/.minci*.  (This can also be in */etc/minci*.)

```
apikey = 12345
apisecret = abcdefghijklmnopqrstuvwxyz123456
#bsdmake = bmake
server = https://yourdomain/cgi-bin/minci.cgi
repo = https://github.com/somename/yourrepo1.git
repo = https://github.com/somename/yourrepo2.git
```

There can be arbitrary space around the equal signs.  `bsdmake` is
useful for Linux machines, since repositories are assumed to use BSD
make and not GNU make, which is the default `make` on some machines.

In this example, there are two repositories, `yourrepo1` and
`yourrepo2`, which must be represented in the database.

Ideally, the runner should be executed daily by `cron`:

```
@daily $HOME/bin/minci >/dev/null 2>&1
```

The `PATH` variable may need to be set to include */usr/local/bin* or as
required by the operating system.

Alternatively, the script may be run manually for instant gratification.

# Security

First, the server only runs on OpenBSD and makes significant use of pledging,
unveiling, and separation using
[openradtool](https://kristaps.bsd.lv/openradtool)'s underlying
[kcgi](https://kristaps.bsd.lv/kcgi) and
[sqlbox](https://kristaps.bsd.lv/sqlbox) features.

Messages between the runner and server are authenticated by hashing the entire
message (in a well-defined order) with the user's secret API key.  The server,
which also has the secret key, hashes the message contents as well and compares
the signature.  If the signature matches, the report is authentic.

