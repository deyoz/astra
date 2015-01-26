if [ -r $1/env_file ] ; then 
. $1/env_file
(cd src && ./Config -f)
for i in 1 2 3 4 5 6 ; do echo ; done
echo now you can run make or MAKE in src    
for i in 1 2 3 4 5 6 ; do echo ; done
else
echo "Usage: config_sirena <path to sirenalibs direciory (after build all)>"
exit 1
fi
