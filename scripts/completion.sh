_minicompiler_completion() {
    local cur prev opts commands
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    commands="lex preprocess parse check ir ssa codegen compile help"

    if [[ ${COMP_CWORD} -eq 1 ]]; then
        COMPREPLY=( $(compgen -W "${commands} -S -c -E -O --help --version --color" -- ${cur}) )
        return 0
    fi

    case "${prev}" in
        --color)
            COMPREPLY=( $(compgen -W "auto always never" -- ${cur}) )
            return 0
            ;;
        -o|--output)
            COMPREPLY=( $(compgen -f -- ${cur}) )
            return 0
            ;;
    esac

    COMPREPLY=( $(compgen -f -- ${cur}) )
}
complete -F _minicompiler_completion minicompiler
