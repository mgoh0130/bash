# bash
bashLT is a simple shell, a baby brother of the Bourne-again shell
bash, and offers a limited subset of bash's functionality:

- execution of simple commands with zero or more arguments

- definition of local environment variables (NAME=VALUE)

- redirection of the standard input (<, <<)

- redirection of the standard output (>, >>)

- execution of pipelines (sequences of one or more simple commands or
  subcommands separated by the pipeline operator |)

- execution of conditional commands (sequences of one or more pipelines
  separated by the command operators && and ||)

- execution of sequences of one or more conditional commands separated by the
  command terminators ; and & and possibly terminated by ; or &

- execution of commands in the background (&)

- subcommands (commands enclosed in parentheses that act like a single, simple
  command)

- reporting the status of the last simple command, pipeline, conditional
  command, or subcommand executed by setting the environment variable $? to
  its "printed" value (e.g., the string "0" if the value is zero).

- directory manipulation:

    cd -p              Print current working directory to the standard output.
    cd DIRNAME         Change current working directory to DIRNAME.
    cd                 Change current working directory to $HOME, where HOME is
                         is an environment variable.

- other built-in commands:

    export NAME=VALUE  Set environment variable NAME to VALUE.
    export -n NAME     Delete environment variable NAME.

    wait               Wait until all child processes of the shell process have
                       died.  The status is 0.

Once the command line has been parsed, the exact semantics of bashLT are those
of bash, except for the status variable and the items noted below 
