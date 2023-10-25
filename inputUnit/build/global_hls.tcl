global env 
set USER_VARS {SOC_INFRA MGC_HOME SEARCH_PATH} 

echo "*** USER SETTINGS ***"
foreach var $USER_VARS {
    if [info exists env($var)] {
        echo "$var = $env($var)"
        set $var $env($var)
    } else {
        echo "Warning: $var not set by user"
        set $var ""
    }   
}

flow package require /SCVerify

# Set options
options set /Input/TargetPlatform x86_64
options set /Input/CppStandard c++11
options set /Input/CompilerFlags {-DCONNECTIONS_ACCURATE_SIM -DHLS_CATAPULT}
options set /Input/SearchPath "$env(SOC_INFRA_HOME)/iplib" -append
options set /Input/SearchPath "$env(SOC_INFRA_HOME)/iplib/mb_shared/pkgs/matchlib/cmod/include" -append
options set /Input/SearchPath ". $SEARCH_PATH" -append

options set /Output/OutputVHDL false


project new 
