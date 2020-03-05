# Introduction

This is a minimal CI ("continuous integration") system used by BSD.lv
projects.
It's brutally simple, using only a shell script
[minci.sh](https://github.com/kristapsdz/minci/blob/master/minci.sh)
as the test runner.
Results are recorded and perusable using the
[BCHS](https://learnbchs.org) back-end server.

The CI test runner executes the following steps:

1. clone or freshen a project's git repository
2. configure the software (all use [oconfigure](https://github.com/kristapsdz/oconfigure))
3. build
4. run tests
5. fake install
6. distribution check (runs build, tests, install on distributed archive)

The results of this sequence are sent to the CI report server.  The
server is just a [BCHS](https://learnbchs.org) CGI program built using
[openradtool](https://kristaps.bsd.lv/openradtool).
