whence ()
{
    local vflag= path=;
    if [ "$#" = "0" ]; then
        echo "whence: argument expected";
        return 1;
    fi;
    case "$1" in
        -v)
            vflag=1;
            shift 1
        ;;
        -*)
            echo "whence: bad option: $1";
            return 1
        ;;
        *)

        ;;
    esac;
    if [ "$#" = "0" ]; then
        echo "whence: bad argument count";
        return 1;
    fi;
    for cmd in "$@";
    do
        if [ "$vflag" ]; then
            echo $(builtin type $cmd | sed 1q);
        else
            path=$(builtin type -path $cmd);
            if [ "$path" ]; then
                echo $path;
            else
                case "$cmd" in
                    /*)
                        if [ -x "$cmd" ]; then
                            echo "$cmd";
                        fi
                    ;;
                    *)
                        case "$(builtin type -type $cmd)" in
                            "")

                            ;;
                            *)
                                echo "$cmd"
                            ;;
                        esac
                    ;;
                esac;
            fi;
        fi;
    done;
    return 0
}
