#!/bin/sh

BUILTINS="
alias
bg
builtin
cd
clear
echo
exit
export
fg
getopts
help
jobs
kill
pwd
read
register
set
shift
source
trap
type
umask
unalias
unset
wait
"

BASE_CMDS="
ls
cat
cp
mv
rm
mkdir
rmdir
touch
grep
find
chmod
chown
head
tail
diff
patch
tar
gzip
gunzip
df
du
ps
top
env
uname
whoami
"

write_comp() {
    cmd="$1"
    file="$cmd.comp"

    if [ -f "$file" ]; then
        return
    fi

    case "$cmd" in
        alias)
            cat >"$file" <<EOF
# completion for alias
command: alias
type: files_and_dirs
EOF
            ;;
        bg)
            cat >"$file" <<EOF
# completion for bg
command: bg
type: files_and_dirs
EOF
            ;;
        builtin)
            cat >"$file" <<EOF
# completion for builtin
command: builtin
type: files_and_dirs
EOF
            ;;
        cd)
            cat >"$file" <<EOF
# completion for cd
command: cd
type: dirs
EOF
            ;;
        clear)
            cat >"$file" <<EOF
# completion for clear
command: clear
type: files_and_dirs
EOF
            ;;
        echo)
            cat >"$file" <<EOF
# completion for echo
command: echo
type: files_and_dirs

options:
  - -n
EOF
            ;;
        exit)
            cat >"$file" <<EOF
# completion for exit
command: exit
type: files_and_dirs
EOF
            ;;
        export)
            cat >"$file" <<EOF
# completion for export
command: export
type: files_and_dirs
EOF
            ;;
        fg)
            cat >"$file" <<EOF
# completion for fg
command: fg
type: files_and_dirs
EOF
            ;;
        getopts)
            cat >"$file" <<EOF
# completion for getopts
command: getopts
type: files_and_dirs
EOF
            ;;
        help)
            cat >"$file" <<EOF
# completion for help
command: help
type: files_and_dirs
EOF
            ;;
        jobs)
            cat >"$file" <<EOF
# completion for jobs
command: jobs
type: files_and_dirs
EOF
            ;;
        kill)
            cat >"$file" <<EOF
# completion for kill
command: kill
type: files_and_dirs

options:
  - -9
  - -15
  - -HUP
  - -TERM
  - -KILL
EOF
            ;;
        pwd)
            cat >"$file" <<EOF
# completion for pwd
command: pwd
type: files_and_dirs
EOF
            ;;
        read)
            cat >"$file" <<EOF
# completion for read
command: read
type: files_and_dirs

options:
  - -r
  - -p
  - -t
EOF
            ;;
        register)
            cat >"$file" <<EOF
# completion for register
command: register
type: files_and_dirs
EOF
            ;;
        set)
            cat >"$file" <<EOF
# completion for set
command: set
type: files_and_dirs

options:
  - -e
  - -x
  - -u
  - -o
EOF
            ;;
        shift)
            cat >"$file" <<EOF
# completion for shift
command: shift
type: files_and_dirs
EOF
            ;;
        source)
            cat >"$file" <<EOF
# completion for source
command: source
type: files

extensions: tl
EOF
            ;;
        trap)
            cat >"$file" <<EOF
# completion for trap
command: trap
type: files_and_dirs

options:
  - -l
  - -p
EOF
            ;;
        type)
            cat >"$file" <<EOF
# completion for type
command: type
type: files_and_dirs
EOF
            ;;
        umask)
            cat >"$file" <<EOF
# completion for umask
command: umask
type: files_and_dirs
EOF
            ;;
        unalias)
            cat >"$file" <<EOF
# completion for unalias
command: unalias
type: files_and_dirs
EOF
            ;;
        unset)
            cat >"$file" <<EOF
# completion for unset
command: unset
type: files_and_dirs
EOF
            ;;
        wait)
            cat >"$file" <<EOF
# completion for wait
command: wait
type: files_and_dirs
EOF
            ;;
        # ---------------------------------------------------------
        # STANDARD COMMANDS (FULL OPTIONS)
        # ---------------------------------------------------------
        ls)
            cat >"$file" <<EOF
# completion for ls
command: ls
type: files_and_dirs

options:
  - -l
  - -a
  - -h
  - -R
  - -S
  - -t
  - --color
EOF
            ;;
        cat)
            cat >"$file" <<EOF
# completion for cat
command: cat
type: files

extensions: txt, md, log

