#!/bin/bash

LANG=C sudo -u postgres psql -c "create user system encrypted password 'manager' superuser;"