options:
  - -n
  - -b
  - -s
  - -E
  - -T
EOF
            ;;
        cp)
            cat >"$file" <<EOF
# completion for cp
command: cp
type: files_and_dirs

options:
  - -r
  - -R
  - -f
  - -i
  - -n
  - -v
  - --preserve
EOF
            ;;
        mv)
            cat >"$file" <<EOF
# completion for mv
command: mv
type: files_and_dirs

options:
  - -f
  - -i
  - -n
  - -v
EOF
            ;;
        rm)
            cat >"$file" <<EOF
# completion for rm
command: rm
type: files

options:
  - -r
  - -R
  - -f
  - -i
  - -v
EOF
            ;;
        mkdir)
            cat >"$file" <<EOF
# completion for mkdir
command: mkdir
type: dirs

options:
  - -p
  - -m
  - -v
EOF
            ;;
        rmdir)
            cat >"$file" <<EOF
# completion for rmdir
command: rmdir
type: dirs

options:
  - -p
  - -v
EOF
            ;;
        touch)
            cat >"$file" <<EOF
# completion for touch
command: touch
type: files

options:
  - -a
  - -m
  - -c
  - -d
  - -t
EOF
            ;;
        grep)
            cat >"$file" <<EOF
# completion for grep
command: grep
type: files

extensions: txt, log, md

options:
  - -i
  - -r
  - -n
  - -v
  - -w
  - -E
  - -F
  - --color
EOF
            ;;
        find)
            cat >"$file" <<EOF
# completion for find
command: find
type: files_and_dirs

options:
  - -name
  - -type
  - -maxdepth
  - -mindepth
  - -size
  - -mtime
EOF
            ;;
        chmod)
            cat >"$file" <<EOF
# completion for chmod
command: chmod
type: files

options:
  - -R
  - -c
  - -v
EOF
            ;;
        chown)
            cat >"$file" <<EOF
# completion for chown
command: chown
type: files

options:
  - -R
  - -c
  - -v
EOF
            ;;
        head)
            cat >"$file" <<EOF
# completion for head
command: head
type: files

options:
  - -n
  - -c
EOF
            ;;
        tail)
            cat >"$file" <<EOF
# completion for tail
command: tail
type: files

options:
  - -n
  - -f
  - -c
EOF
            ;;
        diff)
            cat >"$file" <<EOF
# completion for diff
command: diff
type: files

options:
  - -u
  - -r
  - -q
  - -N
EOF
            ;;
        patch)
            cat >"$file" <<EOF
# completion for patch
command: patch
type: files

options:
  - -p0
  - -p1
  - -p2
  - -R
EOF
            ;;
        tar)
            cat >"$file" <<EOF
# completion for tar
command: tar
type: files_and_dirs

options:
  - -c
  - -x
  - -t
  - -z
  - -j
  - -v
  - -f
EOF
            ;;
        gzip)
            cat >"$file" <<EOF
# completion for gzip
command: gzip
type: files

extensions: gz

options:
  - -d
  - -k
  - -v
EOF
            ;;
        gunzip)
            cat >"$file" <<EOF
# completion for gunzip
command: gunzip
type: files

extensions: gz

options:
  - -k
  - -v
EOF
            ;;
        df)
            cat >"$file" <<EOF
# completion for df
command: df
type: files_and_dirs

options:
  - -h
  - -k
  - -T
EOF
            ;;
        du)
            cat >"$file" <<EOF
# completion for du
command: du
type: files_and_dirs

options:
  - -h
  - -k
  - -s
EOF
            ;;
        ps)
            cat >"$file" <<EOF
# completion for ps
command: ps
type: files_and_dirs

options:
  - -e
  - -f
  - -u
  - -aux
EOF
            ;;
        top)
            cat >"$file" <<EOF
# completion for top
command: top
type: files_and_dirs
EOF
            ;;
        env)
            cat >"$file" <<EOF
# completion for env
command: env
type: files_and_dirs

options:
  - -i
EOF
            ;;
        uname)
            cat >"$file" <<EOF
# completion for uname
command: uname
type: files_and_dirs

options:
  - -a
  - -s
  - -n
  - -r
  - -v
  - -m
EOF
            ;;
        whoami)
            cat >"$file" <<EOF
# completion for whoami
command: whoami
type: files_and_dirs
EOF
            ;;
    esac
}

for CMD in $BUILTINS $BASE_CMDS; do
    write_comp "$CMD"
done
